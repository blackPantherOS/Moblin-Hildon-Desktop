#!/usr/bin/python -tt
# vim: ai ts=4 sts=4 et sw=4
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


import os
import re
import shutil
import sys
import tempfile
import unittest

DESTDIR = os.getenv('DESTDIR')
if DESTDIR:
    print "Not running unittest for build environment"
    sys.exit(0)
sys.path.insert(0, '/usr/share/moblin-media')

# Try importing all of our python modules
import constant
import hint_window
import image_finger
import media_info
import media_plugin
import media_service
import music_view
import photo_playback
import photo_view
import playlist
import settings_dialog
import thumb_base
import thumbnail_creator
import thumbnail_view
import toolbar_view
import top_menu
import utils
import video_view

class TestMoblinMedia(unittest.TestCase):
    def setUp(self):
        self.workdir = tempfile.mkdtemp()
    def tearDown(self):
        if os.path.isdir(self.workdir):
            shutil.rmtree(self.workdir)
    def testInstantiate(self):
        pass

    #The following functions tests utils.py 
    def testTimeString(self):
        print "Test MoblinMedia..."
        timeStr = utils.formatTime(10, 0, 0)
        print "Time String for 10, 0, 0: %s" % timeStr
        timeStr = utils.formatTime(0, 0, 0)
        print "Time String for 0, 0: %s" % timeStr
        timeStr = utils.convertSecondsToString(3661)
        print "Convert Seconds to Hour, Min, Sec: 3661 = %s" % timeStr
    def testMediaType(self):
        mediaType = utils.get_media_type("")
        print "Media Type for \"\": %s" % mediaType
        mediaType = utils.get_media_type("None")
        print "Media Type for \"None\": %s" % mediaType
        mediaType = utils.get_media_type("None.")
        print "Media Type for \"None.\": %s" % mediaType
if __name__ == '__main__':
    unittest.main()
