#!/usr/bin/python -u
# vim: ai ts=4 sts=4 et sw=4
#
# app.py: Main application file
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


import dbus
import dbus.glib
import dbus.service
import gconf
import gettext
import gobject
import gtk
import gtk.gdk
import gtk.glade
import locale
import os
import shutil
import signal
import socket
import sys

import constant
import media_plugin
import music_view
import photo_playback
import photo_view
import thumbnail_view
import toolbar_view
import top_menu
import utils
import video_view
from media_service import MediaService

_ = gettext.lgettext

error_dict = { 0: _(constant.MSG_VIDEO_STREAM_NOT_SUPPORTED),
               1: _(constant.MSG_VIDEO_PROTOCOL_NOT_SUPPORTED),
               2: _(constant.MSG_VIDEO_URL_IS_INVALID),
               3: _(constant.MSG_VIDEO_FILE_IS_CORRUPT),
               4: _(constant.MSG_VIDEO_OUT_OF_MEMORY),
               5: _(constant.MSG_VIDEO_UNKNOWN_ERROR
             )}

class MediaListener(dbus.service.Object):
    def __init__(self, app):
        self.app = app
        self.name = constant.DBUS_MEDIA_INTERFACE
        self.path = constant.DBUS_MEDIA_PATH
        bus_name = dbus.service.BusName(self.name, bus=dbus.SessionBus())
        dbus.service.Object.__init__(self, bus_name, self.path)

    @dbus.service.method(dbus_interface=constant.DBUS_MEDIA_INTERFACE, in_signature='', out_signature='')
    def top_application(self):
        self.app.top_application()

    @dbus.service.method(dbus_interface=constant.DBUS_MEDIA_INTERFACE, in_signature='s', out_signature='s')
    def dbus_service_connect(self, name):
        return name

    @dbus.service.method(dbus_interface=constant.DBUS_MEDIA_INTERFACE, in_signature='ssi', out_signature='')
    def dbus_service_play(self, strUrl, strMimetype, intRepeat):
        # FIXME: As of writting this comment, the browser is sending
        #        meaningless mimetype info, so we can not use that to
        #        determine how to play the content.
        #
        #        Becomes of this we are attempting to determine the content
        #        type from the URI which is error prone.  
        try:
            proto, full_path = strUrl.split('://')
        except:
            proto = 'file'
            full_path = strUrl
        try:
            ignore, ext = os.path.splitext(full_path)
        except:
            ext = ''
        ext = ext.lower().replace('.', '')
        if ext in constant.MediaType['video']:
            self.app.set_video_path(gtk.Button)
        elif ext in constant.MediaType['photo']:
            # We should not get here since photo's should be viewed in the
            # browser, but just in case... ignore the request
            pass
        else:
            # If it doesn't smell like a video, and it doesn't smell
            # like a photo, then it must be music
            self.app.set_music_path(gtk.Button)
        gtk.gdk.threads_enter()
        ret = self.app.open_uri(strUrl)
        if not ret:
            utils.error_msg(_(constant.MSG_OPEN_ERROR), False)
        gtk.gdk.flush()
        gtk.gdk.threads_leave()
        self.app.top_application()

    @dbus.service.method(dbus_interface=constant.DBUS_MEDIA_INTERFACE, in_signature='', out_signature='')
    def refresh(self):
        cur_mode = self.app.cur_mode
        if cur_mode == constant.MODE_AUDIO:
            self.app.view[cur_mode].refresh()
        else:
            self.app.view[cur_mode].thumbnail.refresh()
        
        self.app.uptodate['audio'] = False
        self.app.uptodate['video'] = False
        self.app.uptodate['photo'] = False
        self.app.uptodate[cur_mode] = True

class MusicListener(dbus.service.Object):
    def __init__(self, app):
        self.app = app
        self.name = constant.DBUS_MUSIC_INTERFACE
        self.path = constant.DBUS_MUSIC_PATH
        bus_name = dbus.service.BusName(self.name, bus=dbus.SessionBus())
        dbus.service.Object.__init__(self, bus_name, self.path)

    @dbus.service.method(dbus_interface=constant.DBUS_MUSIC_INTERFACE, in_signature='', out_signature='')
    def top_application(self):
        if self.app.app_state[constant.MODE_AUDIO] == constant.STATE_NULL:
            self.app.app_state[constant.MODE_AUDIO] = constant.STATE_READY
        self.app.set_music_path(gtk.Button)
        self.app.top_application()
        if not self.app.uptodate['audio']:
            self.view['audio'].refresh()
            self.app.uptodate['audio'] = True

    @dbus.service.method(dbus_interface=constant.DBUS_MUSIC_INTERFACE, in_signature='s', out_signature='s')
    def dbus_service_connect(self, name):
        return name

