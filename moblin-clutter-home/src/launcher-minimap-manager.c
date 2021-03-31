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

#include <stdio.h>
#include <string.h>
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>

#include "launcher-minimap-manager.h"

#include "launcher-catmap.h"
#include "launcher-minimap.h"
#include "launcher-menu.h"
#include "launcher-item.h"

G_DEFINE_TYPE (LauncherMinimapManager, launcher_minimap_manager, G_TYPE_OBJECT)

/* GConf keys */
#define MAP_DIR   "/apps/desktop-launcher"
#define MAP_MODE MAP_DIR "/minimap_mode" /* Int matching the enum below */

#define LAUNCHER_MINIMAP_MANAGER_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE(obj, \
        LAUNCHER_TYPE_MINIMAP_MANAGER, LauncherMinimapManagerPrivate))

struct _LauncherMinimapManagerPrivate
{
  LauncherMenu *menu;
  ClutterActor *iconmap;
  ClutterActor *catmap;
  GList *list;

  gint active_map;

  ClutterEffectTemplate *template;
  ClutterTimeline *show_time;

  ClutterTimeline *timeline;
  ClutterAlpha *alpha;
  ClutterBehaviour *behave;
};

enum
{
  ICON_MAP = 0,
  CAT_MAP,

  N_MAPS
};

enum
{
  PROP_0,
  PROP_LIST
};

enum 
{
  LIST_CHANGED,
  ACTIVE_ITEM_CHANGED,

  LAST_SIGNAL
};

static guint _map_signals[LAST_SIGNAL] = { 0 };

/* Public functions */

/* 
 * We move the pointer to the active item 
 */
void
launcher_minimap_manager_set_active_item (LauncherMinimapManager *map, 
                                          LauncherItem *item)
{
 	LauncherMinimapManagerPrivate *priv;

  g_return_if_fail (LAUNCHER_IS_MINIMAP_MANAGER (map));
  priv = map->priv;

  launcher_minimap_set_active_item (LAUNCHER_MINIMAP (priv->iconmap), item);
}

/*
 * Handle events which are in our vacinity
 */
void
launcher_minimap_manager_handle_event (LauncherMinimapManager *map, 
                                       ClutterEvent *event)
{
  LauncherMinimapManagerPrivate *priv;

  g_return_if_fail (LAUNCHER_IS_MINIMAP_MANAGER (map));
  priv = map->priv;
  
  if (priv->active_map == ICON_MAP)
    launcher_minimap_handle_event (LAUNCHER_MINIMAP (priv->iconmap), event);
  else
    launcher_catmap_handle_event (LAUNCHER_CATMAP (priv->catmap), event);
}

void
launcher_minimap_manager_set_list (LauncherMinimapManager *map, GList *list)
{
  g_return_if_fail (LAUNCHER_IS_MINIMAP_MANAGER (map));
  g_object_set (map->priv->iconmap, "list", list, NULL);
}

/* 
 * Hide the current map while launching an application 
 */
void
launcher_minimap_manager_show (LauncherMinimapManager *map, gboolean show)
{
  LauncherMinimapManagerPrivate *priv;
  static ClutterKnot knots[2];
  ClutterActor *active = NULL;

  g_return_if_fail (LAUNCHER_IS_MINIMAP_MANAGER (map));
  priv = map->priv;

  /* Find the active map */
  if (priv->active_map == CAT_MAP)
    active = priv->catmap;
  else
    active = priv->iconmap;

  /* Set the knot values */
  if (!show)
  {
    knots[0].x = clutter_actor_get_x (active);
    knots[0].y = clutter_actor_get_y (active);
    knots[1].x = clutter_actor_get_x (active);
    knots[1].y = CLUTTER_STAGE_HEIGHT ()+LAUNCHER_MINIMAP_HEIGHT ();
  }
  else
  {
    knots[0].x = clutter_actor_get_x (active);
    knots[0].y = clutter_actor_get_y (active);
    knots[1].x = clutter_actor_get_x (active);
    knots[1].y = CLUTTER_STAGE_HEIGHT () - (LAUNCHER_MINIMAP_HEIGHT ()*2);
  }

  /* Create and apply the effect */
  clutter_effect_move (priv->template,
                       active,
                       knots,
                       2,
                       NULL, NULL);
  clutter_timeline_start (priv->show_time);
}

/* Callbacks */
static void 
on_active_item_selected (LauncherMinimap        *iconmap,
                         LauncherItem           *item,
                         LauncherMinimapManager *map)
{
  g_signal_emit (map, _map_signals[ACTIVE_ITEM_CHANGED], 0, item);
}

static void
on_list_changed (LauncherCatmap         *catmap, 
                 GList                  **list,
                 LauncherMinimapManager *map)
{
  g_return_if_fail (LAUNCHER_IS_MINIMAP_MANAGER (map));
  if (map->priv->active_map == CAT_MAP)
  {
    g_signal_emit (map, _map_signals[LIST_CHANGED], 0, list);
  }
}

