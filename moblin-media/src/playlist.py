#!/usr/bin/python -ttu
# -*- mode:python; tab-width:4; indent-tabs-mode:nil;  -*-
# vim: ai ts=4 sts=4 et sw=4
#
# playlist.py: Media Playlist
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
import os
import random
import re
import urllib
from ConfigParser import SafeConfigParser
from paste.util import classinit
_ = gettext.lgettext

import constant

class PlayListError(Exception):
    def __init__(self, message):
        self.message = message

    def __str__(self):
        return self.message
        
class PLSParser(object):
    def __init__(self, parent):
        self.__parent = parent
        if self.__parent.ext != 'pls':
            raise PlayListError('Invalid filetype!')
        if self.__parent.proto == 'file':
            self.__fp = open(self.__parent.full_path, 'r')
        else:
            self.__fp = urllib.urlopen(self.__parent.uri)
        self.__parser = SafeConfigParser()

    def parse_file(self):
        try:
            self.__parser.readfp(self.__fp)
            p = re.compile("file[1-9]*")
            for option in self.__parser.options('playlist'):
                if p.match(option):
                    self.__parent.append({'File': self.__parser.get('playlist',
                                                                    option)})
        except:
            raise PlayListError('Invalid playlist!')
                
    def flush(self):
        # TODO: Implement me
        return
    
    def rename(self, name):
        new_full_path = os.path.join(self.__parent.directory, name) + '.pls'
        if os.path.isfile(new_full_path):
            raise PlayListError('Playlist name collision')
        os.rename(self.__parent.full_path, new_full_path)

class M3UParser(object):
    def __init__(self, parent):
        self._parent = parent
        if self._parent.ext != 'm3u':
            raise PlayListError('Invalid filetype!')

    def _read(self, descriptor):
        for l in descriptor.readlines():
            line = l.strip()
            if line:
                self._parent.append({'File': l.strip()})

    def _parse_local_file(self):
        # If the playlist file does not exists then create a new empty file
        if not os.path.isfile(self._parent.full_path):
            open(self._parent.full_path, 'w').close()
        else:
            f = open(self._parent.full_path, 'r')
            self._read(f)
            f.close()

    def _parse_network_file(self):
        try:
            f = urllib.urlopen(self._parent.uri)
            self._read(f)
        except IOError:
            raise PlayListError('Timeout attempting to load remote playlist')
        
    def parse_file(self):
        if self._parent.proto == 'file':
            self._parse_local_file()
        else:
            self._parse_network_file()
        
    def flush(self):
        f = open(self._parent.full_path, 'w')
        for i in self._parent:
            f.write("%s\n" % (i['File']))
        f.close()

    def rename(self, name):
        new_full_path = os.path.join(self._parent.directory, name) + '.m3u'
        if os.path.isfile(new_full_path):
            raise PlayListError('Playlist name collision')
        os.rename(self._parent.full_path, new_full_path)
    
class DirectoryParser(object):
    def __init__(self, parent):
        self._parent = parent
        if self._parent.proto != 'file':
            raise PlayListError("Unsupported protocol!")
        if not os.path.isdir(self._parent.full_path):
            raise PlayListError('Directory does not exists: %s' % (self._parent.full_path))

    def _is_audio_file(self, filename):
        name, tmp = os.path.splitext(filename)
        ext = tmp.replace('.', '')
        return ext in constant.MediaType['audio']
        
    def parse_file(self):
        for name in os.listdir(self._parent.full_path):
            if self._is_audio_file(name):
                path = os.path.join(self._parent.full_path, name)
                if not os.path.isdir(path):
                    self._parent.append({'File': path})

    def flush(self):
        return

    def rename(self, name):
        return

class BaseList(object):

    __metaclass__ = classinit.ClassInitMeta
    __classinit__ = classinit.build_properties

    def __init__(self):
        self._selected = -1
        self._list = []

    def __delitem__(self, index):
        del self._list[index]
        
    def __getitem__(self, index):
        return self._list[index]

    def __len__(self):
        return len(self._list)

    def __iter__(self):
        return self

    def __str__(self):
        return self._list.__str__()

    def selected_index__get(self):
        return self._selected

    def selected_index__set(self, index):
        if index > (len(self._list) - 1):
            raise IndexError('index out of range')
        self._selected = index

    def next(self):
        try:
            item = self._list[self._index]
            self._index = self._index + 1
            return item
        except:
            self._index = 0
            raise StopIteration()

