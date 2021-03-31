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
#include <math.h>

#include <gconf/gconf.h>
#include <gconf/gconf-client.h>

#include "launcher-background.h"
#include "launcher-behave.h"

G_DEFINE_TYPE (LauncherBackground, launcher_background, CLUTTER_TYPE_GROUP)

#define LAUNCHER_BACKGROUND_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE(obj, \
        LAUNCHER_TYPE_BACKGROUND, LauncherBackgroundPrivate))

/* Gconf keys */
#define BG_PATH    "/apps/desktop-launcher"
#define BG_FILE    BG_PATH "/bg_picture_filename" /* string */
#define BG_OPTION  BG_PATH "/bg_picture_options" 
                   /* none|wallpaper|centred|scaled|stretched */

#define BG_DEFAULT  PKGDATADIR "/default.svg"

struct _LauncherBackgroundPrivate
{
  ClutterActor *texture;
  ClutterActor *fade;
  
  gchar *filename;
  gchar *option;

  ClutterTimeline *timeline;
  ClutterAlpha *alpha;
  ClutterBehaviour *behave;

  gboolean fade_out;
};

/* 
 * Just load the file normally,if its larger than the stage, clip it */
static void
load_normal (const gchar *filename, ClutterActor *texture)
{
  GdkPixbuf *pixbuf = NULL;
  GError *error = NULL;
  gint width, height;
  gint subw = 0, subh = 0; 

  pixbuf = gdk_pixbuf_new_from_file (filename, &error);

  if (error)
  {
    g_warning (error->message);
    g_error_free (error);
    return;
  }

  width = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);

  if (width > CLUTTER_STAGE_WIDTH ())
    subw = CLUTTER_STAGE_WIDTH ();
  else
    subw = width;

  if (height > CLUTTER_STAGE_HEIGHT ())
    subh = CLUTTER_STAGE_HEIGHT ();
  else
    subh = height;

  if (subw && subh)
  {
    GdkPixbuf *temp = pixbuf;

    pixbuf = gdk_pixbuf_new_subpixbuf (temp, 0, 0, subw, subh);
    g_object_unref (temp);
  }

  clutter_texture_set_pixbuf (CLUTTER_TEXTURE (texture), pixbuf, NULL);
  clutter_actor_set_size (texture, 
                          CLUTTER_STAGE_WIDTH (),
                          CLUTTER_STAGE_HEIGHT ());
  clutter_actor_set_position (texture, 0, 0);

  g_object_unref (pixbuf);
}

/*
 * Simply return a stretched version of the image
 */
static void
load_stretched (const gchar *filename, ClutterActor *texture)
{
  GdkPixbuf *pixbuf = NULL;
  GError *error = NULL;

  pixbuf = gdk_pixbuf_new_from_file_at_scale (filename,
                                              CLUTTER_STAGE_WIDTH (),
                                              CLUTTER_STAGE_HEIGHT (),
                                              FALSE,
                                              &error);
  if (error)
  {
    g_warning (error->message);
    g_error_free (error);
  }

  clutter_texture_set_pixbuf (CLUTTER_TEXTURE (texture), pixbuf, NULL);
  clutter_actor_set_size (texture, 
                          CLUTTER_STAGE_WIDTH (),
                          CLUTTER_STAGE_HEIGHT ());
  clutter_actor_set_position (texture, 0, 0);

  g_object_unref (pixbuf);
}

/*
 * Center the image on the stage
 */
static void
load_centred (const gchar *filename, ClutterActor *texture)
{
  GdkPixbuf *pixbuf = NULL;
  GError *error = NULL;
  gint w, h, x, y;

  pixbuf = gdk_pixbuf_new_from_file (filename, &error);

  if (error)
  {
    g_warning (error->message);
    g_error_free (error);
    return;
  }

  clutter_texture_set_pixbuf (CLUTTER_TEXTURE (texture), pixbuf, NULL);
  
  w = gdk_pixbuf_get_width (pixbuf);
  h = gdk_pixbuf_get_height (pixbuf);
  x = (CLUTTER_STAGE_WIDTH ()/2) - (w/2);
  y = (CLUTTER_STAGE_HEIGHT ()/2) - (h/2);

  clutter_actor_set_size (texture, w, h);
  clutter_actor_set_position (texture, x, y);  

  g_object_unref (pixbuf);
}

/*
 * Load the image scaled with the correct aspect ratio 
 */
