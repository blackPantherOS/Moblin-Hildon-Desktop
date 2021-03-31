#!/usr/bin/python -ttu
# -*- mode:python; tab-width:4; indent-tabs-mode:nil;  -*-
# vim: ai ts=4 sts=4 et sw=4
#
# thumbnail_view.py: Manage the Thumbnail View
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


import gettext
import Image
import ImageDraw
import gnome
import gnome.ui
import gtk
import md5
import os
import tempfile
import utils

# Import our libraries
import constant
import media_service

_ = gettext.lgettext

# Constants
KIBIBYTE = 1024
MIBIBYTE = 1024 * KIBIBYTE
GIBIBYTE = 1024 * MIBIBYTE

class ThumbBase(object):
    """Base class of thumbnail for all modules"""

    def __init__(self):
        self.align_len = 40

    def set_align_len(self, length):
        self.align_len = length
        
    def calculate_size(self, bytes):
        """Get human readable size
        @ bytes is the size of file"""
        ret = ""
        if bytes < KIBIBYTE:
            ret = "%s bytes" % bytes
        elif bytes < MIBIBYTE:
            ret = "%.0f KB" % (bytes / KIBIBYTE / 1.0)
        elif bytes < GIBIBYTE:
            ret = "%.0f MB" % (bytes / MIBIBYTE / 1.0)
        else:
            ret = "%.0f GB" % (bytes / GIBIBYTE / 1.0)
        return ret
    
    def get_file_detail_info(self,filepath):
        """Get size and mtime info of given file
        @ filepath is the abs path of given file"""
        DeInfo = os.stat(filepath)
        return  DeInfo[-2],DeInfo[-4] 
    
    def align_name_with_thumb(self, image_file):
        new_image_file = ''
        if len(image_file) < self.align_len:
            return image_file
        i = 0
        while i < len(image_file):
            if i-self.align_len > len(image_file):
                j = i
            else:
                j = i + self.align_len
            new_image_file += image_file[i:j]+'\n'
            i = j
            
        return new_image_file   

    def thumb_filter(self,root,ext,isHide=False):
        """Filter out interested  media file and decide type of the interested ones
        Return values:
          1 : photo type 
          2 : audio type
          3 : video type
        Just check by the extension, not turely to read the media file content"""
        filepath=os.path.join(root,ext)
        pathlist = os.path.split(root)
        filefolder = pathlist[1]
        NewRoot, NewExt = os.path.splitext(filepath)
        NewExt = NewExt.replace('.','').lower()
        if NewExt in constant.MediaType['photo'] and filefolder == 'photo':
            return 1
        elif NewExt in constant.MediaType['audio'] and filefolder == 'audio':
            return 2
        elif  NewExt in  constant.MediaType['video'] and filefolder == 'video':
            return 3  
        return 0

    def select_home_size(self, is_sel_largest=True,
                         thumbs_subdir=constant.THUMBNAIL_DIR):
        config_dir = constant.MediaHomePath
        thumb_dir = constant.THUMBNAIL_DIR
        retval = ''
        if is_sel_largest:
            retval = os.path.join(thumbs_dir, 'largest')
        else: 
            retval = os.path.join(thumbs_dir, 'normal')  
        return retval

    def get_extension(self,uri):
        root,ext=os.path.splitext(uri)
        return ext.replace('.','').lower()

    def trash(self,src):
        root,filename = os.path.split(src)
        title = _(constant.MSG_CONFIRM_DELETE_TITLE)
        msg = _(constant.MSG_CONFIRM_DELETE_QUESTION) % (filename)
        if gtk.RESPONSE_NO == utils.confirm_dialog(title, msg):
            return False
        os.unlink(src)
        return True
    
    def add_rectangle_for_pixbuf(self, pb, color=[255, 255, 255]):
        """Add a wider border to the selected thumbnail's pixbuf in 'color'
        @pb, the pixbuf to be deal with
        @color, the border color"""
        border_color = color
        pb_array = pb.get_pixels_array()
        try:
            #add width edge
            for i in range(len(pb_array[0])):
                for J in range(constant.BORDER):
                    pb_array[J][i] = (border_color)
                    pb_array[len(pb_array)-1-J][i] = (border_color)
            #add height edge
            for i in range(len(pb_array)):
                for j in range(constant.BORDER):
                    pb_array[i][j] = (border_color)
                    pb_array[i][len(pb_array[0])-1-j] = (border_color)
        except:
            pass
