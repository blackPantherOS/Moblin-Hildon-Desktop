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

#ifndef DROP_DOWN_MENU_PLUGIN_H
#define DROP_DOWN_MENU_PLUGIN_H

#include "common-config.h"
#include <libhildonwm/hd-wm.h>
#include <libhildonwm/hd-wm-types.h>

G_BEGIN_DECLS

typedef struct _DropDownMenuPlugin DropDownMenuPlugin;
typedef struct _DropDownMenuPluginClass DropDownMenuPluginClass;
typedef struct _DropDownMenuPluginPrivate DropDownMenuPluginPrivate;

#define DROP_DOWN_MENU_TYPE_PLUGIN             (drop_down_menu_plugin_get_type ())
#define DROP_DOWN_MENU_PLUGIN(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), DROP_DOWN_MENU_TYPE_PLUGIN, DropDownMenuPlugin))
#define DROP_DOWN_MENU_PLUGIN_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass),  DROP_DOWN_MENU_TYPE_PLUGIN, DropDownMenuPluginClass))
#define DROP_DOWN_MENU_IS_PLUGIN(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DROP_DOWN_MENU_TYPE_PLUGIN))
#define DROP_DOWN_MENU_IS_PLUGIN_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass),  DROP_DOWN_MENU_TYPE_PLUGIN))
#define DROP_DOWN_MENU_PLUGIN_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj),  DROP_DOWN_MENU_TYPE_PLUGIN, DropDownMenuPluginClass))

struct _DropDownMenuPlugin 
{
   TaskNavigatorItem tnitem;
   DropDownMenuPluginPrivate *priv;
};

struct _DropDownMenuPluginPrivate
{
   HDWM *hdwm;
   GtkWidget *btn;
   GtkWidget *label;
   GtkWidget *arrowImg;
   gint button_height;
};

struct _DropDownMenuPluginClass 
{
   TaskNavigatorItemClass parent_class;
};

GType  drop_down_menu_plugin_get_type  (void);

G_END_DECLS

#endif //DROP_DOWN_MENU_PLUGIN_H
