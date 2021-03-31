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

#include "close-app-plugin.h"

HD_DEFINE_PLUGIN (CloseAppPlugin, close_app_plugin, TASKNAVIGATOR_TYPE_ITEM);

void close_app_screen_changed (TaskNavigatorItem *item);
static void close_app_finalize (GObject *object);

/* show/hide close button */
static void active_window_update (HDWM *hdwm, CloseAppPlugin *close_plugin)
{
}

static void on_clicked (GtkWidget *button, CloseAppPlugin *close_plugin )
{
   //close active app
   HDWMWindow *wnd = hd_wm_get_active_window();

   if (wnd != NULL) {
      hd_wm_window_close (wnd);
   }
}

static void close_app_plugin_init (CloseAppPlugin *close_plugin)
{
   GtkWidget *btn;
   gint panel_height;

   close_plugin->hdwm = hd_wm_get_singleton ();
   g_object_ref (close_plugin->hdwm);

   panel_height = plugins_get_marquee_panel_height ();

   btn = close_plugin->btn = gtk_button_new();

   gtk_widget_set_size_request (btn, DEFAULT_MARQUEE_PANEL_HEIGHT, panel_height);

   g_signal_connect (btn, "clicked", 
		     G_CALLBACK (on_clicked), (gpointer)close_plugin);
   g_signal_connect (close_plugin->hdwm, "active-window-update",  
		     G_CALLBACK (active_window_update), (gpointer)close_plugin);

   gtk_widget_show_all (btn);
   gtk_container_add (GTK_CONTAINER (close_plugin), btn);
}

void close_app_screen_changed (TaskNavigatorItem *item)
{
  gint height = plugins_get_marquee_panel_height ();
  gtk_widget_set_size_request (GTK_WIDGET(item), DEFAULT_MARQUEE_PANEL_HEIGHT, height);
  gtk_widget_show_all (GTK_WIDGET(item));
}

static void close_app_plugin_class_init (CloseAppPluginClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  TaskNavigatorItemClass *item_class = TASKNAVIGATOR_ITEM_CLASS (class);

  item_class->screen_changed = close_app_screen_changed;
  object_class->finalize = close_app_finalize;
}

static void close_app_finalize (GObject *object)
{
   CloseAppPlugin *close_plugin = CLOSE_APP_PLUGIN (object);	
   g_object_unref (close_plugin->hdwm);
   G_OBJECT_CLASS (close_app_plugin_parent_class)->finalize (object);
}
