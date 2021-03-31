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

#include "showhome-plugin.h"

HD_DEFINE_PLUGIN (ShowhomePlugin, showhome_plugin, TASKNAVIGATOR_TYPE_ITEM);

void showhome_plugin_screen_changed (TaskNavigatorItem *item);
static void showhome_plugin_finalize (GObject *object);

/* btn clicked */
static void btn_clicked (GtkWidget *button, ShowhomePlugin *showhome_plugin )
{
    HDWMWindow *wnd;

    wnd = hd_wm_get_active_window();

    if(wnd)
    {
        g_print ("Cache current active window, and show up desktop.\n");
        hd_wm_top_desktop();
    }
    else
    {
        HDWMEntryInfo *app_info = NULL;

        g_print ("Show up last active window.\n");
        wnd = hd_wm_get_last_active_window();

        if(wnd)
            app_info = HD_WM_ENTRY_INFO(wnd);
        else
        {
            GList *applications_list = NULL;
            HDWMApplication *app;

            applications_list = hd_wm_get_applications (showhome_plugin->hdwm);
            if(applications_list)
            {
                app = HD_WM_APPLICATION (applications_list->data);
                app_info = HD_WM_ENTRY_INFO(hd_wm_application_get_active_window(app));
            }
        }

        if (app_info)
            hd_wm_top_item(app_info);
    }
}

static void showhome_plugin_init (ShowhomePlugin *showhome_plugin)
{
   GtkWidget *btn;
   gint panel_height;

   showhome_plugin->hdwm = hd_wm_get_singleton ();
   g_object_ref (showhome_plugin->hdwm);
   gtk_widget_push_composite_child ();

   panel_height = plugins_get_marquee_panel_height ();

   btn = showhome_plugin->btn = gtk_button_new();

   gtk_widget_set_size_request ((GtkWidget*)showhome_plugin, DEFAULT_MARQUEE_PANEL_HEIGHT, panel_height);

   g_signal_connect (btn, "clicked", G_CALLBACK (btn_clicked), (gpointer)showhome_plugin);

   gtk_widget_show_all (btn);
   gtk_container_add (GTK_CONTAINER (showhome_plugin), btn);

   gtk_widget_pop_composite_child ();
}

void showhome_plugin_screen_changed (TaskNavigatorItem *item)
{
  gint height = plugins_get_marquee_panel_height ();
  g_print ("showhome_plugin_screen_changed: new width %d new height %d\n", DEFAULT_MARQUEE_PANEL_HEIGHT, height);
  gtk_widget_set_size_request (GTK_WIDGET(item), DEFAULT_MARQUEE_PANEL_HEIGHT, height);
}

static void showhome_plugin_class_init (ShowhomePluginClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  TaskNavigatorItemClass *item_class = TASKNAVIGATOR_ITEM_CLASS (class);

  item_class->screen_changed = showhome_plugin_screen_changed;
  object_class->finalize = showhome_plugin_finalize;
}

static void showhome_plugin_finalize (GObject *object)
{
   ShowhomePlugin *showhome_plugin = SHOWHOME_PLUGIN (object);	

   g_object_unref (showhome_plugin->hdwm);
   //g_object_unref (showhome_plugin->icon);
   G_OBJECT_CLASS (showhome_plugin_parent_class)->finalize (object);
}
