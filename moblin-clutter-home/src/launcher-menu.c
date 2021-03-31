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

#define GMENU_I_KNOW_THIS_IS_UNSTABLE 1
#include <gmenu-tree.h>

#include "launcher-menu.h"
#include "launcher-item.h"

G_DEFINE_TYPE (LauncherMenu, launcher_menu, G_TYPE_OBJECT)

#define LAUNCHER_MENU_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE(obj, \
        LAUNCHER_TYPE_MENU, LauncherMenuPrivate))

struct _LauncherMenuPrivate
{
  GList      *applications;
  GList      *categories;
  GList      *menu_apps;
  GMenuTree *app_tree;
  GMenuTree *sys_tree;

  guint      tag;
};

struct _LauncherMenuCategory
{
  GList *applications;
  GdkPixbuf *pixbuf;

  gchar *name;
  gchar *comment;
  gchar *icon;
};

struct _LauncherMenuApplication
{
  LauncherMenuCategory *category;
  GdkPixbuf *pixbuf;
  ClutterActor *item;

  gchar *name;
  gchar *comment;
  gchar *icon;
  gchar *exec;
  gchar *path;
};

enum 
{
  MENU_CHANGED,

  LAST_SIGNAL
};

static guint _menu_signals[LAST_SIGNAL] = { 0 };

/* Forwards */
static void tree_changed (GMenuTree *tree, LauncherMenu *menu);

/* Utility functions */

/* From matchbox-desktop */
static char *
strip_extension (const char *file)
{
        char *stripped, *p;

        stripped = g_strdup (file);

        p = strrchr (stripped, '.');
        if (p &&
            (!strcmp (p, ".png") ||
             !strcmp (p, ".svg") ||
             !strcmp (p, ".xpm")))
	        *p = 0;

        return stripped;
}

/* Gets the pixbuf from a desktop file's icon name. Based on the same function
 * from matchbox-desktop
 */
static GdkPixbuf *
get_icon (const gchar *name, guint size)
{
  static GtkIconTheme *theme = NULL;
  GdkPixbuf *pixbuf = NULL;
  GError *error = NULL;
  gchar *stripped = NULL;

  gint width, height;

  if (theme == NULL)
    theme = gtk_icon_theme_get_default ();

  if (name == NULL)
  {
    pixbuf = gtk_icon_theme_load_icon (theme, "application-x-executable",
                                       size, 0, NULL);
    return pixbuf;
  }

  if (g_path_is_absolute (name))
  {
    if (g_file_test (name, G_FILE_TEST_EXISTS))
    {
      pixbuf = gdk_pixbuf_new_from_file_at_scale (name, size, size, 
                                                  TRUE, &error);
      if (error)
      {
        /*g_warning ("Error loading icon: %s\n", error->message);*/
        g_error_free (error);
        error = NULL;
     }
      return pixbuf;
    } 
  }

  stripped = strip_extension (name);
  
  pixbuf = gtk_icon_theme_load_icon (theme,
                                     stripped,
                                     size,
                                     0, &error);
  if (error)
  {   
    /*g_warning ("Error loading icon: %s\n", error->message);*/
    g_error_free (error);
    error = NULL;
  }
  
  /* Always try and send back something */
  if (pixbuf == NULL)
    pixbuf = gtk_icon_theme_load_icon (theme, "application-x-executable",
                                       size, 0, NULL);
  
  width = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);

  if (width != size || height != size)
  {
    GdkPixbuf *temp = pixbuf;
    pixbuf = gdk_pixbuf_scale_simple (temp, 
                                      size,
                                      size,
                                      GDK_INTERP_HYPER);
    g_object_unref (temp);
  }

  g_free (stripped);

 return pixbuf;
}

/* Public functions */
GList*
launcher_menu_get_categories (LauncherMenu *menu)
{
  g_return_val_if_fail (LAUNCHER_IS_MENU (menu), NULL);
  return menu->priv->categories;
}

GList*
launcher_menu_get_applications (LauncherMenu *menu)
{
  g_return_val_if_fail (LAUNCHER_IS_MENU (menu), NULL);
  return menu->priv->applications;
}

const gchar *
launcher_menu_category_get_name (LauncherMenuCategory *category)
{
  g_return_val_if_fail (category, NULL);

  return category->name;
}

const gchar *
launcher_menu_category_get_comment (LauncherMenuCategory *category)
{
  g_return_val_if_fail (category, NULL);

  return category->comment;
}

GdkPixbuf*
launcher_menu_category_get_icon (LauncherMenuCategory *category)
{
  g_return_val_if_fail (category, NULL);

  return category->pixbuf;
}

GList*
launcher_menu_category_get_applications (LauncherMenuCategory *category)
{
  g_return_val_if_fail (category, NULL);

  return category->applications;

}

GdkPixbuf *
launcher_menu_category_get_pixbuf (LauncherMenuCategory *category,
                                   guint                 size)
{
  const gchar *name;

  g_return_val_if_fail (category, NULL);

  name = category->icon;

  return get_icon (name, size);
}
  
