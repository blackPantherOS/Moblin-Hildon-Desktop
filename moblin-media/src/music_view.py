#!/usr/bin/python -ttu
# -*- mode:python; tab-width:4; indent-tabs-mode:nil;  -*-
# vim: ai ts=4 sts=4 et sw=4
#
# music_view.py: Manage the Music View
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
import gtk.glade
import moko
import os
import pango
import shutil
import sys
import re

# import our app's libraries
from playlist import PlayListCollection, PlayListError
import constant
import image_finger
import media_info
import thumb_base
import utils

_ = gettext.lgettext

(VIEW_LIST_SONGS,   #view of songs (from local files)
 VIEW_LIST_ONLINE,  #view of online playlist
 VIEW_PLAY_SONG,    #view of single song playing
 VIEW_PLAY_ONLINE   #view of single online stream playing
 ) = range (4)

NB_MAIN_LIST = 0
NB_MAIN_PLAYBACK = 1
NB_PLAYLIST_SONGS = 0
NB_PLAYLIST_ONLINE = 1

(COLUMN_STATE,
 COLUMN_URI,
 COLUMN_TITLE,
 COLUMN_ARTIST,
 COLUMN_ALBUM,
 COLUMN_TIME,
 COLUMN_RAWLEN,
 ) = range(7)

(PL_COLUMN_STATE,
 PL_COLUMN_STATE_PIXBUF,
 PL_COLUMN_URI,
 PL_COLUMN_NAME,
 ) = range(4)

(TARGET_PL_LIST,
 TARGET_SONG_LIST,
) = range(2)

(COLUMN_STATE_ACTIVE,
 COLUMN_STATE_INACTIVE,
) = range(2)