static void
on_map_mode_changed (GConfClient            *client,
                     guint                   cid,
                     GConfEntry             *entry,
                     LauncherMinimapManager *map)
{
  LauncherMinimapManagerPrivate *priv;
  GConfValue *value = NULL;

  g_return_if_fail (LAUNCHER_IS_MINIMAP_MANAGER (map));
  priv = map->priv;

  value = gconf_entry_get_value (entry);
  
  priv->active_map = gconf_value_get_int (value);

  if (priv->active_map >= N_MAPS)
    priv->active_map = 0;

  if (priv->active_map == ICON_MAP)
  {
    clutter_actor_hide_all (priv->catmap);
    clutter_actor_show_all (priv->iconmap);
  }
  else if (priv->active_map == CAT_MAP)
  {
    clutter_actor_hide_all (priv->iconmap);
    clutter_actor_show_all (priv->catmap);
  }
}

static gboolean
change_list (LauncherMinimapManager *map)
{
  GList *apps;
  LauncherMenu *menu = launcher_menu_get_default ();  
  
  if (map->priv->active_map == ICON_MAP)
  {
    apps = launcher_menu_get_applications (menu);
  }
  else
  {
    GList *cat_list = launcher_menu_get_categories (menu);
    apps = launcher_menu_category_get_applications (cat_list->data);
  }
  g_signal_emit (map, _map_signals[LIST_CHANGED], 0, &apps);

  return FALSE;
}

static void
on_menu_changed (LauncherMenu *menu, LauncherMinimapManager *map)
{
  LauncherMinimapManagerPrivate *priv;

  g_return_if_fail (LAUNCHER_IS_MINIMAP_MANAGER (map));
  priv = map->priv;

  if (priv->active_map == ICON_MAP)
  {
    GList *apps = launcher_menu_get_applications (menu);
    g_signal_emit (map, _map_signals[LIST_CHANGED], 0, &apps);
  }
}

/* GObject functions */
static void
launcher_minimap_manager_dispose (GObject *object)
{
  G_OBJECT_CLASS (launcher_minimap_manager_parent_class)->dispose (object);
}

static void
launcher_minimap_manager_finalize (GObject *map)
{
  LauncherMinimapManagerPrivate *priv;
  
  g_return_if_fail (LAUNCHER_IS_MINIMAP_MANAGER (map));
  priv = LAUNCHER_MINIMAP_MANAGER (map)->priv;
  
  G_OBJECT_CLASS (launcher_minimap_manager_parent_class)->finalize (map);
}


static void
launcher_minimap_manager_class_init (LauncherMinimapManagerClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class->finalize = launcher_minimap_manager_finalize;
  obj_class->dispose = launcher_minimap_manager_dispose; 

  /* Class signals */
  _map_signals[LIST_CHANGED] = 
    g_signal_new ("list-changed",
                  G_OBJECT_CLASS_TYPE (obj_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LauncherMinimapManagerClass, new_list),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1, G_TYPE_POINTER);
                  
  _map_signals[ACTIVE_ITEM_CHANGED] = 
    g_signal_new ("active-item-changed",
                  G_OBJECT_CLASS_TYPE (obj_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LauncherMinimapManagerClass, active_item),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1, LAUNCHER_TYPE_ITEM);
   g_type_class_add_private (obj_class, sizeof (LauncherMinimapManagerPrivate));

}

static void
launcher_minimap_manager_init (LauncherMinimapManager *map)
{
  LauncherMinimapManagerPrivate *priv;
  ClutterActor *stage = clutter_stage_get_default ();
  GConfClient *client = gconf_client_get_default ();
    
  priv = map->priv = LAUNCHER_MINIMAP_MANAGER_GET_PRIVATE (map);

  priv->active_map = 0;

  /* Connect to menu-changed signals */
  g_signal_connect (launcher_menu_get_default (), "menu-changed",
                    G_CALLBACK (on_menu_changed), map);

  /* Map of icons for quick navigation */
  priv->iconmap = launcher_minimap_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), priv->iconmap);
  g_signal_connect (priv->iconmap, "active-item-changed",
                    G_CALLBACK (on_active_item_selected), (gpointer)map);
  clutter_actor_show_all (priv->iconmap);

  /* Map of categories */
  priv->catmap = launcher_catmap_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), priv->catmap);
  g_signal_connect (priv->catmap, "list-changed",
                    G_CALLBACK (on_list_changed), (gpointer)map);

  /* Init the show/hide effect */
  priv->show_time = clutter_timeline_new (40, 40);
  priv->template = clutter_effect_template_new (priv->show_time,
                                                clutter_sine_inc_func);
	
  /*
   * Load the settings from gconf
   */
  gconf_client_add_dir (client, MAP_DIR, GCONF_CLIENT_PRELOAD_NONE, NULL);

  priv->active_map = gconf_client_get_int (client, MAP_MODE, NULL);
  gconf_client_notify_add (client, MAP_MODE,
                           (GConfClientNotifyFunc)on_map_mode_changed,
                           (gpointer)map,
                           NULL, NULL);
  if (priv->active_map == ICON_MAP)
  {
    clutter_actor_hide (priv->catmap);
    clutter_actor_show_all (priv->iconmap);
  }
  else if (priv->active_map == CAT_MAP)
  {
    clutter_actor_hide (priv->iconmap);
    clutter_actor_show_all (priv->catmap);

  }
  g_timeout_add (200, (GSourceFunc)change_list, map);
}

LauncherMinimapManager*
launcher_minimap_manager_new (void)
{
  return g_object_new (LAUNCHER_TYPE_MINIMAP_MANAGER,
                       NULL);
}