const gchar *
launcher_menu_application_get_name (LauncherMenuApplication *application)
{
  g_return_val_if_fail (application, NULL);

  return application->name;
}


const gchar *
launcher_menu_application_get_comment (LauncherMenuApplication *application)
{
  g_return_val_if_fail (application, NULL);

  return application->comment;
}

GdkPixbuf*
launcher_menu_application_get_icon (LauncherMenuApplication *application)
{
  g_return_val_if_fail (application, NULL);
  return application->pixbuf;
}

LauncherMenuCategory*
launcher_menu_application_get_category (LauncherMenuApplication *application)
{
  g_return_val_if_fail (application, NULL);

  return application->category;
}

GdkPixbuf *
launcher_menu_application_get_pixbuf (LauncherMenuApplication *application,
                                      guint                    size)
{
  const gchar *name;

  g_return_val_if_fail (application, NULL);

  name = application->icon;

  return get_icon (name, size);
 
}

const gchar*
launcher_menu_application_get_exec (LauncherMenuApplication *application)
{
  g_return_val_if_fail (application, NULL);

  return application->exec;
}

const gchar *
launcher_menu_application_get_desktop_filename (LauncherMenuApplication *application)
{
  g_return_val_if_fail (application, NULL);

  return application->exec;
}

/* Private */

static LauncherMenuCategory*
make_category (LauncherMenu *menu, GMenuTreeDirectory *dir)
{
  LauncherMenuPrivate *priv = menu->priv;
  LauncherMenuCategory *category = NULL;

  category = g_slice_new0 (LauncherMenuCategory);
  category->name = g_strdup (gmenu_tree_directory_get_name (dir));
  category->comment = g_strdup (gmenu_tree_directory_get_comment (dir));
  category->icon = g_strdup (gmenu_tree_directory_get_icon (dir));
  category->applications = NULL;
  category->pixbuf = get_icon (gmenu_tree_directory_get_icon (dir),
                               LAUNCHER_ITEM_WIDTH ());

  priv->categories = g_list_append (priv->categories, (gpointer)category);

  return category;
} 

/*
 * Each application has a pointer to its category, and gets added to the 
 * categories application list and the main applicaton list 
 */
static void
make_application (LauncherMenu *menu, 
                  GMenuTreeEntry *entry, 
                  LauncherMenuCategory *category)
{
  LauncherMenuPrivate *priv = menu->priv;
  LauncherMenuApplication *app = NULL;
  ClutterActor *item;

  app = g_slice_new0 (LauncherMenuApplication);
  app->name = g_strdup (gmenu_tree_entry_get_name (entry));
  app->comment = g_strdup (gmenu_tree_entry_get_comment (entry));
  app->icon = g_strdup (gmenu_tree_entry_get_icon (entry));
  app->exec = g_strdup (gmenu_tree_entry_get_exec (entry));
  app->path = g_strdup (gmenu_tree_entry_get_desktop_file_path (entry));
  app->category = category;
  app->pixbuf = get_icon (gmenu_tree_entry_get_icon (entry),
                          LAUNCHER_ITEM_WIDTH ());

  item = launcher_item_new_from_application (app);
  app->item = item;
  g_object_ref (item);
  g_object_ref (item);
  priv->applications = g_list_append (priv->applications, (gpointer)item);
  category->applications = g_list_append (category->applications,
                                          (gpointer)item);
  priv->menu_apps = g_list_append (priv->menu_apps, (gpointer)app);
 
} 

/* 
 * Traverse through the root tree, treating each directory as a category, and
 * each entry as an application. We only want 1st tier categories, so we pass
 * a 'category' variable to the function, which, if present, blocks the
 * 2nd tier directory from becoming a new category, and uses it's parent as the
 * category.
 */
static void
load_menu_from_directory (LauncherMenu *menu, 
                          GMenuTreeDirectory *dir,
                          LauncherMenuCategory *category)
{
  GSList *list, *l;
  LauncherMenuCategory *root = NULL;

  if (dir == NULL)
    return;

  list = gmenu_tree_directory_get_contents (dir);
  for (l = list; l; l = l->next)
  {
    GMenuTreeItem *item = (GMenuTreeItem*)l->data;

    switch (gmenu_tree_item_get_type (item))
    {
      case GMENU_TREE_ITEM_DIRECTORY:
        
        if (!category)
        {
          load_menu_from_directory (menu, 
                                   GMENU_TREE_DIRECTORY (item),
                                   make_category (menu,
                                                  GMENU_TREE_DIRECTORY (item)));

        }
        else
          load_menu_from_directory (menu, 
                                    GMENU_TREE_DIRECTORY (item), category);
        break;
      case GMENU_TREE_ITEM_ENTRY:
        if (category)
          make_application (menu, GMENU_TREE_ENTRY (item), category);
        else
        {
          if (root == NULL)
            root = make_category (menu, dir);
          make_application (menu, GMENU_TREE_ENTRY (item), root);        
        }

        break;
        
      default:
        break;
    }
    gmenu_tree_item_unref (item);
  }  
  g_slist_free (list);
}

