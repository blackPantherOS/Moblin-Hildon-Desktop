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

#include "launcher-animation.h"

G_DEFINE_ABSTRACT_TYPE (LauncherAnimation, launcher_animation, G_TYPE_OBJECT)

/*
 * Handle the current event 
 */
void
launcher_animation_handle_event (LauncherAnimation *anim, ClutterEvent *event)
{
  LauncherAnimationClass *klass;

  g_return_if_fail (LAUNCHER_IS_ANIMATION (anim));
  
  klass = LAUNCHER_ANIMATION_GET_CLASS (anim);
  g_return_if_fail (klass->handle_event != NULL);

  klass->handle_event (anim, event);
}

void
launcher_animation_set_list (LauncherAnimation *anim, GList *list)
{
  LauncherAnimationClass *klass;

  g_return_if_fail (LAUNCHER_IS_ANIMATION (anim));
  
  klass = LAUNCHER_ANIMATION_GET_CLASS (anim);
  g_return_if_fail (klass->set_list != NULL);

  klass->set_list (anim, list);
}

void
launcher_animation_set_active_item (LauncherAnimation *anim,
                                    LauncherItem *item)
{
  LauncherAnimationClass *klass;

  g_return_if_fail (LAUNCHER_IS_ANIMATION (anim));
  g_return_if_fail (LAUNCHER_IS_ITEM (item));
  
  klass = LAUNCHER_ANIMATION_GET_CLASS (anim);
  g_return_if_fail (klass->set_active_item != NULL);

  klass->set_active_item (anim, item);
}
/* GObject functions */
static void
launcher_animation_dispose (GObject *object)
{
  G_OBJECT_CLASS (launcher_animation_parent_class)->dispose (object);
}

static void
launcher_animation_finalize (GObject *animation)
{
  LauncherAnimationPrivate *priv;
  
  g_return_if_fail (LAUNCHER_IS_ANIMATION (animation));
  priv = LAUNCHER_ANIMATION (animation)->priv;


  G_OBJECT_CLASS (launcher_animation_parent_class)->finalize (animation);
}


static void
launcher_animation_class_init (LauncherAnimationClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class->finalize = launcher_animation_finalize;
  obj_class->dispose = launcher_animation_dispose;
}

static void
launcher_animation_init (LauncherAnimation *animation)
{
}

