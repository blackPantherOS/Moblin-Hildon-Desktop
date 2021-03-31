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
#include <stdio.h>
#include <string.h>

#include "launcher-item.h"

#include "launcher-startup.h"
#include "launcher-util.h"

G_DEFINE_TYPE (LauncherItem, launcher_item, CLUTTER_TYPE_GROUP)

#define LAUNCHER_ITEM_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE(obj, \
        LAUNCHER_TYPE_ITEM, LauncherItemPrivate))

struct _LauncherItemPrivate
{
  LauncherMenuApplication *app; 

  ClutterActor *texture;
  ClutterActor *label;

  /* Launching variables */
  gboolean launching;
};

enum
{
  PROP_0,
  PROP_APPLICATION
};

enum 
{
  ITEM_STARTED,
  ITEM_FINISHED,

  LAST_SIGNAL
};

static guint _item_signals[LAST_SIGNAL] = { 0 };

/* Public functions */
LauncherMenuApplication *
launcher_item_get_application (LauncherItem *item)
{
  g_return_val_if_fail (LAUNCHER_IS_ITEM (item), NULL);
  return item->priv->app;
}

void
launcher_item_set_application (LauncherItem *item, LauncherMenuApplication *app)
{
  g_return_if_fail (LAUNCHER_IS_ITEM (item));
  g_object_set (item, "application", app, NULL);
}

ClutterActor *
launcher_item_get_label (LauncherItem *item)
{ 
  g_return_val_if_fail (LAUNCHER_IS_ITEM (item), NULL);
  
  return item->priv->label;
}

const gchar *
launcher_item_get_name (LauncherItem *item)
{
  g_return_val_if_fail (LAUNCHER_IS_ITEM (item), NULL);

  return launcher_menu_application_get_name (item->priv->app);
}

LauncherMenuCategory *
launcher_item_get_category (LauncherItem *item)
{
  g_return_val_if_fail (LAUNCHER_IS_ITEM (item), NULL);

  return launcher_menu_application_get_category (item->priv->app);
}

void
launcher_item_launch_completed (LauncherItem *item)
{
  g_return_if_fail (LAUNCHER_IS_ITEM (item));

  if (item->priv->launching)
  {
    item->priv->launching = FALSE;
    g_signal_emit (item, _item_signals[ITEM_FINISHED], 0);
  }
}

void
launcher_item_activate (LauncherItem *item)
{
  LauncherItemPrivate *priv;
  
  g_return_if_fail (LAUNCHER_IS_ITEM (item));
  priv = item->priv;

  if (!launcher_startup_launch_item (launcher_startup_get_default (), item))
    return;
  
  priv->launching = TRUE;

  /* Let the program know that the launch has started */
  g_signal_emit (item, _item_signals[ITEM_STARTED], 0);
}

/* Utils */

/*
 * This will replace the exising LauncherMenuApplication with a new one,
 * removing the old texture and label and creating a new one for the app.
 * We position the actors, so they are centered on the the groups center. The
 * reason for the is that the animation assumes the 'x' property of the item
 * is it's center (reducing calulations).
 */
static void
reload_application (LauncherItem *item)
{
  LauncherItemPrivate *priv;
  guint iw, ih; /* Item size */
  guint tw, th, tx, ty; /* texture area */
  guint lw, lh, lx, ly;
  GdkPixbuf *icon;
  gchar *font;
  const gchar *name;
  ClutterColor text_col = {0xff, 0xff, 0xff, 0xff};

  g_return_if_fail (LAUNCHER_IS_ITEM (item));
  priv = item->priv;

  if (CLUTTER_IS_ACTOR (priv->texture))
    clutter_actor_destroy (priv->texture);
  if (CLUTTER_IS_ACTOR (priv->label))
    clutter_actor_destroy (priv->label);

  icon = launcher_menu_application_get_icon (priv->app);
  priv->texture = clutter_texture_new_from_pixbuf (icon);
    
  clutter_container_add_actor (CLUTTER_CONTAINER (item), priv->texture);
  clutter_actor_set_size (priv->texture, 
                          gdk_pixbuf_get_width (icon),
                          gdk_pixbuf_get_height (icon));
  
  /* Make sure it is positioned in the middle of the group */
  clutter_actor_get_size (CLUTTER_ACTOR (item), &iw, &ih);
  clutter_actor_get_size (priv->texture, &tw, &th);
  tw = gdk_pixbuf_get_width (icon);
  th = gdk_pixbuf_get_height (icon);
  
  tx = -1 * (tw/2);
  ty = -1 * (th/2);

  clutter_actor_set_position (priv->texture, tx, ty);
  clutter_actor_show (priv->texture);
  
  /* Now the label */
  name = launcher_menu_application_get_name (priv->app);
  font = g_strdup_printf ("Sans %d", th/9);
  priv->label = clutter_label_new_full (font, name, &text_col);
  clutter_container_add_actor (CLUTTER_CONTAINER (item), priv->label);
  clutter_label_set_line_wrap (CLUTTER_LABEL (priv->label), FALSE);

  lw = clutter_actor_get_width (priv->label);
  lh = clutter_actor_get_height (priv->label);

  lx = -1 * (lw/2);
  ly = ty - (lh*1.5);

  clutter_actor_set_position (priv->label, lx, ly);
  clutter_actor_show (priv->label);
}
  

