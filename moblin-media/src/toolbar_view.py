#!/usr/bin/python -ttu
# -*- mode:python; tab-width:4; indent-tabs-mode:nil;  -*-
# vim: ai ts=4 sts=4 et sw=4
#
# toolbar_view.py: Manage Toolbar View
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
import gtk
import gtk.gdk
import gtk.glade
import os
import sys

# import our app's libraries
from playlist import PlayList, PlayListError
import constant
import hint_window
import media_plugin
import thumbnail_creator
import utils

class ToolBarView(object):
    """A class which manage toolbar view."""

    def __init__(self, app, mode):
        self.__mode = mode
        self.__client = app.client
        self.app = app
        self.cb_string = ' '
        self.cb_listType = 0
        self.name_list = {'audio':'wm_mm_toolbar_hb',
                          'video':'wm_vm_toolbar_hb',
                          'photo':'wm_pm_toolbar_hb',
                          'photo-thumb':'wm_ph_th_toolbar_hb',
                          'video-thumb' : 'wm_vd_th_toolbar_hb',
                          'audio-thumb':'wm_ad_th_toolbar_hb'
                          }
        dic = {}
        dic['audio'] = {
                     "on_wm_mm_tb_prev_bn_clicked": self.music_prev,
                     "on_wm_mm_tb_play_bn_clicked" : self.music_play_pause,
                     "on_wm_mm_tb_pause_bn_clicked" : self.music_play_pause,
                     "on_wm_mm_tb_next_bn_clicked" : self.music_next,
                     "on_wm_mm_tb_return_bn_clicked" : self.music_return,
                     "on_wm_mm_song_seekbar_hs_change_value": self.sb_change_value,
                     "on_wm_mm_song_seekbar_hs_button_press_event": self.sb_click_to_pause,
                     "on_wm_mm_song_seekbar_hs_button_release_event": self.sb_click_to_change_audio_playing_time,
                     "on_mm_time_btn_clicked": self.time_btn_clicked,
                     }
        dic['video'] = {
                     "on_wm_vm_tb_prev_bn_clicked": self.video_prev,
                     "on_wm_vm_tb_play_bn_clicked" : self.video_play_pause,
                     "on_wm_vm_tb_pause_bn_clicked" : self.video_play_pause,
                     "on_wm_vm_tb_next_bn_clicked" : self.video_next,
                     "on_wm_vm_tb_return_bn_clicked" : self.video_return,
                     "on_vm_time_btn_clicked": self.time_btn_clicked,
                     "on_wm_vm_seekbar_hs_change_value": self.sb_change_value,
                     "on_wm_vm_seekbar_hs_button_press_event": self.sb_click_to_pause,
                     "on_wm_vm_seekbar_hs_button_release_event": self.sb_click_to_change_video_playing_time,
                     }
        dic['photo'] = {
                     "on_wm_pm_tb_play_bn_clicked" : self.photo_play_pause,
                     "on_wm_pm_tb_pause_bn_clicked" : self.photo_play_pause,
                     "on_wm_pm_tb_prev_bn_clicked" : self.photo_prev,
                     "on_wm_pm_tb_next_bn_clicked" : self.photo_next,
                     "on_wm_pm_tb_return_bn_clicked" : self.photo_return,
                     "on_wm_pm_trash_bn_clicked" : self.trash_delete,
                     "on_wm_pm_i_bn_clicked" : self.file_info,
                     "on_wm_pm_right_bn_clicked" : self.right_rotate,
                     "on_wm_pm_left_bn_clicked" : self.left_rotate,
                     "on_wm_pm_resize_bar_value_changed" : self.resize_photo,
                 }
        dic['photo-thumb'] = {"on_wm_ph_th_browse_bn_clicked" : self.return_play_mode,
                     "on_wm_pm_tb_thumb_size_hs_value_changed": self.__photo_thumbnail_resize,
                     "on_wm_ph_th_trash_bn_clicked" : self.trash_delete,
                     "on_wm_ph_th_i_bn_clicked" : self.file_info,
                     "on_wm_ph_th_right_bn_clicked" : self.right_rotate,
                     "on_wm_ph_th_left_bn_clicked" : self.left_rotate,
                     "on_ph_sort_name_bn_clicked" : self.sort_name_click,
                     "on_ph_sort_date_bn_clicked" : self.sort_date_click,
                 }
        dic['video-thumb'] = {"on_wm_vd_th_browse_bn_clicked" : self.return_play_mode,
                     "on_wm_vm_tb_thumb_size_hs_value_changed": self.__video_thumbnail_resize,
                     "on_wm_vd_th_trash_bn_clicked" : self.trash_delete,
                     "on_wm_vd_th_i_bn_clicked" : self.file_info,
                     "on_vd_sort_name_bn_clicked" : self.sort_name_click,
                     "on_vd_sort_date_bn_clicked" : self.sort_date_click,
                 }

        dic['audio-thumb'] = {
                     "on_wm_ad_th_del_bn_clicked" : self.on_del_song_bn_clicked,
                     "on_wm_ad_th_add_bn_clicked" : self.on_add_radio_bn_clicked,
                     "on_wm_ad_th_repeat_bn_toggled" : self.on_repeat_bn_toggled,
                     "on_shuffle_bn_clicked" : self.on_shuffle_bn_clicked,
                     "on_wm_ad_th_browse_bn_clicked" : self.return_play_mode,
                     "on_wm_ad_th_trash_bn_clicked" : self.trash_delete_audio,
                     "on_wm_ad_th_i_bn_clicked" : self.file_info_audio,
                 }

        self.plugins = self.app.plugins
        # init three toolbar suites and connect signal and functions
        # init default dir
        self.wTree = {}
        self.last_browse_dir = {}
        self.toolbar = {}
        self.plugin_buttons = {}
        self.is_plugin_loaded = {}
        #record the button state from setting
        for name in self.name_list.keys():
            self.wTree[name] = utils.get_widget_tree(constant.MAIN_GLADE_FILE,
                                                     root = self.name_list[name])
            self.wTree[name].signal_autoconnect(dic[name])
            self.toolbar[name] = self.wTree[name].get_widget(self.name_list[name])
            self.last_browse_dir[name] = self.get_default_dir(name)
            self.is_plugin_loaded[name] = False
        # buttons
        self.mm_play_pause_button = self.wTree['audio'].get_widget("wm_mm_tb_play_bn")
        self.mm_pause_button = self.wTree['audio'].get_widget("wm_mm_tb_pause_bn")
        self.mm_prev_button = self.wTree['audio'].get_widget("wm_mm_tb_prev_bn")
        self.mm_next_button = self.wTree['audio'].get_widget("wm_mm_tb_next_bn")
        self.music_seekbar = self.wTree['audio'].get_widget("mm_song_seekbar_hs")
        #self.music_seekbar.modify_fg(gtk.STATE_NORMAL, constant.MediaColor['select_label_fg'])
        self.music_time_btn = self.wTree['audio'].get_widget("mm_time_btn")
        self.music_time_btn.set_label("        ")
        self.wm_shuffle_bn = self.wTree['audio-thumb'].get_widget("wm_mm_shuffle_bn")
        self.ad_th_repeat_bn = self.wTree['audio-thumb'].get_widget("wm_mm_repeat_bn")
        self.vm_play_pause_button = self.wTree['video'].get_widget("wm_vm_tb_play_bn")
        self.vm_pause_button = self.wTree['video'].get_widget("wm_vm_tb_pause_bn")
        self.vm_prev_button = self.wTree['video'].get_widget("wm_vm_tb_prev_bn")
        self.vm_next_button = self.wTree['video'].get_widget("wm_vm_tb_next_bn")
        self.vm_hd_tb_button = self.wTree['video'].get_widget("wm_vm_hd_tb_bn")
        self.vm_sh_tb_button = self.wTree['video'].get_widget("wm_vm_sh_tb_bn")
        self.video_seekbar = self.wTree['video'].get_widget("wm_vm_seekbar_hs")
        #init seekbar x coordinate length here we set a ambiguous value 0~590

        self.seekbar_click_already = False
        self.seekbar_drag_already = False   # seekbar flag ,so we won't enter click handlerwhen we drag the seekbar
        #self.video_seekbar.modify_fg(gtk.STATE_NORMAL, constant.MediaColor['select_label_fg'])
        self.video_time_btn = self.wTree['video'].get_widget('vm_time_btn')
        self.video_time_btn.set_label("        ")
        self.pm_autoplay_button = self.wTree['photo'].get_widget("wm_pm_tb_play_bn")
        self.pm_pause_button = self.wTree['photo'].get_widget("wm_pm_tb_pause_bn")
        self.pm_prev_button = self.wTree['photo'].get_widget("wm_pm_tb_prev_bn")
        self.pm_next_button = self.wTree['photo'].get_widget("wm_pm_tb_next_bn")
        self.pm_return_button = self.wTree['photo'].get_widget("wm_pm_tb_return_bn")
        self.pm_hd_tb_button = self.wTree['photo'].get_widget("wm_pm_hd_tb_bn")
        self.pm_sh_tb_button = self.wTree['photo'].get_widget("wm_pm_sh_tb_bn")
        self.pm_left_btn = self.wTree['photo'].get_widget("wm_pm_left_bn")
        self.pm_right_btn = self.wTree['photo'].get_widget("wm_pm_right_bn")
        self.pm_trash_btn = self.wTree['photo'].get_widget("wm_pm_trash_bn")
        self.pm_info_btn = self.wTree['photo'].get_widget("wm_pm_i_bn")
        self.pm_resize_bar = self.wTree['photo'].get_widget("wm_pm_resize_bar")
        self.pm_thumb_size_bar = self.wTree['photo-thumb'].get_widget("wm_pm_tb_thumb_size_hs")
        key = "%s/photo/thumbnail_level" % (constant.GCONF_PATH)
        level = self.__client.get_float(key)
        if not level:
            level = 7.5
        self.pm_thumb_size_bar.set_value(level)
        self.ph_th_left_btn = self.wTree['photo-thumb'].get_widget("wm_ph_th_left_bn")
        self.ph_th_right_btn = self.wTree['photo-thumb'].get_widget("wm_ph_th_right_bn")
        self.ph_th_trash_btn = self.wTree['photo-thumb'].get_widget("wm_ph_th_trash_bn")
        self.ph_th_info_btn = self.wTree['photo-thumb'].get_widget("wm_ph_th_i_bn")
        self.ph_sort_name_btn = self.wTree['photo-thumb'].get_widget("ph_sort_name_bn")
        self.ph_sort_date_btn = self.wTree['photo-thumb'].get_widget("ph_sort_date_bn")
        self.vm_thumb_size_bar = self.wTree['video-thumb'].get_widget("wm_vm_tb_thumb_size_hs")
        key = "%s/video/thumbnail_level" % (constant.GCONF_PATH)
        level = self.__client.get_float(key)
        if not level:
            level = 7.5
        self.vm_thumb_size_bar.set_value(level)
        self.vd_th_browse_button = self.wTree['video-thumb'].get_widget("wm_vd_th_browse_bn")
        self.vd_th_trash_btn = self.wTree['video-thumb'].get_widget("wm_vd_th_trash_bn")
        self.vd_th_info_btn = self.wTree['video-thumb'].get_widget("wm_vd_th_i_bn")
        self.vd_sort_name_btn = self.wTree['video-thumb'].get_widget("vd_sort_name_bn")
        self.vd_sort_date_btn = self.wTree['video-thumb'].get_widget("vd_sort_date_bn")
        self.addradio_bn = self.wTree['audio-thumb'].get_widget("wm_ad_th_add_bn")
        self.delradio_bn = self.wTree['audio-thumb'].get_widget("wm_ad_th_del_bn")
        self.mm_sort_cb = self.wTree['audio-thumb'].get_widget("mm_sort_cb")
        self.ad_th_browse_button = self.wTree['audio-thumb'].get_widget("wm_ad_th_browse_bn")
        self.ad_th_trash_bn = self.wTree['audio-thumb'].get_widget("wm_ad_th_trash_bn")
        self.ad_th_i_bn = self.wTree['audio-thumb'].get_widget("wm_ad_th_i_bn")

        self.addradio_bn.hide()
        self.delradio_bn.hide()
        self.mm_sort_cb.hide()
        #get images from file
        for toolbar_mode in ("photo","audio-thumb","video-thumb","photo-thumb"):
            wm_info_img = self.wTree[toolbar_mode].get_widget("wm_i_img")
            wm_info_img.set_from_file(os.path.join(self.app.image_path,'mb_media_btn_info.png'))
            wm_trash_img = self.wTree[toolbar_mode].get_widget("wm_trash_img")
            wm_trash_img.set_from_file(os.path.join(self.app.image_path,'mb_media_btn_trash.png'))
        for toolbar_mode in ("audio","video"):
            wm_play_img = self.wTree[toolbar_mode].get_widget("wm_play_img")
            wm_play_img.set_from_file(os.path.join(self.app.image_path,'mb_media_btn_play.png'))
            wm_pause_img = self.wTree[toolbar_mode].get_widget("wm_pause_img")
            wm_pause_img.set_from_file(os.path.join(self.app.image_path,'mb_media_btn_pause.png'))
        wm_play_img = self.wTree['photo'].get_widget("wm_play_img")
        wm_play_img.set_from_file(os.path.join(self.app.image_path,'mb_media_btn_slideshow.png'))

        for toolbar_mode in ("audio","video"):
            wm_next_img = self.wTree[toolbar_mode].get_widget("wm_next_img")
            wm_next_img.set_from_file(os.path.join(self.app.image_path,'mb_media_btn_forward.png'))
            wm_prev_img = self.wTree[toolbar_mode].get_widget("wm_prev_img")
            wm_prev_img.set_from_file(os.path.join(self.app.image_path,'mb_media_btn_rewind.png'))
        wm_next_img = self.wTree['photo'].get_widget("wm_next_img")
        wm_next_img.set_from_file(os.path.join(self.app.image_path,'mb_media_btn_slideshow_next.png'))
        wm_prev_img = self.wTree['photo'].get_widget("wm_prev_img")
        wm_prev_img.set_from_file(os.path.join(self.app.image_path,'mb_media_btn_slideshow_previous.png'))

        wm_back_img = self.wTree['audio'].get_widget("wm_return_img")
        wm_back_img.set_from_file(os.path.join(self.app.image_path,'mb_media_btn_back_music.png'))
        wm_back_img = self.wTree['video'].get_widget("wm_return_img")
        wm_back_img.set_from_file(os.path.join(self.app.image_path,'mb_media_btn_back_video.png'))
        wm_back_img = self.wTree['photo'].get_widget("wm_return_img")
        wm_back_img.set_from_file(os.path.join(self.app.image_path,'mb_media_btn_back_photo.png'))
        for toolbar_mode in ("photo","photo-thumb"):
            ph_left_img = self.wTree[toolbar_mode].get_widget("ph_left_img")
            ph_left_img.set_from_file(os.path.join(self.app.image_path,'mb_media_btn_rotateleft.png'))
            ph_right_img = self.wTree[toolbar_mode].get_widget("ph_right_img")
            ph_right_img.set_from_file(os.path.join(self.app.image_path,'mb_media_btn_rotateright.png'))
        music_add_img = self.wTree["audio-thumb"].get_widget("ad_th_add_img")
        music_add_img.set_from_file(os.path.join(self.app.image_path,'mb_media_btn_radio_add.png'))
        music_del_img = self.wTree["audio-thumb"].get_widget("ad_th_del_img")
        music_del_img.set_from_file(os.path.join(self.app.image_path,'mb_media_btn_radio_delete.png'))
        music_repeat_img = self.wTree["audio-thumb"].get_widget("wm_repeat_img")
        music_repeat_img.set_from_file(os.path.join(self.app.image_path,'mb_media_btn_repeat.png'))
        music_shuffle_img = self.wTree["audio-thumb"].get_widget("wm_shuffle_img")
        music_shuffle_img.set_from_file(os.path.join(self.app.image_path,'mb_media_btn_shuffle.png'))
        for toolbar_mode in ("video-thumb","photo-thumb"):
            wm_sort_name_img = self.wTree[toolbar_mode].get_widget("sort_name_img")
            wm_sort_name_img.set_from_file(os.path.join(self.app.image_path,'mb_media_btn_sort_name.png'))
            wm_sort_date_img = self.wTree[toolbar_mode].get_widget("sort_date_img")
            wm_sort_date_img.set_from_file(os.path.join(self.app.image_path,'mb_media_btn_sort_date.png'))
        for toolbar_mode in ("audio-thumb","video-thumb"):
            wm_browse_img = self.wTree[toolbar_mode].get_widget("wm_browse_img")
            wm_browse_img.set_from_file(os.path.join(self.app.image_path,'mb_media_btn_nowplaying.png'))
        # for photo playback
        self.photo_playback = self.app.view['photo'].get_photo_playback()
        self.is_photo_autoplay = False
        # filter
        self.file_filter = {}
        self.all_filter = gtk.FileFilter()
        self.all_filter.set_name('All')
        for mode in constant.MediaPattern.keys():
            self.file_filter[mode] = gtk.FileFilter()
            self.file_filter[mode].set_name(mode)
            for type in constant.MediaPattern[mode]:
                pattern = '*.' + type
                self.file_filter[mode].add_pattern(pattern)
                self.all_filter.add_pattern(pattern)
        # current postion of video playback
        self.cur_pos = 0
        self.whole_value = 0
        self.pos_time = True
        self.left_value = 0

        # init mode and toolbar_name
        self.cur_mode = mode
        # init prev and next button state, it's a two element list
        self.prev_next_state = [True, True]
        #the last song is the one that call the play-pause button on audio toolbar
        self.last_song = None
        # pack whole toolbar to a VBox
        self.cur_view = gtk.VBox()
        self.cur_toolbar = self.toolbar[mode]
        self.init_ui()
        self.app.register_config_callback('photo', self.__config_handler)
        self.app.register_config_callback('video', self.__config_handler)

    def __update_sort_button(self, visible):
        mode = self.app.get_thumb_current_mode()
        if mode == 'photo':
            sorttype = self.app.view[mode].get_photo_thumbnail().get_sort_type()
            if visible:
                if sorttype == 0:
                    # Sorted by date, so show Sort by Name btn
                    self.ph_sort_name_btn.show()
                    self.ph_sort_date_btn.hide()
                else:
                    # Sort by name, so show Sort by Date btn
                    self.ph_sort_name_btn.hide()
                    self.ph_sort_date_btn.show()
            else:
                self.ph_sort_name_btn.hide()
                self.ph_sort_date_btn.hide()
        elif mode == 'video':
            sorttype = self.app.view[mode].get_video_thumbnail().get_sort_type()
            if visible:
                if sorttype == 0:
                    # Sorted by date, so show Sort by Name btn                    
                    self.vd_sort_name_btn.show()
                    self.vd_sort_date_btn.hide()
                else:
                    # Sort by name, so show Sort by Date btn
                    self.vd_sort_name_btn.hide()
                    self.vd_sort_date_btn.show()
            else:
                self.vd_sort_name_btn.hide()
                self.vd_sort_date_btn.hide()

    def __update_resize_slider(self, visible):
        if self.app.get_thumb_current_mode() == 'photo':
            if visible and self.app.location == constant.TNV:
                self.pm_thumb_size_bar.show()
            elif not visible and self.app.location == constant.TNV:
                self.pm_thumb_size_bar.hide()
        elif self.app.get_thumb_current_mode() == 'video':
            if visible and self.app.location == constant.TNV:
                self.vm_thumb_size_bar.show()
            elif not visible and self.app.location == constant.TNV:
                self.vm_thumb_size_bar.hide()

    def __update_rotate_buttons(self, visible):
        if self.app.get_thumb_current_mode() == 'photo':
            if visible:
                if self.app.location == constant.TNV :
                    self.ph_th_left_btn.show()
                    self.ph_th_right_btn.show()
                elif self.app.location == constant.PBP:
                    self.pm_left_btn.show()
                    self.pm_right_btn.show()
            else:
                if self.app.location == constant.TNV :
                    self.ph_th_left_btn.hide()
                    self.ph_th_right_btn.hide()
                elif self.app.location == constant.PBP:
                    self.pm_left_btn.hide()
                    self.pm_right_btn.hide()

    def __update_properties_button(self, visible):
        if self.app.get_thumb_current_mode() == 'photo':
            if visible:
                if self.app.location == constant.TNV or self.app.location == constant.FLV :
                    self.ph_th_info_btn.show()
                elif self.app.location == constant.PBP:
                    self.pm_info_btn.show()
            else:
                if self.app.location == constant.TNV or self.app.location == constant.FLV :
                    self.ph_th_info_btn.hide()
                elif self.app.location == constant.PBP:
                    self.pm_info_btn.hide()
        elif self.app.get_thumb_current_mode() == 'video':
            if visible and self.app.location == constant.TNV:
                self.vd_th_info_btn.show()
            elif not visible and self.app.location == constant.TNV:
                self.vd_th_info_btn.hide()

    def __update_delete_button(self, visible):
        if self.app.get_thumb_current_mode() == 'photo':
            if visible:
                self.ph_th_trash_btn.show()
            else:
                self.ph_th_trash_btn.hide()
        elif self.app.get_thumb_current_mode() == 'video':
            if visible:
                self.vd_th_trash_btn.show()
            else:
                self.vd_th_trash_btn.hide()

    def __config_handler(self, key, value):
        if key == 'show_sort_button':
            self.__update_sort_button(value.get_bool())
        elif key == 'show_rotate_buttons':
            self.__update_rotate_buttons(value.get_bool())
        elif key == 'show_resizethumbs_slider':
            self.__update_resize_slider(value.get_bool())
        elif key == 'show_properties_button':
            self.__update_properties_button(value.get_bool())
        elif key == 'show_delete_button':
            self.__update_delete_button(value.get_bool())

    def hide_unused(self):
        # audio
        #self.mm_pause_button.hide()
        # video
        self.vm_pause_button.hide()
        # photo
        self.pm_pause_button.hide()
        #video -- thumbnail
        self.vd_th_browse_button.hide()
        #audio -- thumbnail
        self.ad_th_browse_button.hide()

    def init_ui(self):
        self.load_plugins()
        self.cur_view.pack_start(self.cur_toolbar)
        self.update_button(self.cur_mode, None)

    def load_plugins(self):
        if self.plugins.get_plugin_count() == 0:
            return
        else:
            self.plugin_buttons = self.make_plugin_buttons(self.plugins.get_plugin_list())
        self.add_bn_to_toolbar()

    def make_plugin_buttons(self, plugins):
        buttons = {}
        for mode in self.name_list.keys():
            buttons[mode] = []
            for plugin in plugins:
                button = media_plugin.PluginButton()
                button.connect_to_plugin(plugin)
                buttons[mode].append(button)
        return buttons

    def add_bn_to_toolbar(self):
        for mode in self.name_list.keys():
            for button in self.plugin_buttons[mode]:
                self.toolbar[mode].pack_end(button, False, False)

    def get_plugin_buttons(self):
        return self.plugin_buttons

    def add_filter_mode(self, dialog, mode):
        if mode in constant.MediaType.keys():
            dialog.add_filter(self.file_filter[mode])
        elif mode == 'All':
            dialog.add_filter(self.all_filter)

    def open_uri(self, filename):
        self.open_media_file(filename)

    def return_play_mode(self, button):
        if self.app.cur_playing_mode == "video":
            self.app.view['video'].update_state_playback()
            self.app.main_notebook.set_current_page(1)
            self.change_mode('video', self.app.media_type)
            self.vm_pause_button.show()
            self.app.cur_mode = constant.MODE_VIDEO
            self.app.location = constant.PBV
        elif self.app.cur_playing_mode == "audio":
            self.app.view['audio'].update_state_playback()
            self.app.main_notebook.set_current_page(0)
            self.change_mode('audio', self.app.media_type)
            self.mm_pause_button.show()
            self.app.cur_mode = constant.MODE_AUDIO
            self.app.location = constant.PBA

    def open_media_file(self, filename, no_update_ui = False):
        if not filename:
            return
        root, tmp=os.path.splitext(filename)
        ext = tmp.replace('.', '')
        if ext in ['m3u', 'pls']:
            # FIXME:
            # Most (if not all) playlist files that a user would come
            # across on the internet are list of URL's that all return
            # the same content (i.e. like a internet radio stream), with
            # the idea that multiple mirrors are in the playlist in case
            # one of the streaming servers is overloaded.
            #
            # For now we are just picking off the first URL until the
            # media infrastructure takes care of this for us.
            try:
                filename = PlayList(filename)[0]['File']
            except PlayListError:
                return utils.error_msg(_(constant.MSG_OPEN_ERROR), False)
        media_type = utils.get_media_type(filename)
        if not media_type:
            return
        dirname = filename.replace('file://','')
        self.app.view['photo'].set_cur_dir(dirname)
        if -1 == filename.find(":"):
            filename = "file://" + filename
        if media_type in ['audio', 'audio-thumb']:
            self.app.cur_playing_mode = 'audio'
            self.app.set_mediatype('audio')
            self.cur_mode = 'audio'
            self.app.location = constant.PBA
            self.app.view['audio'].song = filename
            if no_update_ui:
                pass
            else:
                self.app.view['audio'].update_state_playback()
                self.app.set_music_mode(None)
            self.app.view['audio'].update_ui()
            loadfile = filename
            self.app.load(self.app.get_mediatype(), loadfile)
        elif media_type in ['video', 'video-thumb']:
            self.app.video_listener.top_application()
            self.app.cur_playing_mode = 'video'
            self.app.set_mediatype('video')
            self.cur_mode = 'video'
            self.app.location = constant.PBV
            self.app.view['video'].update_state_playback()
            self.app.set_video_mode(None)
            loadfile = filename
            self.app.load(self.app.get_mediatype(), loadfile)
        elif media_type in ['photo', 'photo-thumb']:
            self.app.cur_mode = 'photo'
            self.app.set_mediatype('photo')
            self.cur_mode = 'photo'
            self.app.view['photo'].update_state_playback()
            self.app.set_photo_mode(None)
            if(os.path.isfile(dirname)):
                self.app.view['photo'].get_photo_playback().display_image(dirname)
            else:
                utils.error_msg(_(constant.MSG_OPEN_ERROR), False)

    def get_default_dir(self, mode):
        if mode in constant.MediaType.keys():
            return os.path.join(self.app.get_media_directory(), mode)
        else:
            pass

    def get_toolbar_name(self, mode):
        if mode in constant.MediaType.keys():
            return self.name_list[mode]
        else:
            pass

    def change_mode(self, mode, mediatype):
        self.cur_view.remove(self.cur_toolbar)
        self.cur_mode, self.cur_toolbar = mode, self.toolbar[mode]
        self.cur_view.pack_start(self.cur_toolbar)
        self.hide_unused()
        if self.app.state == constant.STATE_PLAYING:
            if mode == "audio-thumb" and mediatype == "audio":
                self.ad_th_browse_button.show()
            elif mode == "video-thumb" and mediatype == "video":
                self.vd_th_browse_button.show()
        self.load_button()
        self.update_button(mode, mediatype)

    def set_stop_button_state(self, state):
        pass

    def set_play_pause_img(self, image):
        if os.path.isfile(image):
            self.cur_play_pause_img.set_from_file(image)

    def update_button(self, mode, mediatype):
        self.set_prev_next_state(self.prev_next_state)
        self.update_plugin_button(mode)

    def update_plugin_button(self, mode):
        pass

    def set_prev_next_state(self, state):
        self.prev_next_state = state
        prev,next = state
        if prev in [True, False] and next in [True, False]:
            if self.cur_mode== 'audio':
                self.mm_prev_button.set_sensitive(prev)
                self.mm_next_button.set_sensitive(next)
            elif self.cur_mode == 'video':
                self.vm_prev_button.set_sensitive(prev)
                self.vm_next_button.set_sensitive(next)
            elif self.cur_mode == 'photo':
                self.pm_prev_button.set_sensitive(prev)
                self.pm_next_button.set_sensitive(next)

    def load_button(self):
        if self.app.get_thumb_current_mode() != 'audio':
            self.toolbar_thumb = self.app.get_current_thumb()
            self.col_size_thumb = self.toolbar_thumb.iconview.get_text_column()
            self.__update_sort_button(self.__get_bool('show_sort_button'))
            self.__update_rotate_buttons(self.__get_bool('show_rotate_buttons'))
            self.__update_resize_slider(self.__get_bool('show_resizethumbs_slider'))
            self.__update_properties_button(self.__get_bool('show_properties_button'))
            self.__update_delete_button(self.__get_bool('show_delete_button'))

    ####################################
    # Audio playback
    ####################################

    def music_prev(self, bn):
        if self.app.get_mediatype() == 'audio':
            self.app.view['audio'].play_pl_prev()

    def music_play_pause(self, bn):
        if not self.app.get_mediatype() == 'audio':
            return
        self.app.play_pause(bn)

    def music_next(self, bn):
        if self.app.get_mediatype() == 'audio':
            self.app.view['audio'].play_pl_next()

    def music_return(self, bn):
        self.app.view['audio'].update_state_playlist()
        self.change_mode('audio-thumb','audio')

    # ###################################
    # for video playback and thumbnail
    # ###################################

    def video_prev(self, bn):
        if self.app.get_mediatype() == 'video':
            self.app.view['video'].pre()

    def video_play_pause(self, bn):
        if self.app.get_mediatype() == 'video':
            self.app.play_pause(bn)

    def video_next(self, bn):
        if self.app.get_mediatype() == 'video':
            self.app.view['video'].next()

    def video_return(self, bn):
        self.app.set_fs_mode(False)
        self.app.view['video'].go_thumbnail()
        self.change_mode('video-thumb','video')

    # ###################################
    # for video and audio progress bar
    # ###################################

    def sb_change_value(self, seekbar, scrolltype, value):
        if self.seekbar_click_already == True:
            return
        self.seekbar_drag_already = True
        self.cur_pos = value
        if self.cur_pos < self.app.media_file_length:
            self.cur_pos = max(0, int(self.cur_pos))
            self.app.set_position(dbus.UInt32(self.cur_pos))
        self.seekbar_drag_already = False

    def sb_click_to_pause (self, widget, event):
        self.app.service.Pause()

    def sb_click_to_change_video_playing_time (self, widget, event):
        print 'video x=',event.x    #used to get x coordinate of toobar
        if self.seekbar_drag_already == True:
            return
        self.seekbar_click_already = True
        self.cur_pos = self.calculate_pos( event.x , constant.SeekBar_Pos['video_x_min'] ,constant.SeekBar_Pos['video_x_max'])

        if self.cur_pos < self.app.media_file_length:
            self.cur_pos = max(0, int(self.cur_pos))
            self.app.set_position(dbus.UInt32(self.cur_pos))
        else:
            self.cur_pos = self.app.media_file_length
            self.app.set_position(dbus.UInt32(self.cur_pos))
        self.seekbar_click_already = False
        self.app.service.Play()


    def sb_click_to_change_audio_playing_time (self, widget, event):
        print 'audio x=',event.x   #used to get x coordinate of toobar
        if self.seekbar_drag_already == True:
            return
        self.seekbar_click_already = True
        self.cur_pos = self.calculate_pos( event.x , constant.SeekBar_Pos['audio_x_min'] ,constant.SeekBar_Pos['audio_x_max'])

        if self.cur_pos < self.app.media_file_length:
            self.cur_pos = max(0, int(self.cur_pos))
            self.app.set_position(dbus.UInt32(self.cur_pos))
        else:
            self.cur_pos = self.app.media_file_length
            self.app.set_position(dbus.UInt32(self.cur_pos))
        self.seekbar_click_already = False
        self.app.service.Play()

    def calculate_pos (self,  click_x_pos,  x_min,  x_max):
        return self.app.media_file_length * ( (click_x_pos - x_min)  / ( x_max - x_min) )




    def get_sb_value(self):
        return self.cur_pos

    def time_btn_clicked(self,btn):
        if self.pos_time == True:
            self.pos_time = False
        else:
            self.pos_time = True

    def playback_set_seekbar_range(self, start, end, mode):
        self.whole_value = end
        if mode == "video":
            self.video_seekbar.set_range(start, end)
        elif mode == "audio":
            self.music_seekbar.set_range(start, end)

    def playback_set_seekbar_value(self, value, mode):
        if mode == "video":
            self.video_seekbar.set_value(value)
        elif mode == "audio":
            self.music_seekbar.set_value(value)
        self.cur_pos = value
        self.cur_value = self.cur_pos
        self.left_value = self.whole_value - self.cur_value
        dis_value = utils.convertSecondsToString(self.cur_value)
        tip_value = utils.convertSecondsToString(self.left_value)
        if self.pos_time == True:
            if mode == "video":
                self.video_time_btn.set_label(dis_value)
            elif mode == "audio":
                self.music_time_btn.set_label(dis_value)
        else:
            if mode == "video":
                self.video_time_btn.set_label(tip_value)
            elif mode == "audio":
                self.music_time_btn.set_label(tip_value)

    def playback_set_seekbar_increments(self, step, page,mode):
        if mode == "video":
            self.video_seekbar.set_increments(step, page)
        elif mode == "audio":
            self.music_seekbar.set_increments(step, page)

    def playback_set_seekbar_seekable(self, seekable,mode):
        if mode == "video":
            self.video_seekbar.set_sensitive(seekable)
        elif mode == "audio":
            self.music_seekbar.set_sensitive(seekable)


    # ###################################
    # for photo playback
    # ###################################

    def resize_photo(self,widget):
        photo_size = widget.get_value()
        self.photo_playback = self.app.view['photo'].get_photo_playback()
        self.photo_playback.resize(photo_size)

    def set_photo_resize_bar_value(self,value):
        self.pm_resize_bar.set_value(value)

    def photo_play_pause(self, bn):
        self.photo_playback = self.app.view['photo'].get_photo_playback()
        if self.photo_playback.is_autoplay() == False:
            self.photo_playback.autoplay()
        else:
            self.photo_playback.stopAutoplay()

    def photo_prev(self, bn):
        self.photo_playback = self.app.view['photo'].get_photo_playback()
        self.photo_playback.previous()

    def photo_next(self, bn):
        self.photo_playback = self.app.view['photo'].get_photo_playback()
        self.photo_playback.next()

    def photo_return(self, bn):
        self.photo_playback.set_fs_mode(False)
        self.photo_playback = self.app.view['photo'].get_photo_playback()
        self.photo_playback.go_thumbnail()
        self.change_mode('photo-thumb','photo')

    def photo_rotate(self, bn):
        pass

    # ###################################
    # for thumbnail and filelist
    # ###################################

    def set_thumb_size_hs_value(self,value):
        self.pm_thumb_size_bar.set_value(float(value))

    def __photo_thumbnail_resize(self, widget):
        thumbnail = self.app.view['photo'].get_photo_thumbnail()
        if (thumbnail == None):
            return
        level = widget.get_value()
        if level <  3.0:
            level = 3.0
        thumbnail.set_thumbnail_size_level(level, False)

    def __video_thumbnail_resize(self, widget):
        thumbnail = self.app.view['video'].get_video_thumbnail()
        if (thumbnail == None):
            return
        level = widget.get_value()
        if level <  3.0:
            level = 3.0
        thumbnail.set_thumbnail_size_level(level, False)

    def pm_thumb_size_bar_scroll(self,widget,event):
        pass

    def on_del_song_bn_clicked(self, bn):
        self.app.view['audio'].on_del_song_bn_clicked(bn)

    def on_add_radio_bn_clicked(self, bn):
        self.app.view['audio'].on_add_radio_bn_clicked(bn)

    def on_repeat_bn_toggled(self, bn):
        self.app.view['audio'].on_repeat_bn_toggled(bn)

    def on_shuffle_bn_clicked(self, bn):
        self.app.view['audio'].on_shuffle_bn_clicked(bn)

    def sort_name_click(self,widget):
        hint_window.HintWindow().show_hint(_(constant.MSG_PHOTO_SORT_BY_NAME))
    
        thumbnail = self.app.get_current_thumb()
        if thumbnail == None :
            return
        try:
            thumbnail.set_sort_type(constant.THUMB_SORT_FILENAME)
            cur_mode = self.app.get_thumb_current_mode()
            self.update_sort_btn('name', cur_mode)
            if self.app.location == constant.TNV:
                thumbnail.update_index_by_sort()
        except:
            pass

    def sort_date_click(self,widget):
        hint_window.HintWindow().show_hint(_(constant.MSG_PHOTO_SORT_BY_DATE))

        thumbnail = self.app.get_current_thumb()
        if thumbnail == None:
            return
        try:
            thumbnail.set_sort_type(constant.THUMB_SORT_DATE)
            cur_mode = self.app.get_thumb_current_mode()
            self.update_sort_btn('date', cur_mode)
            if self.app.location == constant.TNV:
                thumbnail.update_index_by_sort()
        except:
            pass

    def update_sort_btn(self, value, mode):
        if value == 'date':
            if mode == 'photo':
                self.ph_sort_date_btn.hide()
                self.ph_sort_name_btn.show()
            elif mode == 'video':
                self.vd_sort_date_btn.hide()
                self.vd_sort_name_btn.show()
        elif value == 'name':
            if mode == 'photo':
                self.ph_sort_name_btn.hide()
                self.ph_sort_date_btn.show()
            elif mode == 'video':
                self.vd_sort_name_btn.hide()
                self.vd_sort_date_btn.show()

    def set_photo_autoplay_image_change(self):
        self.pm_autoplay_button.hide()
        self.pm_pause_button.show()

    def set_photo_autoplay_image_default(self):
        self.pm_pause_button.hide()
        self.pm_autoplay_button.show()

    def set_fs_mode(self, on):
        pass

    def set_photo_btn_sensitive(self,value):
        self.ph_th_trash_btn.set_sensitive(value)
        self.ph_th_info_btn.set_sensitive(value)
        self.ph_th_left_btn.set_sensitive(value)
        self.ph_th_right_btn.set_sensitive(value)

    def set_video_btn_sensitive(self,value):
        self.vd_th_trash_btn.set_sensitive(value)
        self.vd_th_info_btn.set_sensitive(value)

    def hide_photo_op_buttons(self):
        pass

    def show_photo_op_buttons(self):
        pass

    def trash_delete(self, btn):
        self.app.TopMenu.on_edit_delete(None)

    def trash_delete_audio(self, btn):
        self.app.view['audio'].on_del_song()

    def file_info_audio(self, btn):
        self.app.view['audio'].on_edit_properities(None)

    def file_info(self, btn):
        self.app.TopMenu.on_edit_properties(None)

    def left_rotate(self, btn):
        self.app.TopMenu.on_edit_rotate_counterclockwise(None)

    def right_rotate(self, btn):
        self.app.TopMenu.on_edit_rotate_clockwise(None)

    def __get_bool(self, key):
        mode = self.app.get_thumb_current_mode()
        key = "%s/%s/%s" % (constant.GCONF_PATH, mode, key)
        return self.__client.get_bool(key)

    def __get_float(self, key):
        mode = self.app.get_thumb_current_mode()
        key = "%s/%s/%s" % (constant.GCONF_PATH, mode, key)
        return self.__client.get_float(key)
