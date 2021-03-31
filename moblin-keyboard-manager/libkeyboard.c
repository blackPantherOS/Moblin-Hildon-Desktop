/*
 * Copyright (C) 2007 Intel Corporation
 *
 * Author: Jian Han <jian.han@intel.com> 
 *         Horace Li <horace.li@intel.com>
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

#include <signal.h>
#include <errno.h>
#include <sys/resource.h>

#include <gtk/gtk.h>
#include <gconf/gconf-client.h>

#include <libhildondesktop/hildon-status-bar-item.h>

#define VIRTUAL_KEYBOARD_EXEC "matchbox-keyboard"
#define VIRTUAL_KEYBOARD_GCONF_ENTRY "/desktop/moblin/peripherals/keyboard/kbd"
#define VIRTUAL_KEYBOARD_GCONF_LAYOUT VIRTUAL_KEYBOARD_GCONF_ENTRY"/layout"
#define VIRTUAL_KEYBOARD_GCONF_MODEL VIRTUAL_KEYBOARD_GCONF_ENTRY"/model"

typedef struct _keyboardPlugin
{
  HildonStatusBarItem *item;
  GtkWidget* button;
  GConfClient *client;
  GPid child_pid;
}keyboardPlugin;

void *keyboard_initialize(HildonStatusBarItem *item, GtkWidget **button);
void keyboard_update(void *data, gint value1, gint value2, const gchar *str);
void keyboard_destroy(void *data);
int keyboard_get_priority(void *data);

static void virtual_keyboard_exit_callback (GPid pid, gint status, gpointer data)
{
  keyboardPlugin *keyboard_plugin = (keyboardPlugin *)data;

  keyboard_plugin->child_pid = 0;
}

static void keyboard_plugin_clicked(GtkButton* btn, gpointer data)
{
  keyboardPlugin *keyboard_plugin = (keyboardPlugin *)data;

  if (keyboard_plugin->child_pid)
  {
    kill (keyboard_plugin->child_pid, SIGTERM);
  }
  else
  {
    GError *error = NULL;
    gint argc;
    gchar **argv;
    GPid child_pid;
    guint event_id;
    gchar *command, *layout;

    layout = gconf_client_get_string (keyboard_plugin->client, VIRTUAL_KEYBOARD_GCONF_LAYOUT,  &error);

    if(error)
    {
      g_warning ("Virtual keyboard could not load proper layout. (Error: %s)", error->message);
      g_clear_error (&error);
      layout = g_strdup ("default");
    }

    command = g_strjoin (" ", VIRTUAL_KEYBOARD_EXEC, layout, NULL);

    if (g_shell_parse_argv (command, &argc, &argv, &error)) {
      g_spawn_async ( NULL,           //Child's current working directory or NULL to use parent's
                      argv,           //Child's argument vector. [0] is the path of the program to execute
                      NULL,           //Child's environment or NULL to inherit parent's
                      G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH, //flags from GSpawnFlags
                      NULL,           //Function to run in the child just before exec
                      NULL,           //User data for child_setup
                      &child_pid,     //Return location for child process ID or NULL
                      &error);        //Return location for error
    }

    if (error) {
      g_warning ("Virtual keyboard could not be started up. (Error: %s)", error->message);
      g_clear_error (&error);
    } else {
      setpriority (PRIO_PROCESS, child_pid, 0);
      event_id = g_child_watch_add ( child_pid,
                                     virtual_keyboard_exit_callback,
                                     data);
      keyboard_plugin->child_pid = child_pid;
    }
  }
}

GConfClient *keyboard_gconf_client_new (void)
{
  GConfClient *client;
  GError *error = NULL;
  gchar *layout = NULL;
  gboolean model = FALSE;

  gconf_init (0, NULL, NULL);
  client = gconf_client_get_default ();
  gconf_client_add_dir (client,
                        VIRTUAL_KEYBOARD_GCONF_ENTRY,
                        GCONF_CLIENT_PRELOAD_NONE,
                        NULL);

  layout = gconf_client_get_string (client, VIRTUAL_KEYBOARD_GCONF_LAYOUT, &error);

  if (error || !layout)
    gconf_client_set_string (client, VIRTUAL_KEYBOARD_GCONF_LAYOUT, "default", NULL);

  model = gconf_client_get_bool (client, VIRTUAL_KEYBOARD_GCONF_MODEL, &error);
  if (error)
    gconf_client_set_bool (client, VIRTUAL_KEYBOARD_GCONF_MODEL, FALSE, NULL);

  return client;
}

void keyboard_entry (HildonStatusBarPluginFn_st *fn)
{
  if (NULL != fn)
  {
    fn->initialize = keyboard_initialize;
    fn->destroy = keyboard_destroy;
    fn->update = keyboard_update;
    fn->get_priority = keyboard_get_priority;
  }
}

void *keyboard_initialize(HildonStatusBarItem *item, GtkWidget **button)
{
  keyboardPlugin *keyboard_plugin;
  GConfClient *client;
  GtkWidget *icon;

  keyboard_plugin = g_new0 (keyboardPlugin, 1);

  *button  = gtk_button_new ();
  g_signal_connect (*button, "clicked", G_CALLBACK (keyboard_plugin_clicked), keyboard_plugin);
	
  icon = gtk_image_new_from_icon_name ("vkeyboard", GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER(*button), GTK_WIDGET(icon));	
  gtk_widget_show (icon);
  gtk_widget_show (*button);

  client = keyboard_gconf_client_new ();

  keyboard_plugin->button = *button;
  keyboard_plugin->client = client;
  keyboard_plugin->child_pid = 0;

  return (void *) keyboard_plugin;
}
        
void keyboard_update(void *data, gint value1, gint value2, const gchar *str)
{
}        
 
void keyboard_destroy(void *data)
{
  if (data)
    g_free(data);
}

gint keyboard_get_priority(void *data)
{
  return 42;
}
