/*
 * Copyright (C) 2007 Intel Corporation
 *
 * Author:  Horace Li <horace.li@intel.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */
#ifndef COMMON_CONFIG_H
#define COMMON_CONFIG_H

#include <gtk/gtk.h>
#include <glib-object.h>
#include <libhildondesktop/libhildondesktop.h>
#include <libhildondesktop/tasknavigator-item.h>
#include <string.h>

#define DEFAULT_MARQUEE_PANEL_HEIGHT  52
#define MARQUEE_DROPDOWN "marquee-drowdown"
#define MARQUEE_ENTRY "Marquee"
#define MARQUEE_HEIGHT "X-Size-Height"
#define HILDON_DESKTOP_CONFIG "/etc/hildon-desktop/desktop.conf"

void plugins_popup_preference (const gchar *pref_id);
gint plugins_get_marquee_panel_height (void);

#endif
