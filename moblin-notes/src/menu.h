/*
 *  menu.h
 *  This file is part of Mousepad
 *
 *  Copyright (C) 2005 Erik Harrison
 *  Copyright (C) 2004 Tarot Osuji
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _MENU_H
#define _MENU_H

gboolean menu_toggle_paste_item(void);
void menu_toggle_clipboard_item(gboolean selected);
#if 0
GtkWidget *create_menu_bar(MainWindow *mainwin, StructData *sd);
#else
GtkWidget *create_menu_bar(GtkWidget *window, StructData *sd);
#endif

#endif /* _MENU_H */
