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

#include "launcher-minimap.h"
#include "launcher-behave.h"
#include "launcher-menu.h"
#include "launcher-item.h"

G_DEFINE_TYPE (LauncherMinimap, launcher_minimap, CLUTTER_TYPE_GROUP)

#define LAUNCHER_MINIMAP_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE(obj, \
        LAUNCHER_TYPE_MINIMAP, LauncherMinimapPrivate))

struct _LauncherMinimapPrivate
{
  LauncherMenu *menu;
  ClutterActor *seeker;
  GList *list;

  ClutterTimeline *timeline;
  ClutterAlpha *alpha;
  ClutterBehaviour *behave;

  gint startx;
  gint endx;
};

enum
{
  PROP_0,
  PROP_LIST
};

enum 
{
  NEW_LIST,
  ACTIVE_ITEM_CHANGED,

  LAST_SIGNAL
};

static guint _map_signals[LAST_SIGNAL] = { 0 };

/* Public functions */

/* 
 * We move the pointer to the active item 
 */
void
launcher_minimap_set_active_item (LauncherMinimap *map, LauncherItem *item)
{
  LauncherMinimapPrivate *priv;
  gint x = 0, y = 0;
  GList *list, *l;
  ClutterActor *texture = NULL;

  g_return_if_fail (LAUNCHER_IS_MINIMAP (map));
  priv = map->priv;
  
  list = clutter_container_get_children (CLUTTER_CONTAINER (map));
  for (l = list; l; l = l->next)
  {
    LauncherItem *i = g_object_get_data (G_OBJECT (l->data), "launcher-item");
    if (i == item)
    {
      texture = l->data;
      break;
    }
  }

  /* Find the new position of the seeker */
  clutter_actor_get_abs_position (texture, &x, &y);
  x = clutter_actor_get_x (texture);
  x -= LAUNCHER_MINIMAP_HEIGHT () * 3.25;

  priv->startx = clutter_actor_get_x (priv->seeker);
  priv->endx = x;

  if (clutter_timeline_is_playing (priv->timeline))
  {
    gfloat per = (gfloat)clutter_timeline_get_current_frame (priv->timeline)
                  /clutter_timeline_get_n_frames (priv->timeline);
    if (per > 0.5)
      clutter_timeline_rewind (priv->timeline);
  }
  else
    clutter_timeline_start (priv->timeline);
}

/*
 * Handle events which are in our vacinity
 */
void
launcher_minimap_handle_event (LauncherMinimap *map, ClutterEvent *event)
{
  LauncherMinimapPrivate *priv;
  ClutterActor *stage = clutter_stage_get_default ();
  ClutterActor *icon;
  LauncherItem *item;

  g_return_if_fail (LAUNCHER_IS_MINIMAP (map));
  priv = map->priv;

  if (event->type != CLUTTER_BUTTON_RELEASE)
    return;

  icon = clutter_stage_get_actor_at_pos (CLUTTER_STAGE (stage),
                                         event->button.x,
                                         event->button.y);

  if (!CLUTTER_IS_TEXTURE (icon))
    return;

  item = g_object_get_data (G_OBJECT (icon), "launcher-item");

  if (!LAUNCHER_IS_ITEM (item))
    return;

  g_signal_emit (map, _map_signals[ACTIVE_ITEM_CHANGED], 0, item);
}

void
launcher_minimap_set_list (LauncherMinimap *map, GList *list)
{
  g_return_if_fail (LAUNCHER_IS_MINIMAP (map));
  g_object_set (map, "list", list, NULL);
}

/*
 * This is the mapation function which is called at every iteration of the
 * timeline. It uses the alpha_value variable to calculate a position along
 * the starting x and ending x as a matter of time, and then moves to that
 * position. When alpha_value == CLUTTER_ALPHA_MAX_ALPHA, we are at the end
 * of the timeline, and subsequently reached our destination (priv->endx).
 */
