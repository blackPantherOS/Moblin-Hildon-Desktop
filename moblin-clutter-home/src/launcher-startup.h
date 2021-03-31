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

#ifndef _HAVE_LAUNCHER_STARTUP_H
#define _HAVE_LAUNCHER_STARTUP_H

#include <glib.h>
#include "launcher-item.h"

G_BEGIN_DECLS

#define LAUNCHER_TYPE_STARTUP (launcher_startup_get_type ())

#define LAUNCHER_STARTUP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
        LAUNCHER_TYPE_STARTUP, LauncherStartup))

#define LAUNCHER_STARTUP_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), \
        LAUNCHER_TYPE_STARTUP, LauncherStartupClass))

#define LAUNCHER_IS_STARTUP(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
        LAUNCHER_TYPE_STARTUP))

#define LAUNCHER_IS_STARTUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
        LAUNCHER_TYPE_STARTUP))

#define LAUNCHER_STARTUP_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), \
        LAUNCHER_TYPE_STARTUP, LauncherStartupClass))

typedef struct _LauncherStartup LauncherStartup;
typedef struct _LauncherStartupClass LauncherStartupClass;
typedef struct _LauncherStartupPrivate LauncherStartupPrivate;

struct _LauncherStartup
{
  GObject         parent;

  /*< private >*/
  LauncherStartupPrivate   *priv;
};

struct _LauncherStartupClass 
{
  /*< private >*/
  GObjectClass    parent_class;
  
 /* future padding */
  void (*_launcher_startup_1) (void);
  void (*_launcher_startup_2) (void);
  void (*_launcher_startup_3) (void);
  void (*_launcher_startup_4) (void);
};

GType launcher_startup_get_type (void) G_GNUC_CONST;

LauncherStartup*
launcher_startup_get_default (void);

gboolean
launcher_startup_launch_item (LauncherStartup *startup, LauncherItem *item);

G_END_DECLS

#endif /* _HAVE_LAUNCHER_STARTUP_H */
