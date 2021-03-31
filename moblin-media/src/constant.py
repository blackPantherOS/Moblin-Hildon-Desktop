#!/usr/bin/python -ttu
# vim: ai ts=4 sts=4 et sw=4
#
# constant.py: This module contains variable definitions that can be used
#              across the code
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


import pygtk
pygtk.require("2.0")

import gettext
import gobject
import gtk
import os

_ = gettext.lgettext

MediaConfigsDir         = '/usr/share/media-services'
MediaContentPath        = os.path.expanduser('~/media')

# Gconf related constants
GCONF_PATH = '/apps/moblin-media'
MediaGconfPath_Music    = os.path.join(GCONF_PATH, 'audio', 'setting')
MediaGconfPath_Photo    = os.path.join(GCONF_PATH, 'photo', 'setting')
MediaGconfPath_video    = os.path.join(GCONF_PATH, 'video', 'setting')

# MediaAppPath related constants
MediaAppPath            = '/usr/share/moblin-media'
MediaGladePath          = os.path.join(MediaAppPath, 'moblin-media.glade')
MediaPluginsDir         = os.path.join(MediaAppPath, 'plugins')
MediaSpecImagePath      = os.path.join(MediaAppPath, 'images')
SampleMediaPath         = os.path.join(MediaAppPath, 'sample-media')
# Depends on MediaSpecImagePath
InvalidImageFile        = os.path.join(MediaSpecImagePath, 'invalid.png')

# Theme related constants
MediaThemePath          = '/usr/share/themes/mobilebasic'
MediaImagePath          = os.path.join(MediaThemePath, 'images')

# Media related paths
MediaHomePath           = os.path.expanduser('~/.moblin-media')
MediaAlbumCoverDir      = os.path.join(MediaHomePath, 'album_covers')
MediaHistoryFile        = os.path.join(MediaHomePath, 'media_history')
MediaThumbnailsDir      = os.path.join(MediaHomePath, 'thumbnails')

MAIN_GLADE_FILE = 'moblin_media.glade'

THUMBNAIL_DIR = os.path.expanduser("~/.moblin-media/thumbnails/largest")

MODE_PHOTO = 'photo'
MODE_AUDIO = 'audio'
MODE_VIDEO = 'video'

STATE_NULL = 0
STATE_READY = 2
STATE_PAUSED = 3
STATE_PLAYING = 4

DBUS_MEDIA_INTERFACE = 'org.moblin.media'
DBUS_MEDIA_PATH = '/org/moblin/media'

DBUS_MUSIC_INTERFACE = 'org.moblin.media.music'
DBUS_MUSIC_PATH = '/org/moblin/media/music' 

DBUS_VIDEO_INTERFACE = 'org.moblin.media.video'
DBUS_VIDEO_PATH = '/org/moblin/media/video' 

DBUS_PHOTO_INTERFACE = 'org.moblin.media.photo'
DBUS_PHOTO_PATH = '/org/moblin/media/photo' 

MediaState = {
    'audio': ['MS_AUDIO_BR_ALBUM','MS_AUDIO_BR_ART','MS_AUDIO_PLAYBACK','MS_AUDIO_BR_SONG','MS_AUDIO_DEFAULT'],
    'photo': ['MS_PHOTO_PLAYBACK', 'MS_PHOTO_THUMBNAIL', 'MS_PHOTO_SHARE', 'MS_PHOTO_DEFAULT'],
    'video': ['MS_VIDEO_PLAYBACK', 'MS_VIDEO_THUMBNAIL', 'MS_VIDEO_SHARE', 'MS_VIDEO_DEFAULT'],
}

def extensionListToShellGlob(extension_list):
    """Given a list of extensions (e.g. ['mp3'] ) convert it to a list of
    shell globs with upper and lowercase (e.g. [ '[mM][pP]3' ] )"""
    output_list = []
    for extension in sorted(extension_list):
        shell_glob = ""
        for letter in extension:
            if letter.upper() != letter.lower():
                shell_glob += "[%s%s]" % (letter.lower(), letter.upper())
            else:
                shell_glob += letter
        output_list.append(shell_glob)
    return output_list

MediaType = {
    'audio': ['mp3', 'wav', 'wma', 'aac', 'ac3', 'm4a', 'ram'],
    'photo': ['jpg', 'bmp',  'jpeg', 'png','gif','wbmp', 'svg' ],
    'video': ['mpg', 'mpeg', 'mp4', '3gp', 'ogg','rm', 'rmvb', 'wmv', 'mov', 'avi', 'gif', 'asf', 'rm', 'ogm'],
}

MediaPattern = {}
# Populate MediaPattern with the shell glob extensions that we need
for key, item in MediaType.iteritems():
    MediaPattern[key] = extensionListToShellGlob(item)

# playback video size mode
VSZ_FULL_WINDOW, VSZ_IDEAL_SIZE, VSZ_DOUBLE_SIZE, VSZ_FIT_WINDOW, VSZ_FIT_SCREEN, VSZ_FULL_SCREEN, VSZ_INVALID = range(7)
# call service Pump() interval
PUMP_INTERVAL = 50
# UpdatePos interval
UPDATE_POS_INTERVAL = 50
UPDATE_POS_COUNT = UPDATE_POS_INTERVAL/PUMP_INTERVAL

MediaUrlList = []

