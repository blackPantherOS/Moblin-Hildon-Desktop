/*
 * File: playerlist.h
 * Author: GGZ Dev Team
 * Project: GGZ GTK Client
 * Date: 11/03/2002
 * $Id: playerlist.h 5572 2006-10-14 10:32:58Z andreasr $
 * 
 * List of players in the current room
 * 
 * Copyright (C) 2000-2002 Justin Zaun.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <gtk/gtk.h>

void sensitize_player_list(gboolean sensitive);
void clear_player_list(void);
void update_player_list(void);

GtkWidget *create_player_list(GtkWidget *window);
