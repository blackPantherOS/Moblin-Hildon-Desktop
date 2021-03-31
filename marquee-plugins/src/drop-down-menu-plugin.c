/*
 * Copyright (C) 2007 Intel Corporation
 *
 * Author:  Horace Li <horace.li@intel.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

//#define _GNU_SOURCE
#define _XOPEN_SOURCE 600  //prevent: 'implicit declaration of function 'isascii'
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkevents.h>
#include <ctype.h>  //for isascii (menu title)
#include <X11/Xatom.h>
#include <gconf/gconf-client.h>

#include "drop-down-menu-plugin.h"

#define TEXT_LEFT_MARGIN   30   //so that little arrow shows behind

#define THEME_DIR "/usr/share/themes/mobilebasic"

#define DDM_KEYS_GCONF_PATH  "/apps/marquee-plugins"
#define DDM_GCONF_GROUPS_KEY "/apps/marquee-plugins/categories"
#define DDM_GCONF_CAT_KEY    "/apps/marquee-plugins/active"

#define DROP_DOWN_MENU_PLUGIN_GET_PRIVATE(object) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((object), DROP_DOWN_MENU_TYPE_PLUGIN, DropDownMenuPluginPrivate))

HD_DEFINE_PLUGIN (DropDownMenuPlugin, drop_down_menu_plugin, TASKNAVIGATOR_TYPE_ITEM);

static void drop_down_menu_finalize (GObject *object);
static GdkFilterReturn drop_down_menu_x_event_handler (GdkXEvent *xevent, GdkEvent *event, gpointer data);
void update_app_category (const gchar *cat);
void drop_down_menu_screen_changed (TaskNavigatorItem *item);
static void
gconf_key_changed_callback (GConfClient *client,
			    guint        connection_id,
			    GConfEntry  *entry,
			    gpointer     user_data);
static gboolean home_menu_hide_cb (GtkWidget *widget, gpointer user_data);
static gboolean home_menu_show_cb (GtkWidget *widget, gpointer user_data);

static void updateDownArrow (DropDownMenuPlugin *ddm_plugin, gboolean hasMenuItems) 
{
  // find out if current app has any menu items
/*  Removed until we get the right logic
  if (!hasMenuItems) {
	 gtk_widget_set_size_request (ddm_plugin->priv->arrowImg, 28,52); 
	 gtk_widget_set_child_visible (ddm_plugin->priv->arrowImg, TRUE);
  } else {
	 gtk_widget_set_size_request (ddm_plugin->priv->arrowImg, 12,0);   //moves title over to the left
	 gtk_widget_set_child_visible (ddm_plugin->priv->arrowImg, FALSE); //hides the arrow image widget
  }	
*/
}

static void new_window_title (HDWM *hdwm, DropDownMenuPlugin *ddm_plugin)
{
   HDWMWindow *wnd = NULL;
   const gchar *wnd_title = NULL;
   gint *anid;

   wnd = hd_wm_get_active_window();
   if (wnd) {
      GdkWindow *win_wrapper;

      anid = g_new0 (gint, 1);

      *anid = hd_wm_window_get_x_win(wnd);
      win_wrapper = gdk_window_lookup(*anid);

      gdk_error_trap_push ();

      gdk_window_set_events (win_wrapper, gdk_window_get_events(win_wrapper) | 
                                          GDK_PROPERTY_CHANGE_MASK | 
                                          GDK_STRUCTURE_MASK | 
                                          GDK_VISIBILITY_NOTIFY_MASK);
      gdk_window_add_filter (win_wrapper, drop_down_menu_x_event_handler, ddm_plugin);

      gdk_error_trap_pop();

      wnd_title = hd_wm_window_get_name(wnd);
   }

   if ((NULL == wnd_title) || (strlen(wnd_title) == 0)) {
      /* 
       * We shouldn't ever get here, but just in case, 
       * set something reasonable
       */
       gchar *active = NULL;
       GConfClient *client = gconf_client_get_default ();
       if (!client)
	       return;
       active = gconf_client_get_string(client, DDM_GCONF_CAT_KEY, NULL);
       if (active)
	       gtk_label_set_text ((GtkLabel*)ddm_plugin->priv->label, 
				   gettext(active));
   } else {
      gtk_label_set_text ((GtkLabel*)ddm_plugin->priv->label, gettext(wnd_title));
   }
   gtk_label_set_ellipsize ((GtkLabel*)ddm_plugin->priv->label, PANGO_ELLIPSIZE_END);
}

