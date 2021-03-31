#!/usr/bin/python -ttu
# -*- mode:python; tab-width:4; indent-tabs-mode:nil;  -*-
# vim: ai ts=4 sts=4 et sw=4
#
# thumbnail_view.py: Manage Thumbnail View
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


# This has to be before we import gtk
import pygtk
pygtk.require("2.0")

import gc
import gtk
import md5
import moko
import os
import pango
import threading
import time

# Import our app's libraries
import constant
import thumb_base
import thumbnail_creator
import utils

# Globals for Model indexing
MDL_FILENAME    = 0   # file_name
MDL_DISP_THUMB  = 1   # displayed thumbnail
MDL_TYPE        = 2   # MediaType(photo, audio, video)
MDL_MTIME       = 3   # mtime
MDL_SIZE        = 4   # size of photo
MDL_PATH        = 5   # thumbnail image path
MDL_HAVE_THUMB  = 6   # have thumbnail flag
MDL_TEXT        = 7   # display text
MDL_MTIME_NAME  = 8   # string(mtime+file_name)
MDL_UNUSED1     = 9   # unused
MDL_ANGLE       = 10  # rotate angle
MDL_MTIME_STR   = 11  # descriptive mtime


class ThumbnailFinger(gtk.IconView):
    """ThumbnailFinger inherit from gtk.IconView to show thumbnails"""
    def __init__(self, model = None):
        gtk.IconView.__init__(self, model)
        self.moko_widget = moko.FingerScroll()
        self.moko_widget.show()
        self.moko_widget.set_property('spring_color',0x000000)
        self.moko_widget.add(self)
     
    def get_moko(self):
        return self.moko_widget

