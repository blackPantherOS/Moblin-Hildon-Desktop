/*
 * Copyright (C) 2007 Intel
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Authored by Neil Jagdish Patel <njp@o-hand.com>
 *
 */

#include <glib.h>
#include <gdk/gdkx.h>
#include <gdk/gdk.h>

#include <stdio.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define SN_API_NOT_YET_FROZEN 1
#include <libsn/sn.h>

#include "launcher-startup.h"

#include "launcher-item.h"
#include "launcher-menu.h"
#include "launcher-util.h"

G_DEFINE_TYPE (LauncherStartup, launcher_startup, G_TYPE_OBJECT)

#define LAUNCHER_STARTUP_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE(obj, \
        LAUNCHER_TYPE_STARTUP, LauncherStartupPrivate))

#define TIMEOUT 3000 /* Miliseconds */
#define SN_TIMEOUT 10000

struct _LauncherStartupPrivate
{
  GdkWindow *root_window;
  SnDisplay *sn_display;
  Display *xdisplay;
  
  LauncherItem *active;
  gchar **argv;
  guint tag;
};

/*
 * Public functions
 */

/*
 * Load settings from the desktop file, this is important for the launch
 * sequence.
 */
static void
load_settings (LauncherItem  *item, 
               gboolean      *sn_enable,
               gboolean      *single_instance,
               gchar         ***argv)
{
  LauncherMenuApplication *app;
  GKeyFile *key_file;
  GError *error = NULL;
  const gchar *filename;
  const gchar *exec;

  g_return_if_fail (LAUNCHER_IS_ITEM (item));

  app = launcher_item_get_application (item);

  /* Get the exec string */
  exec = launcher_menu_application_get_exec (app);

  if (exec == NULL)
    return;

  /* Create an argv from the string */
  *argv = launcher_util_exec_to_argv (exec);


  key_file = g_key_file_new ();
  filename = launcher_menu_application_get_desktop_filename (app);

  if (!g_key_file_load_from_file (key_file, filename, G_KEY_FILE_NONE, &error))
  {
    g_key_file_free (key_file);
    g_warning ("Unable to load Desktop File values: %s", error->message);
    g_error_free (error);
    return;
  }

  *single_instance = g_key_file_get_boolean (key_file, "Desktop Entry", 
                                                  "SingleInstance", NULL);
  *sn_enable = g_key_file_get_boolean (key_file, "Desktop Entry", 
                                            "StartupNotify", NULL);

  g_key_file_free (key_file);
}

static void
child_setup (gpointer user_data)
{
  if (user_data)
    sn_launcher_context_setup_child_process (user_data);
}

static gboolean
sn_timeout (LauncherStartup *startup)
{
  LauncherStartupPrivate *priv;

  g_return_val_if_fail (LAUNCHER_IS_STARTUP (startup), FALSE);
  priv = startup->priv;

  if (!LAUNCHER_IS_ITEM (priv->active))
    return FALSE;

  launcher_item_launch_completed (priv->active);
  g_strfreev (priv->argv);
  priv->argv = NULL;
  priv->active = NULL;
  
  return FALSE;
}

gboolean
launcher_startup_launch_item (LauncherStartup *startup, LauncherItem *item)
{
  LauncherStartupPrivate *priv;
  LauncherMenuApplication *app;
  GError *error = NULL;
  SnLauncherContext *context = NULL;
  gboolean sn_enable = FALSE;;
  gboolean single_instance = FALSE;;


  g_return_val_if_fail (LAUNCHER_IS_STARTUP (startup), FALSE);
  priv = startup->priv;
  
  priv->active = item;
  app = launcher_item_get_application (item);

  /* Load the important settings from the desktop file if not already done */
  load_settings (item, &sn_enable, &single_instance, &priv->argv);

  if (sn_enable)
  {
    SnDisplay *sn_dpy;
    Display *display;
    int screen;

    display = priv->xdisplay;
    sn_dpy = priv->sn_display;

    screen = gdk_screen_get_number (gdk_screen_get_default ());
    
    context = sn_launcher_context_new (sn_dpy, screen);

    sn_launcher_context_set_name (context,
                                launcher_menu_application_get_name (app));
    sn_launcher_context_set_binary_name (context, priv->argv[0]);

    sn_launcher_context_initiate (context,
                                  g_get_prgname () ?: "unknown",
                                  priv->argv[0],
                                  CLUTTER_CURRENT_TIME);
  }

  /* Execute the program */
  if (!gdk_spawn_on_screen (gdk_screen_get_default (),
                            NULL, priv->argv, NULL,
                            G_SPAWN_SEARCH_PATH,
                            child_setup,
                            context,
                            NULL,
                            &error))
  {
    g_warning ("Cannot launch %s: %s", priv->argv[0], error->message);
    g_error_free (error);
    if (context)
      sn_launcher_context_complete (context);

    priv->active = NULL;
    g_strfreev (priv->argv);
    return FALSE;
  }
  
  /* 
   * Let the program know that the launch has started, Sn apps get a longer
   * timeout
   */
  if (sn_enable)
    priv->tag = g_timeout_add (SN_TIMEOUT, (GSourceFunc)sn_timeout, startup);
  else
    priv->tag = g_timeout_add (TIMEOUT, (GSourceFunc)sn_timeout, startup);

  return TRUE;
}