static GdkFilterReturn drop_down_menu_x_event_handler (GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
    DropDownMenuPlugin *ddm_plugin = (DropDownMenuPlugin *)data;
    XPropertyEvent *property;
    XVisibilityEvent *visibility;
    HDWMWindow *wnd = NULL;
    GHashTable *windows_list = NULL;
    gchar *wnd_title = NULL;

    switch (((XEvent*)xevent)->type)
    {
        case PropertyNotify:
            property = (XPropertyEvent *)xevent;
            if (property->window != GDK_ROOT_WINDOW())
            {
                if(XA_WM_NAME == property->atom)
                {
                    windows_list = hd_wm_get_windows();
                    if (windows_list)
                    {
                        wnd = g_hash_table_lookup (windows_list, (gconstpointer)&property->window);
                        if (wnd)
                        {
                            XFetchName (GDK_DISPLAY(), property->window, &wnd_title);
                            if (wnd_title)
                                hd_wm_window_set_name (wnd, wnd_title);

                            g_signal_emit_by_name (ddm_plugin->priv->hdwm, "active-window-update");
                        }
                    }
                }
            }
            break;
        case VisibilityNotify:
            visibility = (XVisibilityEvent *)xevent;
            if (visibility->window != GDK_ROOT_WINDOW())
            {
                 if (visibility->state == GDK_VISIBILITY_UNOBSCURED)
                     updateDownArrow (ddm_plugin, FALSE);
                 else if (visibility->state == GDK_VISIBILITY_PARTIAL)
                     updateDownArrow (ddm_plugin, TRUE);
            }
            break;
        default:
            break;
    }
    return GDK_FILTER_CONTINUE;
}

/* update window title */
static void update_window_title (HDWM *hdwm, HDWMEntryInfo *info, 
				 DropDownMenuPlugin *ddm_plugin)
{
   HDWMWindow *wnd = NULL;
   HDWMApplication *app = NULL;
   const gchar *wnd_title = NULL;

   if (HD_WM_IS_APPLICATION (info))
   {
      app = HD_WM_APPLICATION (info);
      wnd = hd_wm_application_get_active_window (app);
   }
   else if (HD_WM_IS_WINDOW (info))
   {
      wnd = HD_WM_WINDOW (info);
   }

   if (wnd) {
	   wnd_title = hd_wm_window_get_name(wnd);
   } 
   else 
   {
       /* Desktop is Active */
       GConfClient *c = gconf_client_get_default ();
       if (c)
           wnd_title = gconf_client_get_string(c, DDM_GCONF_CAT_KEY, NULL);
       updateDownArrow (ddm_plugin, FALSE);
   }

   if (!wnd_title)
	   wnd_title = "";

   gtk_label_set_text ((GtkLabel*)ddm_plugin->priv->label, gettext(wnd_title));
   gtk_label_set_ellipsize ((GtkLabel*)ddm_plugin->priv->label, PANGO_ELLIPSIZE_END);
}

static void drop_down_menu_position_func (GtkMenu  *menu,
                                          gint     *x, 
					  gint     *y,
                                          gboolean *push_in, 
					  gpointer  user_data)
{
  DropDownMenuPlugin *ddm_plugin = (DropDownMenuPlugin *) user_data;
  *x = DEFAULT_MARQUEE_PANEL_HEIGHT;
  *y = ddm_plugin->priv->button_height;

  *push_in = FALSE;
}

static GtkWidget *drop_down_menu_create_home_menu(DropDownMenuPlugin *ddm_plugin)
{
   GtkWidget *home_menu;
   GtkWidget *menu_item;
   GConfClient *client;
   GSList *category_names, *list;
   gchar *active = NULL;

   client = gconf_client_get_default ();
   if (!client)
	   return NULL;

   home_menu = gtk_menu_new();
   category_names = gconf_client_get_list (client, 
					   DDM_GCONF_GROUPS_KEY,
					   GCONF_VALUE_STRING, 
					   NULL);
   for (list = category_names; list; list = g_slist_next(list)) {
	   gchar *c = (gchar *)list->data;

	   menu_item = gtk_menu_item_new_with_label (gettext(c));
	   g_signal_connect_swapped (menu_item, "activate",
				     G_CALLBACK(update_app_category), c);
	   gtk_menu_shell_append (GTK_MENU_SHELL (home_menu), menu_item);
	   gtk_widget_show (menu_item);
   }

   /* Add a special entry for "All" applications */
   menu_item = gtk_menu_item_new_with_label (gettext("All"));
   g_signal_connect_swapped (menu_item, "activate",
			     G_CALLBACK(update_app_category), "All");
   gtk_menu_shell_append (GTK_MENU_SHELL (home_menu), menu_item);
   gtk_widget_show (menu_item);

   active = gconf_client_get_string(client, DDM_GCONF_CAT_KEY, NULL);
   if (!active && category_names)
	   gconf_client_set_string(client, DDM_GCONF_CAT_KEY, 
				   (char *)category_names->data, 
				   NULL);
   
   return home_menu;
}