static void
alpha_func (ClutterBehaviour *behave,
            guint32           alpha_value,
            LauncherMinimap *map)
{
  LauncherMinimapPrivate *priv;
  gfloat factor;
  gint newx;
  gint curx;
  gint offset;
 
  g_return_if_fail (LAUNCHER_IS_MINIMAP (map));
  priv = map->priv;

  factor = (gfloat)alpha_value / CLUTTER_ALPHA_MAX_ALPHA;
  
  curx = clutter_actor_get_x (priv->seeker);
  curx = priv->startx;

  if (curx > priv->endx)
  {
    offset = curx - priv->endx;
    offset *= factor;
    
    newx = curx - offset;
  }
  else
  {
    offset = priv->endx - curx;
    offset *= factor;
    
    newx = curx + offset;
  }

  g_object_set (priv->seeker, "x", newx, NULL);

  clutter_actor_queue_redraw (priv->seeker);
}

/*
 * Remove the old items, and create the new ones. Set the new ones object
 * data to be that of the LauncherMenuEntry.
 */
static void
refresh_list (LauncherMinimap *map)
{
  LauncherMinimapPrivate *priv;
  GList *list = NULL, *l;
  gint x = 0;
  gint size;

  size = LAUNCHER_MINIMAP_HEIGHT ();

  g_return_if_fail (LAUNCHER_IS_MINIMAP (map));
  priv = map->priv;

  if (priv->list == NULL)
    return;

  /* First remove old applications */
  list = clutter_container_get_children (CLUTTER_CONTAINER (map));
  for (l = list; l; l = l->next)
  {
    ClutterActor *actor = l->data;

    if (CLUTTER_IS_ACTOR (actor) && actor != priv->seeker)
      clutter_actor_destroy (actor);
  }

  /* Now add the new ones */
  for (l = priv->list; l; l = l->next)
  {
    LauncherItem *item = l->data;
    LauncherMenuApplication *app = launcher_item_get_application (item);
    ClutterActor *texture;
    GdkPixbuf *pixbuf;

    if (!app)
      continue;
    
    texture = clutter_texture_new ();
    pixbuf = launcher_menu_application_get_pixbuf (app, size);
    g_object_set_data (G_OBJECT (texture), "launcher-item", (gpointer)item);

    clutter_texture_set_pixbuf (CLUTTER_TEXTURE (texture), pixbuf, NULL);
    g_object_unref (G_OBJECT (pixbuf));
    
    clutter_container_add_actor (CLUTTER_CONTAINER (map), texture);
    clutter_actor_set_position (texture, x, 0);
    clutter_actor_show (texture);
    
    x += size*1.5;
  }
  x -= size * 0.5;
  clutter_actor_set_size (CLUTTER_ACTOR (map), x, size);

  clutter_actor_set_position (CLUTTER_ACTOR (map),
                              (CLUTTER_STAGE_WIDTH ()/2) - (x/2),
                              CLUTTER_STAGE_HEIGHT () - (size*2));
}

static void
on_menu_changed (LauncherMenu *menu, LauncherMinimap *map)
{
  LauncherMinimapPrivate *priv;

  g_return_if_fail (LAUNCHER_IS_MINIMAP (map));
  priv = map->priv;

  launcher_minimap_set_list (map, launcher_menu_get_applications (menu));
}

