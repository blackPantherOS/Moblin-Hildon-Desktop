#!/usr/bin/python -ttu
# -*- mode:python; tab-width:4; indent-tabs-mode:nil;  -*-
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


import gc
import gtk
import Image
import os
import pygtk
import stat
import sys
import threading
import time

pygtk.require("2.0")

# Import our app's libraries
import constant
import media_service
import thumb_base
import thumbnail_view
import utils

OPCODE_NOP, OPCODE_GET_THUMB, OPCODE_SET_THUMB = range(3)
TYPE_PHOTO, TYPE_AUDIO, TYPE_VIDEO = range(1,4)

g_thumb_bkgd = Image.open(os.path.join(constant.MediaImagePath,
                                       'thumbnail_background.png'))

class GetFrame(threading.Thread):

    def __init__(self, thumbnail_dir, view):
        self.__base = thumbnail_dir
        self.__view = view
        self.__service = self.__view.app.service
        self.__operation = OPCODE_NOP
        self.__is_running = True
        self.__queue = list()
        self.__event = threading.Event()
        self.__lock = threading.Semaphore(value = 1)
        threading.Thread.__init__(self)

    def __consume(self):
        self.__lock.acquire()
        if self.__queue:
            path = self.__queue.pop()
        else:
            path = ''
        self.__lock.release()
        return path
    
    def produce(self, path):
        if os.path.exists(path):
            self.__lock.acquire()
            self.__queue.insert(0, path)    
            self.__lock.release()

    def wake_up(self):
        self.__operation = OPCODE_GET_THUMB
        self.__event.set()
        self.__event.clear()

    def stop(self):
        self.__event.set()
        self.__event.clear()
        self.__is_running = False

    def __kick_thread(self):
        thread = self.__view.thumbnail_creator
        if thread.get_dir() == self.__view.CurrentImgDir:
            thread.set_operation_type(OPCODE_GET_THUMB)

    def __get_thumbnail_from_service(self, uri):
        """Connect to service exposed by dbus to generate video thumbnails
        @uri, the video content needed to generate thumbnail
        @service, media service to call"""
        root, ext = os.path.splitext(uri)
        head, tail = os.path.split(uri)
        output_file = os.path.join(self.__base, tail)
        try:
            if self.__service.GetVideoFrame('file://' + uri, output_file):
                d = os.path.join(constant.THUMBNAIL_DIR, self.__service.name)
                return utils.create_thumbnail_image(thumbnail_dir = d,
                                             full_image_path = output_file,
                                             canvas = g_thumb_bkgd,
                                             target = uri)
        except Exception, e:
            print e
            # We seem to be choking on thumbnail request, so just return False
            # and let the placeholder icon be used in the iconview
            pass
        return False

    def __create_thumbnail(self, path):
        gc.collect()
        # Throttle down activity for creating thumbnails
        time.sleep(1)
        if self.__view.get_mode() == constant.MODE_VIDEO:
            return self.__get_thumbnail_from_service(path)
        else:
            return utils.create_thumbnail_image(thumbnail_dir = self.__base,
                                                full_image_path = path,
                                                canvas = g_thumb_bkgd)
        
    def run(self):
        while(self.__is_running):
            if self.__operation == OPCODE_NOP:       
                self.__event.wait()
            else:
                path = self.__consume()
                if path:
                    if self.__create_thumbnail(path):
                        self.__kick_thread()
                else:
                    self.__operation == OPCODE_NOP
                    self.__event.wait()