/* GObject functions */

/*
 * Override so we can control the 'paint' function, and make sure we don't
 * unnecessaryly paint when the item is offscreen.
 *
 * TODO: When we don't paint, destroy the texture to save VRAM.
 */
static void
launcher_item_paint (ClutterActor *actor)
{
  LauncherItemPrivate *priv;
  gint x;
  gint leftmost, rightmost;
  
  priv = LAUNCHER_ITEM (actor)->priv;

  x = clutter_actor_get_x (actor);
  x+= clutter_actor_get_x (clutter_actor_get_parent (actor));

  leftmost = -1 * LAUNCHER_ITEM_WIDTH ();
  rightmost = CLUTTER_STAGE_WIDTH () + LAUNCHER_ITEM_WIDTH ();
  
  if (x < leftmost || x > rightmost)
  {
    return;
  }

  clutter_actor_paint (priv->texture);
  clutter_actor_paint (priv->label);
}

static void
launcher_item_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  LauncherItemPrivate *priv;

  g_return_if_fail (LAUNCHER_IS_ITEM (object));
  priv = LAUNCHER_ITEM (object)->priv;

  switch (prop_id)
  {
    case PROP_APPLICATION:
      g_value_set_pointer (value, priv->app);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
launcher_item_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  LauncherItemPrivate *priv;

  g_return_if_fail (LAUNCHER_IS_ITEM (object));
  priv = LAUNCHER_ITEM (object)->priv;

  switch (prop_id)
  {
    case PROP_APPLICATION:
      priv->app = g_value_get_pointer (value);
      reload_application (LAUNCHER_ITEM (object));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}


static void
launcher_item_dispose (GObject *object)
{
  G_OBJECT_CLASS (launcher_item_parent_class)->dispose (object);
}

static void
launcher_item_finalize (GObject *item)
{
  LauncherItemPrivate *priv;
  
  g_return_if_fail (LAUNCHER_IS_ITEM (item));
  priv = LAUNCHER_ITEM (item)->priv;


  G_OBJECT_CLASS (launcher_item_parent_class)->finalize (item);
}


static void
launcher_item_class_init (LauncherItemClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  obj_class->finalize = launcher_item_finalize;
  obj_class->dispose = launcher_item_dispose;
  obj_class->get_property = launcher_item_get_property;
  obj_class->set_property = launcher_item_set_property;

  actor_class->paint = launcher_item_paint;

  g_object_class_install_property (obj_class,
    PROP_APPLICATION,
		g_param_spec_pointer ("application",
		  "Application",
		  "The application this item represents",
		  G_PARAM_CONSTRUCT|G_PARAM_READWRITE));
    
    _item_signals[ITEM_STARTED] = 
      g_signal_new ("item-started",
                  G_OBJECT_CLASS_TYPE (obj_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LauncherItemClass, item_started),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
    
    _item_signals[ITEM_FINISHED] = 
      g_signal_new ("item-finished",
                  G_OBJECT_CLASS_TYPE (obj_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LauncherItemClass, item_finished),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  g_type_class_add_private (obj_class, sizeof (LauncherItemPrivate)); 
}

static void
launcher_item_init (LauncherItem *item)
{
  LauncherItemPrivate *priv;
  gint width, height;
  
  priv = item->priv = LAUNCHER_ITEM_GET_PRIVATE (item);

  priv->launching = FALSE;

  /* Set the size of the item to be a factor of the stage */
  width = LAUNCHER_ITEM_WIDTH ();
  height = LAUNCHER_ITEM_HEIGHT ();
  
  clutter_actor_set_size (CLUTTER_ACTOR (item), width, height);
  clutter_actor_set_position (CLUTTER_ACTOR (item), 10, 10);
  clutter_actor_show (CLUTTER_ACTOR (item));
}

ClutterActor *
launcher_item_new (void)
{
  ClutterActor *item;

  item = g_object_new (LAUNCHER_TYPE_ITEM,
                       NULL);
  return item;
}

ClutterActor *
launcher_item_new_from_application (LauncherMenuApplication *app)
{
  ClutterActor *item;

  item = g_object_new (LAUNCHER_TYPE_ITEM,
                       "application", app,
                       NULL);
  return item; 
}