/* 
 * Startup monitoring functions 
 */

static void
monitor_event_func (SnMonitorEvent *event, LauncherStartup *startup)
{
  LauncherStartupPrivate *priv;
  SnStartupSequence *seq;
  const gchar *b_name;

  g_return_if_fail (LAUNCHER_IS_STARTUP (startup));
  priv = startup->priv;

  if (sn_monitor_event_get_type (event) != SN_MONITOR_EVENT_COMPLETED)
    return;
  
  if (!LAUNCHER_IS_ITEM (priv->active) || priv->argv == NULL)
    return;

  seq = sn_monitor_event_get_startup_sequence (event);
  b_name = sn_startup_sequence_get_binary_name (seq);
  
  if (b_name == NULL)
    return;

  if (strcmp (priv->argv[0], b_name) == 0)
  {
    g_source_remove (priv->tag);

    /* 
     * Give some time for the main animation, otherwise apps that start too
     * fast mess up the animation
     */
    priv->tag = g_timeout_add (1000, (GSourceFunc)sn_timeout, startup);
  }
}

static GdkFilterReturn
filter_func (GdkXEvent        *gdk_xevent,
             GdkEvent         *event,
             LauncherStartup *startup)
{
  XEvent *xevent;
  xevent = (XEvent *) gdk_xevent;
  gboolean ret;

  g_return_val_if_fail (LAUNCHER_IS_STARTUP (startup), GDK_FILTER_CONTINUE);

  ret = sn_display_process_event (startup->priv->sn_display, xevent);

  return GDK_FILTER_CONTINUE;
}

/* GObject functions */
static void
launcher_startup_dispose (GObject *object)
{
  G_OBJECT_CLASS (launcher_startup_parent_class)->dispose (object);
}

static void
launcher_startup_finalize (GObject *startup)
{
  LauncherStartupPrivate *priv;
  
  g_return_if_fail (LAUNCHER_IS_STARTUP (startup));
  priv = LAUNCHER_STARTUP (startup)->priv;


  G_OBJECT_CLASS (launcher_startup_parent_class)->finalize (startup);
}


static void
launcher_startup_class_init (LauncherStartupClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class->finalize = launcher_startup_finalize;
  obj_class->dispose = launcher_startup_dispose;

  g_type_class_add_private (obj_class, sizeof (LauncherStartupPrivate)); 
}

static void
launcher_startup_init (LauncherStartup *startup)
{
  LauncherStartupPrivate *priv;
  SnMonitorContext *context;
  
  priv = startup->priv = LAUNCHER_STARTUP_GET_PRIVATE (startup);

  priv->active = NULL;
  priv->argv = NULL;
  priv->xdisplay = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());
  priv->sn_display = sn_display_new (priv->xdisplay, NULL, NULL);
  
  context = sn_monitor_context_new (priv->sn_display,
                                    DefaultScreen (priv->xdisplay),
                                    (SnMonitorEventFunc)monitor_event_func,
                                    (gpointer)startup,
                                    NULL);

  /*
   * We have to select for property events on at least one root window (but
   * not all and INITIATE messages go to all root windows 
   */
  XSelectInput (priv->xdisplay, 
                DefaultRootWindow (priv->xdisplay), PropertyChangeMask);
  
  priv->root_window = gdk_window_lookup_for_display (
                            gdk_x11_lookup_xdisplay (priv->xdisplay), 0);

  gdk_window_add_filter (priv->root_window, 
                         (GdkFilterFunc)filter_func,
                         (gpointer)startup);
}

LauncherStartup*
launcher_startup_get_default (void)
{
  static LauncherStartup *startup = NULL;
  
  if (startup == NULL)
    startup = g_object_new (LAUNCHER_TYPE_STARTUP, 
                            NULL);

  return startup;
}
