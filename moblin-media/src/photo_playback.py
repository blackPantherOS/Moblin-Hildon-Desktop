#!/usr/bin/python -ttu
# -*- mode:python; tab-width:4; indent-tabs-mode:nil;  -*-
# vim: ai ts=4 sts=4 et sw=4
#
# photo_playback.py: Playback photo
#
# Copyright 2007, 2008 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),  
# to deal in the Software without restriction, including without limitation  
# the rights to use, copy, modify, merge, publish, distribute, sublicense,  
# and/or sell copies of the Software, and to permit persons to whom the  
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in 
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
# DEALINGS IN THE SOFTWARE.


import gtk
import os
import pygtk
import sys
import threading
import time

pygtk.require("2.0")

# Import our app's libraries
import constant
import image_finger
import thumbnail_creator
import thumbnail_view
import utils

class AutoPlay(threading.Thread):
    """background thread achieve slide show for photos"""
    def __init__(self,playback):
        self.playback = playback
        threading.Thread.__init__(self)    
        
    def start(self):
        self.playback.ThreadLock=True
        self.playback.set_fs_mode(True)
        threading.Thread.start(self)
        
    def stop(self):
        self.playback.ThreadLock=False
        
    def run(self):
        while True :
            time.sleep(self.playback.interval)
            if not self.playback.ThreadLock:
                break
            gtk.gdk.threads_enter()
            self.playback.next()
            gtk.gdk.threads_leave()

