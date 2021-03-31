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

#ifndef _HAVE_LAUNCHER_MENU_H
#define _HAVE_LAUNCHER_MENU_H

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define LAUNCHER_TYPE_MENU (launcher_menu_get_type ())

#define LAUNCHER_MENU(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
        LAUNCHER_TYPE_MENU, LauncherMenu))

#define LAUNCHER_MENU_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), \
        LAUNCHER_TYPE_MENU, LauncherMenuClass))

#define LAUNCHER_IS_MENU(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
        LAUNCHER_TYPE_MENU))

#define LAUNCHER_IS_MENU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
        LAUNCHER_TYPE_MENU))

#define LAUNCHER_MENU_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), \
        LAUNCHER_TYPE_MENU, LauncherMenuClass))

typedef struct _LauncherMenu LauncherMenu;
typedef struct _LauncherMenuClass LauncherMenuClass;
typedef struct _LauncherMenuPrivate LauncherMenuPrivate;
typedef struct _LauncherMenuCategory LauncherMenuCategory;
typedef struct _LauncherMenuApplication LauncherMenuApplication;

struct _LauncherMenu
{
  GObject         parent;

  /*< private >*/
  LauncherMenuPrivate   *priv;
};

struct _LauncherMenuClass 
{
  /*< private >*/
  GObjectClass    parent_class;

  /*< public >*/
  void (*menu_changed) (LauncherMenu *menu);
  
 /*< private >*/
  void (*_launcher_menu_1) (void);
  void (*_launcher_menu_2) (void);
  void (*_launcher_menu_3) (void);
  void (*_launcher_menu_4) (void);
};

GType launcher_menu_get_type (void) G_GNUC_CONST;

LauncherMenu*
launcher_menu_get_default (void);

GList*
launcher_menu_get_categories (LauncherMenu *menu);

GList*
launcher_menu_get_applications (LauncherMenu *menu);

/* Category functions */
const gchar *
launcher_menu_category_get_name (LauncherMenuCategory *category);

const gchar *
launcher_menu_category_get_comment (LauncherMenuCategory *category);

GdkPixbuf*
launcher_menu_category_get_icon (LauncherMenuCategory *category);

GList*
launcher_menu_category_get_applications (LauncherMenuCategory *category);

GdkPixbuf *
launcher_menu_category_get_pixbuf (LauncherMenuCategory *category,
                                      guint size);

/* Application functions */
const gchar *
launcher_menu_application_get_name (LauncherMenuApplication *application);

const gchar *
launcher_menu_application_get_comment (LauncherMenuApplication *application);

GdkPixbuf*
launcher_menu_application_get_icon (LauncherMenuApplication *application);

LauncherMenuCategory*
launcher_menu_application_get_category (LauncherMenuApplication *application);

GdkPixbuf *
launcher_menu_application_get_pixbuf (LauncherMenuApplication *application,
                                      guint size);

const gchar*
launcher_menu_application_get_exec (LauncherMenuApplication *application);

const gchar *
launcher_menu_application_get_desktop_filename (LauncherMenuApplication *application);

G_END_DECLS

#endif /* _HAVE_LAUNCHER_MENU_H */