static gboolean home_menu_hide_cb (GtkWidget *widget, gpointer user_data)
{
  DropDownMenuPlugin *ddm_plugin = (DropDownMenuPlugin *)user_data;

  updateDownArrow (ddm_plugin, FALSE);
  return FALSE;
}

static gboolean home_menu_show_cb (GtkWidget *widget, gpointer user_data)
{
  DropDownMenuPlugin *ddm_plugin = (DropDownMenuPlugin *)user_data;

  updateDownArrow (ddm_plugin, TRUE);
  return FALSE;
}

/* btn clicked */
static void on_clicked (GtkWidget *button, DropDownMenuPlugin *ddm_plugin)
{
    HDWMWindow *wnd = hd_wm_get_active_window();
    if (wnd) {
       Atom atom;
       Display *dpy = NULL;
       Window xwin;
       XEvent ev;
       memset(&ev, 0, sizeof(ev));

       atom = hd_wm_get_atom(HD_ATOM_MB_GRAB_TRANSFER);
       xwin = hd_wm_window_get_x_win(wnd);
       dpy = hd_wm_window_get_display(wnd);

       ev.xclient.type = ClientMessage;
       ev.xclient.window = xwin;
       ev.xclient.message_type = atom;
       ev.xclient.format = 32;
       ev.xclient.data.l[0] = 0;
       ev.xclient.data.l[1] = 0;
       ev.xclient.data.l[2] = 0;
       ev.xclient.data.l[3] = 0;
       ev.xclient.data.l[4] = 0;
   
       if (NULL != dpy) {
           XSendEvent(dpy, xwin, False, NoEventMask, &ev);
	   XSync(dpy, False);
       }
    } else {
        GtkWidget *home_menu;

	home_menu = drop_down_menu_create_home_menu (ddm_plugin);
        g_signal_connect (G_OBJECT (home_menu), "hide",
                          G_CALLBACK (home_menu_hide_cb), ddm_plugin);
        g_signal_connect (G_OBJECT (home_menu), "show", 
                          G_CALLBACK (home_menu_show_cb), ddm_plugin);
	gtk_menu_popup (GTK_MENU(home_menu),
			NULL, NULL,
			drop_down_menu_position_func, ddm_plugin,
			0,
			gtk_get_current_event_time());
    }
}

static void drop_down_menu_plugin_init (DropDownMenuPlugin *ddm_plugin)
{
  DropDownMenuPluginPrivate *priv;
  GtkWidget *btn, *label, *hbox, *arrowImg;
  HDWM *hdwm;
  gint panel_height, scn_width = 800, plugin_width;
  GConfClient *client;
  gchar *active = NULL;

  client = gconf_client_get_default ();
  if (!client)
    return;

  panel_height = plugins_get_marquee_panel_height ();

  ddm_plugin->priv = DROP_DOWN_MENU_PLUGIN_GET_PRIVATE (ddm_plugin);
  priv = ddm_plugin->priv;

  hdwm = hd_wm_get_singleton ();
  g_object_ref (hdwm);

  btn = gtk_button_new();
  GdkScreen *screen = gtk_widget_get_screen (GTK_WIDGET(ddm_plugin));
  if (screen != NULL) {
    scn_width = gdk_screen_get_width (screen);
  }

  if (scn_width > 600)
    plugin_width = 52 * (3 + scn_width / 320);
  else
    plugin_width = scn_width - 52 * (5 + scn_width / 320);

  gtk_widget_set_size_request (GTK_WIDGET(ddm_plugin), plugin_width, panel_height);

  // Create the arrow image
  arrowImg = gtk_image_new_from_file (THEME_DIR "/images/mb_marquee_btn_title_arrow.png");

  // Create label and set font
  label = gtk_label_new (NULL);

  //set the font size smaller
  PangoAttribute *pa_size = pango_attr_size_new (17000);
  pa_size->start_index = 0;
  pa_size->end_index = -1;
  PangoAttribute *pa_font = pango_attr_family_new ("Sans");
  pa_font->start_index = 0;
  pa_font->end_index = -1;
  PangoAttribute *pa_color = pango_attr_foreground_new (0xE2E1,0xE2E1,0xE2E1);
  pa_color->start_index = 0;
  pa_color->end_index = -1;
  PangoAttrList *pl = pango_attr_list_new();
  pango_attr_list_insert(pl,pa_size);
  pango_attr_list_insert(pl,pa_font);
  pango_attr_list_insert(pl,pa_color);
  
  gtk_label_set_attributes ((GtkLabel*)label, pl);
  pango_attr_list_unref (pl);

  //gtk_label_set_justify ((GtkLabel*)label, GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment((GtkMisc*)label, 0.0, 0.5);

  //use hbox containing arrowImg and label 
  hbox = gtk_hbox_new(FALSE,0);
  gtk_box_pack_start  ((GtkBox*)hbox, arrowImg, FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER(hbox), label);
  gtk_container_add (GTK_CONTAINER (btn), hbox);
  
  priv->hdwm = hdwm;
  priv->btn = btn;
  priv->arrowImg = arrowImg;  //needed so we can hide it if no menu
  priv->label = label;
  priv->button_height = panel_height;

  g_signal_connect (btn, "clicked", G_CALLBACK (on_clicked), (gpointer)ddm_plugin);
  g_signal_connect (hdwm, "active-window-update", G_CALLBACK (new_window_title), (gpointer)ddm_plugin);
  g_signal_connect (hdwm, "entry_info_stack_changed", G_CALLBACK (update_window_title), (gpointer)ddm_plugin);

  gtk_widget_show_all (btn);

  //setting name (for theme) doesn't help as Hildon overrides first 3 buttons
  gtk_widget_set_name (priv->btn, MARQUEE_DROPDOWN);

  gtk_container_add (GTK_CONTAINER (ddm_plugin), btn);

  gconf_client_add_dir (client, DDM_KEYS_GCONF_PATH, GCONF_CLIENT_PRELOAD_NONE, NULL);
  gconf_client_notify_add (client, DDM_KEYS_GCONF_PATH, gconf_key_changed_callback, ddm_plugin, NULL, NULL);

  active = gconf_client_get_string (client, DDM_GCONF_CAT_KEY, NULL);
  if (!active)
    gconf_client_set_string(client, DDM_GCONF_CAT_KEY, "All", NULL);
}

