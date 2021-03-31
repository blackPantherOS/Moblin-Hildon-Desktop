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

#ifndef _HAVE_LAUNCHER_BACKGROUND_H
#define _HAVE_LAUNCHER_BACKGROUND_H

#include <glib.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define LAUNCHER_TYPE_BACKGROUND (launcher_background_get_type ())

#define LAUNCHER_BACKGROUND(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
        LAUNCHER_TYPE_BACKGROUND, LauncherBackground))

#define LAUNCHER_BACKGROUND_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_CAST ((klass), \
        LAUNCHER_TYPE_BACKGROUND, LauncherBackgroundClass))

#define LAUNCHER_IS_BACKGROUND(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
        LAUNCHER_TYPE_BACKGROUND))

#define LAUNCHER_IS_BACKGROUND_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_TYPE ((klass), LAUNCHER_TYPE_BACKGROUND))

#define LAUNCHER_BACKGROUND_GET_CLASS(obj) \
        (G_TYPE_INSTANCE_GET_CLASS ((obj), \
        LAUNCHER_TYPE_BACKGROUND, LauncherBackgroundClass))

typedef struct _LauncherBackground LauncherBackground;
typedef struct _LauncherBackgroundClass LauncherBackgroundClass;
typedef struct _LauncherBackgroundPrivate LauncherBackgroundPrivate;

struct _LauncherBackground
{
  ClutterGroup         parent; 
  
  /*< private >*/
  LauncherBackgroundPrivate   *priv;
};

struct _LauncherBackgroundClass 
{
  /*< private >*/
  ClutterGroupClass    parent_class;

  /* future padding */
  void (*_launcher_background_1) (void);
  void (*_launcher_background_2) (void);
  void (*_launcher_background_3) (void);
  void (*_launcher_background_4) (void);
};

GType launcher_background_get_type (void) G_GNUC_CONST;

ClutterActor *
launcher_background_new (void);

G_END_DECLS

#endif /* _HAVE_LAUNCHER_BACKGROUND_H */