class PhotoListener(dbus.service.Object):
    def __init__(self, app):
        self.app = app
        self.name = constant.DBUS_PHOTO_INTERFACE
        self.path = constant.DBUS_PHOTO_PATH
        bus_name = dbus.service.BusName(self.name, bus=dbus.SessionBus())
        dbus.service.Object.__init__(self, bus_name, self.path)

    @dbus.service.method(dbus_interface=constant.DBUS_PHOTO_INTERFACE, in_signature='', out_signature='')
    def top_application(self):
        if self.app.app_state[constant.MODE_PHOTO] == constant.STATE_NULL:
            self.app.app_state[constant.MODE_PHOTO] = constant.STATE_READY
        self.app.set_photo_path(gtk.Button)
        self.app.top_application()
        if not self.app.uptodate['photo']:
            self.view['photo'].thumbnail.refresh()
            self.app.uptodate['photo'] = True

    @dbus.service.method(dbus_interface=constant.DBUS_PHOTO_INTERFACE, in_signature='s', out_signature='s')
    def dbus_service_connect(self, name):
        return name

class VideoListener(dbus.service.Object):
    def __init__(self, app):
        self.app = app
        self.name = constant.DBUS_VIDEO_INTERFACE
        self.path = constant.DBUS_VIDEO_PATH
        bus_name = dbus.service.BusName(self.name, bus=dbus.SessionBus())
        dbus.service.Object.__init__(self, bus_name, self.path)

    @dbus.service.method(dbus_interface=constant.DBUS_VIDEO_INTERFACE, in_signature='', out_signature='')
    def top_application(self):
        if self.app.app_state[constant.MODE_VIDEO] == constant.STATE_NULL:
            self.app.app_state[constant.MODE_VIDEO] = constant.STATE_READY
        self.app.set_video_path(gtk.Button)
        self.app.top_application()
        if not self.app.uptodate['video']:
            self.view['video'].thumbnail.refresh()
            self.app.uptodate['video'] = True

    @dbus.service.method(dbus_interface=constant.DBUS_VIDEO_INTERFACE, in_signature='s', out_signature='s')
    def dbus_service_connect(self, name):
        return name