static void
load_scaled (const gchar *filename, ClutterActor *texture)
{
  GdkPixbuf *pixbuf = NULL;
  GError *error = NULL;
  gint w, h, x, y;

  pixbuf = gdk_pixbuf_new_from_file_at_scale (filename,
                                              CLUTTER_STAGE_WIDTH (),
                                              CLUTTER_STAGE_HEIGHT (),
                                              TRUE,
                                              &error);
  if (error)
  {
    g_warning (error->message);
    g_error_free (error);
  }
  
  clutter_texture_set_pixbuf (CLUTTER_TEXTURE (texture), pixbuf, NULL);
  
  w = gdk_pixbuf_get_width (pixbuf);
  h = gdk_pixbuf_get_height (pixbuf);
  x = (CLUTTER_STAGE_WIDTH ()/2) - (w/2);
  y = (CLUTTER_STAGE_HEIGHT ()/2) - (h/2);

  clutter_actor_set_size (texture, w, h);
  clutter_actor_set_position (texture, x, y);  

  g_object_unref (pixbuf);
}

/* 
 * Load the image, and then tile it until it covers the entire stage 
 */
static void
load_wallpaper (const gchar *filename, ClutterActor *texture)
{
  GdkPixbuf *pixbuf = NULL;
  GdkPixbuf *tiled = NULL;
  GError *error = NULL;
  gint w, h, x, y;
  gint rows, cols, r, c;

  pixbuf = gdk_pixbuf_new_from_file (filename, &error);

  if (error)
  {
    g_warning (error->message);
    g_error_free (error);
    return;
  }

  x = y = 0;
  rows = cols = 1;
  w = gdk_pixbuf_get_width (pixbuf);
  h = gdk_pixbuf_get_height (pixbuf);

  /* Find the number of rows and columns */
  if (w < CLUTTER_STAGE_WIDTH ())
    cols = (gint)ceil (CLUTTER_STAGE_WIDTH () / (gdouble)w);
  
  if (h < CLUTTER_STAGE_HEIGHT ())
    rows = (gint)ceil (CLUTTER_STAGE_HEIGHT () / (gdouble)h);

  /* Create the new pixbuf */
  tiled = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, w*cols, h*rows);

  /* For the number of rows, tile the cols */
  for (r = 0; r < rows; r++)
  {
    for (c = 0; c < cols; c++)
    {
      gdk_pixbuf_composite (pixbuf, tiled, x, y, w, h, x, y, 1, 1,
                            GDK_INTERP_BILINEAR, 255);
      x += w;
    }
    y += h;
    x = 0;
  }

  g_object_unref (pixbuf);
  
  clutter_texture_set_pixbuf (CLUTTER_TEXTURE (texture), tiled, NULL);
  clutter_actor_set_position (texture, 0, 0);
}

/* 
 * Use the filename and option values to create a background pixbuf, and set 
 * the internal tetxures pixbuf.
 * We try and get the smallest possible pixbuf to make sure we don't abuse
 * texture memory.
 */
static void
ensure_layout (LauncherBackground *bg)
{
  LauncherBackgroundPrivate *priv;
    
  g_return_if_fail (LAUNCHER_IS_BACKGROUND (bg));
  priv = bg->priv;

  if (priv->filename == NULL || strcmp (priv->filename, "default") == 0)
    priv->filename = g_strdup (BG_DEFAULT);

  if (priv->option == NULL)
    priv->option = g_strdup ("stretched");

  if (priv->option == NULL || strcmp (priv->option, "none") == 0)
  {
    load_normal (priv->filename, priv->texture);
  }
  else if (strcmp (priv->option, "wallpaper") == 0)
  {
    load_wallpaper (priv->filename, priv->texture);
  }
  else if (strcmp (priv->option, "centred") == 0)
  {
    load_centred (priv->filename, priv->texture);
  }
  else if (strcmp (priv->option, "scaled") == 0)
  {
    load_scaled (priv->filename, priv->texture);
  }
  else /* stretched */
  {
    load_stretched (priv->filename, priv->texture);
  }
 
   clutter_actor_queue_redraw (CLUTTER_ACTOR (bg));
}

/*
 * The animation function. At the moment, we just to a simple fade in/fade out
 */
static void
alpha_func (ClutterBehaviour   *behave,
            guint32             alpha_value,
            LauncherBackground *bg)
{
  LauncherBackgroundPrivate *priv;
  gfloat factor;

  g_return_if_fail (LAUNCHER_IS_BACKGROUND (bg));
  priv = bg->priv;

  factor = (gfloat)alpha_value / CLUTTER_ALPHA_MAX_ALPHA;
  
  if (priv->fade_out)
    clutter_actor_set_opacity (CLUTTER_ACTOR (bg), 255 - (255*factor));
  else
    clutter_actor_set_opacity (CLUTTER_ACTOR (bg), 255 * factor);

  clutter_actor_queue_redraw (CLUTTER_ACTOR (bg));
}

