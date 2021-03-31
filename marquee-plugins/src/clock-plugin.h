/*
 * Copyright (C) 2007 Intel Corporation
 *
 * Author:  Xu Bo <bo.b.xu@intel.com>
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

#ifndef CLOCK_PLUGIN_H
#define CLOCK_PLUGIN_H

#include "common-config.h"

G_BEGIN_DECLS

typedef struct _ClockPlugin ClockPlugin;
typedef struct _ClockPluginClass ClockPluginClass;
typedef struct _ClockPluginPrivate ClockPluginPrivate;

#define CLOCK_TYPE_PLUGIN            (clock_plugin_get_type ())
#define CLOCK_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLOCK_TYPE_PLUGIN, ClockPlugin))
#define CLOCK_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLOCK_TYPE_PLUGIN, ClockPluginClass))
#define CLOCK_IS_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLOCK_TYPE_PLUGIN))
#define CLOCK_IS_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLOCK_TYPE_PLUGIN))
#define CLOCK_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLOCK_TYPE_PLUGIN, ClockPluginClass))

struct _ClockPlugin 
{
   TaskNavigatorItem tnitem;
   ClockPluginPrivate *priv;
};

struct _ClockPluginClass 
{
   TaskNavigatorItemClass parent_class;
};

GType  clock_plugin_get_type  (void);

G_END_DECLS

#endif //CLOCK_PLUGIN_H