class App(gtk.Object):
    """
    A class which manage whole app view and funcion
    """
    __gsignals__ = {
        'song-ended':(gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, ()),
        }
    def __init__(self):
        self.__gobject_init__()
        self.__callbacks = []
        self.__playback_error = False
        self.TopMenu = None
        self.location = constant.TNV
        # set the socket timeout to something reasonable
        socket.setdefaulttimeout(3)
        # Parse command line options
        self.options, args = utils.parse_command_line()
        self.cur_mode = self.options.mode
        if self.cur_mode == 'music':
            self.cur_mode = constant.MODE_AUDIO
        if self.options.uri:
            self.cur_mode = utils.get_media_type(self.options.uri)
            if not self.cur_mode:
                self.options.uri = None      # invalid
        self.client = gconf.client_get_default()
        try:
            theme = self.client.get_string("/desktop/gnome/interface/gtk_theme")
            image_path = os.path.join('/usr/share/themes', theme, 'images')
            if os.path.isdir(image_path):
                self.image_path = image_path
            else:
                self.image_path = '/usr/share/themes/mobilebasic/images'
        except:
            self.image_path = '/usr/share/themes/mobilebasic/images'

        try:
            self.client.notify_add("/desktop/gnome/interface/gtk_theme", self.theme_changed);
        except:
            pass

        if not self.options.disable_hildon:
            import hildon
            self.wTree = utils.get_widget_tree(constant.MAIN_GLADE_FILE,
                                               root='wm_main_vb')
        else:
            self.wTree = utils.get_widget_tree(constant.MAIN_GLADE_FILE,
                                               root='mp_window_main')

        dic = {
            "on_music_bn_clicked" : self.set_music_path,
            "on_video_bn_clicked" : self.set_video_path,
            "on_photo_bn_clicked" : self.set_photo_path,
            }

        self.wTree.signal_autoconnect(dic)

        self.media_listener = MediaListener(self)
        self.video_listener = VideoListener(self)
        self.music_listener = MusicListener(self)
        self.photo_listener = PhotoListener(self)
        self.width, self.height = utils.getScreenSize()

        if self.options.disable_hildon:
            mp_window_main = self.wTree.get_widget('mp_window_main')
            self.width = 1024
            self.height = 600
            mp_window_main.set_size_request(self.width, self.height)

        # On systems that support a USB client implementation that requires
        # all user data to be on a seperate partition, there will be a
        # /system/usbc/mount_point key with a default setting that the
        # usb client hotplug script will use to find the mount point.
        #
        # If this key is found, then use that directory for all media content,
        # and expect to recieve SIGHUB signals when the USB client hotplug
        # script needs to unmount the partition.
        self.__media_dir = self.client.get_string('/system/usbc/mount_point')
        if self.__media_dir:
            try:
                # Attempt to create a helpful symbolic link from ~/media
                # to the real mountpoint...
                os.symlink(self.__media_dir, os.path.expanduser('~/media'))
            except:
                # ...but if it fails, then don't try to make this code too
                # smart.  This should only happen on a developers system
                pass

            try:
                # FYI: Supposedly you can NOT create a file that is an
                # extension only on a FAT file system.  So a filename of
                # '.access' will fail on a FAT filesystem
                access_test_filename = os.path.join(self.__media_dir, 'access-test')
                open(os.path.join(access_test_filename), 'w').close()
            except IOError:
                # On USB client enabled systems, the client hotplug script
                # will change the permissions on the mountpoint to 000, so
                # if we see this condition then it means that the app should
                # not be running, so let the user know and exit the app.
                utils.error_msg(_(constant.MSG_DOCKING_ERROR), True)
            else:
                # If no exception raised, then lets delete the access test file
                os.unlink(access_test_filename)
        else:
            # On systems not configured to use a shared usb client
            # mass storage device, then just default to the ~/media directory
            self.__media_dir = os.path.expanduser('~/media')
            if not os.path.isdir(self.__media_dir):
                os.mkdir(self.__media_dir)
            
        # If this is the first time the application has run as this user
        # then intialize the media directory and copy all the sample content
        if os.path.isdir(constant.SampleMediaPath):
            # Okay, when we copy to a file system that has a umask=000 option
            # set (like our /mediasync mount point), then the copytree fails at
            # the end of copying the first directory.  This should be okay
            # because our media sample files that we are copying are in a flat
            # directory structure, so stuff should get copied.
            if not os.path.isdir(os.path.join(self.__media_dir, 'video')):
                try:
                    shutil.copytree(os.path.join(constant.SampleMediaPath,'video'),
                        os.path.join(self.__media_dir, 'video'))
                except:
                    pass
            if not os.path.isdir(os.path.join(self.__media_dir, 'photo')):
                try:
                    shutil.copytree(os.path.join(constant.SampleMediaPath,'photo'),
                        os.path.join(self.__media_dir, 'photo'))
                except:
                    pass
            if not os.path.isdir(os.path.join(self.__media_dir, 'audio')):
                try:
                    shutil.copytree(os.path.join(constant.SampleMediaPath,'audio'),
                        os.path.join(self.__media_dir, 'audio'))
                except:
                    pass
        else:
            # Even if there is no sample media installed, we still need to
            # initialize the directorys under media
            if not os.path.isdir(os.path.join(self.__media_dir, 'video')):
                os.makedirs(os.path.join(self.__media_dir, 'video'))
            if not os.path.isdir(os.path.join(self.__media_dir, 'photo')):
                os.makedirs(os.path.join(self.__media_dir, 'photo'))
            if not os.path.isdir(os.path.join(self.__media_dir, 'audio')):
                os.makedirs(os.path.join(self.__media_dir, 'audio'))

        # init some state variables
        self.state = constant.STATE_NULL
        self.update_pos_counter = 0

        # init some gui variable
        self.mainview_box = self.wTree.get_widget('wm_mainview_vb')

        self.main_notebook = self.wTree.get_widget('wm_main_nbk')
        self.main_notebook.set_show_tabs(False)

        self.toolbar_box = self.wTree.get_widget('wm_toolbar_vb')

        self.media_type = None
        self.media_file_length = 0

        # set the default view
        self.cur_playing_mode  = None
        self.fs_mode = False

        # init plugins
        self.plugins = media_plugin.MediaPlugin()

        self.service = MediaService(constant.MediaConfigsDir)
        self.service.connect_to_signal("StatusChange", self.on_status_changed)
        self.service.connect_to_signal("UpdateMediaInfo", self.on_update_media_info)
        self.service.connect_to_signal("UpdatePos", self.on_update_pos)
        self.service.connect_to_signal("ErrorOccur", self.on_error_occur)
        self.service.connect_to_signal("EOSOccur", self.on_eos_occur)

        # init three main views
        self.view = {}

        self.thumb_cur_mode = 'photo'
        
        self.view['photo'] = photo_view.PhotoView(self)
        self.view['audio'] = music_view.MusicView(self)
        self.view['video'] = video_view.VideoView(self)
        self.view['toolbar'] = toolbar_view.ToolBarView(self, 'video')
        self.toolbar_box.pack_start(self.view['toolbar'].cur_view)

        # append three mode windows
        self.music_page = self.wTree.get_widget('wm_main_music_eb')
        self.music_page.add(self.view['audio'].view_nbk)
        self.video_page = self.wTree.get_widget('wm_main_video_eb')
        self.video_page.add(self.view['video'].view_nbk)
        self.photo_page = self.wTree.get_widget('wm_main_photo_eb')
        self.photo_page.add(self.view['photo'].view_nbk)

        self.main_notebook.show()
        self.read_url()

        # Initialize the toplevel window
        if not self.options.disable_hildon:
            self.program = hildon.Program()
            self.window = hildon.Window()
            self.TopMenu = top_menu.TopMenu(self, self.window)
            self.program.add_window(self.window)
            vbox = self.wTree.get_widget("wm_main_vb")
            self.window.add(vbox)
            #add for resize main window
            self.width = gtk.Window.get_screen(self.window).get_width()
            self.height = gtk.Window.get_screen(self.window).get_height()
            self.window.set_size_request(self.width, self.height)
            self.window.connect('delete_event', self.delete_event)
            self.window.connect('destroy', self.destroy)
            self.window.show_all()
        else:
            self.window = self.wTree.get_widget("mp_window_main")
            self.window.connect('delete_event', self.delete_event)
            self.window.connect('destroy', self.destroy)            
            self.window.show_all()

        self.uptodate = {}
        self.uptodate['audio'] = True
        self.uptodate['video'] = True
        self.uptodate['photo'] = True

        if self.options.mode == 'photo':
            self.set_photo_path(gtk.Button)
        elif self.options.mode == 'video':
            self.set_video_path(gtk.Button)
        else:
            self.set_music_path(gtk.Button)
         
        # connect custom signal with child handlers
        self.connect('song-ended', self.view['audio'].on_song_ended)

        if self.options.uri:
            self.open_uri(self.options.uri)

        signal.signal(signal.SIGHUP, self.__hangup_handler)

        self.client.add_dir(constant.GCONF_PATH, gconf.CLIENT_PRELOAD_NONE)
        self.client.notify_add(constant.GCONF_PATH, self.__on_config_update)

        # who's alive and who started the media playback
        self.app_state = { constant.MODE_PHOTO: constant.STATE_NULL,
                           constant.MODE_AUDIO: constant.STATE_NULL,
                           constant.MODE_VIDEO: constant.STATE_NULL }
        self.app_state[self.cur_mode] = constant.STATE_READY

    def register_config_callback(self, mode, handler):
        self.__callbacks.append([mode, handler])
        
    def __on_config_update(self, client, connection_id, entry, args):
        # Expecting keys of the form /apps/PLAYER/MODE/NAME
        mode, name = entry.get_key().split('/')[3:]
        for c in self.__callbacks:
            if c[0] == mode:
                c[1](name, entry.get_value())
        
    def go_thumbnail_page(self):
        if (self.thumb_cur_mode == 'photo'):
            self.main_notebook.set_current_page(2)
            self.view['photo'].update_state_thumbnail()
        elif (self.thumb_cur_mode == 'video'):
            self.__disarm_timer()
            self.main_notebook.set_current_page(1)
            self.view['video'].update_state_thumbnail()

    def get_current_thumb(self):
        thumb_mode = self.get_thumb_current_mode()
        thumbnail = None
        if (thumb_mode == 'photo'):
            thumbnail = self.view['photo'].get_photo_thumbnail()
        elif (thumb_mode == 'video'):
            thumbnail = self.view['video'].get_video_thumbnail()
        return thumbnail

    def get_current_index(self):
        thumb_mode = self.get_thumb_current_mode()
        if (thumb_mode == 'photo'):
            return self.view['photo'].get_index()
        elif (thumb_mode == 'video'):
            return self.view['video'].get_index()

    def get_media_directory(self):
        return self.__media_dir

    def update_current_index(self,index):
        thumb_mode = self.get_thumb_current_mode()
        if (thumb_mode == 'photo'):
            return self.view['photo'].update_index(index)
        elif (thumb_mode == 'video'):
            return self.view['video'].update_index(index)

    def get_thumb_current_mode(self):
        return self.thumb_cur_mode        

    def run(self):
        gtk.gdk.threads_enter()
        self.main_loop = gtk.main
        self.main_quit = gtk.main_quit
        gtk.main()
        gtk.gdk.threads_leave()

    def delete_event(self, widget, event, data=None):
        if self.view['photo'].photoplayback.is_autoplay():
            self.view['photo'].photoplayback.stop_auto_play()

        if self.app_state[self.cur_mode] > constant.STATE_READY:
            try:
                self.service.Stop()
                #self.service.Close()
            except:
                # We don't really care if the media service has issues
                pass
        self.app_state[self.cur_mode] = constant.STATE_NULL

        quit = True
        for index in self.app_state:
            if self.app_state[index] > constant.STATE_NULL:
                quit = False
        if quit:
            # emit the "destroy" signal
            return False
        else:
            # don't emit the "destroy" signal
            self.window.hide()
            return True

    def destroy(self, widget, data=None):
        gtk.main_quit()

    def __arm_timer(self):
        self.__disarm_timer()
        self.__timeout_id = gobject.timeout_add(3000,
                                                self.__fullscreen_timeout)

    def __disarm_timer(self):
        try:
            gobject.source_remove(self.__timeout_id)
        except:
            # Ignore errors... it just means that the timer was not
            # really armed
            pass
        
    def __fullscreen_timeout(self):
        if self.get_fs_mode() == False and \
           self.get_thumb_current_mode() == 'video' and \
           self.location == constant.PBV:
            self.set_fs_mode(True)

    def get_fs_mode(self):
        return self.fs_mode

    def set_fs_mode(self , on):
        self.fs_mode = on       
        if on :
            self.__disarm_timer()
            if not self.state in [constant.STATE_READY]:
                self.window.fullscreen()
            self.toolbar_box.hide()
        else :
            self.window.unfullscreen()
            self.toolbar_box.show()
            if self.get_thumb_current_mode() == 'video' and \
                   self.location == constant.PBV:
                self.__arm_timer()

    def on_full_screen( self, args ):
        self.set_fs_mode( True )

    def on_unfull_screen( self, args ):
        self.set_fs_mode( False )

    def read_url(self):
        self.url_list_count = 0
        self.url_file = open(constant.MediaHistoryFile, 'a+')
        for line in self.url_file:
            line = line.rstrip('\n')
            constant.MediaUrlList.insert(0, line)
            self.url_list_count = self.url_list_count + 1

    def on_status_changed(self, prev, new, pending):
        if prev == constant.STATE_PAUSED and new == constant.STATE_READY:
            self.view['toolbar'].update_button(self.cur_mode, self.media_type)
            self.set_fs_mode(False)
            try:
                self.service.SetPosition(dbus.UInt32(0))
            except media_service.MediaServiceError:
                pass
            if self.media_type == 'audio':
                self.view["toolbar"].playback_set_seekbar_value(0, self.media_type)
            if self.media_type in ['audio', 'video']:
                self.view["toolbar"].playback_set_seekbar_value(0, self.media_type)
        if new == constant.STATE_PAUSED or new == constant.STATE_READY:
            if self.cur_mode == constant.MODE_AUDIO:
                self.view['toolbar'].mm_pause_button.hide()
                self.view['toolbar'].mm_play_pause_button.show()
            elif self.cur_mode == constant.MODE_VIDEO:
                self.view['toolbar'].vm_pause_button.hide()
                self.view['toolbar'].vm_play_pause_button.show()
                self.__disarm_timer()
        elif new == constant.STATE_PLAYING:
            if self.cur_mode == constant.MODE_AUDIO:
                self.view['toolbar'].mm_play_pause_button.hide()
                self.view['toolbar'].mm_pause_button.show()
            elif self.cur_mode == constant.MODE_VIDEO:
                self.view['toolbar'].vm_play_pause_button.hide()
                self.view['toolbar'].vm_pause_button.show()
                if self.get_fs_mode() == False:
                    self.__arm_timer()
        self.state = new
        self.set_app_state (new)

    def set_app_state (self,new):
        if self.app_state[self.cur_mode] == new:
            return
        # steal the playback flag and put it on current (only one at a time)
        for index in self.app_state:
            if self.app_state[index] > constant.STATE_READY:
                self.app_state[index] = constant.STATE_READY
        self.app_state[self.cur_mode] = new
            
    def on_update_media_info(self, key, value ):
        self.view[self.media_type].update_media_info(key, value)

    def on_update_pos(self , cur_value ):
        cur_value = cur_value/1000
        if cur_value <= self.media_file_length:
            if self.media_type in ['audio', 'video']:
                if cur_value == -1:
                    cur_value = 0
                self.view['toolbar'].playback_set_seekbar_value(cur_value,self.media_type)

    def on_surface_expose(self, widget,event):
        pass

    def on_dialog_close( self , dialog ):
        if self.cur_mode == constant.MODE_VIDEO:
            self.view['toolbar'].video_return(None)
        elif self.cur_mode == constant.MODE_AUDIO:
            self.view['toolbar'].music_return(None)
        dialog.destroy()

    def on_dialog_response( self , dialog ,response_id ):
        if self.cur_mode == constant.MODE_VIDEO:
            if self.view['video'].get_thumb_fs_mode():
                self.set_fs_mode(True)
            else:
                self.set_fs_mode(False)
            self.view['toolbar'].video_return(None)
        elif self.cur_mode == constant.MODE_AUDIO:
            self.view['toolbar'].music_return(None)
            self.view['audio'].reset_play_icon()
        dialog.destroy()

    def on_error_occur( self, error_code, msg):
        self.__playback_error = True
        dialog = gtk.MessageDialog(None,
                                   gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT,
                                   gtk.MESSAGE_ERROR,
                                   gtk.BUTTONS_CLOSE,
                                   error_dict[error_code])
        dialog.set_title(_(constant.MSG_ERROR_TITLE))
        dialog.connect( "close" , self.on_dialog_close )
        dialog.connect( "response" , self.on_dialog_response )
        dialog.show()   
        
    def on_eos_occur(self):
        if self.media_type == 'audio':
            if self.__playback_error:
                # If we have seen a playback error, then mask 
                # the song-ended signal so the next song on the playlist
                # doesn't automatically start
                self.__playback_error = False
            else:
                self.emit('song-ended')

    def set_music_path(self, button):
        self.window.set_title(_(constant.MSG_MUSIC_WINDOW_TITLE))
        self.thumb_cur_mode = 'audio'
        self.cur_mode = constant.MODE_AUDIO

        cur_view = self.view['audio'].update_view()
        if cur_view == music_view.VIEW_LIST_SONGS:
            self.set_music_thumb_mode(None)
        elif cur_view == music_view.VIEW_LIST_ONLINE:
            self.set_music_thumb_mode(None)
        elif cur_view == music_view.VIEW_PLAY_ONLINE or cur_view == music_view.VIEW_PLAY_SONG:
            self.set_music_mode(None)
        self.TopMenu.create_menu(self.window)

    def set_video_path(self, button):
        self.window.set_title(_(constant.MSG_VIDEO_WINDOW_TITLE))
        self.thumb_cur_mode = 'video'
        self.location = constant.TNV
        self.set_video_thumb_mode(None)
        self.get_current_thumb().update_view_by_index(self.get_current_index())
        thumb = self.get_current_thumb()
        thumb.make_all_image_thumbnail(os.path.join(self.__media_dir, "video"))
        self.go_thumbnail_page()
        self.cur_mode = constant.MODE_VIDEO
        if  self.get_current_thumb().get_item_selected() == -1:
            self.view['toolbar'].set_video_btn_sensitive(False)
        self.TopMenu.create_menu(self.window)

    def set_photo_path(self, button):
        self.window.set_title(_(constant.MSG_PHOTO_WINDOW_TITLE))
        self.thumb_cur_mode = 'photo'
        self.location = constant.TNV
        self.set_photo_thumb_mode(None)
        thumb = self.get_current_thumb()
        thumb.update_view_by_index(self.get_current_index())
        thumb.make_all_image_thumbnail(os.path.join(self.__media_dir, "photo"))
        self.go_thumbnail_page()
        self.cur_mode = constant.MODE_PHOTO
        if  self.get_current_thumb().get_item_selected() == -1:
            self.view['toolbar'].set_photo_btn_sensitive(False)
        self.TopMenu.create_menu(self.window)

    def set_music_mode(self, button):
        self.main_notebook.set_current_page(0)
        self.view['toolbar'].change_mode('audio', self.media_type)

    def set_music_thumb_mode(self, button):
        self.main_notebook.set_current_page(0)
        self.view['toolbar'].change_mode('audio-thumb', self.media_type)
        self.view['audio'].update_song_model()

    def set_video_mode(self, button):
        self.main_notebook.set_current_page(1)
        self.view['toolbar'].change_mode('video', self.media_type)

    def set_photo_mode(self, button):
        self.main_notebook.set_current_page(2)
        self.view['toolbar'].change_mode('photo', self.media_type)

    def set_photo_thumb_mode(self, button):
        self.main_notebook.set_current_page(2)
        self.view['toolbar'].change_mode('photo-thumb', self.media_type)

    def set_video_thumb_mode(self, button):
        self.main_notebook.set_current_page(1)
        self.view['toolbar'].change_mode('video-thumb', self.media_type)

    def set_mediatype(self, mediatype):
        if mediatype in constant.MediaType.keys():
            self.media_type = mediatype

    def get_mediatype(self):
        return self.media_type

    def play_pause(self, button):
        if self.state == constant.STATE_PAUSED or \
           self.state == constant.STATE_READY:
            if self.media_type == 'video':
                self.__arm_timer()
            self.service.Play()
        elif self.state == constant.STATE_PLAYING:
            self.service.Pause()

    def load(self, media_type, filename):
        if self.state != constant.STATE_NULL:
            self.service.Stop()
        if media_type in ['audio', 'video']:
            surface = self.view[media_type].get_render_surface()
            surface.show()
            gtk.gdk.flush()
            if None != surface.window:
                self.service.SetWindow(surface.window.xid)
                self.service.OpenUri(filename)
                self.service.Play()

    def set_position(self, position):
        self.service.SetPosition(dbus.UInt32(position * 1000))

    def top_application(self):
        self.window.present()

    def theme_changed(self, client, connection_id, entry, args):
        if (entry.get_value().type == gconf.VALUE_STRING):
            self.image_path = os.path.join('/usr/share/themes',
                                           entry.get_value().get_string(),
                                           'images')

    def debug(self, msg):
        if self.options.enable_debug:
            print "%s\n" % (msg)

    def open_uri(self, url_text):
        media_type = utils.get_media_type(url_text)
        if url_text and media_type:
            if (url_text.find("http://") == 0):
                if (self.cur_mode == constant.MODE_AUDIO) and (media_type == 'audio'):
                    self.view['audio'].radio_add_url(None, url_text)
                    self.view['audio'].play_pl_song(url_text)
                elif (media_type == 'audio') or (media_type == 'video'):
                    self.view['toolbar'].open_media_file(url_text, False)
                else:
                    media_type = None
            elif media_type in ('audio','video','photo'):
                if url_text[0] == '/':
                    url_text = 'file://' + url_text
                self.view['toolbar'].open_media_file(url_text, False)
            else:
                media_type = None
        return (media_type is not None)

    def __hangup_handler(self, signal, frame):
        if not self.delete_event(self, None):
            self.destroy(self)
