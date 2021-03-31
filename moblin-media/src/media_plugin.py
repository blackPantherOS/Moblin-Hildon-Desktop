#!/usr/bin/python -ttu
# vim: ai ts=4 sts=4 et sw=4
#
#  media_plugin.py: Manage Plugins
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


import constant
import utils
import sys, os, os.path, string
import pygtk, gtk
import subprocess
import ConfigParser

class Plugin(object):
    def __init__(self, filename, section='Media Plugin'):
        parser = ConfigParser.ConfigParser()
        parser.read(filename)
        self.name = parser.get(section, 'Name')
        self.cmd = parser.get(section, 'Exec')
        self.icon = parser.get(section, 'Icon')
        self.ext = []
        try:
            self.hint = parser.get(section, 'Hint')
        except:
            self.hint = ''
        try:
            tmp = parser.get(section, 'Ext')
            for type in string.split(tmp, ','):
                type = type.strip().lower()             
                try:
                    type = type.encode("ascii")
                except:
                    continue
                self.ext.append(type)
        except:
            pass

    def __str__(self):
        return ("<name='%s' hint='%s' exec='%s' icon='%s' ext='%s'>" %
                (self.name, self.hint, self.cmd, self.icon, self.ext))

class PluginButton(gtk.Button):
    def __init__(self):
        gtk.Button.__init__(self)
        self.set_relief(gtk.RELIEF_NONE)
            
    def connect_to_plugin(self, plugin):
        self.plugin = plugin
        self.icon = self.set_button_image(plugin.icon)
        self.set_tooltip(plugin.hint)
        self.pack(self.icon)
        self.args = []
        self.command = []
        self.connect_signal()
        
    def set_button_image(self, image):
        icon = gtk.Image()
        if os.path.isfile(image) == False:
            icon.set_from_stock(gtk.STOCK_NO, gtk.ICON_SIZE_SMALL_TOOLBAR)
        else:
            icon.set_from_file(image)
        return icon
    
    def pack(self, image):
        box = gtk.VBox()
        box.pack_start(image, True, True)
        self.add(box)
        
    def set_tooltip(self, tip):
        """
        add hint
        """
        self.tooltip = gtk.Tooltips()
        self.tooltip.set_tip(self, tip)
    
    def connect_signal(self):
        self.connect('clicked', self.launch, self)
      
    def launch(self, button, plugin):
        # make a command sequence
        self.make_cmd(self.plugin.cmd, self.args)
        
        try:
            # it's a async call of another process
            retpid = subprocess.Popen(self.command).pid
        except OSError, e:
            pass

     
    def make_cmd(self, exe, args):
        self.command = []
        self.command.append(exe)
        if args.count == 0:
            # enough for launch
            return
        else:
            # args should only contain files with proper extentions
            args = filter(self.matchtype, args)
            for arg in args:
                self.command.append(arg)

    def update_button_status(self, args):
        if args != None:
            self.args = args
            
        final_args = filter(self.matchtype, self.args)
        if len(self.args) == 0 or len(final_args) == 0 or len(self.args) != len(final_args):
            self.hide_bn()
        else:
            self.show_bn()
    
    def matchtype(self, args):
        return os.path.splitext(args)[1][1:].lower() in self.plugin.ext
            
    def hide_bn(self):
        self.hide()
        
    def show_bn(self):
        self.show_all()
        

class MediaPlugin(object):
    """
    Manage Plugins for Mobile Player
    """
    def __init__(self):
        self.plugin_list = []
        if os.path.isdir(constant.MediaPluginsDir):
            for f in os.listdir(constant.MediaPluginsDir):
                try:
                    self.plugin_list.append(
                        Plugin(os.path.join(constant.MediaPluginsDir, f)))
                except:
                    continue

    def get_plugin_count(self):
        return self.plugin_list.count
    
    def get_plugin_list(self):
        return self.plugin_list


		