static GMenuTree *
load_menu_from_tree (LauncherMenu *menu, const gchar *name)
{
  LauncherMenuPrivate *priv;
  GMenuTreeDirectory *root = NULL;
  GMenuTree *tree = NULL;

  g_return_val_if_fail (LAUNCHER_IS_MENU (menu), NULL);
  priv = menu->priv;
 
  tree = gmenu_tree_lookup (name, GMENU_TREE_FLAGS_NONE);
  if (!tree)
  {
    g_warning ("Unable to find %s", name);
    return NULL;
  }
  root = gmenu_tree_get_root_directory (tree);
  load_menu_from_directory (menu, root, NULL); 
  gmenu_tree_item_unref (root);

  gmenu_tree_add_monitor (tree, (GMenuTreeChangedFunc)tree_changed, menu);

  return tree;
}

static void
free_menu (LauncherMenu *menu)
{
  LauncherMenuPrivate *priv;
  GList *l;

  g_return_if_fail (LAUNCHER_IS_MENU (menu));
  priv = menu->priv;

  for (l = priv->menu_apps; l; l = l->next)
  {
    LauncherMenuApplication *app = l->data;
    
    g_free (app->name);
    g_free (app->comment);
    g_free (app->icon);
    g_free (app->exec);
    g_free (app->path);
    g_object_unref (app->pixbuf);
    clutter_actor_destroy (app->item); 
    
    g_slice_free (LauncherMenuApplication, app);
    app = NULL;
  }
  g_list_free (priv->menu_apps);
  g_list_free (priv->applications);
  priv->applications = priv->menu_apps = NULL;

  for (l = priv->categories; l; l = l->next)
  {
    LauncherMenuCategory *cat = l->data;
    
    g_free (cat->name);
    g_free (cat->comment);
    g_free (cat->icon);
    g_list_free (cat->applications);
    g_object_unref (cat->pixbuf);
    g_slice_free (LauncherMenuCategory, cat);
    cat = NULL;
  }
  g_list_free (priv->categories);
  priv->categories = NULL;

}

static gboolean
list_changed (LauncherMenu *menu)
{
  LauncherMenuPrivate *priv;
  GMenuTreeDirectory *root;

  g_return_val_if_fail (LAUNCHER_IS_MENU (menu), FALSE);
  priv = menu->priv;

  free_menu (menu);

  root = gmenu_tree_get_root_directory (priv->app_tree);
  load_menu_from_directory (menu, root, NULL);
  gmenu_tree_item_unref (root);

  root = gmenu_tree_get_root_directory (priv->sys_tree);
  load_menu_from_directory (menu, root, NULL);
  gmenu_tree_item_unref (root);
  
  g_signal_emit (menu, _menu_signals[MENU_CHANGED], 0);
  menu->priv->tag = 0;  
  
  return FALSE;
}

static void
tree_changed (GMenuTree *tree, LauncherMenu *menu)
{
  LauncherMenuPrivate *priv;
  GMenuTreeDirectory *root;
  
  g_return_if_fail (LAUNCHER_IS_MENU (menu));
  priv = menu->priv;

  root = gmenu_tree_get_root_directory (tree);
  gmenu_tree_item_unref (root);  
  
  if (priv->tag)
  {
    return;
  }
  priv->tag = g_timeout_add (500, (GSourceFunc)list_changed, menu);
}

/* GObject functions */
static void
launcher_menu_dispose (GObject *object)
{
  free_menu (LAUNCHER_MENU (object));

  G_OBJECT_CLASS (launcher_menu_parent_class)->dispose (object);
}

static void
launcher_menu_finalize (GObject *menu)
{
  LauncherMenuPrivate *priv;
  
  g_return_if_fail (LAUNCHER_IS_MENU (menu));
  priv = LAUNCHER_MENU (menu)->priv;

  G_OBJECT_CLASS (launcher_menu_parent_class)->finalize (menu);
}


static void
launcher_menu_class_init (LauncherMenuClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class->finalize = launcher_menu_finalize;
  obj_class->dispose = launcher_menu_dispose;

   _menu_signals[MENU_CHANGED] = 
      g_signal_new ("menu-changed",
                  G_OBJECT_CLASS_TYPE (obj_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LauncherMenuClass, menu_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

 
  g_type_class_add_private (obj_class, sizeof (LauncherMenuPrivate)); 
}

static void
launcher_menu_init (LauncherMenu *menu)
{
  LauncherMenuPrivate *priv;
    
  priv = menu->priv = LAUNCHER_MENU_GET_PRIVATE (menu);

  priv->applications = NULL;
  priv->categories = NULL;
  priv->menu_apps = NULL;
  priv->tag = 0;

  priv->app_tree = load_menu_from_tree (menu, "applications.menu");
  priv->sys_tree = load_menu_from_tree (menu, "settings.menu");
}

LauncherMenu*
launcher_menu_get_default (void)
{
  static LauncherMenu *menu = NULL;
  
  if (menu == NULL)
    menu = g_object_new (LAUNCHER_TYPE_MENU, 
                         NULL);

  return menu;
}
