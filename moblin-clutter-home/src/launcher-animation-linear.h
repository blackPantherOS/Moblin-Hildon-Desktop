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

#ifndef _HAVE_LAUNCHER_ANIMATION_LINEAR_H
#define _HAVE_LAUNCHER_ANIMATION_LINEAR_H

#include <glib.h>
#include <clutter/clutter.h>

#include "launcher-animation.h"

G_BEGIN_DECLS

#define LAUNCHER_TYPE_ANIMATION_LINEAR (launcher_animation_linear_get_type ())

#define LAUNCHER_ANIMATION_LINEAR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
        LAUNCHER_TYPE_ANIMATION_LINEAR, LauncherAnimationLinear))

#define LAUNCHER_ANIMATION_LINEAR_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_CAST ((klass), \
        LAUNCHER_TYPE_ANIMATION_LINEAR, LauncherAnimationLinearClass))

#define LAUNCHER_IS_ANIMATION_LINEAR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
        LAUNCHER_TYPE_ANIMATION_LINEAR))

#define LAUNCHER_IS_ANIMATION_LINEAR_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_TYPE ((klass), LAUNCHER_TYPE_ANIMATION_LINEAR))

#define LAUNCHER_ANIMATION_LINEAR_GET_CLASS(obj) \
        (G_TYPE_INSTANCE_GET_CLASS ((obj), \
        LAUNCHER_TYPE_ANIMATION_LINEAR, LauncherAnimationLinearClass))

typedef struct _LauncherAnimationLinear LauncherAnimationLinear;
typedef struct _LauncherAnimationLinearClass LauncherAnimationLinearClass;
typedef struct _LauncherAnimationLinearPrivate LauncherAnimationLinearPrivate;

struct _LauncherAnimationLinear
{
  LauncherAnimation         parent; 
  /*< private >*/
  LauncherAnimationLinearPrivate   *priv;
};

struct _LauncherAnimationLinearClass 
{
  /*< private >*/
  LauncherAnimationClass    parent_class;

  /* signals */
  void (*active_item_changed) (LauncherAnimation *anim, LauncherItem *item);
  void (*launch_started) (LauncherAnimation *anim, LauncherItem *item);
  void (*launch_finished) (LauncherAnimation *anim, LauncherItem *item);
  
  /* future padding */
  void (*_launcher_animation_linear_1) (void);
  void (*_launcher_animation_linear_2) (void);
  void (*_launcher_animation_linear_3) (void);
  void (*_launcher_animation_linear_4) (void);
};

GType launcher_animation_linear_get_type (void) G_GNUC_CONST;

LauncherAnimation *
launcher_animation_linear_new (void);


G_END_DECLS

#endif /* _HAVE_LAUNCHER_ANIMATION_LINEAR_H */
