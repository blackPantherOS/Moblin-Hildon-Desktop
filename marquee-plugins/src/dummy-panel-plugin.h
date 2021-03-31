/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Author:  Lucas Rocha <lucas.rocha@nokia.com>
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
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

#ifndef DUMMY_PANEL_PLUGIN_H
#define DUMMY_PANEL_PLUGIN_H

#include "common-config.h"

G_BEGIN_DECLS

typedef struct _DummyPanelPlugin DummyPanelPlugin;
typedef struct _DummyPanelPluginClass DummyPanelPluginClass;
typedef struct _DummyPanelPluginPrivate DummyPanelPluginPrivate;

#define DUMMY_TYPE_PANEL_PLUGIN            (dummy_panel_plugin_get_type ())
#define DUMMY_PANEL_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), DUMMY_TYPE_PANEL_PLUGIN, DummyPanelPlugin))
#define DUMMY_PANEL_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  DUMMY_TYPE_PANEL_PLUGIN, DummyPanelPluginClass))
#define DUMMY_IS_PANEL_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DUMMY_TYPE_PANEL_PLUGIN))
#define DUMMY_IS_PANEL_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  DUMMY_TYPE_PANEL_PLUGIN))
#define DUMMY_PANEL_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  DUMMY_TYPE_PANEL_PLUGIN, DummyPanelPluginClass))

struct _DummyPanelPlugin 
{
  TaskNavigatorItem tnitem;

  DummyPanelPluginPrivate *priv;
};

struct _DummyPanelPluginClass 
{
  TaskNavigatorItemClass parent_class;
};

GType  dummy_panel_plugin_get_type  (void);

G_END_DECLS

#endif
