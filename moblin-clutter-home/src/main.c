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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include <gdk/gdkx.h>

#include <clutter/clutter.h>
#include <clutter/clutter-glx.h>

#include "launcher-animation.h"
#include "launcher-animation-linear.h"
#include "launcher-background.h"
#include "launcher-menu.h"
#include "launcher-minimap-manager.h"
#include "launcher-minimap.h"
#include "launcher-startup.h"

typedef struct
{
  LauncherMenu           *menu;
  LauncherAnimation      *anim;
  LauncherMinimapManager *minimap;

} LauncherApp;

static void on_active_item_selected (LauncherMinimapManager *map,
                                     LauncherItem    *item,
                                     LauncherApp     *app);
static void on_list_changed  (LauncherMinimapManager *map,
                                     GList           **list,
                                     LauncherApp     *app);
static void on_launch_started (LauncherAnimation *anim,
                               LauncherItem *item,
                               LauncherApp *app);
static void on_launch_finished (LauncherAnimation *anim,
                                LauncherItem *item,
                                LauncherApp *app);
static void on_stage_event (ClutterStage      *stage, 
                            ClutterEvent      *event,
                            LauncherApp       *app);
static void on_new_active_item (LauncherAnimation *anim,
                                LauncherItem      *item,
                                LauncherApp       *app);
static void set_hints (ClutterStage *stage);

/* Command line options */
static gboolean   windowed = FALSE;

static GOptionEntry entries[] =
{
  {
    "windowed",
    'w', 0,
    G_OPTION_ARG_NONE,
    &windowed,
    "Launch in windowed mode (for testing)",
    NULL
  },
  {
    NULL
  }
};
 
int
main (int argc, gchar *argv[])
{
  LauncherApp *app = g_new0 (LauncherApp, 1);
  ClutterActor *stage, *bg;
  GError *error = NULL;
  ClutterColor black = { 0x00, 0x00, 0x00, 0xff };
  LauncherMenu *menu;
  LauncherAnimation *anim;
  LauncherMinimapManager *map;
  LauncherStartup *startup;

  g_thread_init (NULL);

  g_set_application_name ("Desktop Launcher");
  
  gtk_init (&argc, &argv);
  clutter_init_with_args (&argc, &argv,
                          " - Desktop Launcher", entries,
                          NULL,
                          &error);
  if (error)
  {
      g_print ("Unable to run Desktop Launcher: %s", error->message);
      g_error_free (error);
      return EXIT_FAILURE;
  }

  stage = clutter_stage_get_default ();
  clutter_actor_set_size (stage, 800, 480);
  /*clutter_stage_hide_cursor (CLUTTER_STAGE (stage));*/
  
  if (!windowed)
  {
    set_hints (CLUTTER_STAGE (stage));
    clutter_stage_fullscreen (CLUTTER_STAGE (stage));
  }

  clutter_stage_set_color (CLUTTER_STAGE (stage), &black);

  /* The desktop background */
  bg = launcher_background_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), bg);
  clutter_actor_set_size (bg, CLUTTER_STAGE_WIDTH (), CLUTTER_STAGE_HEIGHT ());
  clutter_actor_set_position (bg, 0, 0);
  clutter_actor_show (bg);

  menu = app->menu = launcher_menu_get_default ();
  startup = launcher_startup_get_default ();
  anim = app->anim = launcher_animation_linear_new ();
  g_signal_connect (anim, "active-item-changed",
                    G_CALLBACK (on_new_active_item), (gpointer)app);
  g_signal_connect (anim, "launch-started",
                    G_CALLBACK (on_launch_started), (gpointer)app);
  g_signal_connect (anim, "launch-finished",
                    G_CALLBACK (on_launch_finished), (gpointer)app); 
  
  /* 
   * Start the minimap manger which swaps between the iconmap and category maps
   * depending on the current gconf settings
   */
  map = app->minimap = launcher_minimap_manager_new ();
  launcher_minimap_manager_set_list (map,
                             launcher_menu_get_applications (menu));
  g_signal_connect (map, "active-item-changed",
                    G_CALLBACK (on_active_item_selected), (gpointer)app);
  g_signal_connect (map, "list-changed",
                    G_CALLBACK (on_list_changed), (gpointer)app);

  g_signal_connect (stage, "event",
                    G_CALLBACK (on_stage_event), (gpointer)app);

  
  clutter_actor_show (CLUTTER_ACTOR (stage));
  clutter_main ();

  g_object_unref (menu);
  
  return EXIT_SUCCESS;
}