class ThumbnailCreator(threading.Thread, thumb_base.ThumbBase):
    def __init__(self, thumbnail_view):
        self.__counter_lock = threading.Semaphore(value = 1)
        self.__view = thumbnail_view
        self.__operation = OPCODE_NOP
        self.__is_running = True
        self.__current_directory = self.__view.CurrentImgDir
        self.__event = threading.Event()
        self.__history = list()
        cache = os.path.join(constant.MediaImagePath, 'mime-video.png')
        self.__video_pixbuf = gtk.gdk.pixbuf_new_from_file(cache)
        cache = os.path.join(constant.MediaImagePath, 'mime-photo.png')
        self.__photo_pixbuf = gtk.gdk.pixbuf_new_from_file(cache)
        self.__request_queue = []
        threading.Thread.__init__(self)
        thumb_base.ThumbBase.__init__(self)

    def __lock_acquire(self):
        self.__view.ListStorLock.acquire()

    def __lock_release(self):
        self.__view.ListStorLock.release() 
   
    def __has_entry(self, filename, mtime, exact = True):
        res = False
        self.__lock_acquire()
        for i in self.__view.model:
            if (i[thumbnail_view.MDL_FILENAME].replace('\n','') == filename
            and mtime == i[thumbnail_view.MDL_MTIME]):
                if i[thumbnail_view.MDL_HAVE_THUMB] or not exact:
                    res = True
                    break
        self.__lock_release()
        return res      

    def __update(self, filename, mtime, pb, thumbnail_cache, have_thumbnail=True):
        """This method is useful when you want to replace the video's thumbnail
        from a fake one to a true one"""
        self.__lock_acquire()
        path = 0 
        for i in self.__view.model:
            if i[thumbnail_view.MDL_FILENAME].replace('\n','') == filename:
                if self.__view.iconview.path_is_selected((path,)):
                    self.__view.update_last_selection(pb.copy())
                    self.__view.add_rectangle_for_pixbuf(pb)
                i[thumbnail_view.MDL_DISP_THUMB] = pb
                i[thumbnail_view.MDL_PATH] = thumbnail_cache
                i[thumbnail_view.MDL_HAVE_THUMB] = have_thumbnail
                i[thumbnail_view.MDL_MTIME] = mtime
                self.__lock_release()
                return True
            path = path + 1
        self.__lock_release()
        return False

    def __append(self, imgfile, pb, MediaType, mtime, filesize, thumbnail_cache, have_thumbnail=True):
        gtk.gdk.threads_enter()
        if filesize > sys.maxint:
            filesize = sys.maxint
        self.__view.append(filename = imgfile,
                           media_type = MediaType,
                           size = filesize,
                           pixbuf = pb,
                           thumbnail_cache = thumbnail_cache,
                           mtime = mtime,
                           have_thumbnail = have_thumbnail)
        gtk.gdk.flush()
        gtk.gdk.threads_leave()

    def __queue_thumbnail_request(self, imgdir, imgfile, MediaType, mtime, filesize):
        path = os.path.join(imgdir, imgfile)
        if not path in self.__history:
            self.__request_queue.append(path)

    def __load_place_holder(self, imgdir, imgfile, MediaType, mtime, filesize):
        if MediaType == TYPE_VIDEO:
            pb = self.__video_pixbuf.copy()
        else:
            pb = self.__photo_pixbuf.copy()
        k = "%s/%s/thumbnail_level" % (constant.GCONF_PATH,
                                       self.__view.get_mode())
        level = self.__view.app.client.get_float(k)
        if not level:
            level = 7.5
        width = int(level * 20)
        if self.__has_entry(imgfile, mtime, False):
            # A placeholder has already been appended to the model, so just
            # update the placeholder pixbuf
            pb = pb.scale_simple(width, width, gtk.gdk.INTERP_BILINEAR)
            self.__update(imgfile, mtime, pb, '', False)
        else:
            pb = pb.scale_simple(width, width, gtk.gdk.INTERP_BILINEAR)
            self.__append(imgfile,
                          pb,
                          MediaType,
                          mtime,
                          filesize,
                          '',
                          False)
        
    def __load_from_cache(self, imgdir, imgfile, MediaType, mtime, filesize):
        if self.__has_entry(imgfile, mtime):
            # No need to load anything from cache, it's already in the model
            return True
        if MediaType == TYPE_VIDEO:
            full_path = os.path.join(imgdir, imgfile)
            d = os.path.join(constant.THUMBNAIL_DIR,
                             self.__view.app.service.name)
            cache = utils.get_thumbnail_path(d, full_path)
            if not os.path.exists(cache):
                return False
        else:
            full_path = os.path.join(imgdir, imgfile)
            cache = utils.get_thumbnail_path(constant.THUMBNAIL_DIR,
                                             full_image_path = full_path)
            if not os.path.exists(cache):
                # For now, lets go ahead and just create the image thumbnails
                # immediately instead of delaying their creation till the
                # GetFrame thread.  Just return false to cause a placeholder
                # image to be loaded and thumbnail creation to be delayed
                result = utils.create_thumbnail_image(constant.THUMBNAIL_DIR,
                                             full_image_path = full_path,
                                             canvas = g_thumb_bkgd)
                if not result:
                    return False
        # We have a thumbnail image in the cache, but the model has not
        # been populated with an entry containing a pixbuf from that image
        try:
            pb = gtk.gdk.pixbuf_new_from_file(cache)
        except:
            return False
        k = "%s/%s/thumbnail_level" % (constant.GCONF_PATH,
                                       self.__view.get_mode())
        level = self.__view.app.client.get_float(k)
        width = int(level * 20)
        pb = pb.scale_simple(width, width, gtk.gdk.INTERP_BILINEAR)
        if not self.__update(imgfile, mtime, pb, cache, True):
            self.__append(imgfile,
                          pb,
                          MediaType,
                          mtime,
                          filesize,
                          cache,
                          True)
        return True
    
    def __count_media_files(self, CurrentDir):
        """Count the sum of the media content of CurrentDir
        @CurrentDir, the dir needs to counting"""
        self.__counter_lock.acquire()
        self.media_counter = [0, 0]
        self.__counter_lock.release()
        if os.path.exists(CurrentDir):
            for i in os.listdir(CurrentDir):
                MediaType = self.thumb_filter(CurrentDir,i)
                if MediaType == TYPE_PHOTO:
                    self.__counter_lock.acquire()
                    self.media_counter[0] += 1
                    self.__counter_lock.release()
                elif MediaType == TYPE_VIDEO:
                   self.__counter_lock.acquire()
                   self.media_counter[1] += 1 
                   self.__counter_lock.release()
                  
    def __get_media_sum(self):
        self.__counter_lock.acquire()
        if self.__view.media_mode == "photo":
            Sum = self.media_counter[0]
        else:
            Sum = self.media_counter[1]
        self.__counter_lock.release()        
        return Sum

    def __get_model_length(self):
        """Get the length of IconView's model"""
        self.__lock_acquire()
        length = len(self.__view.model)
        self.__lock_release()
        return length

    def __get_sorted_file_list(self):
        """In order to avoid the thumbnails jumping around when app ups,
        presorting the media contents were needed"""
        d = self.__view.CurrentImgDir
        list = os.listdir(d)
        if self.__view.SortType == constant.THUMB_SORT_FILENAME:
            # Sort by Name
            list.sort()
        if self.__view.SortType == 2:
            # Sort by Size
            list.sort(lambda a, b: int(os.stat(os.path.join(d,a))[stat.ST_SIZE] - os.stat(os.path.join(d,b))[stat.ST_SIZE]))    
        return list        

    def __set_operation(self, operation):
        self.__operation = operation
        
    def set_operation_type(self, operation):
        self.__event.set()
        self.__event.clear()
        self.__set_operation(operation)

    def get_dir(self):
        return self.__current_directory
    
    def set_dir(self, directory):
        self.__current_directory = directory
        self.__count_media_files(directory)
        self.__view.initialize_view()
        self.__lock_acquire()
        self.__view.model.clear()
        self.__lock_release()
        self.set_operation_type(OPCODE_GET_THUMB)

    def stop(self):
        self.__event.set()
        self.__event.clear()
        self.__is_running = False

    def run(self):
        while(self.__is_running):
            if self.__operation == OPCODE_NOP:
                if self.__request_queue:
                    for i in range(len(self.__request_queue)):
                        path = self.__request_queue.pop()
                        if not path in self.__history:
                            self.__history.append(path)
                            self.__view.GetFrame.produce(path)
                    self.__view.GetFrame.wake_up()
                self.__event.set()
                self.__event.clear()
                self.__set_operation(OPCODE_GET_THUMB)
                self.__event.wait()
            else :
                self.__set_operation(OPCODE_NOP)
                imgdir = self.__view.CurrentImgDir
                if not imgdir or not os.path.exists(imgdir):
                    continue
                ##############################################################
                # Iterate through a sorted list of media files and append
                # or update the thumbnail view as needed.
                #
                # NOTE: If we add thumbnails in a different order then
                #       the Gtk widget presents them, then it will appear
                #       to the user as if the images were jumping around.
                original_sort_type = self.__view.SortType
                for imgfile in self.__get_sorted_file_list() :
                    MediaType = self.thumb_filter(imgdir, imgfile)
                    if not MediaType:
                        continue

                    full_path = os.path.join(imgdir, imgfile)
                    mtime, filesize = self.get_file_detail_info(full_path)
                    if not self.__load_from_cache(imgdir,
                                                  imgfile,
                                                  MediaType,
                                                  mtime,
                                                  filesize):
                        self.__load_place_holder(imgdir,
                                                 imgfile,
                                                 MediaType,
                                                 mtime,
                                                 filesize)
                        self.__queue_thumbnail_request(imgdir,
                                                       imgfile,
                                                       MediaType,
                                                       mtime,
                                                       filesize)
                        
                    if original_sort_type != self.__view.SortType:
                        # The user selected a different sorting order
                        # in the middle of generating thumbnails, so
                        # re-enter this for-loop so that we start adding
                        # thumbnails in the same order that the Gtk widget
                        # renders them
                        self.__set_operation(OPCODE_GET_THUMB)
                        break
                