MediaColor = {
    'ctrl_box_bg':        gtk.gdk.Color(5000,5000,5000,0),
    'ctrl_ebox_focus':    gtk.gdk.Color(40000,40000,40000,0),
    'ctrl_ebox_unfocus':  gtk.gdk.Color(20000,20000,20000,0),
    'default_label_bg':   gtk.gdk.Color(30000,30000,30000,0),
    'mm_playback_bg':     gtk.gdk.Color(30000,30000,30000,0),
    'select_label_fg':    gtk.gdk.Color(60000,60000,60000,0),
    'select_window_bg':   gtk.gdk.Color(0,0,0,0),
    'toolbar_bg':         gtk.gdk.Color(20000,20000,20000,0),
    'toolbar_shadow':     gtk.gdk.Color(40000,40000,40000,0),
}

MediaString = {'open_dlg_title' : 'Choose A File'}

# Location constants:
# TNV: ThumbNail View ???
# FLV: Full Length View ???
# PLV: Photo ? ?  (Does not seem to be used)
# PBA: Play Back Audio
# PBV: Play Back Video
# PBP: Play Back Photo
# TRASH: Trash
TNV, FLV, PLV, PBA, PBV, PBP, TRASH = range(0,7)

BORDER = 3

# We cannot get the coordinates of a widget through an existing function, so
# x_min and x_max's coordinates of seekbars should be configured here
SeekBar_Pos = {
    'audio_x_max':  665,
    'audio_x_min':  0,
    'video_x_max': 563,
    'video_x_min':  0,
}

POPUP_TOOL_BAR_DURATION  = 3
POPUP_TOOL_BAR_INIT_TIME = 0.0
POPUP_TOOL_BAR_PRESSING  = -1

##############################################################################
# Translatable Strings

MSG_DELETE_PLAYING_SONG_ERROR = _("A song can not be deleted while it is playing.")
MSG_EDIT_PLAYING_SONG_ERROR = _("A song can not be edited while it is playing.")
MSG_EDIT_PLAYING_VIDEO_ERROR = _("Can not edit playing video")
MSG_DOCKING_ERROR = _("Application is disabled while in docking mode")
MSG_OPEN_ERROR = _("Unable to open file")
MSG_PLAY_ERROR = _("Unable to play file")
MSG_CONFIRM_DELETE_TITLE = _("Delete Media")
MSG_CONFIRM_DELETE_QUESTION = _("Are you sure you want to delete \n\'%s\'?")
MSG_INFO_NO_ITEM_FOR_DELETE = _("Please select a song to delete.")
MSG_RENAME_ERROR_TITLE = _("Rename Conflict")
MSG_RENAME_QUESTION = _("The file \'%s\' already exists.  Do you want to replace it?")
MSG_EXTENSION_CHANGE_TITLE = _("Extension Changed")
MSG_EXTENSION_CHANGE_QUESTION = _("Are you sure you want to change the file extension from %s to %s?")
MSG_MUSIC_WINDOW_TITLE = _("Music")
MSG_VIDEO_WINDOW_TITLE = _("Video")
MSG_PHOTO_WINDOW_TITLE = _("Photos")
MSG_PROPERTIES_TITLE = _("Properties")
MSG_INFO_NO_ITEM_FOR_PROPS = _("Please select an item to see its propreties.")
MSG_EMPTY_STRING_ERROR = _("The name can not be empty")
MSG_ERROR_TITLE = _("Error")
MSG_ONLINE_MUSIC_NO_TITLE = _("Please input a title. \n\nA title can be any descriptive " \
                              "text to identify this online music.")
MSG_ONLINE_MUSIC_INVALID_URL  =   _("Please enter a valid URL")
MSG_ONLINE_MUSIC_ADD_ERROR  =   _("An error occurred when adding the online music entry.")
MSG_VIDEO_STREAM_NOT_SUPPORTED =   _("The stream type is not supported.  Playback can not continue.")
MSG_VIDEO_PROTOCOL_NOT_SUPPORTED = _("The protocol is not supported.")
MSG_VIDEO_URL_IS_INVALID =         _("The URL is invalid.")
MSG_VIDEO_FILE_IS_CORRUPT =        _("The file is corrupt.")
MSG_VIDEO_OUT_OF_MEMORY =          _("Out of memory.")
MSG_VIDEO_UNKNOWN_ERROR =          _("An unknown error has occurred.")
MSG_PHOTO_SORT_BY_NAME = _("Sorting by Name")
MSG_PHOTO_SORT_BY_SIZE = _("Sorting by Size")
MSG_PHOTO_SORT_BY_DATE = _("Sorting by Date")

MUSIC_TITLE = _("Title")
MUSIC_LENGTH = _("Length")
MUSIC_ARTIST = _("Artist")
MUSIC_ALBUM = _("Album")
MUSIC_URL = _("URL")
MUSIC_PLAYLIST = _("Playlist")

ONLINE_MUSIC_PLAYLIST_NAME = _('Online Music')
ALL_SONG_PLAYLIST_NAME = _('All Songs')

MENU_OPEN_LOCATION = _("Open Location...")
MENU_EDIT = _("Edit")
MENU_SORT = _("Sort by")
MENU_SETTINGS = _("Settings...")
MENU_ABOUT = _("About")
MENU_QUIT = _("Quit")
MENU_DELETE = _("Delete")
MENU_PROPERTIES = _("File Properties...")
MENU_ROTATE_CLOCKWISE = _("Rotate clockwise")
MENU_ROTATE_COUNTERCLOCKWISE = _("Rotate counter-clockwise")
MENU_NAME = _("Name")
MENU_SIZE = _("Size")

THUMB_SORT_FILENAME = 0
THUMB_SORT_DATE = 1
THUMB_SORT_SIZE = 2