/*
 * Retrieve the XWindow the stage is rendering to, and set the 'desktop' hint
 * onto it, so it will be treated as the user's desktop.
 */
static void
set_hints (ClutterStage *stage)
{
  Window stage_win;
  Atom atom;
  GdkDisplay *display = gdk_display_get_default ();
  
  stage_win = clutter_glx_get_stage_window (stage);
  atom = gdk_x11_get_xatom_by_name_for_display (display, 
                                                "_NET_WM_WINDOW_TYPE_DESKTOP");
  
  XChangeProperty (GDK_DISPLAY_XDISPLAY (display), stage_win,
                   gdk_x11_get_xatom_by_name_for_display (display,
                                                       "_NET_WM_WINDOW_TYPE"),
                   XA_ATOM, 32, PropModeReplace,
                   (guchar *)&atom, 1);

}

/*
 * Map events to the correct place depending on their position.
 */
static void 
on_stage_event (ClutterStage      *stage, 
                ClutterEvent      *event,
                LauncherApp       *app)
{
  static gboolean map_event = FALSE;
  gint minimap_y = CLUTTER_STAGE_HEIGHT () - (LAUNCHER_MINIMAP_HEIGHT ()*2);

  switch (event->type)
  {
    case CLUTTER_BUTTON_PRESS:
      if (event->button.y > minimap_y)
      {
        map_event = TRUE;
        launcher_minimap_manager_handle_event (app->minimap, event);
      }
      else
      {
        map_event = FALSE;
        launcher_animation_handle_event (app->anim, event);
      }
      return;
      break;
    case CLUTTER_BUTTON_RELEASE:
      if (map_event)
        launcher_minimap_manager_handle_event (app->minimap, event);
      else
        launcher_animation_handle_event (app->anim, event);
      return;
    case CLUTTER_MOTION:
      if (map_event)
        launcher_minimap_manager_handle_event (app->minimap, event);
      else
        launcher_animation_handle_event (app->anim, event);
      return;
    default:
      launcher_animation_handle_event (app->anim, event);
  }
}

/*
 * When the user selects an item, we need to tell the animation to move to that
 * animation.
 */
static void 
on_active_item_selected (LauncherMinimapManager *map,
                         LauncherItem    *item,
                         LauncherApp     *app)
{
  launcher_animation_set_active_item (app->anim, item);
}

/*
 * Update the animation with a new set of apps
 */
static void 
on_list_changed (LauncherMinimapManager  *map,
                 GList                  **list,
                 LauncherApp             *app)
{
  launcher_animation_set_list (app->anim, *list);
}

/*
 * As the user manipulates the animation, we need to let the minimap-manger
 * know that the 'active' item has changed, so the iconmap can keep up.
 */
static void 
on_new_active_item (LauncherAnimation *anim,
                    LauncherItem      *item,
                    LauncherApp       *app)
{
  launcher_minimap_manager_set_active_item (app->minimap, item);
}

/*
 * Hide the current map when a launch is started.
 */
static void on_launch_started (LauncherAnimation *anim,
                               LauncherItem *item,
                               LauncherApp *app)
{
  launcher_minimap_manager_show (app->minimap, FALSE);
}

/*
 * Show the current map once a launch has completed 
 */
static void on_launch_finished (LauncherAnimation *anim,
                                LauncherItem *item,
                                LauncherApp *app)
{
  launcher_minimap_manager_show (app->minimap, TRUE);
}
