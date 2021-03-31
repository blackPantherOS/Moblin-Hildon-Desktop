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

#ifndef _HAVE_LAUNCHER_MINIMAP_H
#define _HAVE_LAUNCHER_MINIMAP_H

#include <glib.h>
#include <clutter/clutter.h>

#include "launcher-animation.h"

G_BEGIN_DECLS

#define LAUNCHER_TYPE_MINIMAP (launcher_minimap_get_type ())

#define LAUNCHER_MINIMAP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
        LAUNCHER_TYPE_MINIMAP, LauncherMinimap))

#define LAUNCHER_MINIMAP_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_CAST ((klass), \
        LAUNCHER_TYPE_MINIMAP, LauncherMinimapClass))

#define LAUNCHER_IS_MINIMAP(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
        LAUNCHER_TYPE_MINIMAP))

#define LAUNCHER_IS_MINIMAP_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_TYPE ((klass), LAUNCHER_TYPE_MINIMAP))

#define LAUNCHER_MINIMAP_GET_CLASS(obj) \
        (G_TYPE_INSTANCE_GET_CLASS ((obj), \
        LAUNCHER_TYPE_MINIMAP, LauncherMinimapClass))

#define LAUNCHER_MINIMAP_HEIGHT() (CLUTTER_STAGE_HEIGHT () * 0.07)

typedef struct _LauncherMinimap LauncherMinimap;
typedef struct _LauncherMinimapClass LauncherMinimapClass;
typedef struct _LauncherMinimapPrivate LauncherMinimapPrivate;

struct _LauncherMinimap
{
  ClutterGroup         parent; 
  /*< private >*/
  LauncherMinimapPrivate   *priv;
};

struct _LauncherMinimapClass 
{
  /*< private >*/
  ClutterGroupClass    parent_class;

  /* signals */
  void (*new_list) (LauncherMinimap *map, GList **list);
  void (*active_item) (LauncherMinimap *map, LauncherItem *item);
  
  /* future padding */
  void (*_launcher_minimap_1) (void);
  void (*_launcher_minimap_2) (void);
  void (*_launcher_minimap_3) (void);
  void (*_launcher_minimap_4) (void);
};

GType launcher_minimap_get_type (void) G_GNUC_CONST;

ClutterActor*
launcher_minimap_new (void);

void
launcher_minimap_set_list (LauncherMinimap *map, GList *list);

void
launcher_minimap_set_active_item (LauncherMinimap *map, LauncherItem *item);

void
launcher_minimap_handle_event (LauncherMinimap *map, ClutterEvent *event);

G_END_DECLS

#endif /* _HAVE_LAUNCHER_MINIMAP_H */
