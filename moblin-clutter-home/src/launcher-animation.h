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

#ifndef _HAVE_LAUNCHER_ANIMATION_H
#define _HAVE_LAUNCHER_ANIMATION_H

#include <glib.h>
#include <clutter/clutter.h>

#include "launcher-item.h"

G_BEGIN_DECLS

#define LAUNCHER_TYPE_ANIMATION (launcher_animation_get_type ())

#define LAUNCHER_ANIMATION(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
        LAUNCHER_TYPE_ANIMATION, LauncherAnimation))

#define LAUNCHER_ANIMATION_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), \
        LAUNCHER_TYPE_ANIMATION, LauncherAnimationClass))

#define LAUNCHER_IS_ANIMATION(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
        LAUNCHER_TYPE_ANIMATION))

#define LAUNCHER_IS_ANIMATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
        LAUNCHER_TYPE_ANIMATION))

#define LAUNCHER_ANIMATION_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), \
        LAUNCHER_TYPE_ANIMATION, LauncherAnimationClass))

typedef struct _LauncherAnimation LauncherAnimation;
typedef struct _LauncherAnimationClass LauncherAnimationClass;
typedef struct _LauncherAnimationPrivate LauncherAnimationPrivate;

struct _LauncherAnimation
{
  GObject         parent;

  /*< private >*/
  LauncherAnimationPrivate   *priv;
};

struct _LauncherAnimationClass 
{
  /*< private >*/
  GObjectClass    parent_class;

  /* VTable */
  void (*handle_event) (LauncherAnimation *anim, ClutterEvent *event);
  void (*set_list) (LauncherAnimation *anim, GList *list);
  void (*set_active_item) (LauncherAnimation *anim, LauncherItem *item);
  
  /* future padding */
  void (*_launcher_animation_1) (void);
  void (*_launcher_animation_2) (void);
  void (*_launcher_animation_3) (void);
  void (*_launcher_animation_4) (void);
};

GType launcher_animation_get_type (void) G_GNUC_CONST;

void
launcher_animation_handle_event (LauncherAnimation *anim, ClutterEvent *event);

void
launcher_animation_set_list (LauncherAnimation *anim, GList *list);

void
launcher_animation_set_active_item (LauncherAnimation *anim,
                                    LauncherItem *item);


G_END_DECLS

#endif /* _HAVE_LAUNCHER_ANIMATION_H */
