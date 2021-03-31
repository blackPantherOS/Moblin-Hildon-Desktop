/*
 * Copyright (C) 2007 Intel Corporation
 *
 * Author:  Bob Spencer <bob.spencer@intel.com>
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

#ifndef CLOSE_APP_PLUGIN_H
#define CLOSE_APP_PLUGIN_H

#include "common-config.h"
#include <libhildonwm/hd-wm.h>

G_BEGIN_DECLS

typedef struct _CloseAppPlugin CloseAppPlugin;
typedef struct _CloseAppPluginClass CloseAppPluginClass;
typedef struct _CloseAppPluginPrivate CloseAppPluginPrivate;

#define CLOSE_APP_TYPE_PLUGIN            (close_app_plugin_get_type ())
#define CLOSE_APP_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLOSE_APP_TYPE_PLUGIN, CloseAppPlugin))
#define CLOSE_APP_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLOSE_APP_TYPE_PLUGIN, CloseAppPluginClass))
#define CLOSE_APP_IS_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLOSE_APP_TYPE_PLUGIN))
#define CLOSE_APP_IS_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLOSE_APP_TYPE_PLUGIN))
#define CLOSE_APP_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLOSE_APP_TYPE_PLUGIN, CloseAppPluginClass))

struct _CloseAppPlugin 
{
   TaskNavigatorItem tnitem;
   HDWM *hdwm;
   GtkWidget *imgUp;
   GtkWidget *imgDn;
   GtkWidget *btn;
   CloseAppPluginPrivate *priv;
};

struct _CloseAppPluginClass 
{
   TaskNavigatorItemClass parent_class;
};

GType  close_app_plugin_get_type  (void);

G_END_DECLS

#endif //CLOSE_APP_PLUGIN_H
