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

#ifndef _HAVE_LAUNCHER_ITEM_H
#define _HAVE_LAUNCHER_ITEM_H

#include <glib.h>
#include <clutter/clutter.h>

#include "launcher-menu.h"

G_BEGIN_DECLS

#define LAUNCHER_TYPE_ITEM (launcher_item_get_type ())

#define LAUNCHER_ITEM(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
        LAUNCHER_TYPE_ITEM, LauncherItem))

#define LAUNCHER_ITEM_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_CAST ((klass), \
        LAUNCHER_TYPE_ITEM, LauncherItemClass))

#define LAUNCHER_IS_ITEM(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
        LAUNCHER_TYPE_ITEM))

#define LAUNCHER_IS_ITEM_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_TYPE ((klass), LAUNCHER_TYPE_ITEM))

#define LAUNCHER_ITEM_GET_CLASS(obj) \
        (G_TYPE_INSTANCE_GET_CLASS ((obj), \
        LAUNCHER_TYPE_ITEM, LauncherItemClass))

#define LAUNCHER_ITEM_WIDTH() (CLUTTER_STAGE_HEIGHT () / 3)
#define LAUNCHER_ITEM_HEIGHT() (CLUTTER_STAGE_HEIGHT () / 3)

typedef struct _LauncherItem LauncherItem;
typedef struct _LauncherItemClass LauncherItemClass;
typedef struct _LauncherItemPrivate LauncherItemPrivate;

struct _LauncherItem
{
  ClutterGroup         parent; 
  /*< private >*/
  LauncherItemPrivate   *priv;
};

struct _LauncherItemClass 
{
  /*< private >*/
  ClutterGroupClass    parent_class;

  /* signals */
  void (*item_started) (LauncherItem *item);
  void (*item_finished) (LauncherItem *item);

  /* future padding */
  void (*_launcher_item_1) (void);
  void (*_launcher_item_2) (void);
  void (*_launcher_item_3) (void);
  void (*_launcher_item_4) (void);
};

GType launcher_item_get_type (void) G_GNUC_CONST;

ClutterActor *
launcher_item_new (void);

ClutterActor *
launcher_item_new_from_application (LauncherMenuApplication *app);

LauncherMenuApplication *
launcher_item_get_application (LauncherItem *item);

void
launcher_item_set_application (LauncherItem *item, LauncherMenuApplication *ap);

const gchar *
launcher_item_get_name (LauncherItem *item);

LauncherMenuCategory *
launcher_item_get_category (LauncherItem *item);

ClutterActor *
launcher_item_get_label (LauncherItem *item);

void
launcher_item_activate (LauncherItem *item);

void
launcher_item_launch_completed (LauncherItem *item);

G_END_DECLS

#endif /* _HAVE_LAUNCHER_ITEM_H */