class MusicView(thumb_base.ThumbBase):
    """A class which manages the audio view."""

    def __init__(self, app):
        """init function"""
        thumb_base.ThumbBase.__init__(self)
        self.__updating_song_list = False
        self.set_align_len(40)
        self.app = app
        self._repeat_mode = False
        self.wTree = utils.get_widget_tree(constant.MAIN_GLADE_FILE,
                                           root='mp_music_nbk')

        #Top-level GtkNotebook has two views: list and singe-song
        self.view_nbk = self.wTree.get_widget('mp_music_nbk')
        self.view_nbk.set_show_tabs(False)

        #Playlist page contains GtkNotebook with two sub-pages for songs and online music
        self.pl_nbk = self.wTree.get_widget('playlist_nbk')
        self.pl_nbk.set_show_tabs(False)

        self.item_selected = -1

        self.__current_view = VIEW_LIST_SONGS #self.state_playlist
        self.song_radio_status_current = -1 #self.state_song

        # if first launch
        self.is_first_launch = True

        # song is the column selected in the playlist songtree
        self.song = None
        self.title = ''
        self.album = ''
        self.artist = ''
        self.rawlen = 0

        self.index = None

        self.init_playback_screen()
        
        # init playlist view
        self.music_window = self.wTree.get_widget('mp_music_window')
        self.music_playlist_view = self.wTree.get_widget('mm_playlist_view_eb')
        self.pl_tree = self.wTree.get_widget('mm_pl_playlist_tree')
        self.song_tree = gtk.TreeView()
        self.radio_tree = gtk.TreeView()

        #add moko into notebook
        self.moko_list = moko.FingerScroll()
        self.moko_list.set_property('spring_color',0x000000)
        self.moko_list.set_property('is_detect_double_click', False)
        self.pl_nbk.append_page(self.moko_list)
        self.moko_list.add(self.song_tree)

        self.moko_radio_list = moko.FingerScroll()
        self.moko_radio_list.set_property('spring_color',0x000000)
        self.moko_radio_list.set_property('is_detect_double_click', False)
        self.pl_nbk.append_page(self.moko_radio_list)
        self.moko_radio_list.add(self.radio_tree)
        #####################################################################
        # FIXME:
        # This variable should be a static value, which the new playlist 
        # button will use to make a new list naming "playlist%d" % self.count
        self.count = 3
        list_dir = os.path.join(self.app.get_media_directory(), '.playlist')
        audio_dir = os.path.join(self.app.get_media_directory(), 'audio')
        self.media = PlayListCollection(list_dir, audio_dir)
        self.view_playlist = self.cur_playlist = self.media[0]
        self.pl_model = gtk.ListStore(int, # Column state
                                      str, # Status pixbuf
                                      str, # Source URI
                                      str, # Source Name
                                      )
        self.pl_tree.append_column(TextColumn(title=_(constant.MUSIC_PLAYLIST),
                                              model_index=PL_COLUMN_NAME,
                                              expand=False,
                                              editable=False,
                                              width=300))
        self.pl_tree.set_model(self.pl_model)
        self.update_pl_view()
        self.pl_tree.get_selection().set_mode(gtk.SELECTION_SINGLE)
        self.pl_tree.get_selection().connect("changed", self._on_pl_sel_changed)

        self.song_model, self.song_modelsort = self.create_song_model()
        self.song_tree.set_model(self.song_model)
        self.song_tree.set_search_column(COLUMN_TITLE)
        self.song_tree.get_selection().set_mode(gtk.SELECTION_SINGLE)
        self.radio_model, self.radio_modelsort = self.create_radio_model()
        self.radio_tree.set_model(self.radio_model)
        self.radio_tree.set_search_column(COLUMN_TITLE)        
        self.radio_tree.get_selection().set_mode(gtk.SELECTION_SINGLE)
        self.dnd_target = [
            ('pl_list', 0, TARGET_PL_LIST),
            ('song_list', 0, TARGET_SONG_LIST),
        ]
        self._pending_event = None
        self.song_tree.connect("button-press-event", self._on_song_tree_button_press_event)
        self.song_tree.connect("button-release-event", self.button_release_event, None)
        self.radio_tree.connect("button-release-event", self.button_release_event, None)

        widget = self.wTree.get_widget('mm_pl_view_paned')
        widget.set_position(150)
        self.music_album_vp = self.wTree.get_widget('mm_album_view_port')

        self._art_dir = constant.MediaAlbumCoverDir
        if not os.path.isdir(self._art_dir):
            os.makedirs(self._art_dir)

        db = os.path.join(self.app.get_media_directory(), '.playlist', 'online.db')
        self.__db = media_info.MediaDatabase(db)

        self.update_view()

    def init_playback_screen(self):
        widget = self.wTree.get_widget('mm_playback_view_eb')
        widget.modify_bg(gtk.STATE_NORMAL, constant.MediaColor['select_window_bg'])

        # init playback view
        self.title_lb = self.wTree.get_widget("mm_song_title_lb")
        self.title_lb.modify_font(pango.FontDescription("sans bold 18"))
        self.title_lb.modify_fg(gtk.STATE_NORMAL, constant.MediaColor['select_label_fg'])
        
        self.album_lb = self.wTree.get_widget("mm_album_title_lb")
        self.album_lb.modify_fg(gtk.STATE_NORMAL, constant.MediaColor['select_label_fg'])
        
        self.artist_lb = self.wTree.get_widget("mm_song_artist_lb")
        self.artist_lb.modify_fg(gtk.STATE_NORMAL, constant.MediaColor['select_label_fg'])
        
        self.album_year_lb = self.wTree.get_widget("mm_album_year_lb")
        self.album_year_lb.modify_fg(gtk.STATE_NORMAL, constant.MediaColor['select_label_fg'])

        self.albumimg_alloc = None

        self.albumimg_hb = self.wTree.get_widget("mm_albumimg_hb")
        self.albumimg_hb.connect ("size-allocate", self.on_image_size_allocate)

        self.album_img = image_finger.image_finger(False)
        self.album_img.modify_bg(gtk.STATE_NORMAL, constant.MediaColor['select_window_bg'])
        self.albumimg_hb.pack_start(self.album_img)

        self.playback_surface = None

    def update_view(self):
        if self.__current_view == VIEW_LIST_SONGS:
            self.view_nbk.set_current_page(NB_MAIN_LIST) # "mm_playlist_view_eb"
            self.pl_nbk.set_current_page(NB_PLAYLIST_SONGS)
        elif self.__current_view == VIEW_LIST_ONLINE:
            self.view_nbk.set_current_page(NB_MAIN_LIST) # "mm_playlist_view_eb"
            self.pl_nbk.set_current_page(NB_PLAYLIST_ONLINE)
        elif self.__current_view == VIEW_PLAY_ONLINE or self.__current_view == VIEW_PLAY_SONG:
            self.view_nbk.set_current_page(NB_MAIN_PLAYBACK) # "mm_playlist_view_eb"
        return self.__current_view
	
    def update_ui(self):
        song_data = media_info.MediaInfo(uri=self.song, db=self.__db)
        self.title = song_data['title']
        self.album = song_data['album']
        self.artist = song_data['artist']
        self.rawlen = song_data['rawlen']
        try:
            self.title_lb.set_text(song_data['title'])
        except:
            self.title_lb.set_text('')
        try:
            self.album_lb.set_text(song_data['album'])
            self.artist_lb.set_text(song_data['artist'])
            self.album_year_lb.set_text('')
        except TypeError:
            self.album_lb.set_text('')
            self.artist_lb.set_text('')
            self.album_year_lb.set_text('')
            # No metadata found, so leave the song view entries empty
            pass
        art = os.path.join(self._art_dir, self.song.replace('/', '_'))

        # try image. If fails, show missing art image
        is_valid_art = False
        if os.path.isfile(art):
            is_valid_art = self.set_album_img(art)
        if not is_valid_art:
            if (self.__current_view == VIEW_PLAY_SONG):
                self.set_album_img(os.path.join(constant.MediaSpecImagePath,'AlbumArtMissing.png'))
            else:
                self.set_album_img(os.path.join(constant.MediaSpecImagePath,'AlbumArtMissingOnline.png'))

    def update_media_info(self, key, value):
        info = media_info.MediaInfo(uri=self.song, db=self.__db)
        if key == "title":
            if value and value != self.title:
                self.playback_set_title_lb( "%s" % value )
                self.title = value
                info['title'] = value
                self.update_song_tree_item(COLUMN_TITLE, value)
        if key == "album":
            if value and value != self.album:
                self.playback_set_album_lb( "%s" % value )
                self.album = value
                info['album'] = value   
                self.update_song_tree_item(COLUMN_ALBUM, value) 
        if key == "artist":
            if value and value != self.artist:
                self.playback_set_artist_lb( "%s" % value )
                self.artist = value
                info['artist'] = value   
                self.update_song_tree_item(COLUMN_ARTIST, value) 
        if key == "album_year":
            if value:
                self.playback_set_album_year_lb( "%s" % value )
        if key == "image":
            target = os.path.join(self._art_dir, self.song.replace('/', '_'))
            if not os.path.isfile(target):
                shutil.copy('/tmp/cover.jpg',  target)
            self.set_album_img(target)
        if key == "duration":
            if value == -1:
                value = 0
            self.app.media_file_length = value/1000
            self.app.view['toolbar'].playback_set_seekbar_range(0, self.app.media_file_length, 'audio')
            self.app.view['toolbar'].playback_set_seekbar_increments(1, 0, 'audio')
            if value/1000 != self.rawlen:
                self.rawlen = value/1000
                info['rawlen'] = self.rawlen
                length = utils.convertSecondsToString(self.rawlen)
                self.update_song_tree_item(COLUMN_RAWLEN, self.rawlen)
                self.update_song_tree_item(COLUMN_TIME, length)

    def update_state_playlist(self):
        if self.__current_view == VIEW_PLAY_ONLINE:
            self.__current_view = VIEW_LIST_ONLINE 
        else:
            self.__current_view = VIEW_LIST_SONGS 
        self.update_view()

    def update_state_playback(self):
        if self.__current_view == VIEW_LIST_ONLINE:
            self.__current_view = VIEW_PLAY_ONLINE
        else:
            self.__current_view = VIEW_PLAY_SONG

        self.update_view()

    def get_render_surface(self):
        self.playback_surface = self.wTree.get_widget('mp_music_nbk')
        return self.playback_surface

    def is_first_load(self):
        return (self.is_first_launch == True)

    def mark_default_flag(self):
        self.is_first_launch = False
        self.update_ui()  

    def on_image_size_allocate(self , widget , allocation):
        if (not self.app.get_fs_mode()):
            self.albumimg_alloc = allocation

    def set_album_img(self, filename):
        return self.album_img.set_picture(filename, self.app.get_fs_mode())
        
    def playback_set_title_lb(self, text):
        self.title_lb.set_text(text)

    def playback_set_album_lb(self, text):
        self.album_lb.set_text(text)

    def playback_set_artist_lb(self, text):
        self.artist_lb.set_text(text)

    def playback_set_album_year_lb(self, text):
        self.album_year_lb.set_text(text)

    #update playlist from view to playlist, for sorting
    def update_pl(self, pl):
        if self.__updating_song_list:
            return
        entry = pl.selected
        pl.clear()
        for row in self.song_model:
            pl.append({"File": row[COLUMN_URI]})
        if entry:
            pl.selected = entry
        pl.flush()

   #two single clicks to play a song 
    def button_release_event(self,widget,event, data):
        model = widget.get_model()
        item = widget.get_path_at_pos(int(event.x), int(event.y))
        if item:
            song = model[item[0][0]][COLUMN_URI]
            if song == self.item_selected:
                self.app.view['toolbar'].last_song = song
                self.play_pl_song(song)
            else:
                self.item_selected = song
        else:
            self.item_selected = -1
                
    # ###################################
    # for playlist view signal handler
    # ###################################
            
    #switch between song and radio/streams view
    def _on_pl_sel_changed(self, sel, resort=False):
        model, path_list = sel.get_selected_rows()
        if len(path_list) == 0:
            return
        
        self.item_selected = -1
        name = model[path_list[0]][PL_COLUMN_NAME]
        pl = self.media.get_list(name)
        if name == _(constant.ALL_SONG_PLAYLIST_NAME):
            self.__current_view = VIEW_LIST_SONGS
            self.view_playlist = pl
            self.app.view['toolbar'].ad_th_trash_bn.show()
            self.app.view['toolbar'].ad_th_repeat_bn.show()
            self.app.view['toolbar'].wm_shuffle_bn.show()
            self.app.view['toolbar'].delradio_bn.hide()
            self.app.view['toolbar'].addradio_bn.hide()
            self.update_song_view(pl)
        else:  #ONLINE playlist
            self.__current_view = VIEW_LIST_ONLINE
            self.app.view['toolbar'].ad_th_trash_bn.hide()
            self.app.view['toolbar'].ad_th_repeat_bn.hide()
            self.app.view['toolbar'].wm_shuffle_bn.hide()
            self.app.view['toolbar'].delradio_bn.show()
            self.app.view['toolbar'].addradio_bn.show()
            self.update_radio_view(pl)
        self.update_view()
                
    def get_pl_tree_selected_item(self):
        model, path_list = self.pl_tree.get_selection().get_selected_rows()
        if not len(path_list) == 0:
            name = model[path_list[0]][PL_COLUMN_NAME]
            pl = self.media.get_list(name)
            self.view_playlist = pl

    def on_mm_pl_playlist_tree_row_activated(self, treeview, path, column):
        model = treeview.get_model()
        name = model[path][PL_COLUMN_NAME]
        pl = self.media.get_list(name)
        if name == __(constant.ONLINE_MUSIC_PLAYLIST_NAME):
                self.update_radio_view(pl)
        else:
                self.update_song_view(pl)
    
    def _on_song_tree_button_press_event(self, treeview, event):
        if event.button == 1:
            return False
        else:
            return False

    def _set_song_tree_row_status(self, model, stock_id=None, row=None):
        if row is None:
            row = self.cur_playlist.selected_index
        if row < 0 or row >= len(self.cur_playlist):
            return
        try:
            iter = model.get_iter(row)
            model.set_value(iter, COLUMN_STATE, stock_id)
        except:
            print "exception in iter"


    def update_song_tree_item(self, item_index, item_value):
        if self.__current_view == VIEW_LIST_SONGS:
            model, iter = self.song_tree.get_selection().get_selected()
        else:
            if item_index != COLUMN_TITLE:
                # The ONLINE playlist only contains 'Title' and 'URL'
                # fields, and of that, only the title is ever updated from
                # the stream
                return
            model, iter = self.radio_tree.get_selection().get_selected()
        if iter:
            model.set_value(iter, item_index, item_value)
    
    def get_selected_rows(self, treeview):
        '''Returns a sorted list of the selected rows for special treeview'''
        rows = []
        model, path_list = treeview.get_selection().get_selected_rows()
        iters = [model.get_iter(path) for path in path_list]
        for iter in iters:
            row = model.get_path(iter)[0]
            rows.append(row)
        rows.sort()
        return rows
    
    def create_song_model(self):
        lstore = gtk.ListStore(str, # Column state
                                str, # Source URI
                                str, # Source title
                                str, # Source artist
                                str, # Source album
                                str, # Source time (formatted)
                                int, # Source length (no formatted)
                               )
        lstore_sort = gtk.TreeModelSort(lstore)
        lstore.set_default_sort_func(self.default_sort_function)
        lstore_sort.set_sort_column_id(COLUMN_TITLE, gtk.SORT_ASCENDING)
        lstore.connect('rows-reordered', self.on_rows_reordered)
        self.add_song_columns()
        return lstore, lstore_sort

    #The default sort function will not compare the two elements and just returns 0
    def default_sort_function(self, treemodel, iter1, iter2):
        return 0
    
    def on_rows_reordered(self, model, path, iter, new_order):
        self.update_pl(self.view_playlist)
        if self._repeat_mode:
            ctxt = [True, True]
            self.app.view['toolbar'].set_prev_next_state(ctxt)
        else:
            entry = self.view_playlist.selected
            if entry:
                uri = entry['File']
                ctxt = self.cur_playlist.context_info(uri)
                self.app.view['toolbar'].set_prev_next_state(ctxt)

    def create_radio_model(self):
        lstore = gtk.ListStore(str, # Column state
                                str, # Source title
                                str, # Source URI
                                str, # Source time (formatted)
                               )
        lstore_sort = gtk.TreeModelSort(lstore)
        lstore_sort.set_sort_column_id(COLUMN_TITLE, gtk.SORT_ASCENDING)
        self.add_radio_columns()
        return lstore, lstore_sort

    def refresh(self):
        self.media[0].refresh()
        self.update_song_view(self.media[0])

    def reset_play_icon(self):
        self.cur_playlist.selected_index = -1
        if self.__current_view == VIEW_LIST_SONGS:
            self.update_song_list_status_pixmap()
        else:
            self.update_radio_list_status_pixmap()

    def update_song_view(self, pl):
        self.view_playlist = pl
        self.update_song_model()
        
    def update_radio_view(self, pl):
        self.view_playlist = pl
        self.update_radio_model()
                
    def update_song_model(self):
        if self.view_playlist.name == _(constant.ONLINE_MUSIC_PLAYLIST_NAME):
            return
        self.__updating_song_list = True
        #Shuffle should not sort the list. using default sort function 
        #which return 0 on compare
        self.song_model.set_sort_column_id(-1, gtk.SORT_ASCENDING)
        self.song_model.clear()
        for entry in self.view_playlist:
            song = entry['File']
            newrow = self.new_song_model_row(song)
            if newrow:
                row = self.song_model.append(newrow)
                self.song_modelsort.convert_child_iter_to_iter(None, row)
        self.__updating_song_list = False
        self.song_tree.set_model(self.song_model)
        self.update_song_list_status_pixmap()
        
    def update_radio_model(self):
        if self.view_playlist.name != _(constant.ONLINE_MUSIC_PLAYLIST_NAME):
            return
        self.radio_model.clear()
        for entry in self.view_playlist:
            try:
                info = media_info.MediaInfo(uri=entry['File'], db=self.__db)
            except:
                print "Exception adding radio. Ignoring entry: %s." % entry['File']
                continue
            newrow = self.new_radio_model_row(entry['File'], info['title'])
            row = self.radio_model.append(newrow)
        self.radio_tree.set_model(self.radio_model)
        self.update_radio_list_status_pixmap()

    def add_song_columns(self):
        self.song_tree.append_column(StateColumn(width=30))
        self.song_tree.append_column(TextColumn(title=_(constant.MUSIC_TITLE),
                                                expand=True,
                                                model_index=COLUMN_TITLE,
                                                sort_index=COLUMN_TITLE,
                                                width=200))
        self.song_tree.append_column(TextColumn(title=_(constant.MUSIC_LENGTH),
                                                model_index=COLUMN_TIME,
                                                sort_index=COLUMN_RAWLEN,
                                                width=100))
        self.song_tree.append_column(TextColumn(title=_(constant.MUSIC_ARTIST),
                                                model_index=COLUMN_ARTIST,
                                                sort_index=COLUMN_ARTIST,
                                                width=200))
        self.song_tree.append_column(TextColumn(title=_(constant.MUSIC_ALBUM),
                                                model_index=COLUMN_ALBUM,
                                                sort_index=COLUMN_ALBUM,
                                                width=200))
        
    def add_radio_columns(self):
        self.radio_tree.append_column(StateColumn(width=30))
        self.radio_tree.append_column(TextColumn(title=_(constant.MUSIC_TITLE),
                                                 model_index=COLUMN_TITLE,
                                                 expand=True,
                                                 width=200))
        self.radio_tree.append_column(TextColumn(title=_(constant.MUSIC_URL),
                                                 model_index=COLUMN_URI,
                                                 expand=True,
                                                 width=200))
        
    def new_song_model_row(self, song):
        song_data = media_info.MediaInfo(uri=song, db=self.__db)
        return [None,
                song,
                song_data['title'],
                song_data['artist'],
                song_data['album'],
                song_data['length'],
                song_data['rawlen'],
            ]
            
    def new_radio_model_row(self, song, url):
        return [None, song, url, None]

    def on_del_song_bn_clicked(self, bn):
        self.on_del_song()

    def on_del_song(self):

        model, path_list = self.pl_tree.get_selection().get_selected_rows()
        if not len(path_list) == 0:
            list_name = self.pl_model[path_list[0]][PL_COLUMN_NAME]
        else:
            list_name = self.pl_model[0][PL_COLUMN_NAME]
        pl = self.media.get_list(list_name)


        # get list of playlist titles (All Songs, Online Music, etc)
        if self.__current_view == VIEW_LIST_SONGS:
            model, path_list = self.song_tree.get_selection().get_selected_rows()
        elif self.__current_view == VIEW_LIST_ONLINE:
            model, path_list = self.radio_tree.get_selection().get_selected_rows()

        #if nothing selected, exit
        if len(path_list) == 0:
            utils.info_dialog(_(constant.MSG_CONFIRM_DELETE_TITLE),
                               _(constant.MSG_INFO_NO_ITEM_FOR_DELETE))
            return

        #if multiselect, songs could be multiple, thus the "songs" name
        songs = [model[path][COLUMN_URI] for path in path_list]
        titles = [model[path][COLUMN_TITLE] for path in path_list]

        cur_item = pl.selected
        if not cur_item:
            cur_song = None
        else:
            cur_song = cur_item['File']

        if list_name == _(constant.ALL_SONG_PLAYLIST_NAME):
            for song in songs:
                if cur_song == song:
                    utils.error_msg(_(constant.MSG_DELETE_PLAYING_SONG_ERROR))
                    continue
                if self.trash(song):
                    pl.selected = ({'File': song})
                    index = pl.selected_index  #store current index
                    del pl[pl.selected_index]
                    try:
                        # want to select next avail item, but can't figure it out
                        count = len(pl)
                        if count == 0:
                            return
                        if index == count:
                            index = index - 1
                        pl.selected = ({'File': index})
                    except PlayListError:
                        pass
            self.update_song_view(pl)
        else:
            for song in songs:
                if cur_song == song:
                    utils.error_msg(_(constant.MSG_DELETE_PLAYING_SONG_ERROR))
                    continue
                dlg_title = _(constant.MSG_CONFIRM_DELETE_TITLE)
                msg = _(constant.MSG_CONFIRM_DELETE_QUESTION) % (titles[songs.index(song)])
                if gtk.RESPONSE_YES == utils.confirm_dialog(dlg_title, msg):
                    pl.selected = ({'File': song})
                    del pl[pl.selected_index]
                    try:
                        pl.selected = ({'File': cur_song})
                    except PlayListError:
                        pass
            self.update_radio_view(pl)    

    def on_edit_properities(self,widget):
        filename =None
        size = None
        if self.__current_view == VIEW_LIST_SONGS:
            model, path_list = self.song_tree.get_selection().get_selected_rows()
            #if nothing selected, exit
            if len(path_list) == 0:
                utils.info_dialog(_(constant.MSG_PROPERTIES_TITLE),
                                   _(constant.MSG_INFO_NO_ITEM_FOR_PROPS))
                return
                
            item = self.song_model[path_list[0]]
            title = item[COLUMN_TITLE]
            artist = item[COLUMN_ARTIST]
            album = item[COLUMN_ALBUM]
            file_dir = item[COLUMN_URI]
            file_name = (os.path.split(file_dir))[1]
            time = item[COLUMN_TIME]
            file_info = os.stat(file_dir)
            size = file_info[-4]

            audio_glade = utils.get_widget_tree('dialogs.glade','properties_audio')
            prop_dlg = audio_glade.get_widget('properties_audio')
            prop_dlg.set_modal(True)
            title_lb = audio_glade.get_widget('title_lb')
            artist_lb = audio_glade.get_widget('artist_lb')
            album_lb = audio_glade.get_widget('album_lb')
            filename_lb = audio_glade.get_widget('filename_lb')
            length_lb = audio_glade.get_widget('length_lb')
            filesize_lb = audio_glade.get_widget('filesize_lb')

            title_lb.set_label(title)
            artist_lb.set_label(artist)
            album_lb.set_label(album)
            filename_lb.set_label(file_name)
            length_lb.set_label(time)
            filesize_lb.set_label(self.calculate_size(size))
            
            prop_dlg.run()        
            prop_dlg.destroy()   

        elif self.__current_view == VIEW_LIST_ONLINE:
            model, path_list = self.radio_tree.get_selection().get_selected_rows()            
            if len(path_list) == 0:
                utils.info_dialog(_(constant.MSG_PROPERTIES_TITLE),
                                   _(constant.MSG_INFO_NO_ITEM_FOR_PROPS))
                return

            item = self.radio_model[path_list[0]]
            title = item[COLUMN_TITLE]
            url = item[COLUMN_URI]

            # don't allow editing playing song
            if (self.song == url):
                utils.error_msg(_(constant.MSG_EDIT_PLAYING_SONG_ERROR))
                return

            new_title, new_url = self.show_radio_dialog (title, url)
            if (len(new_title)==0):  #user hit cancel
                return
            item[COLUMN_TITLE] = new_title
            item[COLUMN_URI] = new_url

            # we delete the old item and re-add it (tbd: fix--url should not be a key)
            index = self.view_playlist.selected_index
            # select old record (key is url)
            self.view_playlist.selected = ({'File': url})
            del self.view_playlist[self.view_playlist.selected_index]
            self.view_playlist.selected_index = index
            self.radio_add_url(new_title, new_url)
            self.view_playlist.selected = ({'File': new_url})

    def on_add_radio_bn_clicked(self, bn):
        title, url = self.show_radio_dialog ()
        if (len(title)!=0):
            self.radio_add_url(title, url)
 
    def show_radio_dialog(self, title="", url="http://"):
        audio_properties_glade = utils.get_widget_tree('dialogs.glade','properties_online_music')
        prop_dlg = audio_properties_glade.get_widget('properties_online_music')
        prop_dlg.set_modal(True)
        title_edit = audio_properties_glade.get_widget('title_edit')
        url_edit   = audio_properties_glade.get_widget('url_edit')
        title_edit.set_text(title)
        title_edit.grab_focus()
        url_edit.set_text(url)

        while True:
            if prop_dlg.run() == gtk.RESPONSE_OK:
                title = title_edit.get_text()
                url = url_edit.get_text()
                # tweak values and update, then check correctness
                title = title.strip()  # trim whitespace
                url = url.strip()      # trim whitespace
                # if check for <protocol>://<something>.  prepend "http" if not exist
                p = re.compile(r'^([^:/?#]+)://.*')
                if not p.match(url):
                    url = "http://" + url
                title_edit.set_text(title)
                url_edit.set_text(url)
                # check correctness of entries.  Repeat if invalid 
                if len (title)==0 :
                    utils.info_dialog (_(constant.ONLINE_MUSIC_PLAYLIST_NAME),
                                       _(constant.MSG_ONLINE_MUSIC_NO_TITLE))
                    title_edit.grab_focus()
                    continue
                if not utils.is_valid_url(url):
                    utils.info_dialog (_(constant.ONLINE_MUSIC_PLAYLIST_NAME),
                                       _(constant.MSG_ONLINE_MUSIC_INVALID_URL))
                    url_edit.grab_focus()
                    continue
                break  # Success
            else : # Cancel button clicked
                title = url = ""
                break
        prop_dlg.destroy()
        return title, url

    def radio_add_url(self, title, url):
        try:
            pl = self.media.get_list(_(constant.ONLINE_MUSIC_PLAYLIST_NAME))
            pl.append({'File': url})
            pl.flush()
            # create MediaInfo db object and set title
            info = media_info.MediaInfo(uri=url, db=self.__db)
            if title:
                info['title'] = title

            for i in range(len(self.pl_model)):
                if self.pl_model[i][PL_COLUMN_NAME] == _(constant.ONLINE_MUSIC_PLAYLIST_NAME):
                    self.pl_tree.set_cursor((i,))
            self.update_radio_view(pl)
        except:
            print "Exception adding radio", sys.exc_info()[0]
            utils.info_dialog (_(constant.MSG_ERROR_TITLE),
                               _(constant.MSG_ONLINE_MUSIC_ADD_ERROR))
            
    def update_pl_view(self, path=0):
        self.update_pl_model()
        if type(path) == type((1,)):
            path = path[0]
        if path >= len(self.pl_model):
            path = len(self.pl_model)-1
        elif path < 0:
            path = 0
        self.pl_tree.set_cursor(path)
    
    def update_pl_model(self):
        self.pl_model.clear()
        for pl in self.media:
            self.pl_model.append(self.new_pl_model_row(pl))
        self.pl_tree.set_model(self.pl_model)
    
    def new_pl_model_row(self, pl):
        return [PL_COLUMN_STATE,
                None,
                None,
                pl.name,
               ]
        
    def on_shuffle_bn_clicked(self, bn):
        # Shuffle the playlist while making sure to keep
        # the currently selected song correct
        entry = self.cur_playlist.selected
        self.cur_playlist.shuffle()
        if entry:
            context = self.cur_playlist.context_info(entry['File'])
            self.app.view['toolbar'].set_prev_next_state(context)
            self.cur_playlist.selected = entry      

        # Update the UI to reflect the new playlist order, 
        # and be sure to get the 'prev' and 'next' button
        # states correct for the new ordering
        self.update_song_view(self.cur_playlist)
                                                     
    def on_repeat_bn_toggled(self, bn):
            # If we are in repeat mode, then automatically rewind
        self._repeat_mode = bn.get_active()
        if self._repeat_mode:
            # While in repeat mode, there is always a 'next' and 'previous'
            # song in the playlist
            self.app.view['toolbar'].set_prev_next_state([True, True])
        else:
            # Now that we are no longer in repeat mode, set the 'next' and
            # 'prev' buttons to reflect reality
            entry = self.cur_playlist.selected
            if entry:
                context = self.cur_playlist.context_info(entry['File'])
                self.app.view['toolbar'].set_prev_next_state(context)
        self.update_song_view(self.cur_playlist)
        
    def on_song_ended(self, object):
        self.play_pl_next()
        
    def play_pl_next(self):
        try:
            self.cur_playlist.selected_index = self.cur_playlist.selected_index + 1
        except IndexError:
            # If we are in repeat mode, then automatically rewind back to
            # the beginning of the playlist
            if self._repeat_mode:
                self.cur_playlist.selected_index = 0
            else:
                raise IndexError('Attempting to play past end of playlist')
        uri = self.cur_playlist[self.cur_playlist.selected_index]['File']
        self.play(uri, True)
            
    def play_pl_prev(self):
        try:
            self.cur_playlist.selected_index = self.cur_playlist.selected_index - 1
        except IndexError:
            # If we are in repeat mode, then automatically jump to the last
            # entry in the playlist
            if self._repeat_mode:
                self.cur_playlist.selected_index = len(self.cur_playlist) - 1
            else:
                raise IndexError('Attempting to play past end of playlist')
        uri = self.cur_playlist[self.cur_playlist.selected_index]['File']
        self.play(uri, True)
        
    def play_pl_song(self, song, no_update_ui = False):
        self.cur_playlist = self.view_playlist
        self.cur_playlist.selected = {'File': song}
        self.play(song, no_update_ui)
        
    def play(self, uri, update_ui):
        self.song = uri
        if self.cur_playlist.name == _(constant.ONLINE_MUSIC_PLAYLIST_NAME):
            self.update_radio_list_status_pixmap()
        else:
            self.update_song_list_status_pixmap()
        if self._repeat_mode:
            ctxt = [True, True]
        else:
            ctxt = self.cur_playlist.context_info(uri)
        self.app.view['toolbar'].set_prev_next_state(ctxt)
        if uri[0] == '/':
            uri = 'file://' + uri
        self.app.view['toolbar'].open_media_file(uri, update_ui)

    def update_song_list_status_pixmap(self):
        # update the state pixmap
        if self.cur_playlist != self.view_playlist:
            return
        for i in range(0, len(self.cur_playlist)):
            self._set_song_tree_row_status(self.song_model, None, i)
        cur_index = self.cur_playlist.selected_index
        if cur_index >= 0:
            self._set_song_tree_row_status(self.song_model,
                                           gtk.STOCK_MEDIA_PLAY,
                                           cur_index)
            self.song_tree.set_cursor(cur_index)

    def update_radio_list_status_pixmap(self):
        # update the state pixmap
        if self.cur_playlist != self.view_playlist:
            return
        for i in range(0, len(self.cur_playlist)):
            self._set_song_tree_row_status(self.radio_model, None, i)
        cur_index = self.cur_playlist.selected_index
        if cur_index >= 0:
            self._set_song_tree_row_status(self.radio_model,
                                           gtk.STOCK_MEDIA_PLAY,
                                           cur_index)
            self.radio_tree.set_cursor(cur_index)

