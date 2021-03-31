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

#include <gtk/gtk.h>
#include <glib-object.h>
#include <libhildondesktop/libhildondesktop.h>
#include <libhildondesktop/hildon-desktop-panel-window-dialog.h>
#include <libhildondesktop/hildon-desktop-panel-expandable.h>

#include "hm-statusbar-container.h"
#include "hd-plugin-manager.h"
#include "hd-config.h"

#define THEME_DIR "/usr/share/themes/mobilebasic"
#define STATUSBAR_CONF_USER_PATH    ".config/hildon-desktop/"
#define STATUSBAR_CONF "statusbar.conf"
#define HD_PANEL_WINDOW_DIALOG_BUTTON_NAME  "HildonStatusBarItem"

#define HM_STATUSBAR_CONTAINER_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HM_TYPE_STATUSBAR_CONTAINER, StatusbarContainerPrivate))

G_DEFINE_TYPE (StatusbarContainer, statusbar_container, TASKNAVIGATOR_TYPE_ITEM);

struct _StatusbarContainerPrivate 
{
   HDWM *hdwm;

   GtkContainer *statusbar;
   GObject *pm;
};

void statusbar_container_screen_changed (TaskNavigatorItem *item);
static void statusbar_container_finalize (GObject *object);

static GList *
plugin_list_from_conf (const gchar *config_file)
{
  GKeyFile *keyfile;
  gchar **groups;
  gboolean is_to_load = TRUE;
  GList *plugin_list = NULL;
  GError *error = NULL;
  gint i;

  g_return_val_if_fail (config_file != NULL, NULL);

  keyfile = g_key_file_new ();

  g_key_file_load_from_file (keyfile,
                             config_file,
                             G_KEY_FILE_NONE,
                             &error);

  if (error)
  {
    g_warning ("Error loading container configuration file %s: %s", config_file, error->message);
    g_error_free (error);

    return NULL;
  }

  groups = g_key_file_get_groups (keyfile, NULL);

  for (i = 0; groups[i]; i++)
  {
    is_to_load = g_key_file_get_boolean (keyfile,
                                         groups[i],
                                         HD_DESKTOP_CONFIG_KEY_LOAD,
                                         &error);

    if (error)
    {
      is_to_load = TRUE;
      g_error_free (error);
      error = NULL;
    }

    if (is_to_load)
      plugin_list = g_list_append (plugin_list, groups[i]);
  }

  g_free (groups);
  g_key_file_free (keyfile);

  return plugin_list;
}

static void
hm_statusbar_container_cadd (HildonDesktopPanelExpandable *container,
                      GtkWidget *widget,
                      gpointer user_data)
{
  gtk_widget_set_name (widget, HD_PANEL_WINDOW_DIALOG_BUTTON_NAME);
  gtk_widget_set_name (GTK_BIN (widget)->child, HD_PANEL_WINDOW_DIALOG_BUTTON_NAME);
}

static void statusbar_container_init (StatusbarContainer *statusbar_container)
{
   GtkContainer *statusbar = NULL;
   int scn_width = 800, plugin_width;
   GdkScreen *screen = NULL;
   GList *plugin_list = NULL;
   gchar *user_config = NULL;

   StatusbarContainerPrivate *priv = HM_STATUSBAR_CONTAINER_GET_PRIVATE (statusbar_container);

   statusbar_container->priv = priv;
   statusbar_container->priv->hdwm = hd_wm_get_singleton ();
   statusbar_container->priv->pm = hd_plugin_manager_new ();

   screen = gtk_widget_get_screen (GTK_WIDGET(statusbar_container));
   if (screen != NULL) {
      scn_width = gdk_screen_get_width(screen);
   }

   g_object_ref (statusbar_container->priv->hdwm);
   gtk_widget_push_composite_child ();

   statusbar = g_object_new (HILDON_DESKTOP_TYPE_PANEL_EXPANDABLE, "items_row", (3 + scn_width / 320), NULL);

   //Hard code orientation as horizontal first, this should get from marquee orientation ultmately.
   if (NULL != statusbar)
   {
       g_signal_connect (G_OBJECT (statusbar),
                    "queued-button",
                    G_CALLBACK (hm_statusbar_container_cadd),
                    NULL);

       g_object_set (G_OBJECT (statusbar),
                  "orientation",
                  GTK_ORIENTATION_HORIZONTAL,
                  NULL);
   }

   plugin_width = 52 * (3 + scn_width / 320);
   gtk_widget_set_size_request ((GtkWidget*)statusbar_container, plugin_width, 52);

   user_config = g_build_filename (g_get_home_dir (), STATUSBAR_CONF_USER_PATH, STATUSBAR_CONF, NULL);
   if (!g_file_test (user_config, G_FILE_TEST_EXISTS))
   {
      gchar *default_conf, *buffer;
      gsize buffer_len;

      default_conf = g_build_filename (HD_DESKTOP_CONFIG_PATH, STATUSBAR_CONF, NULL);

      if ((g_file_test (default_conf, G_FILE_TEST_EXISTS)) &&
          (g_file_get_contents (default_conf, &buffer, &buffer_len, NULL)))
         g_file_set_contents (user_config, buffer, buffer_len, NULL);

      g_free (default_conf);
      default_conf = NULL;
   }

   if (user_config)
   {
      plugin_list = plugin_list_from_conf (user_config);
      hd_plugin_manager_load (HD_PLUGIN_MANAGER (statusbar_container->priv->pm),
                              plugin_list,
                              statusbar,
                              NULL);
   }

   statusbar_container->priv->statusbar = statusbar;

   gtk_container_add (GTK_CONTAINER(statusbar_container), GTK_WIDGET(statusbar));
   gtk_widget_show_all(GTK_WIDGET(statusbar));

   gtk_widget_pop_composite_child ();
}

void statusbar_container_screen_changed (TaskNavigatorItem *item)
{
  StatusbarContainer *statusbar_container = (StatusbarContainer *)item;
  StatusbarContainerPrivate *priv = HM_STATUSBAR_CONTAINER_GET_PRIVATE (statusbar_container);
  GdkScreen *screen = gdk_screen_get_default ();
  gint scn_width = 800, width;
  scn_width = gdk_screen_get_width (screen);

  width = 52 * (3 + scn_width / 320);
  g_object_set (G_OBJECT (priv->statusbar), "items_row", (width / 52), NULL);
  g_print ("statusbar_container_screen_changed: new width %d\n", width);
  gtk_widget_set_size_request (GTK_WIDGET(statusbar_container), width, 52);
}

static void statusbar_container_class_init (StatusbarContainerClass *class)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (class);
  TaskNavigatorItemClass *item_class = TASKNAVIGATOR_ITEM_CLASS (class);

  item_class->screen_changed = statusbar_container_screen_changed;
  object_class->finalize    = statusbar_container_finalize;
  g_type_class_add_private (object_class, sizeof (StatusbarContainerPrivate));
}

static void statusbar_container_finalize (GObject *object)
{
   g_return_if_fail (object != NULL);
   g_return_if_fail (IS_STATUSBAR_CONTAINER (object));

   StatusbarContainer *statusbar_container = STATUSBAR_CONTAINER (object);
   StatusbarContainerPrivate *priv = HM_STATUSBAR_CONTAINER_GET_PRIVATE (statusbar_container);

   if (priv->pm != NULL)
   {
       g_object_unref (priv->pm);
       priv->pm = NULL;
   }

   G_OBJECT_CLASS (statusbar_container_parent_class)->finalize (object);
}
