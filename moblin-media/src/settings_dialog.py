#!/usr/bin/python -ttu
# vim: ai ts=4 sts=4 et sw=4
#
# settings_dialog.py: Settings dialog
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


import gconf
import gtk
import gtk.glade
import os
import sys

import constant
import utils

class SettingsDialog(object):
    def __init__(self, mode):
        self.__mode = mode
        self.__base_path = "%s/%s" % (constant.GCONF_PATH, self.__mode)
        self.__client =  gconf.client_get_default()
        self.__tree = utils.get_widget_tree("settings_dialog.glade",
                                            "setting_win_top_box")
        box = self.__tree.get_widget("setting_win_top_box")
        tmp = utils.get_widget_tree("settings_dialog.glade", "Setting_box")
        self.__dialog = tmp.get_widget("Setting_box")
        tmp.get_widget("setting_box_to_pack").pack_start(box)
        tmp = self.__tree.get_widget('Default_Btn')
        tmp.connect('button-release-event', self.__on_default_button)
        tmp = self.__tree.get_widget('OK_Btn')
        tmp.connect('button-release-event', self.__on_ok_button)
        tmp = self.__tree.get_widget('Cancel_Btn')
        tmp.connect('button-release-event',self.__on_cancel_button)
        self.__checkboxes = {}
        for name in ['show_name_label', 'show_ext_label', 'show_size_label',
                     'show_fullscreen_button', 'show_sort_button',
                     'show_slideshow_button', 'show_rotate_buttons',
                     'show_resizethumbs_slider', 'show_delete_button',
                     'show_properties_button']:
            self.__checkboxes[name] = self.__tree.get_widget(name)
        self.__interval = self.__tree.get_widget('slideshow_interval')
        self.__sample_label_text= self.__tree.get_widget('label_sample_text')
        self.__sample_size_text= self.__tree.get_widget('label_sample_size')
        self.__checkboxes['show_name_label'].connect('toggled',
                                                     self.__on_show_name_label)
        self.__checkboxes['show_ext_label'].connect('toggled',
                                                    self.__on_show_ext_label)
        self.__checkboxes['show_size_label'].connect('toggled',
                                                     self.__on_show_size_label)
        tabs = self.__tree.get_widget('Slide_Show_Tab')
        box = self.__tree.get_widget('slide_show_vbox')            
        if self.__mode == constant.MODE_VIDEO:
            self.__checkboxes['show_rotate_buttons'].set_child_visible(False)
            self.__checkboxes['show_rotate_buttons'].set_sensitive(False)
            tabs.set_child_visible(False)
            tabs.set_sensitive(False)                
            box.set_sensitive(False)
            box.hide()
        elif self.__mode == constant.MODE_PHOTO:
            self.__checkboxes['show_rotate_buttons'].set_child_visible(True)
            self.__checkboxes['show_rotate_buttons'].set_sensitive(True)
            tabs.set_child_visible(True)
            tabs.set_sensitive(True)
            box.set_sensitive(True)
            box.show()
            self.__interval.select_region(0, -1)
            self.__interval.set_value(self.__get_int('slideshow_interval'))
        for key in self.__checkboxes:
            self.__checkboxes[key].set_active(self.__get_bool(key))
        self.__on_show_size_label(self.__checkboxes['show_size_label'])
        self.__on_show_ext_label(self.__checkboxes['show_ext_label'])
        self.__on_show_name_label(self.__checkboxes['show_name_label'])

    def __get_bool(self, key):
        return self.__client.get_bool("%s/%s" % (self.__base_path, key))

    def __set_bool(self, key, value):
        self.__client.set_bool("%s/%s" % (self.__base_path, key), value)
        
    def __get_int(self, key):
        return self.__client.get_int("%s/%s" % (self.__base_path, key))

    def __set_int(self, key, value):
        self.__client.set_int("%s/%s" % (self.__base_path, key), value)
        
    def __get_default_value(self, key):
        return self.__client.get_default_from_schema("%s/%s" % (self.__base_path, key))
    
    def __get_default_bool(self, key):
        return self.__get_default_value(key).get_bool()

    def __get_default_int(self, key):
        return self.__get_default_value(key).get_int()
    
    def __on_show_name_label(self, widget):
        if widget.get_active():
            if self.__checkboxes['show_ext_label'].get_active():
                self.__sample_label_text.set_text('PhotoSample.jpg')
            else:
                self.__sample_label_text.set_text('PhotoSample')
        else:
            self.__checkboxes['show_ext_label'].set_active(False)
            if self.__checkboxes['show_ext_label'].get_active():
                self.__sample_label_text.set_text('.jpg')      
            else :
                self.__sample_label_text.set_text('')
        
    def __on_show_ext_label(self, widget):
        if widget.get_active():
            self.__checkboxes['show_name_label'].set_active(True)
            if self.__checkboxes['show_name_label'].get_active():
                self.__sample_label_text.set_text('PhotoSample.jpg')
            else:
                self.__sample_label_text.set_text('.jpg')
        else:
            if self.__checkboxes['show_name_label'].get_active():
                self.__sample_label_text.set_text('PhotoSample')
            else:
                self.__sample_label_text.set_text('')

    def __on_show_size_label(self, widget):
        if widget.get_active():
            self.__sample_size_text.set_text('(1.4MB)')
        else :
            self.__sample_size_text.set_text('')
            
    def __on_default_button(self, widget, event):
        for key in self.__checkboxes:
            widget = self.__checkboxes[key]
            if widget.state != gtk.STATE_INSENSITIVE:
                widget.set_active(self.__get_default_bool(key))
        if self.__interval.state != gtk.STATE_INSENSITIVE:
            self.__interval.set_value(self.__get_default_int('slideshow_interval'))

    def __on_ok_button(self, widget, event):
        for key in self.__checkboxes:
            widget = self.__checkboxes[key]
            if widget.state != gtk.STATE_INSENSITIVE:
                self.__set_bool(key, widget.get_active())
        self.__set_int('slideshow_interval',
                       int(self.__interval.get_value()))
        self.__dialog.destroy()
        
    def __on_cancel_button(self, widget, event):
        self.__dialog.destroy()

        
class VideoSettingsDialog(SettingsDialog):
    def __init__(self, settings):
        SettingsDialog.__init__(self, mode = 'video')


class PhotoSettingsDialog(SettingsDialog):
    def __init__(self, settings):
        SettingsDialog.__init__(self, mode = 'photo')


if __name__ == '__main__':
    mode = sys.argv[1]
    if mode == 'video':
        dialog = VideoSettingsDialog(mode)
    elif mode == 'photo':
        dialog = PhotoSettingsDialog(mode)
    else:
        print "USAGE: %s video|photo"
        exit -1

    gtk.main()
