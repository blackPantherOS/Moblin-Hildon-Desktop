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

#include "launcher-catmap.h"
#include "launcher-behave.h"
#include "launcher-menu.h"
#include "launcher-item.h"

G_DEFINE_TYPE (LauncherCatmap, launcher_catmap, CLUTTER_TYPE_HBOX)

#define LAUNCHER_CATMAP_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE(obj, \
        LAUNCHER_TYPE_CATMAP, LauncherCatmapPrivate))

struct _LauncherCatmapPrivate
{
  LauncherMenu *menu;
  ClutterActor *seeker;
  ClutterActor *active;
  GList *list;

  ClutterTimeline *timeline;
  ClutterAlpha *alpha;
  ClutterBehaviour *behave;

  gint startx;
  gint endx;

  ClutterActor *popup;
  ClutterActor *popup_texture;
  ClutterActor *popup_label;
  
  ClutterTimeline *pop_show;
  ClutterEffectTemplate *pop_show_temp;
};

enum
{
  PROP_0,
  PROP_LIST
};

enum 
{
  LIST_CHANGED,

  LAST_SIGNAL
};

static guint _map_signals[LAST_SIGNAL] = { 0 };

static void
show_popup (LauncherCatmap *map)
{
  LauncherCatmapPrivate *priv;

  priv = map->priv;

  if (clutter_timeline_is_playing (priv->pop_show))
    clutter_timeline_stop (priv->pop_show);

  if (clutter_actor_get_opacity (priv->popup) == 255)
    return;

  g_object_unref (priv->pop_show);
  
  priv->pop_show = clutter_effect_fade (priv->pop_show_temp,
                       priv->popup,
                       clutter_actor_get_opacity (priv->popup),
                       255,
                       NULL, NULL);
  clutter_timeline_start (priv->pop_show);
}

static void
hide_popup (LauncherCatmap *map)
{
  LauncherCatmapPrivate *priv;

  priv = map->priv;

  if (clutter_timeline_is_playing (priv->pop_show))
    clutter_timeline_stop (priv->pop_show);

  if (clutter_actor_get_opacity (priv->popup) == 0)
    return; 

  g_object_unref (priv->pop_show);
  
  priv->pop_show = clutter_effect_fade (priv->pop_show_temp,
                       priv->popup,
                       clutter_actor_get_opacity (priv->popup),
                       0,
                       NULL, NULL);
  clutter_timeline_start (priv->pop_show);
}


static void
update_popup (LauncherCatmap *map, gint x, gint y)
{
  LauncherCatmapPrivate *priv;
  ClutterActor *stage = clutter_stage_get_default ();
  ClutterActor *icon = NULL;
  LauncherMenuCategory *cat;

  g_return_if_fail (LAUNCHER_IS_CATMAP (map));
  priv = map->priv;

  icon = clutter_stage_get_actor_at_pos (CLUTTER_STAGE (stage),
                                         x,
                                         y);

  if (!CLUTTER_IS_TEXTURE (icon))
  {
    hide_popup (map);
    return;
  }
  
  if (icon == priv->seeker)
    icon = priv->active;

  /* Let the application know that the user has picked a new category */
  cat = g_object_get_data (G_OBJECT (icon), "category");
  if (!cat)
  {
    hide_popup (map);
    return;
  }

  clutter_label_set_text (CLUTTER_LABEL (priv->popup_label), 
                          launcher_menu_category_get_name (cat));
  g_object_set (priv->popup_label, "x",
                -1*(clutter_actor_get_width (priv->popup_label)/2), NULL);
  
  if (clutter_actor_get_opacity (priv->popup) != 255)
    show_popup (map);
}

/*
 * Handle events which are in our vacinity
 */