class TextColumn(gtk.TreeViewColumn):
    def __init__(self, title, model_index, expand=False, editable=False,
                 width=None, sort_index=-1):
        gtk.TreeViewColumn.__init__(self, title)
        renderer = gtk.CellRendererText()
        renderer.set_property('ellipsize', pango.ELLIPSIZE_END)
        renderer.set_property('editable', editable)
        self.pack_start(renderer, True)
        self.add_attribute(renderer, 'text', model_index)
        self.set_resizable(True)
        self.set_reorderable(True)
        self.set_property('expand', expand)
        if sort_index <> -1:
            self.set_sort_column_id(sort_index)
        if not width == None:
            self.set_sizing(gtk.TREE_VIEW_COLUMN_FIXED)
            self.set_fixed_width(width)

class StateColumn(gtk.TreeViewColumn):
    def __init__(self, width=None):
        gtk.TreeViewColumn.__init__(self, ' ' * 2)
        renderer = gtk.CellRendererPixbuf()
        renderer.set_property('stock-size', gtk.ICON_SIZE_MENU)
        self.pack_start(renderer, False)
        self.set_resizable(False)
        self.set_sizing(gtk.TREE_VIEW_COLUMN_AUTOSIZE)
        self.set_reorderable(False)
        self.add_attribute(renderer, 'stock-id', COLUMN_STATE)
        if not width == None:
            self.set_sizing(gtk.TREE_VIEW_COLUMN_FIXED)
            self.set_fixed_width(width)
        
