#!/usr/bin/python -ttu
# vim: ai ts=4 sts=4 et sw=4
#
# media_info.py: Keeps track of file meta data, will store it in a SQLite
#                database, so that it won't need to reread the files. 
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


import mutagen
import os
import sqlite3

import utils
import constant

class MediaInfoError(Exception):
    def __init__(self, message):
        self.message = message

    def __str__(self):
        return self.message

class MediaDatabaseError(Exception):
    def __init__(self, message):
        self.message = message

    def __str__(self):
        return self.message

class MediaDatabase(object):
    """sqlite database for media metadata"""
    def __init__(self, path):
        self.__connect = sqlite3.connect(path)
        self.__cursor = self.__connect.cursor()

        # Attempt to create a new table, and if that succeeds then
        self.__cursor.execute('''CREATE TABLE IF NOT EXISTS MediaInfo
                                 (uri TEXT primary key,
                                 title TEXT,
                                 artist TEXT,
                                 album TEXT,
                                 tracknum TEXT,
                                 date TEXT,
                                 length TEXT,
                                 rawlen INTEGER)''')

    def get(self, uri):
        self.__cursor.execute("SELECT * FROM MediaInfo WHERE uri=?", (uri,))
        try:
            return self.__cursor.fetchall()[0]
        except IndexError:
            raise MediaDatabaseError("record for %s does not exists" % (uri))

    def insert(self, uri, title, artist, album, tracknum, date, length, rawlen):
        self.__cursor.execute("insert into MediaInfo " \
                              "(uri, title, artist, album, tracknum, date, length, rawlen)" \
                              "values (?, ?, ?, ?, ?, ?, ?, ?)",
                              (uri,
                               title,
                               artist,
                               album,
                               tracknum,
                               date,
                               length,
                               rawlen))
        self.__connect.commit()

    def update(self, uri, key, value):
        self.__cursor.execute("update MediaInfo set %s='%s' where uri='%s'" %
                              (key, value, uri))
        self.__connect.commit()

    def close(self):
        self.__connect.close()


class MediaInfo(object):
    """Single media object metadata. Metadata stored in record in sqlite db"""
    def __init__(self, uri, db):
        self.__db = db
        if uri[0] == '/':
            self.__uri = 'file://' + uri
        else:
            self.__uri = uri

        url_protocol = None
        url_domain = None
        try:
            url_protocol, url_domain = self.__uri.split('://')
        except ValueError:
            raise MediaInfoError("Invalid URL: %s!" % (self.__uri))
        tmp, filename = os.path.split(url_domain)
        self.__name, self.__ext = os.path.splitext(filename)
        self.__ext = self.__ext.replace('.', '')
        self.__info = {}
        try:
            res = self.__db.get(uri=self.__uri)
            self.__info['uri'] = self.__uri
            self.__info['title'] = str(res[1])
            if res[2]:
                self.__info['artist'] = str(res[2])
            else:
                self.__info['artist'] = ""
            if res[3]:
                self.__info['album'] = str(res[3])
            else:
                self.__info['album'] = ""
            self.__info['tracknum'] = str(res[4])
            if res[5]:
                self.__info['date'] = str(res[5])
            else:
                self.__info['date'] = ""
            self.__info['length'] = str(res[6])
            self.__info['rawlen'] = res[7]

        except MediaDatabaseError:
            if url_protocol == 'file':
                try:
                    self.__metadata = mutagen.File(url_domain)
                    if 'audio/mp4' in self.__metadata.mime:
                        self.__parse_mp4_file()
                    elif 'audio/mp3' in self.__metadata.mime:
                        self.__parse_mp3_file()
                    else:
                        self.__fill_in_empty_info()
                except:
                    self.__fill_in_empty_info()
            else:
                self.__fill_in_empty_info()
            self.__db.insert(uri = self.__uri,
                             title = self.__info['title'],
                             artist = self.__info['artist'],
                             album = self.__info['album'],
                             tracknum = self.__info['tracknum'],
                             date = self.__info['date'],
                             length = self.__info['length'],
                             rawlen = self.__info['rawlen'])

    def __fill_in_empty_info(self):
        self.__info['title'] = self.__name
        self.__info['artist'] = ""
        self.__info['album'] = ""
        self.__info['tracknum'] = ""
        self.__info['date'] = ""
        self.__info['length'] = ""
        self.__info['rawlen'] = -1

    def __parse_mp3_file(self):
        try:
            self.__info['title'] = str(self.__metadata['TIT2'])
        except:
            self.__info['title'] = self.__name
        try:
            self.__info['artist'] = str(self.__metadata['TPE1'])
        except:
            self.__info['artist'] = ""
        try:
            self.__info['album'] = str(self.__metadata['TALB'])
        except:
            self.__info['album'] = ""
        try:
            self.__info['tracknum'] = str(self.__metadata['TRCK'])
        except:
            self.__info['tracknum'] = ""
        try:
            self.__info['date'] = str(self.__metadata['TDRC'])
        except:
            self.__info['date'] = ""
        try:
            info = self.__metadata.info
            self.__info['rawlen'] = info.length
            self.__info['length'] = utils.convertSecondsToString(info.length)
        except:
            self.__info['length'] = ""
            self.__info['rawlen'] = -1

    def __parse_mp4_file(self):
        try:
            self.__info['title'] = str(self.__metadata['\xa9nam'][0])
        except:
            self.__info['title'] = self.__name
        try:
            self.__info['artist'] = str(self.__metadata['\xa9ART'][0])
        except:
            self.__info['artist'] = ""
        try:
            self.__info['album'] = str(self.__metadata['\xa9alb'][0])
        except:
            self.__info['album'] = ""
        try:
            self.__info['tracknum'] = str(self.__metadata['trkn'][0])
        except:
            self.__info['tracknum'] = ""
        try:
            self.__info['date'] = str(self.__metadata['\xa9day'][0])
        except:
            self.__info['date'] = ""
        try:
            info = self.__metadata.info
            self.__info['rawlen'] = info.length
            self.__info['length'] = utils.convertSecondsToString(info.length)
        except:
            self.__info['length'] = ""
            self.__info['rawlen'] = -1
            
    def __getitem__(self, index):
        return self.__info[index]

    def __setitem__(self, key, value):
        if type(value) == type(''):
            value = value.replace('\\\'', '') 
        self.__db.update(self.__uri, key, value)
        self.__info[key] = value

    def __len__(self):
        return len(self.__info)

    def __iter__(self):
        return self

    def __str__(self):
        return self.__info.__str__()

    def next(self):
        try:
            item = self.__info[self.__index]
            self.__index = self.__index + 1
            return item
        except:
            self.__index = 0
            raise StopIteration()

if __name__ == '__main__':
    import sys

    if len(sys.argv) < 3:
        print "USAGE: %s DATABASE_PATH FILE [FILE ...]" % (sys.argv[0])
        sys.exit(1)

    db = MediaDatabase(sys.argv[1])
    for d in sys.argv[2:]:
        if not os.path.isdir(d):
            print MediaInfo(d, db)
        else:
            for f in os.listdir(d):
                print MediaInfo('file://' + os.path.join(d, f), db)