class ThumbnailSupport(thumb_base.ThumbBase):
    def __init__(self, app, mode):
        self.__client = app.client
        self.app = app
        self.__mode = mode
        self.__last_selection = None
        self.media_mode = mode
        self.item_selected = -1
        self.SortType = constant.THUMB_SORT_FILENAME

        ######################################################################
        # If you edit this, make sure to edit the MDL_* variables near the top
        # of this file
        self.model = gtk.ListStore(str,            # 0 file_name 
                                   gtk.gdk.Pixbuf, # 1 displayed thumbnail 
                                   int,            # 2 MediaType(photo, audio, video) 
                                   int,            # 3 mtime 
                                   long,           # 4 size of photo 
                                   str,            # 5 thumbnail image path
                                   bool,           # 6 have thumbnail flag
                                   str,            # 7 display text 
                                   str,            # 8 string(mtime+file_name) 
                                   long,           # 9 unused 
                                   int,            # 10 rotate angle 
                                   str)            # 11 descriptive mtime 
        # If you edit this, make sure to edit the MDL_* variables near the top
        # of this file
        ######################################################################

        self.CurrentImgDir = None
        self.SelectedItems = list()
        self.ListStorLock = threading.Semaphore(value =1)
        self.counter = 0
        self.iconview = ThumbnailFinger()
        self.iconview.set_selection_mode(gtk.SELECTION_SINGLE)
        self.iconview.set_orientation(gtk.ORIENTATION_VERTICAL)
        self.iconview.set_reorderable(False)
        self.iconview.connect("button-release-event", self.button_release_event, None)
        self.iconview.connect("selection-changed", self._on_iconview_select_changed )
        self.iconview.set_item_width(-1)
        self.iconview.show()
        self.display_ext = self.__get_bool('show_ext_label')
        self.display_name = self.__get_bool('show_name_label')
        self.display_size = self.__get_bool('show_size_label')
        self.iconview.modify_base(gtk.STATE_NORMAL,constant.MediaColor['select_window_bg'])
        self.iconview.modify_text(gtk.STATE_NORMAL,constant.MediaColor['select_label_fg'])
        font= pango.FontDescription("sans bold 8")
        self.iconview.modify_font(font)
        self.makeCacheDirs()
        self.thumbnail_creator = thumbnail_creator.ThumbnailCreator(self)
        self.thumbnail_creator.setDaemon(True)
        self.thumbnail_creator.start()
        if self.__mode == constant.MODE_VIDEO:
            self.GetFrame = thumbnail_creator.GetFrame(self.thumbdirVideoTmp, self)
            self.GetFrame.setDaemon(True)
            if self.__get_bool('enable_video_thumbnails'):
                self.GetFrame.start()
                if self.app.service.name == "helix":
                    try:
                        # temporary hack to deal with helix service
                        # brain damage
                        self.app.service.GetVideoFrame('', '')
                    except media_service.MediaServiceError:
                        # FIXME: The media service was restarted due to
                        #        a critical error.  Need a better recovery
                        pass
        else:
            self.GetFrame = thumbnail_creator.GetFrame(self.thumbdirSizeLargest, self)
            self.GetFrame.setDaemon(True)
            self.GetFrame.start()
        self.app.register_config_callback(self.__mode, self.__config_handler)
        thumb_base.ThumbBase.__init__(self)
        
    def __config_handler(self, key, value):
        if key in ['show_ext_label', 'show_name_label',  'show_size_label']:
            if key == 'show_ext_label':
                self.display_ext = value.get_bool()
            elif key == 'show_name_label':
                self.display_name = value.get_bool()
            elif key == 'show_size_label':
                self.display_size = value.get_bool()
            if self.display_name or self.display_size:
                self.iconview.set_text_column(MDL_TEXT)
            #FIXME: Attempting to turn off iconview text column (by setting -1)
            #       seems like the right thing to do when we see that the
            #       label string is now empty, but doing this after the
            #       iconview is populated with entries containing non-empty
            #       labels, and with the text column set to that label, will
            #       result in python segfaulting.
            #
            #       Instead of following this block with an:
            #           'else: self.iconview.set_text_column(-1)'
            #       ...I am leaving the text column set for an empty column
            self.__update_thumbnail_labels()
        elif key == 'enable_video_thumbnails':
            if value.get_bool():
                self.GetFrame.start()
            else:
                self.GetFrame.stop()

    def __construct_label(self, file_name, raw_size):
        # We allow x possible forms of a thumbnail label:
        #    * NAME
        #    * NAME \n(SIZE)
        #    * NAME.EXT <-- which is just the filename
        #    * NAME.EXT \n(SIZE)
        #    * (SIZE)
        size = self.calculate_size(raw_size)
        file_name = file_name.replace('\n','')
        name, ext = os.path.splitext(file_name)      
        label = '' 
        if self.display_name:
            label = name
        if self.display_ext:
            label = file_name
        if self.display_size:
            if label:
                label = "%s\n(%s)" % (label, size)
            else:
                label = "(%s)" % (size)
        return label
    
    def __update_thumbnail_labels(self):
        for i in self.model:
            i[MDL_TEXT] = self.__construct_label(i[MDL_FILENAME], i[MDL_SIZE])

    def append(self, filename, media_type, size, pixbuf, thumbnail_cache, mtime, have_thumbnail):
        label = self.__construct_label(filename, size)
        mtime_long = time.strftime("%Y-%m-%d %H:%M", time.gmtime(mtime))
        self.model.append([filename,
                           pixbuf,
                           media_type,
                           mtime,
                           size,
                           thumbnail_cache,
                           have_thumbnail,
                           label,
                           size,
                           size,
                           0,
                           mtime_long])
    
    def __get_bool(self, key):
        return self.__client.get_bool("%s/%s/%s" %
                                      (constant.GCONF_PATH, self.__mode, key))

    def __get_float(self, key):
        return self.__client.get_float("%s/%s/%s" %
                                       (constant.GCONF_PATH, self.__mode, key))

    def __set_float(self, key, value):
        key = "%s/%s/%s" % (constant.GCONF_PATH, self.__mode, key)
        self.__client.set_float(key, value)
    
    def get_scroll_widget(self):
        return self.iconview.get_moko()

    def set_cur_dir(self, filename):
        """Set currend dir from abs path of the filename
        @filename, the dir or a file with abs path"""        
        if os.path.isdir(filename):
            self.CurrentImgDir = filename
            return
        head, tail = os.path.split(filename)
        self.CurrentImgDir = head
    
    def set_selected_item_name(self, i, file_name):
        """Set selected item name of 'i' to name
        @i, the index of selected item
        @file_name, new name you want to change to"""
        size = self.model[i][MDL_SIZE]
        self.model[i][MDL_TEXT] = self.__construct_label(file_name, size)
        self.model[i][MDL_FILENAME] = file_name
  
    def get_current_model(self):
        """Return gtk.ListStore(model) attached to Iconview"""
        return self.model 
        
    def get_selected_items(self):
        return self.CurrentImgDir, self.SelectedItems
    
    def get_iconview(self):       
        return self.iconview

    def get_mode(self):
        return self.__mode
    
    def rotate(self, clock):
        """Rotate selected thumbnail by 90 degrees
        @clock, if clock = 'r', counter-clockwise,
                if clock = 'R', clockwise."""
        i = self.iconview.get_selected_items()[0][0]
        if clock == 'R':
            self.model[i][MDL_ANGLE] += 90
            self.model[i][MDL_DISP_THUMB] = self.model[i][MDL_DISP_THUMB].rotate_simple(gtk.gdk.PIXBUF_ROTATE_CLOCKWISE)
            if self.__last_selection:
                pb = self.__last_selection[MDL_DISP_THUMB]
                self.__last_selection[MDL_DISP_THUMB] = pb.rotate_simple(gtk.gdk.PIXBUF_ROTATE_CLOCKWISE)
        else:
            self.model[i][MDL_ANGLE] += 270            
            self.model[i][MDL_DISP_THUMB]=self.model[i][MDL_DISP_THUMB].rotate_simple(gtk.gdk.PIXBUF_ROTATE_COUNTERCLOCKWISE)
            if self.__last_selection:
                pb = self.__last_selection[MDL_DISP_THUMB]
                self.__last_selection[MDL_DISP_THUMB] = pb.rotate_simple(gtk.gdk.PIXBUF_ROTATE_COUNTERCLOCKWISE)
        self.model[i][MDL_ANGLE] %= 360
        self.add_rectangle_for_pixbuf(self.model[i][MDL_DISP_THUMB])
        
    def button_release_event(self,widget,event, data):
        item = widget.get_selected_items()
        if item:
            self.app.TopMenu.update_menu(True)
            if self.app.get_thumb_current_mode()=='photo':
                self.app.view['toolbar'].set_photo_btn_sensitive(True)
            elif self.app.get_thumb_current_mode()=='video':
                self.app.view['toolbar'].set_video_btn_sensitive(True)
            if item[0][0] == self.item_selected:
                self.view_in_big_size(self.CurrentImgDir,widget.get_selected_items()[0][0])
                self.item_selected = -1
            else:
                self.item_selected = item[0][0]
        else:
            self.app.TopMenu.update_menu(False)
            self.item_selected = -1
            if self.media_mode=='photo':
                self.app.view['toolbar'].set_photo_btn_sensitive(False)
            elif self.media_mode=='video':
                self.app.view['toolbar'].set_video_btn_sensitive(False)
            self.app.update_current_index(-1)

        num = len (self.SelectedItems)-1
        while num >= 0:
            self.SelectedItems.remove(self.SelectedItems[num])
            num -= 1
        SelItemsPath = widget.get_selected_items()
        if SelItemsPath:
            for i in SelItemsPath:
                self.SelectedItems.append(self.path_to_file_name(i)) 
        if self.SelectedItems:
            self.SelectedItems.sort()
        
    def add_frame2selected_items(self):
        """Add border to selected thumbnail"""
        sel_items = self.iconview.get_selected_items()
        if sel_items:
           for items in sel_items:
               self.add_rectangle_for_pixbuf(self.model[items[0]][MDL_DISP_THUMB])

    def update_last_selection(self, pb):
        if self.__last_selection:
            self.__last_selection[MDL_DISP_THUMB] = pb

    # the select-changed event of iconview
    def _on_iconview_select_changed(self, iconview):
        """Process the selection changed event of IconView"""
        file_list = []
        dir = self.CurrentImgDir
        if self.__last_selection:
            # restore the original pixbuf of the previously selected item
            self.__last_selection[0][MDL_DISP_THUMB] = self.__last_selection[MDL_DISP_THUMB]
        sel_items = self.iconview.get_selected_items()
        if sel_items:
            item = self.model[sel_items[0][0]]
            pb = self.model[sel_items[0][0]][MDL_DISP_THUMB]
            self.__last_selection = [item, pb.copy()]
            self.add_rectangle_for_pixbuf(pb)
            filename = item[MDL_FILENAME].replace('\n','')
            self.app.view[self.media_mode].update_index(sel_items[0][0])
            file_list.append(os.path.join(dir, filename))
        else:
            self.__last_selection = None
        # update plugin args
        if self.app.view['toolbar'].plugins.get_plugin_count():
            buttons = self.app.view['toolbar'].get_plugin_buttons()
            str_mode = self.app.get_thumb_current_mode() + '-thumb'
            for bn in buttons[str_mode]:
                bn.update_button_status(file_list)
                
    def get_index_by_name(self,name):
        """Return an index of the item's name
        @name, the item name along with its extension displayed as thumbnail's name"""
        index = -1
        for i in range(len(self.model)):
            if self.model[i][MDL_FILENAME] == name:
                index = i
                break
        return index

    def update_index_by_sort(self):
        sel_items = self.iconview.get_selected_items()
        if sel_items:
            self.app.view[self.media_mode].update_index(sel_items[0][0])
            self.update_view_by_index(sel_items[0][0])

    def update_view_by_index(self, index):
        self.iconview.unselect_all()
        if index == -1:
            return
        self.iconview.scroll_to_path((index,),True,True,True)
        self.iconview.select_path((index,))
        self._on_iconview_select_changed(self.iconview)

    def key_press_event(self,widget,event):
        keyval = gtk.gdk.keyval_name(event.keyval)
        if (keyval > 'a'and keyval<'z') or (keyval >'A' and keyval <'Z') or (keyval >'1',keyval<'9'):   
            self.key_to_scroll(keyval,self.iconview)
        
    def set_thumbnail_size_level(self, size, whole=False):
        """This is the interface to change the thumbnail size, largest level of
        thumbnail is 10, and smallest is 3 @size, new level you want to resize
        to"""
        gc.collect()
        original_level = self.__get_float('thumbnail_level')
        self.__set_float('thumbnail_level', size)
        if abs(original_level - size) < 0.5:
            # Reduce our workload by filtering out really small request
            # (usually triggered by jittery touchscreen hardware)
            return
        if size > original_level:
            rebuild_thumbnails = True
        else:
            rebuild_thumbnails = False
        width = int(size * 20)
        self.ListStorLock.acquire()
        for entry in self.model:
            # Regardless of if we are scaling the size of the pixbuf up
            # or down, we just do a simple scale operation on the existing
            # pixbuf. This causes the image to become fuzzy when we increase
            # the size.
            #
            # If we were to just rebuild the thumbnail from the thumbnail
            # image on disk, then resizing would take too long (on the order
            # of twice the time to just scale the existing pixbuf.)  Because
            # of this, we mark the thumbnail as needing further processing,
            # and then kick the thumbnail creation thread which will read
            # the image off disk to adjust the pixbuf in the background.
            # The user will initially see the thumbnail become fuzzy (when
            # up scaling), and then notice the image clear up.
            entry[MDL_DISP_THUMB] = entry[MDL_DISP_THUMB].scale_simple(width,
                                             width,
                                             gtk.gdk.INTERP_BILINEAR)
            if self.__last_selection:
                pb = self.__last_selection[MDL_DISP_THUMB]
                self.__last_selection[MDL_DISP_THUMB] = pb.scale_simple(width,
                                                           width,
                                                           gtk.gdk.INTERP_BILINEAR)
            if rebuild_thumbnails:
                # This tells the thumbnail creation thread that this entry
                # is just a placeholder, and needs a new thumbnail built
                entry[MDL_HAVE_THUMB] = False
        self.iconview.set_item_width(width)
        for items in self.iconview.get_selected_items():
            self.add_rectangle_for_pixbuf(self.model[items[0]][MDL_DISP_THUMB])
        self.ListStorLock.release()
        if rebuild_thumbnails:
            # Kick the thumbnail creation thread
            opcode = thumbnail_creator.OPCODE_GET_THUMB
            self.thumbnail_creator.set_operation_type(opcode)
        gtk.gdk.flush()

    def key_to_scroll(self,keyval,widget):
        """use key to scroll to the selected item ,more friendly than scroll
        use cursor"""
        if keyval.isupper:
            corkey = keyval.lower()
        else:
            corkey = keyval.upper()
        for i in range(len(self.model)):    
            if keyval == self.model[i][MDL_FILENAME][0]or corkey == self.model[i][MDL_FILENAME][0]:
                widget.scroll_to_path((i,),True,True,True)
                widget.select_path((i,))
                break 

    def get_current_dir(self):
        return self.CurrentImgDir
     
    def get_model(self):
        return self.model
                  
    def initialize_view(self):
        self.iconview.set_model(self.model)
        if self.display_name or self.display_size:
            self.iconview.set_text_column(MDL_TEXT)
        else:
            self.iconview.set_text_column(-1)
        self.iconview.set_pixbuf_column(1)    
        
    def sync_every_time(self,MobileHomeDir):
        for thumbfile in os.listdir(MobileHomeDir):
            path = thumbfile[32:].replace('\n','/')
            if not os.path.exists(path):
                os.remove(os.path.join(MobileHomeDir,thumbfile))
                continue
            mtime,filesize = self.get_file_detail_info(path)
            md5sum = md5.new(path+str(mtime)+str(filesize)).hexdigest()
            if not md5sum == thumbfile[:32]:
                os.remove(os.path.join(MobileHomeDir,thumbfile))
                          
    def makeCacheDirs(self, thumbdir = constant.MediaThumbnailsDir):
        """Make the cache dirs if they don't exist yet to store generated
        thumbnails"""
        self.thumbdir = os.path.abspath(os.path.expanduser(thumbdir))
        if not os.path.exists(self.thumbdir):
            os.makedirs(self.thumbdir)
        self.thumbdirSizeLargest = os.path.join(self.thumbdir,'largest')
        self.thumbdirSizeNormal = os.path.join(self.thumbdir,'normal')
        self.thumbdirVideoTmp =  os.path.join(self.thumbdir,'video_tmp')
        cache_dir = os.path.join(self.thumbdirSizeLargest,
                                 self.app.service.name)
        sucDir = os.path.join(cache_dir,'suc')
        failDir = os.path.join(cache_dir,'fail')
        if not os.path.exists(cache_dir):
            os.makedirs(cache_dir)
        if not os.path.exists(sucDir):
            os.mkdir(sucDir)
        if not os.path.exists(failDir):
            os.mkdir(failDir)
        if not os.path.exists(self.thumbdirSizeLargest):
            os.mkdir(self.thumbdirSizeLargest)
        if not os.path.exists(self.thumbdirSizeNormal):
            os.mkdir(self.thumbdirSizeNormal)
        if not os.path.exists(self.thumbdirVideoTmp):
            os.mkdir(self.thumbdirVideoTmp)

    def make_all_image_thumbnail(self, imgdir, IsHidden=False, thumbsize=(200,200),subdir='.mobilethumbnail'):
        """This is the interface to make thumbnails for a directory.
        @imgdir, the directory to be make thumbnails for
        @IsHidden, default not deal with hidden file and directory
        @thumbsize, defalut largest thumbnail size, advise not to set if you familiar with its influence
        @subdir, location of the cached thumbnails"""
        if self.CurrentImgDir == imgdir and len(self.model):
            return
        self.CurrentImgDir = imgdir
        if not os.path.exists(imgdir):
            return
        self.makeCacheDirs()
        self.set_sort_type(self.SortType)
        self.thumbnail_creator.set_dir(self.CurrentImgDir)

    def refresh(self):
        self.thumbnail_creator.set_dir(self.CurrentImgDir)

    def continue_browse_dir(self,CurrentImgDir,SelectedDirIcon):
        self.make_all_image_thumbnail(os.path.join(CurrentImgDir,SelectedDirIcon))
        
    def back_browse_dir(self):
        if len(self.CurrentImgDir) == 1 and self.CurrentImgDir == '/':
            CurDir = '/'
        elif len(self.CurrentImgDir) > 1 :
            if self.CurrentImgDir[-1] == '/':
                self.CurrentImgDir.replace(self.CurrentImgDir[-1],'')
            head,tailr = os.path.split(self.CurrentImgDir)
            if not head: #if at '/' exist a .png,then root should be '/'
                head = '/'
            CurDir = head
        else:   
            CurDir = '/'
        self.make_all_image_thumbnail(CurDir)

    def delete(self,index = -1):
        """
        Delete item (photo or video)
        @index, the index of the item to be deleted
        """
        if index < 0:
            index = self.iconview.get_selected_items()[0][0]
        filename = self.model[index][MDL_FILENAME].replace('\n','')
        p = "file://%s" % (os.path.join(self.app.get_media_directory(),
                                        'video',
                                        filename))
        if self.app.cur_mode == 'video' and self.app.service.GetUri() == p:
            self.app.service.Stop()
            self.app.view['toolbar'].hide_unused()
        src = os.path.join(self.get_current_dir(), filename)
        if self.trash(src):
            self.model.remove(self.model.get_iter(index))
            # select next one (if there is)
            count = self.get_num_items()
            if count == 0:
                return
            if index == count:  #if deleted last item in list
                index = index - 1
            self.iconview.select_path(index)
            self._on_iconview_select_changed(self.iconview)
            self.iconview.grab_focus()  #takes away gray selection

    def get_num_items(self):
        return self.model.iter_n_children(None)
        
    def properties(self):
        """view thumbnail's properties"""
        if self.iconview.get_selected_items():
            filename = self.model[self.iconview.get_selected_items()[0][0]][MDL_FILENAME]
            size = self.model[self.iconview.get_selected_items()[0][0]][MDL_SIZE]
            return filename,size   
        return 0,0

    def view_in_big_size(self,CurrentImgDir,i):
        """view_in_big_size means view small thumbnail in big size(for photo), for some 
        other media content, means play the media file
        @CurrentImgDir, current dir of which content you want to view in big size
        @i, the index of thumbnail to be play"""
        self.set_cur_dir(os.path.join(CurrentImgDir,self.model[i][MDL_FILENAME]))
        if self.model[i][MDL_TYPE] == 0:
            self.back_browse_dir()
        elif self.model[i][MDL_TYPE] == 4:
            self.continue_browse_dir(CurrentImgDir,self.model[i][MDL_FILENAME])
        elif self.model[i][MDL_TYPE] == thumbnail_creator.TYPE_AUDIO:
            pass
        elif self.model[i][MDL_TYPE] == thumbnail_creator.TYPE_VIDEO:
            # appears to be video
            self.app.view['toolbar'].open_media_file('file://'+os.path.join(CurrentImgDir,self.model[i][MDL_FILENAME]))
            self.thumb_fs_mode = self.app.get_fs_mode()
            self.app.set_fs_mode(True)
        elif self.model[i][MDL_TYPE] == 5:
            utils.error_msg(_(constant.MSG_PLAY_ERROR), False)
        elif self.model[i][MDL_TYPE] == thumbnail_creator.TYPE_PHOTO:
            # Appears to be photo
            filename = self.model[i][MDL_FILENAME]
            root,ext = os.path.splitext(filename)
            # Attempt to use Helix to display animated gifs if we hae Helix
            # running
            if ext.lower() == '.gif':
                if self.app.service.name == 'helix':
                    self.app.view['toolbar'].open_media_file('file://'+os.path.join(CurrentImgDir,filename))   
                    return
            self.app.location = constant.PBP
            self.app.view['photo'].go_photo_playback_page()
            self.app.view['photo'].set_cur_picture(os.path.join(CurrentImgDir,self.model[i][MDL_FILENAME]))
        #self.app.view['toolbar'].set_prev_next_state([True,True])

    def load_picture(self):
        """Helper function that will cause the currently selected picture in
        thumbnail view to be loaded up by the photo playback code."""
        i = self.iconview.get_selected_items()[0][0]
        # Appears to be photo
        filename = self.model[i][MDL_FILENAME]
        root,ext = os.path.splitext(filename)
        self.app.view['photo'].set_cur_picture(os.path.join(self.CurrentImgDir,self.model[i][MDL_FILENAME]))

    def get_fs_mode (self):
        return self.app.get_fs_mode()

    def get_sort_type(self):
        """Return current sort type"""
        return self.SortType

    def set_sort_type(self, sort_type = constant.THUMB_SORT_FILENAME):
        self.ListStorLock.acquire()
        self.SortType = sort_type
        if self.SortType == constant.THUMB_SORT_FILENAME:
            # Sort by filename
            self.model.set_sort_column_id(MDL_FILENAME, gtk.SORT_ASCENDING)
        elif self.SortType == constant.THUMB_SORT_DATE:
            # Sort by Date
            self.model.set_sort_column_id(MDL_MTIME_NAME, gtk.SORT_ASCENDING)
        elif self.SortType == constant.THUMB_SORT_SIZE:
            # sort by size
            self.model.set_sort_column_id(MDL_SIZE, gtk.SORT_ASCENDING)
        self.ListStorLock.release()

    def set_item_selected(self, item):
        self.item_selected = item

    def get_item_selected(self):
        return self.item_selected
        
    def path_to_file_name(self,path):
        return self.model[path[0]][MDL_FILENAME].replace('\n','')

    def __del__(self):
        self.sync_every_time(os.path.join(constant.MediaThumbnailsDir, 'normal'))
