#!/usr/bin/python -ttu
# -*- mode:python; tab-width:4; indent-tabs-mode:nil;  -*-
# vim: ai ts=4 sts=4 et sw=4
# 
# media_service.py: This file provides the MediaService class.  Which is a way
#                    of abstracting the underlying media service engine
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


import ConfigParser
import dbus
import gtk
import os
import signal
import string
import sys

from paste.util import classinit


class MediaServiceError(Exception):
    def __init__(self, message):
        self.message = message

    def __str__(self):
        return self.message


class MediaService(object):
    """The MediaService class manages/abstracts access to media services (for
    example gstreamer or some other media service).  Along with all the details
    for connecting over dbus."""
    __metaclass__ = classinit.ClassInitMeta
    __classinit__ = classinit.build_properties

    def __init__(self, config_dir):
        """Sample config file that would be in the config_dir:
        [Engine]
        enabled=yes
        name=helix
        DBusName=org.helixcommunity.MobileMediaService
        DBusPath=/org.helixcommunity/MobileMediaService"""
        self.__config_dir = config_dir
        self.__connected = False
        self.__callbacks = {}
        self.__private_callbacks = {}        
        self.__service_name = ''
        self.__dbus_name = ''
        self.__dbus_path = ''
        self.__xid = None
        self.__width = None
        self.__height = None
        self.__uri = None
        for f in os.listdir(self.__config_dir):
            try:
                cf = ConfigParser.ConfigParser()
                cf.read(os.path.join(self.__config_dir, f))
                if cf.get('Engine','enabled').lower() == 'yes':
                    self.__service_name = cf.get('Engine','name')
                    self.__dbus_name = cf.get('Engine','DBusName')
                    self.__dbus_path = cf.get('Engine','DBusPath')
                    break
            except:
                # Skip over any problematic configuration files
                pass
        if not self.__dbus_path:
            raise MediaServiceError('No valid media services found')

    def __connect_service(self):
        self.__bus = dbus.SessionBus()
        self.__player = self.__bus.get_object(self.__dbus_name, self.__dbus_path)
        self.__iface = dbus.Interface(self.__player, self.__dbus_name)
        session = self.__bus.get_object("org.freedesktop.DBus",
                                        "/org/freedesktop/DBus")
        self.__pid = session.GetConnectionUnixProcessID(self.__dbus_name)
        self.__connected = True
        if self.__xid:
            self.__player.SetWindow(self.__xid)
        if self.__width and self.__height:
            self.__player.SetGeometry(self.__width, self.__height)
        if self.__uri:
            self.__player.OpenUri(self.__uri)
        for entry in self.__callbacks:
            try:
                self.__iface.connect_to_signal(entry, self.__callbacks[entry])
            except:
                # delete any problematic callbacks
                del self.__callbacks[entry]
        if 'service-connected' in self.__private_callbacks:
            self.__private_callbacks['service-connected']()

    def __handle_service_exception(self):
        # If anything fishy is happening with the service, then just
        # shot it in the head and bubble up an exception letting the
        # caller know what's going on.
        #
        # The next call back into this object will automatically
        # reconnect to a new media service
        try:
            os.kill(self.__pid, signal.SIGKILL)
        except:
            # Its possible the service was already dead
            pass
        self.__connect_service()
        raise MediaServiceError('Remote media service restart')
    
    def connect_to_signal(self, signal, callback):
        if signal in ['service-connected']:
            # 'service-connected' is a signal that the user of the object can
            # register for that indicates that the service has been connected.
            # It is not implemented by the media service, but by our code.  We
            # do not actually connect to the media service till we need
            # something
            self.__private_callbacks[signal] = callback
        else:
            self.__callbacks[signal] = callback
            if self.__connected:
                self.__iface.connect_to_signal(signal, callback)
                self.__callbacks[signal] = callback

    def name__get(self):
        return self.__service_name
        
    def SetWindow(self, xid):
        try:
            self.__xid = xid
            if not self.__connected:
                self.__connect_service()
            return self.__player.SetWindow(xid)
        except dbus.exceptions.DBusException:
            return self.__handle_service_exception()

    def GetWindow(self):
        try:
            if not self.__connected:
                self.__connect_service()
            return self.__player.GetWindow()
        except dbus.exceptions.DBusException:
            return self.__handle_service_exception()

    def SetGeometry(self, width, height):
        try:
            self.__width = width
            self.__height = height
            if not self.__connected:
                self.__connect_service()
            return self.__player.SetGeometry(width, height)
        except dbus.exceptions.DBusException:
            return self.__handle_service_exception()

    def GetGeometry(self):
        try:
            if not self.__connected:
                self.__connect_service()
            return self.__player.GetGeometry()
        except dbus.exceptions.DBusException:
            return self.__handle_service_exception()

    def GetUri(self):
        return self.__uri

    def OpenUri(self, uri):
        try:
            self.__uri = uri
            if not self.__connected:
                self.__connect_service()
            return self.__player.OpenUri(uri)
        except dbus.exceptions.DBusException:
            return self.__handle_service_exception()

    def Play(self):
        try:
            if not self.__connected:
                self.__connect_service()
            return self.__player.Play()
        except dbus.exceptions.DBusException:
            return self.__handle_service_exception()
        
    def Pause(self):
        try:
            if not self.__connected:
                self.__connect_service()
            return self.__player.Pause()
        except dbus.exceptions.DBusException:
            return self.__handle_service_exception()
    
    def Stop(self):
        try:
            if not self.__connected:
                self.__connect_service()
            return self.__player.Stop()
        except dbus.exceptions.DBusException:
            return self.__handle_service_exception()

    def Status(self):
        try:
            if not self.__connected:
                self.__connect_service()
            return self.__player.Status()
        except dbus.exceptions.DBusException:
            return self.__handle_service_exception()

    def SetVolume(self, volume):
        try:
            if not self.__connected:
                self.__connect_service()
            return self.__player.SetVolume(volume)
        except dbus.exceptions.DBusException:
            return self.__handle_service_exception()

    def GetVolume(self):
        try:
            if not self.__connected:
                self.__connect_service()
            return self.__player.GetVolume()
        except dbus.exceptions.DBusException:
            return self.__handle_service_exception()

    def SetPosition(self, position):
        try:
            if not self.__connected:
                self.__connect_service()
            return self.__player.SetPosition(position)
        except dbus.exceptions.DBusException:
            return self.__handle_service_exception()

    def GetPosition(self):
        try:
            if not self.__connected:
                self.__connect_service()
            return self.__player.GetPosition()
        except dbus.exceptions.DBusException:
            return self.__handle_service_exception()

    def Close(self):
        self.__width = None
        self.__height = None
        self.__uri = None
        try:
            if self.__connected:
                self.__connected = False
                self.__player.Close()
        except dbus.exceptions.DBusException:
            return self.__handle_service_exception()

    def Quit(self):
        self.__width = None
        self.__height = None
        self.__uri = None
        try:
            if self.__connected:
                self.__connected = False
                self.__player.Quit()
        except dbus.exceptions.DBusException:
            return self.__handle_service_exception()
    
    def Expose(self):
        try:
            if not self.__connected:
                self.__connect_service()
            return self.__player.Expose()
        except dbus.exceptions.DBusException:
            return self.__handle_service_exception()
	
    def GetVideoFrame(self, uri, output_file):
        try:
            if not self.__connected:
                self.__connect_service()
            return self.__player.GetVideoFrame(uri, output_file)
        except dbus.exceptions.DBusException:
            return self.__handle_service_exception()

    def ResizeWindow(self,size):
        try:
            if not self.__connected:
                self.__connect_service()
            return self.__player.ResizeWindow(size)
        except dbus.exceptions.DBusException:
            return self.__handle_service_exception()
            

if __name__ == '__main__':
    service = MediaService(sys.argv[1])
    print "MediaService(%s): %s" % (sys.argv[1], service)
