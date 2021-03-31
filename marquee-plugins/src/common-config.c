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

#include <glib-object.h>
#include <errno.h>
#include <sys/resource.h>

#include "common-config.h"

enum
{
  PREF_ID_LIST_APPEND,
  PREF_ID_LIST_REMOVE,
  PREF_ID_LIST_GET_SINGLETON,
  N_OPERATION
};

static GList *plugins_operate_pref_id_list (gint op_id, gpointer data)
{
  static GList *pref_id_list = NULL;

  switch (op_id)
  {
    case PREF_ID_LIST_APPEND:
      pref_id_list = g_list_append (pref_id_list, data);
      break;
    case PREF_ID_LIST_REMOVE:
      pref_id_list = g_list_remove (pref_id_list, data);
      break;
    case PREF_ID_LIST_GET_SINGLETON:
    default:
      break;
  }

  return pref_id_list;
}

static void plugins_pref_watch_func (GPid pid, gint status, gpointer data)
{
  gchar *pref_id = (gchar *)data;
  GList *pref_id_list, *iter;

  pref_id_list = plugins_operate_pref_id_list (PREF_ID_LIST_GET_SINGLETON, NULL);

  for (iter = pref_id_list; iter; iter = iter->next)
  {
    if (!g_ascii_strcasecmp((gchar *)iter->data, pref_id))
      pref_id_list = plugins_operate_pref_id_list (PREF_ID_LIST_REMOVE, iter->data);
  }
}

void plugins_popup_preference (const gchar *pref_id)
{
  GError *error = NULL;
  gint argc;
  gchar **argv;
  GPid child_pid;
  gchar *command;
  GList *pref_id_list = NULL, *iter;
  guint event_id;

  pref_id_list = plugins_operate_pref_id_list (PREF_ID_LIST_GET_SINGLETON, NULL);

  for (iter = pref_id_list; iter; iter = iter->next)
  {
    if(!g_ascii_strcasecmp (iter->data, pref_id))
    {
      return;
    }
  }

  command = g_strjoin (" ", "moblin-applets", pref_id, NULL);

  if (command)
  {

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
      g_warning ("Background Properties dialog could not be started. (Error: %s)", error->message);
      g_clear_error (&error);
    } else {
      setpriority (PRIO_PROCESS, child_pid, 0);
      event_id = g_child_watch_add ( child_pid,
                                     plugins_pref_watch_func,
                                     g_strdup(pref_id));
      pref_id_list = plugins_operate_pref_id_list (PREF_ID_LIST_APPEND, g_strdup(pref_id));
    }
  }
}

gint plugins_get_marquee_panel_height (void)
{
   GKeyFile *key_file;
   GError *error = NULL;
   gint panel_height;

   key_file = g_key_file_new();

   g_key_file_load_from_file (key_file,
                              HILDON_DESKTOP_CONFIG,
                              G_KEY_FILE_NONE,
                              &error);

   if (error)
   {
      g_warning ("Error loading desktop configuration file: %s",
                 error->message);
      g_error_free (error);
      panel_height = DEFAULT_MARQUEE_PANEL_HEIGHT;
   }
   else
   {
      panel_height = g_key_file_get_integer (key_file,
                                             MARQUEE_ENTRY,
                                             MARQUEE_HEIGHT,
                                             &error);

      if (error)
      {
         g_warning ("Error loading desktop configuration file: %s",
                    error->message);
         g_error_free (error);
         panel_height = DEFAULT_MARQUEE_PANEL_HEIGHT;
      }
   }
   
   return panel_height;
}