void
launcher_catmap_handle_event (LauncherCatmap *map, ClutterEvent *event)
{
  LauncherCatmapPrivate *priv;
  ClutterActor *stage = clutter_stage_get_default ();
  ClutterActor *icon;
  LauncherMenuCategory *cat;
  GList *list = NULL;
  gint x=0;
  static gboolean hover = FALSE;

  g_return_if_fail (LAUNCHER_IS_CATMAP (map));
  priv = map->priv;

  if (event->type == CLUTTER_BUTTON_PRESS)
  {
    hover = TRUE;
    clutter_actor_set_position (priv->popup, event->button.x,
      event->button.y-(clutter_actor_get_height (priv->popup_texture)*1.3));
    
    update_popup (map, event->button.x, event->button.y);   
    
    return;
  }
  else if (event->type == CLUTTER_MOTION)
  {
    if (!hover)
      return;
    
    clutter_actor_set_position (priv->popup, event->motion.x,
        event->motion.y-(clutter_actor_get_height (priv->popup_texture)*1.3));

    update_popup (map, event->motion.x, event->motion.y);
    return;
  }
  else if (event->type != CLUTTER_BUTTON_RELEASE)
    return;

  hover = FALSE;
  hide_popup (map);

  icon = clutter_stage_get_actor_at_pos (CLUTTER_STAGE (stage),
                                         event->button.x,
                                         event->button.y);

  if (!CLUTTER_IS_TEXTURE (icon))
    return;

  priv->active = icon;
  
  /* Let the application know that the user has picked a new category */
  cat = g_object_get_data (G_OBJECT (icon), "category");
  if (!cat)
    return;

  list = launcher_menu_category_get_applications (cat);
  g_signal_emit (map, _map_signals[LIST_CHANGED], 0, &list);  
  
  /* Find the new position of the seeker */
  x = clutter_actor_get_x (icon);
  x -= LAUNCHER_CATMAP_HEIGHT () * 0.25;

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
 * This is the mapation function which is called at every iteration of the
 * timeline. It uses the alpha_value variable to calculate a position along
 * the starting x and ending x as a matter of time, and then moves to that
 * position. When alpha_value == CLUTTER_ALPHA_MAX_ALPHA, we are at the end
 * of the timeline, and subsequently reached our destination (priv->endx).
 */
static void
alpha_func (ClutterBehaviour *behave,
            guint32           alpha_value,
            LauncherCatmap *map)
{
  LauncherCatmapPrivate *priv;
  gfloat factor;
  gint newx;
  gint curx;
  gint offset;
 
  g_return_if_fail (LAUNCHER_IS_CATMAP (map));
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
refresh_list (LauncherCatmap *map)
{
  LauncherCatmapPrivate *priv;
  GList *list = NULL, *l;
  ClutterActor *first = NULL;
  gint x = 0;
  gint size;

  size = LAUNCHER_CATMAP_HEIGHT ();

  g_return_if_fail (LAUNCHER_IS_CATMAP (map));
  priv = map->priv;

  if (priv->list == NULL)
    return;

  /* First remove old applications */
  list = clutter_container_get_children (CLUTTER_CONTAINER (map));
  for (l = list; l; l = l->next)
  {
    ClutterActor *actor = l->data;

    if (CLUTTER_IS_ACTOR (actor) && actor != priv->seeker)
    {
      clutter_actor_hide (actor);
      g_object_set_data (G_OBJECT (actor), "category", NULL);
      clutter_container_remove_actor (CLUTTER_CONTAINER (map), actor);

      if (CLUTTER_IS_ACTOR (actor))
        g_object_unref (actor);
    }
  }

  /* Now add the new ones */
  for (l = priv->list; l; l = l->next)
  {
    LauncherMenuCategory *cat = l->data;
    ClutterActor *texture;
    GdkPixbuf *pixbuf;

    if (!cat)
      continue;
    
    texture = clutter_texture_new ();
    pixbuf = launcher_menu_category_get_pixbuf (cat, size);
    g_object_set_data (G_OBJECT (texture), "category", (gpointer)cat);

    clutter_texture_set_pixbuf (CLUTTER_TEXTURE (texture), pixbuf, NULL);
    g_object_unref (G_OBJECT (pixbuf));
    
    clutter_container_add_actor (CLUTTER_CONTAINER (map), texture);
    clutter_actor_set_position (texture, x, 0);
    clutter_actor_show (texture);

    if (!first)
      first = texture;
    
    x += size*1.5;
  }
  
  x -= size * 0.5;
  clutter_actor_set_size (CLUTTER_ACTOR (map), x, size);
  clutter_actor_set_position (CLUTTER_ACTOR (map),
                              (CLUTTER_STAGE_WIDTH ()/2) - (x/2),
                              CLUTTER_STAGE_HEIGHT () - (size*2));

  x = clutter_actor_get_x (first);
  x -= LAUNCHER_CATMAP_HEIGHT () * 0.25;

  priv->startx = clutter_actor_get_x (priv->seeker);
  priv->endx = x;
  priv->active = first;

  clutter_timeline_start (priv->timeline);
}

static void
on_menu_changed (LauncherMenu *menu, LauncherCatmap *map)
{
  GList *apps, *cats;
  g_return_if_fail (LAUNCHER_IS_CATMAP (map));
 
  cats = launcher_menu_get_categories (menu);
  map->priv->list = cats;
  refresh_list (map); 

  apps = launcher_menu_category_get_applications (cats->data);
  g_signal_emit (map, _map_signals[LIST_CHANGED], 0, &apps);
}
/* GObject functions */
static void
launcher_catmap_dispose (GObject *object)
{
  G_OBJECT_CLASS (launcher_catmap_parent_class)->dispose (object);
}

static void
launcher_catmap_finalize (GObject *map)
{
  LauncherCatmapPrivate *priv;
  
  g_return_if_fail (LAUNCHER_IS_CATMAP (map));
  priv = LAUNCHER_CATMAP (map)->priv;


  
  G_OBJECT_CLASS (launcher_catmap_parent_class)->finalize (map);
}


static void
launcher_catmap_class_init (LauncherCatmapClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class->finalize = launcher_catmap_finalize;
  obj_class->dispose = launcher_catmap_dispose; 

 /* Class signals */
  _map_signals[LIST_CHANGED] = 
    g_signal_new ("list-changed",
                  G_OBJECT_CLASS_TYPE (obj_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LauncherCatmapClass, new_list),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1, G_TYPE_POINTER);
                  
  g_type_class_add_private (obj_class, sizeof (LauncherCatmapPrivate));

}

static void
launcher_catmap_init (LauncherCatmap *map)
{
  LauncherCatmapPrivate *priv;
  ClutterActor *stage = clutter_stage_get_default ();
  GdkPixbuf *pixbuf = NULL;
  gint width;
  gchar *font;
  gint font_size;
  ClutterColor white = { 0xff, 0xff, 0xff, 0xff };
   
  priv = map->priv = LAUNCHER_CATMAP_GET_PRIVATE (map);

  priv->menu = launcher_menu_get_default ();
  /* Connect to menu-changed signals */
  g_signal_connect (launcher_menu_get_default (), "menu-changed",
                    G_CALLBACK (on_menu_changed), map);


  priv->list = launcher_menu_get_categories (priv->menu);
  priv->startx = CLUTTER_STAGE_WIDTH ();
  priv->endx = CLUTTER_STAGE_WIDTH ()/2;

  /* Create the 'seeker' texture. This stays behind the other icons, and gives
   * and idication to where the user is in the list of applications */
  width = (LAUNCHER_CATMAP_HEIGHT () * 1.5);
  pixbuf = gdk_pixbuf_new_from_file_at_size (PKGDATADIR "/hilight.svg",
                                             width, 
                                             width,
                                             NULL);
                                              
  priv->seeker = clutter_texture_new_from_pixbuf (pixbuf);
  g_object_unref (pixbuf);
  clutter_container_add_actor (CLUTTER_CONTAINER (map),
                               priv->seeker);

  clutter_actor_set_position (priv->seeker, 100,
        0 - (LAUNCHER_CATMAP_HEIGHT ()*0.25));
   clutter_actor_set_size (priv->seeker, width, width);
  clutter_actor_show (priv->seeker);

  priv->timeline = clutter_timeline_new (30, 60);
  priv->alpha = clutter_alpha_new_full (priv->timeline,
                                        clutter_ramp_inc_func,
                                        NULL, NULL);
  priv->behave = launcher_behave_new (priv->alpha, 
                                      (LauncherBehaveAlphaFunc)alpha_func,
                                      (gpointer)map);

  /* The popup actors */
  font_size = CLUTTER_STAGE_HEIGHT ()/30;
  font = g_strdup_printf ("Sans %d", font_size);
  
  priv->popup = clutter_group_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), priv->popup);
  clutter_actor_set_position (priv->popup, 
                              CLUTTER_STAGE_WIDTH ()/2,
                              CLUTTER_STAGE_HEIGHT ());
  clutter_actor_set_opacity (priv->popup, 0);

  pixbuf = gdk_pixbuf_new_from_file_at_scale (PKGDATADIR "/catmap-hover.svg",
                                             font_size * 13,
                                             (font_size * 4)-(font_size/2),
                                             FALSE,
                                             NULL);
                                              
  priv->popup_texture = clutter_texture_new_from_pixbuf (pixbuf);
  g_object_unref (pixbuf);
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->popup),
                               priv->popup_texture);
  clutter_actor_set_position (priv->popup_texture,
                      -1*(clutter_actor_get_width (priv->popup_texture)/2), 0);

  priv->popup_label = clutter_label_new_full (font, "Sound & Video", &white);
  clutter_label_set_line_wrap (CLUTTER_LABEL (priv->popup_label), FALSE);
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->popup), 
                               priv->popup_label);
  clutter_actor_set_position (priv->popup_label, 
                            -1*(clutter_actor_get_width (priv->popup_label)/2),
                              font_size/2);
  clutter_actor_show_all (priv->popup);

  /* Init the show/hide effect */
  priv->pop_show = clutter_timeline_new (40, 40);
  priv->pop_show_temp = clutter_effect_template_new (priv->pop_show,
                                                clutter_sine_inc_func);
 
  refresh_list (map);

}

ClutterActor*
launcher_catmap_new (void)
{
  ClutterActor *map;

  map = g_object_new (LAUNCHER_TYPE_CATMAP,
                       NULL);
  return map;
}