class PlayList(BaseList):

    __metaclass__ = classinit.ClassInitMeta
    __classinit__ = classinit.build_properties

    def __init__(self, uri):
        BaseList.__init__(self)
        self._index = 0
        self._uri = uri
        try:
            self._proto, self._full_path = self._uri.split('://')
        except ValueError:
            raise PlayListError("Invalid PlayList URI: %s!" % (self._uri))
        self._directory, self._filename = os.path.split(self._full_path)
        self._name, tmp = os.path.splitext(self._filename)
        self._ext = tmp.replace('.', '')
        if self._ext == 'm3u':
            self._parser = M3UParser(self)
        elif self._ext == 'pls':
            self._parser = PLSParser(self)
        elif self._ext == '':
            self._parser = DirectoryParser(self)
        else:
            raise PlayListError("Unsupported File Format: %s!" % (self._ext))
        self._parser.parse_file()

    def __delitem__(self, index):
        BaseList.__delitem__(self, index)
        self.flush()
        
    def selected__get(self):
        if self._selected == -1:
            return None
        else:
            return self._list[self._selected]

    def selected__set(self, entry):
        i = 0
        for e in self._list:
            if e['File'] == entry['File']:
                self._selected = i
                return
            i = i + 1
        self._selected = -1
        raise PlayListError("%s not found in PlayList!" % (entry['File']))

    def directory__get(self):
        return self._directory
    
    def ext__get(self):
        return self._ext

    def full_path__get(self):
        return self._full_path

    def name__get(self):
        return self._name

    def name__set(self, name):
        self._parser.rename(name)
        self._name = name

    def proto__get(self):
        return self._proto

    def uri__get(self):
        return self._uri

    def append(self, entry):
        if type(entry) != type({}) or not 'File' in entry:
            raise PlayListError('Invalid PlayList Entry!')
        for e in self._list:
            if e['File'] == entry['File']:
                return
        self._list.append(entry)

    def context_info(self, name):
        i = 0
        for entry in self._list:
            if entry['File'] == name:
                return [i > 0, i < len(self._list) - 1] 
            i = i + 1
            
    def flush(self):
        self._parser.flush()

    def shuffle(self):
        """Shuffle the playlist"""
        random.shuffle(self._list)

    def clear(self):
        self._list = []

    def refresh(self):
        self.clear()
        self._parser.parse_file()

class PlayListCollection(BaseList):

    __metaclass__ = classinit.ClassInitMeta
    __classinit__ = classinit.build_properties

    def __init__(self, playlist_dir, media_dir):
        BaseList.__init__(self)
        self._playlist_dir = playlist_dir
        if not os.path.isdir(self._playlist_dir):
            os.makedirs(self._playlist_dir)
        self._media_dir = media_dir
        self._index = 0
        self._list = []
        all_songs = PlayList("file://" + self._media_dir)
        all_songs.name = _(constant.ALL_SONG_PLAYLIST_NAME)
        self._list.append(all_songs)
        for fname in os.listdir(self._playlist_dir):
            try:
                l = PlayList("file://%s" % (os.path.join(self._playlist_dir, fname)))
                if l.name != all_songs.name:
                    self._list.append(l)
            except:
                pass
        # If no ONLINE playlist has been found, then create an empty one
        try:
            self.create_playlist(_(constant.ONLINE_MUSIC_PLAYLIST_NAME), 'm3u')
        except PlayListError:
            # An existing ONLINE playlist was already in the list, so
            # not a problem
            pass
    def __delitem__(self, index):
        os.remove(self._list[index].full_path)
        BaseList.__delitem__(self, index)

    def append(self, playlist):
        self._list.append(playlist)

    def current__get(self):
        return self._list[self._index]
            
    def create_playlist(self, name, playlist_type):
        for l in self._list:
            if l.name == name:
                raise PlayListError('Duplicate PlayList')
        uri = "file://%s.%s" % (os.path.join(self._playlist_dir, name),
                                playlist_type)
        new_list = PlayList(uri)
        self._list.append(new_list)
        return new_list

    def delete(self, name):
        i = 0
        for l in self._list:
            if l.name == name:
                return self.__delitem__(i)
            i = i + 1
        raise PlayListError('Unknown PlayList')
    
    def get_list(self, name):
        for l in self._list:
            if l.name == name:
                return l
        raise PlayListError('Unknown List: %s' % (name))

if __name__ == '__main__':
    import sys
    try:
        # The following code will print a summary of a playlist
        # collection created from a given playlist and music file
        # directory
        collection = PlayListCollection(sys.argv[1], sys.argv[2])
        for playlist in collection:
            print "playlist[%s]: %s" % (playlist.name, playlist)
    except IndexError:
        print "USAGE: %s PLAYLIST_DIR MUSIC_LIBRARY_DIR\n"