void drop_down_menu_screen_changed (TaskNavigatorItem *item)
{
  GdkScreen *screen = gdk_screen_get_default ();
  gint scn_width = 800, width, height;
  GList *children, *iter;
  
  scn_width = gdk_screen_get_width (screen);
  if (scn_width > 600)
    width = 52 * (3 + scn_width / 320);
  else
    width = scn_width - 52 * (5 + scn_width / 320);
  height = plugins_get_marquee_panel_height ();

  children = gtk_container_get_children (GTK_CONTAINER(item));
  for (iter = children ; iter ; iter = g_list_next (iter))
  {
    if (GTK_IS_WIDGET (iter->data))
    {
      gtk_widget_set_size_request (GTK_WIDGET(iter->data), width, height);
    }
  }

  gtk_widget_set_size_request (GTK_WIDGET(item), width, height);
}

static void drop_down_menu_plugin_class_init (DropDownMenuPluginClass *class)
{
   GObjectClass *object_class = G_OBJECT_CLASS (class);
   TaskNavigatorItemClass *item_class = TASKNAVIGATOR_ITEM_CLASS (class);

   item_class->screen_changed = drop_down_menu_screen_changed;
   object_class->finalize = drop_down_menu_finalize;

   g_type_class_add_private (object_class, sizeof (DropDownMenuPluginPrivate));
}

static void drop_down_menu_finalize (GObject *object)
{
   DropDownMenuPlugin *ddm_plugin = DROP_DOWN_MENU_PLUGIN (object);	
   DropDownMenuPluginPrivate *priv = DROP_DOWN_MENU_PLUGIN_GET_PRIVATE (ddm_plugin);

   g_object_unref (priv->hdwm);
   G_OBJECT_CLASS (drop_down_menu_plugin_parent_class)->finalize (object);
}

void update_app_category (const gchar *cat)
{
   GConfClient *client = gconf_client_get_default ();
   if (!client)
	   return;

    gconf_client_set_string(client, DDM_GCONF_CAT_KEY, cat, NULL);
}

static void
gconf_key_changed_callback (GConfClient *client,
			    guint        connection_id,
			    GConfEntry  *entry,
			    gpointer     user_data)
{
    DropDownMenuPlugin *ddm_plugin = (DropDownMenuPlugin *)user_data;
    GConfValue *gvalue = NULL;
    const char *gkey = NULL;

    gkey = gconf_entry_get_key(entry);
    gvalue = gconf_entry_get_value(entry);

    if (!ddm_plugin || !ddm_plugin->priv || !gkey || !gvalue)
	    return;

    if (g_str_equal(gkey, DDM_GCONF_CAT_KEY)) {
	    GtkLabel *label = (GtkLabel *)(ddm_plugin->priv->label);
	    if (label)
		    gtk_label_set_text (label, gconf_value_get_string(gvalue));
    }
}
