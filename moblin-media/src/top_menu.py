#!/usr/bin/python -ttu
# -*- mode:python; tab-width:4; indent-tabs-mode:nil;  -*-
# vim: ai ts=4 sts=4 et sw=4
#
# top_menu.py: top menu
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
import gconf
import gtk
import gtk.gdk
import gtk.glade
import os
import sys

# Import our app's libraries
import constant
import thumb_base 
import hint_window
import utils
import settings_dialog

_ = gettext.lgettext

class TopMenu(thumb_base.ThumbBase):
    def __init__(self, app, window):
        thumb_base.ThumbBase.__init__(self)
        self.__client = gconf.client_get_default()
        self.app=app
        self.window = window
        self.properties_glade = None
        
    def create_menu(self, window):
        mode = self.app.cur_mode
        
        self.menu_file = gtk.Menu ()
        window.set_menu (self.menu_file)

        if (mode == constant.MODE_VIDEO):
            self.item_open_uri = gtk.MenuItem(_(constant.MENU_OPEN_LOCATION))
            self.item_open_uri.connect ('activate', self.on_open_uri )
            self.menu_file.append (self.item_open_uri)

            self.menu_file.append (gtk.SeparatorMenuItem ())

        if (mode == constant.MODE_VIDEO or mode == constant.MODE_PHOTO):

            self.item_edit = gtk.MenuItem(_(constant.MENU_EDIT))
            self.item_edit.set_submenu(self.create_edit_sub_menu())
            self.menu_file.append (self.item_edit)

            self.item_sort = gtk.MenuItem(_(constant.MENU_SORT))
            self.item_sort.set_submenu(self.create_sort_sub_menu())
            self.menu_file.append (self.item_sort)

            self.menu_file.append (gtk.SeparatorMenuItem ())

            self.item_settings = gtk.MenuItem(_(constant.MENU_SETTINGS))
            self.item_settings.connect('activate',self.__on_settings)
            self.menu_file.append (self.item_settings)
            
            self.menu_file.append(gtk.SeparatorMenuItem ())
            
        self.item_about = gtk.MenuItem(_(constant.MENU_ABOUT))
        self.item_about.connect ('activate', self.on_about)
        self.menu_file.append (self.item_about)

        self.item_quit = gtk.MenuItem(_(constant.MENU_QUIT))        
        self.item_quit.connect ('activate', self.on_quit)
        self.menu_file.append (self.item_quit)

        # request notification when menu is about to show (disable items for context)
        self.menu_file.connect('motion-notify-event',self.on_activate_topmenu)
        self.menu_file.show_all ()

    def create_edit_sub_menu(self):
        menu_edit = gtk.Menu()

        self.item_delete = gtk.MenuItem(_(constant.MENU_DELETE))
        self.item_delete.connect('activate',self.on_edit_delete)
        menu_edit.append(self.item_delete)

        self.item_properties = gtk.MenuItem(_(constant.MENU_PROPERTIES))
        self.item_properties.connect('activate',self.on_edit_properties)
        menu_edit.append(self.item_properties)

        if self.app.cur_mode == constant.MODE_PHOTO:
            self.item_rotate_right = gtk.MenuItem(_(constant.MENU_ROTATE_CLOCKWISE))
            self.item_rotate_right.connect('activate',self.on_edit_rotate_clockwise)
            menu_edit.append(self.item_rotate_right)
        
            self.item_rotate_left = gtk.MenuItem(_(constant.MENU_ROTATE_COUNTERCLOCKWISE))
            self.item_rotate_left.connect('activate',self.on_edit_rotate_counterclockwise)
            menu_edit.append(self.item_rotate_left)
        
        return menu_edit

    def create_sort_sub_menu(self):
        menu_sort = gtk.Menu()
        self.item_name = gtk.MenuItem(_(constant.MENU_NAME))
        self.item_name.connect('activate',self.on_sort_name)
        menu_sort.append(self.item_name)
        
        self.item_size = gtk.MenuItem(_(constant.MENU_SIZE))
        self.item_size.connect('activate',self.on_sort_size)
        menu_sort.append(self.item_size)        
        return menu_sort

    def update_menu(self, item_is_selected):
        mode = self.app.cur_mode
        if mode == constant.MODE_PHOTO or mode == constant.MODE_VIDEO:
            self.item_edit.set_sensitive(item_is_selected )

    def on_activate_topmenu(self,widget,event):
        if self.app.cur_mode == constant.MODE_AUDIO:
            return
        
        location = self.app.location
        ##menu special determined by the self.app.location   
        self.item_delete.set_sensitive(True)
        if location == constant.TNV:
            self.item_sort.set_sensitive(True) 
        elif location == constant.PBP:
            self.item_sort.set_sensitive(False)
        else:
            self.item_sort.set_sensitive(False)
            self.item_edit.set_sensitive(False)
            self.item_delete.set_sensitive(True)

    def on_open_uri(self,widget):
        self.url_xml=utils.get_widget_tree('dialogs.glade','OpenLocation')
        self.url_dialog = self.url_xml.get_widget("OpenLocation")
        self.url_dialog.set_modal(True)
        self.url_file = open(constant.MediaHistoryFile, 'a+')
        self.url_combo = self.url_xml.get_widget("houd_uri_combo")
        self.url_entry = self.url_xml.get_widget("houd_uri_entry")
        self.url_combo.set_popdown_strings(constant.MediaUrlList)
        if (gtk.RESPONSE_OK == self.url_dialog.run()):
            self.url_text = self.url_entry.get_text()
            if self.url_text in constant.MediaUrlList:
                constant.MediaUrlList.remove(self.url_text)
            else:
                self.url_file.write(self.url_text + '\n')
            constant.MediaUrlList.insert(0, self.url_text)
            self.url_dialog.destroy()
            self.app.open_uri(self.url_text)
        else:
            self.url_dialog.destroy()
        self.url_file.close()

    def on_edit_delete(self,widget):###'delete'
        thumbnail = self.app.get_current_thumb()
        if thumbnail == None:
            return
        if self.app.location == constant.PBP:
            ## go to implement the delete in photo playback mode
            self.app.view['photo'].photoplayback.delete()
        elif self.app.location == constant.TNV:
            thumbnail.delete(-1)
        else:
            pass

    def on_edit_properties(self,widget):###'Photo properties'
        filename =None
        size = None
        thumbnail = self.app.get_current_thumb()
        if thumbnail == None:
            return
        if not self.app.get_thumb_current_mode == "audio":
            filename,size = thumbnail.properties()
        if filename or size:
            properties_glade = utils.get_widget_tree('dialogs.glade','properties_photo')
            pro_dialog = properties_glade.get_widget('properties_photo')
            pro_dialog.set_title(_(constant.MSG_PROPERTIES_TITLE))
            pro_dialog.set_modal(True)

            name_entry = properties_glade.get_widget('name_edit')
            size_label = properties_glade.get_widget('size_label') 
            root_name,ext = os.path.splitext(filename.replace('\n',''))
            name_entry.set_text(filename.replace('\n',''))

            name_entry.select_region(0,len(root_name))

            size = self.calculate_size(size)
            size_label.set_text(size)

            while True:
                if gtk.RESPONSE_OK == pro_dialog.run():
                    ##FIXME : should go to the user confirm the change message?
                    index = None
                    new_name = name_entry.get_text()

                    if not new_name:
                        msg = _(constant.MSG_EMPTY_STRING_ERROR)
                        title = _(constant.MSG_ERROR_TITLE)
                        flags = gtk.DIALOG_MODAL | \
                                gtk.DIALOG_DESTROY_WITH_PARENT
                        dialog = gtk.MessageDialog(None,
                                                   flags,
                                                   gtk.MESSAGE_ERROR,
                                                   gtk.BUTTONS_CLOSE,
                                                   msg)
                        dialog.set_title(title)
                        dialog.run()
                        dialog.destroy()                        
                        continue

                    old_index = thumbnail.get_index_by_name(filename)
                    new_index = thumbnail.get_index_by_name(new_name)

                    if self.app.cur_mode == constant.MODE_VIDEO:
                        video_path = os.path.join(self.app.get_media_directory(),"video")

                    if not old_index == new_index:
                        if  new_index>=0:
                            title = _(constant.MSG_RENAME_ERROR_TITLE)
                            msg = _(constant.MSG_RENAME_QUESTION )% (new_name)
                            res = utils.confirm_dialog(title, msg)
                            if not res == gtk.RESPONSE_YES:
                                continue
                            else:
                                thumbnail.RemoveItem(new_index)
                    new_root,new_ext = os.path.splitext(new_name)
                    if not new_ext.lower() == ext.lower():
                        title = _(constant.MSG_EXTENSION_CHANGE_TITLE)
                        msg = _(constant.MSG_EXTENSION_CHANGE_QUESTION) % \
                              (ext, new_ext)
                        res = utils.confirm_dialog(title, msg)
                        if not res == gtk.RESPONSE_YES:
                            name_entry.set_text(new_root + ext)
                            continue
                    if not self.app.get_thumb_current_mode == "audio":
                            index = thumbnail.iconview.get_selected_items()[0][0]
                    thumbnail.set_selected_item_name(index,new_name)
                    cur_dir = thumbnail.get_current_dir()
                    os.chdir(cur_dir)
                    os.rename(filename.replace('\n',''),new_name)
                    os.chdir(constant.MediaAppPath)
                    pro_dialog.destroy()   
                    break
                else:           
                    pro_dialog.destroy()
                    break
            pro_dialog.destroy()   
        else:
            ##FIXME : To promote some message to tell the correct operation sequence
            pass

    def on_edit_rotate_clockwise(self,widget):
        thumbnail = self.app.get_current_thumb()
        if (thumbnail == None):
            return
        if self.app.location == constant.TNV:
            # If we are in the thumbnail view then we want to make sure that
            # our photo gets loaded so that the photo rotate will work
            thumbnail.load_picture()
        if self.app.location == constant.PBP or self.app.location == constant.TNV:
            # Rotate both the photo and the thumbnail
            self.app.view['photo'].photoplayback.rotate('R')
            thumbnail.rotate('R')

    def on_edit_rotate_counterclockwise(self,widget):
        thumbnail = self.app.get_current_thumb()
        if (thumbnail == None):
            return
        if self.app.location == constant.TNV:
            # If we are in the thumbnail view then we want to make sure that
            # our photo gets loaded so that the photo rotate will work
            thumbnail.load_picture()
        if self.app.location == constant.PBP or self.app.location == constant.TNV:
            # Rotate both the photo and the thumbnail
            self.app.view['photo'].photoplayback.rotate('r')
            thumbnail.rotate('r')

    def on_active_item_view(self,wiget,event):
        self.update_show_hide_labels_menu()

    def on_sort_name(self,widget):
        hint_window.HintWindow().show_hint(_(constant.MSG_PHOTO_SORT_BY_NAME))

        thumbnail = self.app.get_current_thumb()
        if (thumbnail == None):
            return
        thumbnail.set_sort_type(constant.THUMB_SORT_FILENAME)
        cur_mode = self.app.get_thumb_current_mode()
        mode = cur_mode + '-thumb'
        self.app.view['toolbar'].change_mode(mode,cur_mode)

    def on_sort_size(self,widget):
        hint_window.HintWindow().show_hint(_(constant.MSG_PHOTO_SORT_BY_SIZE))

        thumbnail = self.app.get_current_thumb()
        if (thumbnail == None):
            return
        thumbnail.set_sort_type(constant.THUMB_SORT_SIZE)


    ##for Mode menu handler
    def on_mode_normal(self,widget):
        pass

    def on_mode_reorder(self,widget):
        pass

    def on_mode_multiselect(self,widget):
        pass

    
    def __on_settings(self, widget):
        settings_dialog.SettingsDialog(self.app.get_thumb_current_mode())

    def on_about(self,widget):
        self.about_xml=utils.get_widget_tree('dialogs.glade','AboutDlg')
        about_dialog = self.about_xml.get_widget('AboutDlg')
        try:
            version_file = open(os.path.join(constant.MediaAppPath, "version"))
            version = version_file.readline()
            version_file.close()
            version = version.strip()
        except:
            version = None
        if version:
            print "Version: %s" % version
            about_dialog_version = self.about_xml.get_widget('about_version')
            about_dialog_version.set_text(version)

        # set the image into the dialog
        about_image = self.about_xml.get_widget('about_image')
        pb = gtk.gdk.pixbuf_new_from_file(os.path.join(constant.MediaSpecImagePath,
                                                       'about.png'))
        about_image.set_from_pixbuf(pb)
        about_dialog.set_modal(True)
        about_dialog.run();
        about_dialog.destroy()

    def on_quit(self,widget):
        if self.app:
            if not self.app.delete_event(self.app, None):
                self.app.destroy(self.app)
        
    def on_test_name_toggled(self,widget):
        pass