/* GObject functions */
static void
launcher_minimap_get_property (GObject    *object,
                                        guint       prop_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  LauncherMinimapPrivate *priv;

  g_return_if_fail (LAUNCHER_IS_MINIMAP (object));
  priv = LAUNCHER_MINIMAP (object)->priv;

  switch (prop_id)
  {
    case PROP_LIST:
      g_value_set_pointer (value, priv->list);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
launcher_minimap_set_property (GObject      *object,
                                        guint         prop_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  LauncherMinimapPrivate *priv;

  g_return_if_fail (LAUNCHER_IS_MINIMAP (object));
  priv = LAUNCHER_MINIMAP (object)->priv;

  switch (prop_id)
  {
    case PROP_LIST:
      priv->list = g_value_get_pointer (value);
      if (priv->list)
        refresh_list (LAUNCHER_MINIMAP (object));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}


static void
launcher_minimap_dispose (GObject *object)
{
  G_OBJECT_CLASS (launcher_minimap_parent_class)->dispose (object);
}

static void
launcher_minimap_finalize (GObject *map)
{
  LauncherMinimapPrivate *priv;
  
  g_return_if_fail (LAUNCHER_IS_MINIMAP (map));
  priv = LAUNCHER_MINIMAP (map)->priv;


  
  G_OBJECT_CLASS (launcher_minimap_parent_class)->finalize (map);
}


static void
launcher_minimap_class_init (LauncherMinimapClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class->finalize = launcher_minimap_finalize;
  obj_class->dispose = launcher_minimap_dispose; 
  obj_class->get_property = launcher_minimap_get_property;
  obj_class->set_property = launcher_minimap_set_property;

  /* Class properties */
  g_object_class_install_property (obj_class,
    PROP_LIST,
    g_param_spec_pointer ("list",
      "List",
      "A GList of applications",
      G_PARAM_CONSTRUCT|G_PARAM_READWRITE));

  /* Class signals */
  _map_signals[NEW_LIST] = 
    g_signal_new ("new-list",
                  G_OBJECT_CLASS_TYPE (obj_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LauncherMinimapClass, new_list),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__POINTER,
                  G_TYPE_NONE, 0);
                  
  _map_signals[ACTIVE_ITEM_CHANGED] = 
    g_signal_new ("active-item-changed",
                  G_OBJECT_CLASS_TYPE (obj_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LauncherMinimapClass, active_item),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1, LAUNCHER_TYPE_ITEM);
   g_type_class_add_private (obj_class, sizeof (LauncherMinimapPrivate));

}

static void
launcher_minimap_init (LauncherMinimap *map)
{
  LauncherMinimapPrivate *priv;
  GdkPixbuf *pixbuf = NULL;
  gint width;
  
  priv = map->priv = LAUNCHER_MINIMAP_GET_PRIVATE (map);

  priv->menu = launcher_menu_get_default ();
  
  g_signal_connect (priv->menu, "menu-changed",
                    G_CALLBACK (on_menu_changed), map);

  priv->startx = CLUTTER_STAGE_WIDTH ();
  priv->endx = CLUTTER_STAGE_WIDTH ()/2;

  /* Create the 'seeker' texture. This stays behind the other icons, and gives
   * and idication to where the user is in the list of applications */
  width = (LAUNCHER_MINIMAP_HEIGHT () * 7.5);
  pixbuf = gdk_pixbuf_new_from_file_at_scale (PKGDATADIR "/hilight-long.svg",
                                             width, 
                                             LAUNCHER_MINIMAP_HEIGHT ()*1.5,
                                             FALSE,
                                             NULL);
                                              
  priv->seeker = clutter_texture_new_from_pixbuf (pixbuf);
  g_object_unref (pixbuf);
  clutter_container_add_actor (CLUTTER_CONTAINER (map),
                               priv->seeker);
  clutter_actor_set_position (priv->seeker, 100,
        0 - (LAUNCHER_MINIMAP_HEIGHT ()*0.25));
  //clutter_actor_set_size (priv->seeker, width, width);
  clutter_actor_show (priv->seeker);

  priv->timeline = clutter_timeline_new (30, 60);
  priv->alpha = clutter_alpha_new_full (priv->timeline,
                                        clutter_ramp_inc_func,
                                        NULL, NULL);
  priv->behave = launcher_behave_new (priv->alpha, 
                                      (LauncherBehaveAlphaFunc)alpha_func,
                                      (gpointer)map);
}

ClutterActor*
launcher_minimap_new (void)
{
  ClutterActor *map;

  map = g_object_new (LAUNCHER_TYPE_MINIMAP,
                       NULL);
  return map;
}

