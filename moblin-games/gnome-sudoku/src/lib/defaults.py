import os, os.path
import errno
import gettext

APP_DATA_DIR = os.path.join('/usr/local', 'share') 
IMAGE_DIR = os.path.join(APP_DATA_DIR, 'pixmaps', 'gnome-sudoku')
LOCALEDIR = os.path.join(APP_DATA_DIR, 'locale')
GLADE_DIR = os.path.join(APP_DATA_DIR,'gnome-sudoku')
BASE_DIR = os.path.join(APP_DATA_DIR,'gnome-sudoku')

DOMAIN = 'gnome-games'
gettext.bindtextdomain(DOMAIN, LOCALEDIR)
gettext.textdomain(DOMAIN)
from gettext import gettext as _
import gtk.glade
gtk.glade.bindtextdomain (DOMAIN, LOCALEDIR)
gtk.glade.textdomain (DOMAIN)

VERSION = "0.1"
APPNAME = _("GNOME Sudoku")
APPNAME_SHORT = _("Sudoku")
COPYRIGHT = 'Copyright \xc2\xa9 2005-2007, Thomas M. Hinkle'
DESCRIPTION = _('GNOME Sudoku is a simple sudoku generator and player. Sudoku is a japanese logic puzzle.\n\nGNOME Sudoku is a part of GNOME Games.')
AUTHORS = ["Thomas M. Hinkle"]
WEBSITE       = 'http://www.gnome.org/projects/gnome-games/'
WEBSITE_LABEL = _('GNOME Games web site')
AUTO_SAVE= True
MIN_NEW_PUZZLES = 90

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

DATA_DIR = os.path.expanduser('~/.gnome2/gnome-sudoku/')

def initialize_games_dir ():
    try:
        os.makedirs(DATA_DIR)
    except OSError, e:
        if e.errno != errno.EEXIST:
            print _('Unable to make data directory %(dir)s: %(error)s') % {'dir': DATA_DIR, 'error': e.strerror}
