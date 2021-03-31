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

#ifndef _HAVE_LAUNCHER_CATMAP_H
#define _HAVE_LAUNCHER_CATMAP_H

#include <glib.h>
#include <clutter/clutter.h>

#include "launcher-animation.h"

G_BEGIN_DECLS

#define LAUNCHER_TYPE_CATMAP (launcher_catmap_get_type ())

#define LAUNCHER_CATMAP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
        LAUNCHER_TYPE_CATMAP, LauncherCatmap))

#define LAUNCHER_CATMAP_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_CAST ((klass), \
        LAUNCHER_TYPE_CATMAP, LauncherCatmapClass))

#define LAUNCHER_IS_CATMAP(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
        LAUNCHER_TYPE_CATMAP))

#define LAUNCHER_IS_CATMAP_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_TYPE ((klass), LAUNCHER_TYPE_CATMAP))

#define LAUNCHER_CATMAP_GET_CLASS(obj) \
        (G_TYPE_INSTANCE_GET_CLASS ((obj), \
        LAUNCHER_TYPE_CATMAP, LauncherCatmapClass))

#define LAUNCHER_CATMAP_HEIGHT() (CLUTTER_STAGE_HEIGHT () * 0.07)

typedef struct _LauncherCatmap LauncherCatmap;
typedef struct _LauncherCatmapClass LauncherCatmapClass;
typedef struct _LauncherCatmapPrivate LauncherCatmapPrivate;

struct _LauncherCatmap
{
  ClutterHBox         parent; 
  /*< private >*/
  LauncherCatmapPrivate   *priv;
};

struct _LauncherCatmapClass 
{
  /*< private >*/
  ClutterHBoxClass    parent_class;

  /* signals */
  void (*new_list) (LauncherCatmap *map, GList **list);
  
  /* future padding */
  void (*_launcher_catmap_1) (void);
  void (*_launcher_catmap_2) (void);
  void (*_launcher_catmap_3) (void);
  void (*_launcher_catmap_4) (void);
};

GType launcher_catmap_get_type (void) G_GNUC_CONST;

ClutterActor*
launcher_catmap_new (void);

void
launcher_catmap_set_list (LauncherCatmap *map, GList *list);

void
launcher_catmap_handle_event (LauncherCatmap *map, ClutterEvent *event);

G_END_DECLS

#endif /* _HAVE_LAUNCHER_CATMAP_H */
