#!/usr/bin/python -ttu
# vim: ai ts=4 sts=4 et sw=4
#
# photo_view.py: Manage the Photo View
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
import photo_playback
import thumbnail_view

class PhotoView(object):
    """A class which manages the photo view."""
    state_thumbnail, state_playback, state_share, state_default, state_filelist = range(0, 5)
    def __init__(self, app):
        self.app = app
        self.wTree = utils.get_widget_tree(constant.MAIN_GLADE_FILE,
                                           root='mp_photo_nbk')
        dic = {"on_mp_photo_nbk_key_press_event": self.dispatch_key_event,}
        self.wTree.signal_autoconnect(dic)
        self.view_nbk = self.wTree.get_widget('mp_photo_nbk')
        self.view_nbk.set_show_tabs(False)
        self.state_current = self.state_thumbnail
        self.is_first_launch = True
        # for thumbnail
        self.CurrentDir = self.app.get_media_directory()
        thumbnail = thumbnail_view.ThumbnailSupport(app, 'photo')
        widget = self.wTree.get_widget("pm_thumbnail_sw")
        widget.pack_start(thumbnail.get_scroll_widget())
        thumbnail.initialize_view()
        self.thumbnail = thumbnail
        # for playback
        self.photoplayback = photo_playback.PhotoPlayback(self.wTree,self.app)
        self.item_index = -1
        self.init_ui()
        self.alloc = None

    def get_photo_playback(self):
        return self.photoplayback
        
    def get_photo_thumbnail(self):
        return self.thumbnail

    def update_index(self, index):
        self.item_index = index

    def get_index(self):
        return self.item_index
        
    def set_cur_picture(self, filename):
        self.photoplayback.init_size()
        return self.photoplayback.display_image(filename)

    def init_ui(self):
        self.view_nbk.set_current_page(self.state_current)
        widget = self.wTree.get_widget('pm_thumbnail_eb')
        widget.modify_bg(gtk.STATE_NORMAL, constant.MediaColor['select_window_bg'])
        widget = self.wTree.get_widget('pm_filelist_eb')
        widget.modify_bg(gtk.STATE_NORMAL, constant.MediaColor['select_window_bg'])
        widget = self.wTree.get_widget('pm_default_view_eb')
        widget.modify_bg(gtk.STATE_NORMAL, constant.MediaColor['select_window_bg'])
        widget = self.wTree.get_widget('pm_default_view_lb')
        widget.modify_fg(gtk.STATE_NORMAL, constant.MediaColor['select_label_fg'])

    def update_ui(self):
        self.update_status(self.state_current)
	
    def update_state_thumbnail(self):
        self.state_current = self.state_thumbnail
        self.photoplayback.stop_auto_play()
        self.view_nbk.set_current_page(self.state_current)
    
    def update_state_filelist(self):
        self.state_current = self.state_filelist
        self.photoplayback.stop_auto_play()
        self.view_nbk.set_current_page(self.state_current)

    def update_state_playback(self):
        self.state_current = self.state_playback
        self.view_nbk.set_current_page(self.state_current)
        self.app.view['toolbar'].set_photo_resize_bar_value(0)

    def update_state_share(self):
        self.state_current = self.state_share
        self.view_nbk.set_current_page(self.state_current)
        
    def update_state_default(self):
        self.state_current = self.state_default
        self.view_nbk.set_current_page(self.state_current)
        
    def is_first_load(self):
        return (self.is_first_launch == True)

    def mark_default_flag(self):
        self.is_first_launch = False
        self.update_ui()

    def set_fs_mode(self, on):
        pass

    def go_photo_playback_page(self):
        self.app.main_notebook.set_current_page(2)
        self.app.view['toolbar'].change_mode('photo','photo')
        self.update_state_playback()

    def set_cur_dir(self, filename):
        if os.path.isdir(filename):
	        self.CurrentDir = filename
	        return
        head, tail = os.path.split(filename)
        self.CurrentDir = head
	
    def update_status(self, status):
	    if status == self.state_default:
	        self.update_state_default()
	    elif status == self.state_playback:
	        self.update_state_playback()
	    elif status == self.state_thumbnail:
	        self.update_state_thumbnail()
	    elif status == self.state_filelist:
	        self.update_state_filelist()
	    elif status == self.state_share:
	        self.update_state_share()
	    else:
	        pass
	
    def dispatch_key_event(self,widget,event):
	    keyval=gtk.gdk.keyval_name(event.keyval)
	    if self.state_current==self.state_playback:
	        self.photoplayback.key_press_event(widget,event)
	    elif self.state_current==self.state_thumbnail:
	        self.thumbnail.key_press_event(widget,event)
