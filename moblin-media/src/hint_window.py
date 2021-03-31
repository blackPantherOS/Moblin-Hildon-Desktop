#!/usr/bin/python -ttu
# vim: ai ts=4 sts=4 et sw=4
#
# hint_window.py: A simple window that shows a hint to the user
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
import gtk.gdk
import gtk.glade
import os
import sys
import time
import pango

# Import our app's libraries
import utils

DEFAULT_DISPLAY_TIME = 2000  # 2 sec to show window if not overridden
TEXT_OFFSET = 40             # text offset from edge
LINE_OFFSET = 3              # border offset from edge
COLOR_BLACK  = "#000000"     # color of background
COLOR_LTGRAY = "#E2E2E2"     # color of text
COLOR_GRAY =   "#ADADAD"     # color of border line
COLOR_DKGRAY = "#404040"     # color of border line2

# Show image in a borderless window for display_time milliseconds.
# FIXME: Do we know that this works correctly?  One question I have is are we
# just counting on garbage collection not happening while the hint is being
# displayed since the object is being created and then immediately discarded in
# our current usage?

"""Note: This class is a singleton.  See remarks at bottom"""
class _HintWindow(object):
    """A borderless window with message to user"""
    def __init__(self):
        self.__timeout_id = None
        
        #Display short status window centered (e.g. 'Sorting by name')
        self.ht_window = gtk.Window(type=gtk.WINDOW_POPUP)
        self.ht_window.set_position(gtk.WIN_POS_CENTER_ALWAYS)
        self.ht_window.set_decorated(True)
        self.ht_window.set_modal(False)
        self.ht_window.set_default_size(0,0)

        self.area = gtk.DrawingArea()
        #self.area.set_size_request (0,0) #self.width, self.height)
        self.ht_window.add(self.area)
        self.area.connect('expose_event', self.expose_event)

        # set bkgd color
        self.area.modify_bg(gtk.STATE_NORMAL ,
                            gtk.gdk.color_parse(COLOR_BLACK))

        # create colors for lines and text
        self.ltgray = self.area.get_colormap().alloc_color(COLOR_LTGRAY)
        self.gray =   self.area.get_colormap().alloc_color(COLOR_GRAY)
        self.dkgray = self.area.get_colormap().alloc_color(COLOR_DKGRAY)

    def show_hint(self, msg, display_time=DEFAULT_DISPLAY_TIME):
        self.msg = msg                       #used in expose_event method
        self.timer_start(display_time)

        self.ht_window.hide()
        self.area.hide()        
        self.ht_window.show()
        self.area.show()        

    def timer_start(self, time):
        self.timer_stop()
        self.__timeout_id = gobject.timeout_add(time, self.timer_callback)
        return True

    def timer_stop(self):
        if self.__timeout_id:
            gobject.source_remove(self.__timeout_id)
            self.__timeout_id = None
            return True
        return False
    
    def timer_callback(self):
        self.ht_window.hide()
        return False  #stop timer


    '''expose_event occurs when drawarea exposed'''
    def  expose_event(self, area, event):
        # get graphics context (gc) and drawing window
        gc = event.window.new_gc()
        drawwnd = area.window

        # set text, font, get text width/height
        layout  = area.create_pango_layout(self.msg)
        layout.set_font_description(pango.FontDescription('Sans'))
        tw, th = layout.get_pixel_size()

        # size window according to size of text + offset
        w = tw + TEXT_OFFSET       #width of parent window
        h = th + TEXT_OFFSET       #height of parent window
        self.ht_window.resize(w,h)
        self.area.set_size_request(w,h)

        # draw text
        gc.foreground = self.ltgray #dkwhite
        x = (w - tw) / 2
        y = (h - th) / 2
        drawwnd.draw_layout(gc, x, y, layout=layout)

        # create simple double line border 
        o = LINE_OFFSET           #save me some typing
        gc.foreground = self.dkgray
        drawwnd.draw_line(gc, o,    o+1, w-o-2,o+1  ) #top
        drawwnd.draw_line(gc, w-o-2,o+1, w-o-2,h-o)   #right
        drawwnd.draw_line(gc, w-o-2,h-o, o,    h-o)   #bottom
        drawwnd.draw_line(gc, o,    h-o, o,    o+1  ) #left
        gc.foreground = self.gray  
        drawwnd.draw_line(gc, o+1,  o,    w-o-1,o  )  #top
        drawwnd.draw_line(gc, w-o-1,o,    w-o-1,h-o-1)#right
        drawwnd.draw_line(gc, w-o-1,h-o-1,o+1,  h-o-1)#bottom
        drawwnd.draw_line(gc, o+1,  h-o-1,o+1,  o  )  #left

        return True



"""
The HintWindow is a Singleton. (granted a little awkward in python)
This ensures callers get the same instance of the HintWindow and
if they sort repeatedly a bunch of new windows aren't created.
It also prevents two windows from showing at the same time.
"""

_hintwindow = _HintWindow()
def HintWindow (): return _hintwindow
