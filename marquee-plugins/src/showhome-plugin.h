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

#ifndef SHOWHOME_PLUGIN_H
#define SHOWHOME_PLUGIN_H

#include "common-config.h"
#include <libhildonwm/hd-wm.h>

G_BEGIN_DECLS

typedef struct _ShowhomePlugin ShowhomePlugin;
typedef struct _ShowhomePluginClass ShowhomePluginClass;
typedef struct _ShowhomePluginPrivate ShowhomePluginPrivate;

#define SHOWHOME_TYPE_PLUGIN            (showhome_plugin_get_type ())
#define SHOWHOME_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SHOWHOME_TYPE_PLUGIN, ShowhomePlugin))
#define SHOWHOME_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  SHOWHOME_TYPE_PLUGIN, ShowhomePluginClass))
#define SHOWHOME_IS_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SHOWHOME_TYPE_PLUGIN))
#define SHOWHOME_IS_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  SHOWHOME_TYPE_PLUGIN))
#define SHOWHOME_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  SHOWHOME_TYPE_PLUGIN, ShowhomePluginClass))

struct _ShowhomePlugin 
{
   TaskNavigatorItem tnitem;
   HDWM *hdwm;
   GtkWidget *icon;
   GtkWidget *btn;
   ShowhomePluginPrivate *priv;
};

struct _ShowhomePluginClass 
{
   TaskNavigatorItemClass parent_class;
};

GType  showhome_plugin_get_type  (void);

G_END_DECLS

#endif //SHOWHOME_PLUGIN_H