static void
on_timeline_completed (ClutterTimeline *timeline, LauncherBackground *bg)
{
  LauncherBackgroundPrivate *priv;

  g_return_if_fail (LAUNCHER_IS_BACKGROUND (bg));
  priv = bg->priv;

  if (priv->fade_out)
  {
    ensure_layout (bg);
    priv->fade_out = FALSE;
    clutter_timeline_start (priv->timeline);
  }
  else
  {
    priv->fade_out = TRUE;
  }
}

/* Gconf callbacks */
static void
on_bg_filename_changed (GConfClient        *client,
                        guint               cid,
                        GConfEntry         *entry,
                        LauncherBackground *bg)
{
  LauncherBackgroundPrivate *priv;
  GConfValue *value = NULL;

  g_return_if_fail (LAUNCHER_IS_BACKGROUND (bg));
  priv = bg->priv;

  value = gconf_entry_get_value (entry);
  if (priv->filename)
    g_free (priv->filename);

  priv->filename = g_strdup (gconf_value_get_string (value));

  clutter_timeline_start (priv->timeline);
}

static void
on_bg_option_changed (GConfClient        *client,
                      guint               cid,
                      GConfEntry         *entry,
                      LauncherBackground *bg)
{
  LauncherBackgroundPrivate *priv;
  GConfValue *value = NULL;

  g_return_if_fail (LAUNCHER_IS_BACKGROUND (bg));
  priv = bg->priv;

  value = gconf_entry_get_value (entry);
  if (priv->option)
    g_free (priv->option);

  priv->option = g_strdup (gconf_value_get_string (value));

  clutter_timeline_start (priv->timeline);
}
/* GObject functions */
static void
launcher_background_dispose (GObject *object)
{
  G_OBJECT_CLASS (launcher_background_parent_class)->dispose (object);
}

static void
launcher_background_finalize (GObject *background)
{
  LauncherBackgroundPrivate *priv;
  
  g_return_if_fail (LAUNCHER_IS_BACKGROUND (background));
  priv = LAUNCHER_BACKGROUND (background)->priv;


  G_OBJECT_CLASS (launcher_background_parent_class)->finalize (background);
}


static void
launcher_background_class_init (LauncherBackgroundClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class->finalize = launcher_background_finalize;
  obj_class->dispose = launcher_background_dispose;

  g_type_class_add_private (obj_class, sizeof (LauncherBackgroundPrivate)); 
}

static void
launcher_background_init (LauncherBackground *background)
{
  LauncherBackgroundPrivate *priv;
  GConfClient *client = gconf_client_get_default ();

  priv = background->priv = LAUNCHER_BACKGROUND_GET_PRIVATE (background);
  priv->fade_out = TRUE;

  priv->texture = clutter_texture_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (background), priv->texture);
  clutter_actor_set_size (priv->texture, 
                          CLUTTER_STAGE_WIDTH (),
                          CLUTTER_STAGE_HEIGHT ());
  clutter_actor_set_position (priv->texture, 0, 0);
  clutter_actor_show (priv->texture);

  gconf_client_add_dir (client, BG_PATH, GCONF_CLIENT_PRELOAD_NONE, NULL);

  priv->filename = g_strdup (gconf_client_get_string (client, BG_FILE, NULL));
  gconf_client_notify_add (client, BG_FILE,
                           (GConfClientNotifyFunc)on_bg_filename_changed,
                           background, NULL, NULL);
  
  priv->option = g_strdup (gconf_client_get_string (client, BG_OPTION, NULL));
  gconf_client_notify_add (client, BG_OPTION,
                           (GConfClientNotifyFunc)on_bg_option_changed,
                           background, NULL, NULL);
  

  priv->timeline = clutter_timeline_new (40, 80);
  priv->alpha = clutter_alpha_new_full (priv->timeline,
                                        clutter_sine_inc_func,
                                        NULL, NULL);
  priv->behave = launcher_behave_new (priv->alpha, 
                                      (LauncherBehaveAlphaFunc)alpha_func,
                                      (gpointer)background);

  g_signal_connect (priv->timeline, "completed",
                    G_CALLBACK (on_timeline_completed), (gpointer)background);

  ensure_layout (background);
}

ClutterActor *
launcher_background_new (void)
{
  ClutterActor *background;

  background = g_object_new (LAUNCHER_TYPE_BACKGROUND,
                       NULL);
  return background;
}
