#!/usr/bin/python -ttu
# vim: ai ts=4 sts=4 et sw=4
#
# video_view.py: Manage Video View
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


import gobject
import gtk
import gtk.gdk
import gtk.glade
import os
import pygtk
import sys

import constant
import utils
import thumbnail_view

class VideoView(object):
    """A class which manage video view. """
    state_thumbnail, state_playback, state_share, state_default, state_filelist = range(0, 5)
    def __init__(self, app):
        """init function"""
        self.app = app
        self.wTree = utils.get_widget_tree(constant.MAIN_GLADE_FILE,
                                           root='mp_video_nbk')
        dic = {
               "on_vm_playback_view_eb_button_release_event": self.button_release_handler,
        }
        self.wTree.signal_autoconnect(dic)
        self.view_nbk = self.wTree.get_widget('mp_video_nbk')
        self.view_nbk.set_show_tabs(False)
        self.state_current = self.state_thumbnail
        # if first launch
        self.is_first_launch = True
        # init GUI variables
        self.video_playback_view = self.wTree.get_widget('wm_vm_playback_wd_vb')
        self.playback_surface = self.wTree.get_widget('wm_vm_drawarea_da')
        self.playback_surface.modify_bg(gtk.STATE_NORMAL,
                                        gtk.gdk.Color(0,0,0,0))
        self.playback_surface.connect('size-allocate', self.size_allocate)

        # for thumbnail
        thumbnail = thumbnail_view.ThumbnailSupport(app, 'video')
        thumbnail_sw = self.wTree.get_widget("vm_thumbnail_sw")
        thumbnail_sw.pack_start(thumbnail.get_scroll_widget())
        thumbnail.initialize_view()
        self.thumbnail = thumbnail
        self.item_index = -1
        self.fs_mode = False
        self.init_ui()

    def size_allocate(self, widget, event):
        if self.app.get_fs_mode():
            self.app.service.ResizeWindow(constant.VSZ_FULL_SCREEN)
        else:
            self.app.service.ResizeWindow(constant.VSZ_FULL_WINDOW)

    def go_thumbnail(self):
        self.app.location = constant.TNV
        self.app.go_thumbnail_page()
  
    def get_video_thumbnail(self):
        return self.thumbnail

    def update_index(self, index):
        self.item_index = index

    def get_index(self):
        return self.item_index


    def init_ui(self):
        self.view_nbk.set_current_page(self.state_current)
        widget = self.wTree.get_widget('vm_thumbnail_eb')
        widget.modify_bg(gtk.STATE_NORMAL, constant.MediaColor['select_window_bg'])
        widget = self.wTree.get_widget('vm_filelist_eb')
        widget.modify_bg(gtk.STATE_NORMAL, constant.MediaColor['select_window_bg'])
        widget = self.wTree.get_widget('vm_default_view_eb')
        widget.modify_bg(gtk.STATE_NORMAL, constant.MediaColor['select_window_bg'])
        widget = self.wTree.get_widget('vm_default_view_lb')
        widget.modify_fg(gtk.STATE_NORMAL, constant.MediaColor['select_label_fg'])
        widget = self.wTree.get_widget('vm_playback_view_eb')
        widget.modify_fg(gtk.STATE_NORMAL, constant.MediaColor['select_label_fg'])  

    def update_ui(self):
        self.view_nbk.set_current_page(self.state_current)

    def update_state_thumbnail(self):
        self.state_current = self.state_thumbnail
        self.view_nbk.set_current_page(self.state_current)
    
    def update_state_filelist(self):
        self.state_current = self.state_filelist
        self.view_nbk.set_current_page(self.state_current)

    def update_state_playback(self):
        self.state_current = self.state_playback
        self.view_nbk.set_current_page(self.state_current)

    def update_state_share(self):
        self.state_current = self.state_share
        self.view_nbk.set_current_page(self.state_current)

    def update_state_default(self):
        self.state_current = self.state_default
        self.view_nbk.set_current_page(self.state_current)

    def get_render_surface(self):
        self.playback_surface = self.wTree.get_widget('wm_vm_drawarea_da')
        return self.playback_surface

    def is_first_load(self):
        return (self.is_first_launch == True)

    def mark_default_flag(self):
        self.is_first_launch = False
        self.update_ui()

    def next(self,media_type=3):
        model = self.thumbnail.model
        curdir = self.thumbnail.get_current_dir()
        index = self.app.view['video'].get_index()
        if index >= len(model)-1:
            index = 0
        else:
            index += 1
        while True:
            imgfile = model[index][0].replace('\n','')
            nextImg = os.path.join(curdir,imgfile)
            if not model[index][2] == media_type:
                if index >= len(model)-1:
                    index = 0
                else:
                    index += 1
            else:
                if media_type == 3:
                    self.app.view['toolbar'].open_media_file('file://'+os.path.join(curdir,imgfile.replace('\n','')))
                    self.update_index(index)
                    thumbnail = self.get_video_thumbnail()
                    thumbnail.update_view_by_index(index)
                    thumbnail.set_item_selected(index)
                    break

    def pre(self,media_type=3):
        model = self.thumbnail.model
        curdir = self.thumbnail.get_current_dir()
        index = self.app.view['video'].get_index()
        if index <= 0:
            index = len(model) - 1
        else:
            index -= 1
        while True:
            imgfile = model[index][0].replace('\n','')
            PreImg = os.path.join(curdir,imgfile)
            if not model[index][2] == media_type:
                if index <= 0:
                    index = len(model)-1
                else:
                    index -= 1
            else:
                if media_type == 3:
                    self.app.view['toolbar'].open_media_file('file://'+os.path.join(curdir,imgfile.replace('\n','')))
                    self.update_index(index)
                    thumbnail = self.get_video_thumbnail()
                    thumbnail.update_view_by_index(index)
                    thumbnail.set_item_selected(index)
                    break

    def button_release_handler(self, widget, event):
        if event.button == 1 and event.type == gtk.gdk.BUTTON_RELEASE:
            if self.app.get_fs_mode() == True:
                self.app.set_fs_mode(False)
            else:
                self.app.set_fs_mode(True)
        
    def get_thumb_fs_mode (self):
        return self.thumbnail.get_fs_mode()
    def update_media_info(self, key, value ):                 
        if key == "duration":
            if value == -1:
                value = 0
            self.app.media_file_length = value/1000
            self.app.view['toolbar'].playback_set_seekbar_range(0, self.app.media_file_length, 'video')
            self.app.view['toolbar'].playback_set_seekbar_increments(1, 0, 'video')
