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

/* This is a utility ClutterBehaviour-derived class, in which you can set the
   alphanotify function. It is useful for situations where you do not need the
   full capabilities of the ClutterBehvaiour class, you just want a function to
   be called for each iteration along the timeline
*/

#ifndef _LAUNCHER_BEHAVE_H_
#define _LAUNCHER_BEHAVE_H_

#include <glib-object.h>
#include <clutter/clutter.h>

#define LAUNCHER_TYPE_BEHAVE (launcher_behave_get_type ())

#define LAUNCHER_BEHAVE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
	LAUNCHER_TYPE_BEHAVE, LauncherBehave))

#define LAUNCHER_BEHAVE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass),\
	LAUNCHER_TYPE_BEHAVE, LauncherBehaveClass))

#define CLUTTER_IS_BEHAVE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
	LAUNCHER_TYPE_BEHAVE))

#define CLUTTER_IS_BEHAVE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
	LAUNCHER_TYPE_BEHAVE))

#define LAUNCHER_BEHAVE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),\
	LAUNCHER_TYPE_BEHAVE, LauncherBehaveClass))

typedef struct _LauncherBehave        LauncherBehave;
typedef struct _LauncherBehaveClass   LauncherBehaveClass;
typedef struct _LauncherBehavePrivate LauncherBehavePrivate;
 
struct _LauncherBehave
{
  ClutterBehaviour        parent;	
};

struct _LauncherBehaveClass
{
  ClutterBehaviourClass   parent_class;
};

typedef void (*LauncherBehaveAlphaFunc) (ClutterBehaviour *behave, 
                                      guint32 		      alpha_value,
                                      gpointer		      data);

GType launcher_behave_get_type (void) G_GNUC_CONST;

ClutterBehaviour* 
launcher_behave_new (ClutterAlpha         *alpha, 
                  LauncherBehaveAlphaFunc  func,
                  gpointer		          data);


#endif /* _LAUNCHER_BEHAVE_H_ */