class PhotoPlayback(object):
    """PhotoPlayback is the every class for all operations associated with
    photo playback.  it will work closely with image_finger class which is
    located in ImageFinger."""
    def __init__(self, wTree, app):
        self.app = app
        self.wTree = wTree
        self.RotatedLock=False 
        self.ThreadLock=False
        self.bTrackMouse = False
        self.ResizeLock = False
        self.ResizeIncreaseBy = 0
        self.mouseTrackStr = ''
        self.CurPb = None
        self.NextPb = None
        self.PrePb = None
        self.Angle = 0
        self.imagePlayback = image_finger.image_finger()
        self.imagePlayback.modify_bg(gtk.STATE_NORMAL, constant.MediaColor['select_window_bg'])
        self.imagePlayback.connect("button_press_event", self.button_press_event, None)
        self.imagePlayback.connect("key_press_event", self.key_press_event, None)
        self.pm_sw = self.wTree.get_widget("pm_scroll_hb")
        self.pm_sw.pack_start(self.imagePlayback.get_moko())
        self.dclick = False
        self.dclick_timer = 0
        self.alloc = None
        key = "%s/photo/slideshow_interval" % (constant.GCONF_PATH)
        self.interval = app.client.get_int(key)
        self.app.register_config_callback('photo', self.__config_handler)

    def __config_handler(self, key, value):
        if key == 'slideshow_interval':
            self.interval = value.get_int()

    def get_thumbnail_view(self):
        return self.app.view['photo'].get_photo_thumbnail()
    
    def get_current_dir(self):
        return self.get_thumbnail_view().get_current_dir()

    def key_press_event(self,widget,event, data):
        """some operations use keyboard"""
        keyval=gtk.gdk.keyval_name(event.keyval)
        if keyval not in ['r','R']:
            self.RotatedLock=False
        if  keyval in ['n', 'N','g','G']:
            self.next()
        elif keyval in ['p','P']:
            self.previous()
        elif keyval in ['r','R']:
            self.imagePlayback.rotate(keyval)
        elif keyval in ['a','A']:
            self.autoplay()
        elif keyval in['s','S']:
            self.stop_auto_play()
        elif keyval in ['Escape']:
            if self.app.get_fs_mode() == True:
                self.app.set_fs_mode(False)
        elif keyval in ['i','I','o','O']:         
            self.resize(keyval)
        elif keyval in ['d','D','Delete']:
            self.delete()
        else:
            pass

    def set_fs_mode(self, full_screen):
        """go to or exit full screen in photo playback by setting fs to True or
        False"""
        if full_screen:
            self.app.set_fs_mode(True)
            self.imagePlayback.set_full_screen(True)
        else:
            self.app.set_fs_mode(False)
            self.imagePlayback.set_full_screen(False)
        
    def button_press_event(self, widget, event, data):
        if event.button == 1 and event.type == gtk.gdk.BUTTON_PRESS:
            if self.app.get_fs_mode() == True:
                self.app.set_fs_mode(False)
                if self.is_autoplay():
                    self.stop_auto_play()
            else:
                self.app.set_fs_mode(True)
            
    def display_image(self,filename):
        """this is the most important interface for this module which exposed
        to other module for viewing photo filename is the photo to be show"""
        image_file = os.path.join(self.app.view['photo'].CurrentDir,filename)
        self.imagePlayback.set_picture(image_file, self.app.get_fs_mode())
        if self.app.view['toolbar'].plugins.get_plugin_count() == 0:
            # no plugins
            return
        else:
            buttons = self.app.view['toolbar'].get_plugin_buttons()
            args = []
            args.append(image_file)
            for bn in buttons['photo']:
                bn.update_button_status(args)
         
    def next(self, media_type = thumbnail_creator.TYPE_PHOTO):
        """go to play next photo,not next media file"""
        self.app.view['toolbar'].set_photo_resize_bar_value(0)
        self.Angle = 0 
        model = self.get_thumbnail_view().model
        curdir = self.get_current_dir()
        index = self.app.view['photo'].get_index()
        if index >= len(model)-1:
            index = 0
        else:
            index += 1
        while True:
            imgfile = model[index][thumbnail_view.MDL_FILENAME].replace('\n','')
            nextImg = os.path.join(curdir,imgfile)
            if not model[index][thumbnail_view.MDL_TYPE] == media_type:
                if index >= len(model)-1:
                    index = 0
                else:
                    index += 1
            else:
                if media_type == thumbnail_creator.TYPE_PHOTO:
                    self.display_image(nextImg)
                    self.app.view['photo'].update_index(index)
                    thumbnail = self.get_thumbnail_view()
                    thumbnail.update_view_by_index(index)
                    thumbnail.set_item_selected(index)
                    break
     
    def previous(self, media_type = thumbnail_creator.TYPE_PHOTO):
        """play previous photo, not previous media file"""
        self.app.view['toolbar'].set_photo_resize_bar_value(0)
        self.Angle = 0 
        model = self.get_thumbnail_view().model
        curdir = self.get_current_dir()
        index = self.app.view['photo'].get_index()
        if index <= 0:
            index = len(model) - 1
        else:
            index -= 1

        while True:
            imgfile = model[index][thumbnail_view.MDL_FILENAME].replace('\n','')
            PreImg = os.path.join(curdir,imgfile)
            if not model[index][thumbnail_view.MDL_TYPE] == media_type:
                if index <= 0:
                    index = len(model)-1
                else:
                    index -= 1
            else:
                if media_type == thumbnail_creator.TYPE_PHOTO:
                    self.display_image(PreImg)
                    self.app.view['photo'].update_index(index)
                    thumbnail = self.get_thumbnail_view()
                    thumbnail.update_view_by_index(index)
                    thumbnail.set_item_selected(index)
                    break

    def set_interval(self,interval):
        """set the interval time for the slide show"""
        self.interval = interval

    def autoplay(self):
        """new a thread to achieve background slide show"""
        self.RotatedLock = False
        if not self.ThreadLock:
            self.newThread = AutoPlay(self)
            self.newThread.setDaemon(True)
            self.newThread.start()
            self.app.view['toolbar'].set_photo_autoplay_image_change()
            
    def init_size(self):
        self.imagePlayback.init_size()

    def stop_auto_play(self):
        """stop the background thread to stop the slide show"""
        self.RotatedLock = False
        if self.ThreadLock:
            self.newThread.stop()
            self.app.view['toolbar'].set_photo_autoplay_image_default()

    def is_autoplay(self):
        """test slide show status for ui buttons to update status"""
        return self.ThreadLock
    
    def delete(self):
        """delete displayed photo"""
        current_index = self.app.view['photo'].get_index()
        if self.get_thumbnail_view().delete(current_index):
            #TBD: select next picture (these lines don't do it)
            self.get_thumbnail_view().update_last_selection(None)
            self.app.view['photo'].update_index(current_index-1)
            #self.next()
        
    def go_thumbnail(self):
        """switch to thumbnail tab"""
        self.app.location = constant.TNV
        self.stop_auto_play()
        self.app.go_thumbnail_page()

    def rotate(self, keyval):
        """rotate the current photo 90 degrees, keyval = 'R' for clockwise, 'r' for Unclockwise"""
        return self.imagePlayback.rotate(keyval)    

    def resize(self, keyval):
        """resize photo, 'i' or 'I' short for zoom in, 'o' or 'O' for zoom out"""
        if keyval in ['i','I','o','O']:	
            ret_val = self.imagePlayback.resize(keyval)
            self.app.view['toolbar'].set_photo_resize_bar_value(self.imagePlayback.get_resize_increaseby())
            return ret_val
        else: 
            self.imagePlayback.set_pic_size(keyval)
