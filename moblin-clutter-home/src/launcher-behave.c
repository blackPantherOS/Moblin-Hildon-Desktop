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

#include "launcher-behave.h"

G_DEFINE_TYPE (LauncherBehave, launcher_behave, CLUTTER_TYPE_BEHAVIOUR);

#define LAUNCHER_BEHAVE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  LAUNCHER_TYPE_BEHAVE, \
  LauncherBehavePrivate))

struct _LauncherBehavePrivate
{
  LauncherBehaveAlphaFunc     func;
  gpointer		data;
};

static void
launcher_behave_alpha_notify (ClutterBehaviour *behave, guint32 alpha_value)
{
  LauncherBehave *launcher_behave = LAUNCHER_BEHAVE(behave);
  LauncherBehavePrivate *priv;
	
  priv = LAUNCHER_BEHAVE_GET_PRIVATE (launcher_behave);
	
  if (priv->func != NULL) 
    priv->func (behave, alpha_value, priv->data);
}

static void
launcher_behave_class_init (LauncherBehaveClass *klass)
{
  GObjectClass        *gobject_class = G_OBJECT_CLASS (klass);
  ClutterBehaviourClass *behave_class = CLUTTER_BEHAVIOUR_CLASS (klass);

  behave_class->alpha_notify = launcher_behave_alpha_notify;
	
  g_type_class_add_private (gobject_class, sizeof (LauncherBehavePrivate));
}

static void
launcher_behave_init (LauncherBehave *self)
{
  LauncherBehavePrivate *priv;
	
  priv = LAUNCHER_BEHAVE_GET_PRIVATE (self);
	
  priv->func = NULL;
  priv->data = NULL;
}

ClutterBehaviour*
launcher_behave_new (ClutterAlpha 		       *alpha,
                     LauncherBehaveAlphaFunc 	func,
                     gpointer		              data)
{
  LauncherBehave *behave;
  LauncherBehavePrivate *priv;
	
  behave = g_object_new (LAUNCHER_TYPE_BEHAVE, 
                         "alpha", alpha,
                         NULL);

  priv = LAUNCHER_BEHAVE_GET_PRIVATE (behave);  
	
  priv->func = func;
  priv->data = data;
		
  return CLUTTER_BEHAVIOUR(behave);
}
