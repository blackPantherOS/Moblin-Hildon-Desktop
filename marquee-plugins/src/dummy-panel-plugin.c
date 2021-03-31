/*
 * This file is part of example-plugins 
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

#include "dummy-panel-plugin.h"

HD_DEFINE_PLUGIN (DummyPanelPlugin, dummy_panel_plugin, TASKNAVIGATOR_TYPE_ITEM);

static void
dummy_panel_plugin_init (DummyPanelPlugin *panel_plugin)
{
  GtkWidget *button;

  button = gtk_button_new ();

  gtk_button_set_image (GTK_BUTTON (button), 
                        gtk_image_new_from_stock (GTK_STOCK_CONVERT,
                                                  GTK_ICON_SIZE_LARGE_TOOLBAR));

  gtk_widget_set_size_request (button, 48, 48);

  gtk_widget_show_all (button);

  gtk_container_add (GTK_CONTAINER (panel_plugin), button);
}

static void
dummy_panel_plugin_class_init (DummyPanelPluginClass *class)
{
}
