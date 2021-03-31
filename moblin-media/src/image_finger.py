#!/usr/bin/python -ttu
# vim: ai ts=4 sts=4 et sw=4
#
# image_finger.py: image class that can do panning, resize, rotate and etc
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


import gc
import gobject
import gtk
import moko
import os
import pygtk
import sys

pygtk.require("2.0")

# Import our app's libraries
import constant

class image_finger(gtk.EventBox):
    INDICATOR_AUTO, INDICATOR_SHOW, INDICATOR_HIDE = range(0,3)
    def __init__(self, panning = True):
        gtk.EventBox.__init__(self)
        self.set_above_child(False)
        self.show()
        self.image = gtk.Image()
        self.image.show()
        self.add(self.image)
        self.panning = panning
        self.valid_image = False
        if panning:
            self.moko_widget = moko.FingerScroll()
            self.moko_widget.show()
            self.moko_widget.set_property('spring_color',0x000000)
            self.moko_widget.add_with_viewport(self)
        self.image_file = None
        self.image_format = None
        self.image_pb = None
        self.fscreen = False
        self.resize_increaseby = 0
        self.area_size = None

    def get_resize_increaseby(self):
        return self.resize_increaseby        

    def get_moko(self):
        if self.panning :
            return self.moko_widget

    def get_image(self):
        return self.image

    def set_pic_size(self,value):
        if value <0 or value >30:
           return
        self.resize_increaseby = value
        return self._display_image_fit()

    def set_panning_vindicator(self, mode):
        if (self.panning):
            self.moko_widget.set_property("vindicator_mode", mode)
    
    def set_panning_hindicator(self, mode):
        if (self.panning):
            self.moko_widget.set_property("hindicator_mode", mode)

    def set_picture(self, image_file, fscreen = False):
        self.valid_image = False
        if not os.path.isfile(image_file):
            return False
        try:
            self.image_pb = gtk.gdk.pixbuf_new_from_file(image_file)
            self.valid_image = True
            self.image_format = gtk.gdk.pixbuf_get_file_info(image_file)[0]['name']
        except:
            self.image_pb = gtk.gdk.pixbuf_new_from_file(constant.InvalidImageFile)
            self.image_format = gtk.gdk.pixbuf_get_file_info(constant.InvalidImageFile)[0]['name']
        self.image_file = image_file
        self.fscreen = fscreen
        self.resize_increaseby = 0
        self._display_image_fit()
        if self.valid_image == False:
            return False
        return True
      
    def set_full_screen(self,fullscreen):
        if (self.image_file == None):
            return False
        self.fscreen = fullscreen
        return self._display_image_fit()

    def init_size(self):
        self.area_size = self.get_allocation()

    def resize(self, keyval):
        if (self.image_file == None):
            return False
        if keyval in ['i','I']:
            self.resize_increaseby += 10
        if keyval in ['o','O']:
            self.resize_increaseby -= 10
        if self.resize_increaseby < 0:
            self.resize_increaseby = 0
        if self.resize_increaseby >30:
            self.resize_increaseby = 30
        return self._display_image_fit()
        
    def rotate(self, keyval):
        if (self.image_file == None):
            return False
        return self._display_image_fit(keyval)

    def _display_image_fit(self, rotate=None):
        if self.fscreen:
            size = gtk.gdk.Rectangle(0,0,0,0)
            size.width = self.image.get_screen().get_width()
            size.height = self.image.get_screen().get_height()
        else:
            if (self.area_size != None):
                size = self.area_size
            else:
                try:
                    size = self.image.get_parent().get_parent().get_allocation()
                except:
                    return
        # We only want to rotate the image if it is a valid image, also we only
        # want to save if it is a valid image
        if self.valid_image:
            if rotate == 'r':
                self.image_pb = self.image_pb.rotate_simple(gtk.gdk.PIXBUF_ROTATE_COUNTERCLOCKWISE)
            if rotate == 'R':
                self.image_pb = self.image_pb.rotate_simple(gtk.gdk.PIXBUF_ROTATE_CLOCKWISE)
            # If we rotated the image, need to save it back to disk, before we do
            # the image scaling for display, otherwise we will lose the image
            # dimensions
            if rotate in ['R','r']:
                extra_param = {}
                if self.image_format == "jpeg":
                    extra_param = {"quality":"100"}
                self.image_pb.save(self.image_file, self.image_format, extra_param)
        width = self.image_pb.get_width()
        height = self.image_pb.get_height()
        IncreaseBy = self.resize_increaseby
        image_pb = self.image_pb
        if IncreaseBy:
            if ((width > size.width) or (height > size.height)):
                if(width * size.height > height * size.width):
                    image_pb = self.image_pb.scale_simple( int( (1+IncreaseBy*0.05)*size.width ), int( (1+IncreaseBy*0.05)*height*size.width/width ), gtk.gdk.INTERP_BILINEAR )
                else:
                    image_pb = self.image_pb.scale_simple( int ((1+IncreaseBy*0.05)*width*size.height/height ), int ((1+IncreaseBy*0.05)*size.height ), gtk.gdk.INTERP_BILINEAR )
            else :
                image_pb = self.image_pb.scale_simple( int ((1+IncreaseBy*0.05)*width*size.height/height ), int ((1+IncreaseBy*0.05)*size.height ), gtk.gdk.INTERP_BILINEAR )
        else:
            if ((width > size.width) or (height > size.height)):
                if(width * size.height > height * size.width):
                    image_pb = self.image_pb.scale_simple( size.width , height*size.width/width, gtk.gdk.INTERP_BILINEAR )
                else:
                    image_pb = self.image_pb.scale_simple( width*size.height/height, size.height, gtk.gdk.INTERP_BILINEAR )
        self.image.set_from_pixbuf(image_pb)
        self.image.show()
        gc.collect()
