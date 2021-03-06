# -*- coding: utf-8 -*-

import os, os.path
import errno
import gettext 

APP_DATA_DIR     = os.path.join('/usr/local', 'share') 
ICON_DIR         = os.path.join(APP_DATA_DIR, 'pixmaps')
TEXTURE_DIR      = os.path.join(ICON_DIR, 'glchess')
GLADE_DIR        = os.path.join(APP_DATA_DIR, 'glchess')
BASE_DIR         = os.path.join(APP_DATA_DIR, 'glchess')
LOCALEDIR        = os.path.join(APP_DATA_DIR, 'locale')
DATA_DIR         = os.path.expanduser('~/.gnome2/glchess/')
LOG_DIR          = os.path.join(DATA_DIR, 'logs')
CONFIG_FILE      = os.path.join(DATA_DIR, 'config.xml')
HISTORY_DIR      = os.path.join(DATA_DIR, 'history')
UNFINISHED_FILE  = os.path.join(HISTORY_DIR, 'unfinished')
LOCAL_AI_CONFIG  = os.path.join(DATA_DIR, 'ai.xml')

DOMAIN = 'gnome-games'
gettext.bindtextdomain(DOMAIN, LOCALEDIR)
gettext.textdomain(DOMAIN)
from gettext import gettext as _

import gtk.glade
gtk.glade.bindtextdomain(DOMAIN, LOCALEDIR)
gtk.glade.textdomain(DOMAIN)

VERSION   = "0.1"
APPNAME   = _("glChess")
ICON_NAME = 'gnome-glchess'

COPYRIGHT     = _('Copyright 2005-2007 Robert Ancell (and contributors)')
DESCRIPTION   = _('The 2D/3D chess game for GNOME. \n\nglChess is a part of GNOME Games.')
WEBSITE       = 'http://www.gnome.org/projects/gnome-games/'
WEBSITE_LABEL = _('GNOME Games web site')
AUTHORS       = ['Robert Ancell <bob27@users.sourceforge.net>']
ARTISTS       = ['John-Paul Gignac (3D Models)', 'Max Froumentin (2D Models)', 'Hylke Bons <h.bons@student.rug.nl> (icon)']

# The GPL license string will be translated, and the game name inserted.
# This license is the same as in libgames-support/games-stock.c 
LICENSE = [_("%s is free software; you can redistribute it and/or modify " 
       "it under the terms of the GNU General Public License as published by " 
       "the Free Software Foundation; either version 2 of the License, or " 
       "(at your option) any later version.").replace("%s", APPNAME),
_("%s is distributed in the hope that it will be useful, "
       "but WITHOUT ANY WARRANTY; without even the implied warranty of "
       "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
       "GNU General Public License for more details.").replace("%s", APPNAME),
_("You should have received a copy of the GNU General Public License "
       "along with %s; if not, write to the Free Software Foundation, Inc., "
       "51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA").replace("%s", APPNAME)]

# License for 2D models is the following, which is GPL compatible, and therefore not included in the license dialog:
#2D Models:
#Copyright World Wide Web Consortium, (Massachusetts Institute of Technology, Institut National de Recherche en Informatique et en Automatique, Keio University). All Rights Reserved. http://www.w3.org/Consortium/Legal/"""

try:
    os.makedirs(DATA_DIR)
except OSError, e:
    if e.errno != errno.EEXIST:
       print _('Unable to make data directory %(dir)s: %(error)s') % {'dir': DATA_DIR, 'error': e.strerror}
