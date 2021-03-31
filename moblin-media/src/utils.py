#!/usr/bin/python -ttu
# -*- mode:python; tab-width:4; indent-tabs-mode:nil;  -*-
# vim: ai ts=4 sts=4 et sw=4
#
# utils.py: This module define some common funcions
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
import gtk
import gtk.gdk
import Image
import md5
import os
import sys
import re
from optparse import OptionParser
from stat import *

import constant

_ = gettext.lgettext

def convertSecondsToHoursMinutesSeconds(sec):
    """Given a value of seconds, return back the corresponding hours, minutes,
    and seconds"""
    hours = int(sec / 3600)
    mins = int((sec % 3600) / 60)
    secs = int(sec % 60)
    return (hours, mins, secs)

def formatTime(hours, mins, secs):
    """Return back a formatted time string, given hours, minutes, and
    seconds"""
    if not hours:
        time_str = '%d:%02d' % (mins, secs)
    else:
        time_str = '%d:%02d:%02d' % (hours, mins, secs)
    return time_str

def convertSecondsToString(seconds):
    """Given seconds, return back a formatted time string"""
    hours, mins, secs = convertSecondsToHoursMinutesSeconds(seconds)
    return formatTime(hours, mins, secs)

def parse_command_line():
    parser = OptionParser(add_help_option=False)
    parser.add_option("--disable-hildon",
        help="Do not use the hildon application framework",
        dest="disable_hildon", action="store_true", default=False)
    parser.add_option("--uri", dest="uri", help="Media file URI")
    parser.add_option("--mode",
        dest="mode", help="Media type: music, video or photo",
        default="photo")
    parser.add_option("--debug", help="Enable debugging messages",
        dest="enable_debug", action="store_true", default=False)
    parser.add_option("--help", help="show this help message and exit",
                    dest = "help", action="store_true", default=False)
    (options, args) = parser.parse_args()
    # validate options
    if not options and not args:
        # If nothing on command line, then return
        return options, args
    if options.help:
        parser.print_help()
        sys.exit(0)
    if options.mode:
        if options.mode not in ("music", "photo", "video"):
            parser.error("no such media mode, only support music, video and photo")
    return options, args

def get_media_type(filename):
    """Determine media type based on file extension"""
    if not filename:
        return None
    base,ext = os.path.splitext(filename)
    ext = ext.lower().replace('.', '')
    if ext in constant.MediaType['audio']:
        return 'audio'
    elif ext in constant.MediaType['video']:
        return 'video'
    elif ext in constant.MediaType['photo']:
        return 'photo'
    else:
        return None

def on_err_dialog_response_exit(dialog, response_id):
    sys.exit()
    
def error_msg(msg, exit=False):
    """Display an error message dialog.  Display the msg string, if exit is set
    then setup for being able to exit"""
    dialog = gtk.MessageDialog(None,
                               gtk.DIALOG_MODAL,
                               gtk.MESSAGE_ERROR,
                               gtk.BUTTONS_CLOSE,
                               _(msg))
    dialog.set_title(_(constant.MSG_ERROR_TITLE))
    if exit:
        dialog.connect("response", on_err_dialog_response_exit )
    dialog.show()
    dialog.run()
    dialog.destroy()

def info_dialog (title, msg):
    """Display dialog with OK button
    http://www.pygtk.org/docs/pygtk/class-gtkmessagedialog.html
    """
    dialog = gtk.MessageDialog(None,
                               gtk.DIALOG_MODAL,
                               gtk.MESSAGE_INFO,
                               gtk.BUTTONS_OK,
                               _(msg));
    dialog.set_title(_(title))
    dialog.run()
    dialog.destroy()

def confirm_dialog (title, msg):
    """Display confirmation dialog (yes/no)"""
    dialog = gtk.MessageDialog(None,
                               gtk.DIALOG_MODAL,
                               gtk.MESSAGE_QUESTION,
                               gtk.BUTTONS_YES_NO,
                               _(msg));
    dialog.set_title(_(title))
    retval = dialog.run()
    dialog.destroy()
    return retval

def getScreenSize():
    """Return back a tuple (width, height) aka (x, y)"""
    x = gtk.gdk.screen_width()
    y = gtk.gdk.screen_height()
    return (x, y)

def get_thumbnail_path(thumbnail_dir, full_image_path):
    info = os.stat(full_image_path)
    mtime = info[ST_MTIME]
    filesize = info[ST_SIZE]
    tmp = os.path.join(thumbnail_dir, full_image_path)
    root, ext = os.path.splitext(tmp)
    md5dest = md5.new("%s%s%s" % (tmp, mtime, filesize)).hexdigest() + \
              tmp.replace('/','\n')
    return os.path.join(thumbnail_dir, "%s.jpg" % (md5dest))

def create_thumbnail_image(thumbnail_dir, full_image_path, canvas, target=None):
    if target:
        output_name = get_thumbnail_path(thumbnail_dir, target)
    else:
        output_name = get_thumbnail_path(thumbnail_dir, full_image_path)
    if os.path.exists(output_name):
        return True
    name, mtype = os.path.splitext(full_image_path)
    mtype = mtype.replace(".", "")

    # open a 200x200 canvas img to put thumbnail on
    thumb_bkgd = canvas.copy()
    bkgd_width = thumb_bkgd.size[0]
    bkgd_height = thumb_bkgd.size[1]

    try:
        photo = Image.open(full_image_path)
    except:
        try:
            photo = Image.open(constant.InvalidImageFile)
        except:
            return False
    photo.thumbnail((bkgd_width, bkgd_height))
    
    # Place into box so it is centered, regardless of the
    # dimensions and/or orientation
    h_start = bkgd_width - int((photo.size[0] + bkgd_width)/2)
    v_start = bkgd_height - int((photo.size[1] + bkgd_height)/2)
    thumb_bkgd.paste(photo, (h_start, v_start))
    # Save the image data to the target thumbnail file
    thumb_bkgd.save(output_name, "JPEG")
    return True

def get_widget_tree(file, root):
    return gtk.glade.XML(os.path.join(constant.MediaAppPath, file), root)

def is_valid_url (url):
    # official regexp from  http://gbiv.com/protocols/uri/rfc/rfc3986.html#regexp
    # p = re.compile(r'^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\?([^#]*))?(#(.*))?')
    # We require <protocol>://domain, so modify above regexp a little
    p = re.compile(r'^([^:/?#]+)://[^/?#]*([^?#]*)(\?([^#]*))?(#(.*))?')
    if not p.match(url):
        return False
    return True
