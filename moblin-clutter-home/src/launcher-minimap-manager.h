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

#ifndef _HAVE_LAUNCHER_MINIMAP_MANAGER_H
#define _HAVE_LAUNCHER_MINIMAP_MANAGER_H

#include <glib.h>
#include <clutter/clutter.h>

#include "launcher-animation.h"

G_BEGIN_DECLS

#define LAUNCHER_TYPE_MINIMAP_MANAGER (launcher_minimap_manager_get_type ())

#define LAUNCHER_MINIMAP_MANAGER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
        LAUNCHER_TYPE_MINIMAP_MANAGER, LauncherMinimapManager))

#define LAUNCHER_MINIMAP_MANAGER_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_CAST ((klass), \
        LAUNCHER_TYPE_MINIMAP_MANAGER, LauncherMinimapManagerClass))

#define LAUNCHER_IS_MINIMAP_MANAGER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
        LAUNCHER_TYPE_MINIMAP_MANAGER))

#define LAUNCHER_IS_MINIMAP_MANAGER_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_TYPE ((klass), LAUNCHER_TYPE_MINIMAP_MANAGER))

#define LAUNCHER_MINIMAP_MANAGER_GET_CLASS(obj) \
        (G_TYPE_INSTANCE_GET_CLASS ((obj), \
        LAUNCHER_TYPE_MINIMAP_MANAGER, LauncherMinimapManagerClass))

#define LAUNCHER_MINIMAP_MANAGER_HEIGHT() (CLUTTER_STAGE_HEIGHT () * 0.07)

typedef struct _LauncherMinimapManager LauncherMinimapManager;
typedef struct _LauncherMinimapManagerClass LauncherMinimapManagerClass;
typedef struct _LauncherMinimapManagerPrivate LauncherMinimapManagerPrivate;

struct _LauncherMinimapManager
{
  GObject         parent; 
  /*< private >*/
  LauncherMinimapManagerPrivate   *priv;
};

struct _LauncherMinimapManagerClass 
{
  /*< private >*/
  GObjectClass    parent_class;

  /* signals */
  void (*new_list) (LauncherMinimapManager *map, GList **list);
  void (*active_item) (LauncherMinimapManager *map, LauncherItem *item);
  
  /* future padding */
  void (*_launcher_minimap_manager_1) (void);
  void (*_launcher_minimap_manager_2) (void);
  void (*_launcher_minimap_manager_3) (void);
  void (*_launcher_minimap_manager_4) (void);
};

GType launcher_minimap_manager_get_type (void) G_GNUC_CONST;

LauncherMinimapManager*
launcher_minimap_manager_new (void);

void
launcher_minimap_manager_set_list (LauncherMinimapManager *map, GList *list);

void
launcher_minimap_manager_set_active_item (LauncherMinimapManager *map, 
                                          LauncherItem *item);

void
launcher_minimap_manager_handle_event (LauncherMinimapManager *map, 
                                       ClutterEvent *event);

void
launcher_minimap_manager_show (LauncherMinimapManager *map, gboolean show);

G_END_DECLS

#endif /* _HAVE_LAUNCHER_MINIMAP_MANAGER_H */
