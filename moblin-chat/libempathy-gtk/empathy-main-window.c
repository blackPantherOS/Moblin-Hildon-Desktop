/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2007 Collabora Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 * 
 * Authors: Xavier Claessens <xclaesse@gmail.com>
 */

#include <config.h>

#include <sys/stat.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <glib/gi18n.h>

#include <libempathy/empathy-conf.h>
#include <libempathy/empathy-contact.h>
#include <libempathy/empathy-debug.h>
#include <libempathy/empathy-utils.h>
#include <libempathy/empathy-chatroom-manager.h>
#include <libempathy/empathy-chatroom.h>
#include <libempathy/empathy-contact-list.h>
#include <libempathy/empathy-contact-manager.h>
#ifdef USE_HILDON
#include <hildon/hildon-program.h>
#include <hildon/hildon-window.h>
#include <glade/glade-build.h>
#include <moko-finger-scroll.h>
#endif /* def USE_HILDON */

#include "empathy-main-window.h"
#include "empathy-contact-dialogs.h"
#include "ephy-spinner.h"
#include "empathy-contact-list-store.h"
#include "empathy-contact-list-view.h"
#ifndef USE_HILDON
#include "empathy-presence-chooser.h"
#endif
#include "empathy-ui-utils.h"
#include "empathy-status-presets.h"
#include "empathy-geometry.h"
#include "empathy-preferences.h"
#include "empathy-accounts-dialog.h"
#include "empathy-about-dialog.h"
#include "empathy-new-chatroom-dialog.h"
#include "empathy-chatrooms-window.h"
#include "empathy-log-window.h"
#include "empathy-gtk-enum-types.h"

#define DEBUG_DOMAIN "MainWindow"

/* Minimum width of roster window if something goes wrong. */
#define MIN_WIDTH 50

/* Accels (menu shortcuts) can be configured and saved */
#define ACCELS_FILENAME "accels.txt"

/* Name in the geometry file */
#define GEOMETRY_NAME "main-window"

typedef struct {
	EmpathyContactListView  *list_view;
	EmpathyContactListStore *list_store;
	MissionControl         *mc;
	EmpathyChatroomManager  *chatroom_manager;

	/* Main widgets */
	GtkWidget              *window;
	GtkWidget              *main_vbox;

	/* Tooltips for all widgets */
#ifndef USE_HILDON
	GtkTooltips            *tooltips;
#endif

	/* Menu widgets */
#ifdef USE_HILDON
        GtkWidget              *main_menu;
        GtkWidget              *errors_vbox;
        GtkWidget              *errors_label;
#endif
	GtkWidget              *room;
	GtkWidget              *room_menu;
	GtkWidget              *room_sep;
	GtkWidget              *room_join_favorites;
	GtkWidget              *edit_context;
	GtkWidget              *edit_context_separator;

	/* Throbber */
#ifndef USE_HILDON
	GtkWidget              *throbber;
#endif

#ifdef USE_HILDON
	/* Button */
	GtkWidget	       *button_add;
	GtkWidget	       *button_remove;
#endif

	/* Widgets that are enabled when there is... */
	GList                  *widgets_connected;	/* ... connected accounts */
	GList                  *widgets_disconnected;	/* ... disconnected accounts */

	/* Status popup */
#ifndef USE_HILDON
	GtkWidget              *presence_toolbar;
	GtkWidget              *presence_chooser;
#endif
	/* Misc */
	guint                   size_timeout_id;
} EmpathyMainWindow;

static void     main_window_destroy_cb                         (GtkWidget                       *widget,
								EmpathyMainWindow               *window);
static void     main_window_favorite_chatroom_menu_setup       (EmpathyMainWindow               *window);
static void     main_window_favorite_chatroom_menu_added_cb    (EmpathyChatroomManager           *manager,
								EmpathyChatroom                  *chatroom,
								EmpathyMainWindow               *window);
static void     main_window_favorite_chatroom_menu_removed_cb  (EmpathyChatroomManager           *manager,
								EmpathyChatroom                  *chatroom,
								EmpathyMainWindow               *window);
static void     main_window_favorite_chatroom_menu_activate_cb (GtkMenuItem                     *menu_item,
								EmpathyChatroom                  *chatroom);
static void     main_window_favorite_chatroom_menu_update      (EmpathyMainWindow               *window);
static void     main_window_favorite_chatroom_menu_add         (EmpathyMainWindow               *window,
								EmpathyChatroom                  *chatroom);
static void     main_window_favorite_chatroom_join             (EmpathyChatroom                  *chatroom);
static void     main_window_chat_quit_cb                       (GtkWidget                       *widget,
								EmpathyMainWindow               *window);
static void     main_window_chat_new_message_cb                (GtkWidget                       *widget,
								EmpathyMainWindow               *window);
static void     main_window_chat_history_cb                    (GtkWidget                       *widget,
								EmpathyMainWindow               *window);
static void     main_window_room_join_new_cb                   (GtkWidget                       *widget,
								EmpathyMainWindow               *window);
static void     main_window_room_join_favorites_cb             (GtkWidget                       *widget,
								EmpathyMainWindow               *window);
static void     main_window_room_manage_favorites_cb           (GtkWidget                       *widget,
								EmpathyMainWindow               *window);
static void     main_window_chat_add_contact_cb                (GtkWidget                       *widget,
								EmpathyMainWindow               *window);
static void     main_window_chat_show_offline_cb               (GtkCheckMenuItem                *item,
								EmpathyMainWindow               *window);
static gboolean main_window_edit_button_press_event_cb         (GtkWidget                       *widget,
								GdkEventButton                  *event,
								EmpathyMainWindow               *window);
static void     main_window_edit_accounts_cb                   (GtkWidget                       *widget,
								EmpathyMainWindow               *window);
static void     main_window_edit_personal_information_cb       (GtkWidget                       *widget,
								EmpathyMainWindow               *window);
static void     main_window_edit_preferences_cb                (GtkWidget                       *widget,
								EmpathyMainWindow               *window);
static void     main_window_help_about_cb                      (GtkWidget                       *widget,
								EmpathyMainWindow               *window);
#ifndef USE_HILDON
static void     main_window_help_contents_cb                   (GtkWidget                       *widget,
								EmpathyMainWindow               *window);
static gboolean main_window_throbber_button_press_event_cb     (GtkWidget                       *throbber_ebox,
								GdkEventButton                  *event,
								EmpathyMainWindow               *window);
#endif
static void     main_window_status_changed_cb                  (MissionControl                  *mc,
								TelepathyConnectionStatus        status,
								McPresence                       presence,
								TelepathyConnectionStatusReason  reason,
								const gchar                     *unique_name,
								EmpathyMainWindow               *window);
static void     main_window_update_status                      (EmpathyMainWindow               *window);
static void     main_window_accels_load                        (void);
static void     main_window_accels_save                        (void);
static void     main_window_connection_items_setup             (EmpathyMainWindow               *window,
								GladeXML                        *glade);
static gboolean main_window_configure_event_timeout_cb         (EmpathyMainWindow               *window);
static gboolean main_window_configure_event_cb                 (GtkWidget                       *widget,
								GdkEventConfigure               *event,
								EmpathyMainWindow               *window);
static void     main_window_notify_show_offline_cb             (EmpathyConf                      *conf,
								const gchar                     *key,
								gpointer                         check_menu_item);
static void     main_window_notify_show_avatars_cb             (EmpathyConf                      *conf,
								const gchar                     *key,
								EmpathyMainWindow               *window);
static void     main_window_notify_compact_contact_list_cb     (EmpathyConf                      *conf,
								const gchar                     *key,
								EmpathyMainWindow               *window);
static void     main_window_notify_sort_criterium_cb           (EmpathyConf                      *conf,
								const gchar                     *key,
								EmpathyMainWindow               *window);
#ifdef USE_HILDON
static void     main_window_add_clicked_cb                     (GtkWidget		        *button,
							        EmpathyMainWindow               *window);
static void     main_window_remove_clicked_cb                  (GtkWidget		        *button,
							        EmpathyMainWindow               *window);
#endif


#ifdef USE_HILDON
static GtkWidget* 
glade_hildon_window_new(GladeXML *xml, GType type, GladeWidgetInfo *info)
{
        return hildon_window_new();
}
#endif

GtkWidget *
empathy_main_window_show (void)
{
	static EmpathyMainWindow *window = NULL;
	EmpathyContactList       *list_iface;
	GladeXML                 *glade;
	EmpathyConf               *conf;
	GtkWidget                *sw;
	GtkWidget                *show_offline_widget;
#ifndef USE_HILDON
	GtkWidget                *ebox;
	GtkToolItem              *item;
	gchar                    *str;
#endif
	gboolean                  show_offline;
#ifdef USE_HILDON
        gboolean                  gconf_ret;
	GtkWidget		 *moko;
#endif
	gboolean                  show_avatars;
	gboolean                  compact_contact_list;
	gint                      x, y, w, h;

	if (window) {
		gtk_window_present (GTK_WINDOW (window->window));
		return window->window;
	}

	window = g_new0 (EmpathyMainWindow, 1);
#ifdef USE_HILDON
        glade_register_widget(HILDON_TYPE_WINDOW,
                              glade_hildon_window_new,
                              glade_standard_build_children,
                              NULL);
#endif

	/* Set up interface */
	glade = empathy_glade_get_file ("empathy-main-window.glade",
#ifndef USE_HILDON
				       "main_window",
#else
				       NULL,
#endif
				       NULL,
				       "main_window", &window->window,
#ifdef USE_HILDON
                                       "main_menu", &window->main_menu,
                                       "errors_vbox", &window->errors_vbox,
#endif
				       "main_vbox", &window->main_vbox,
				       "chat_show_offline", &show_offline_widget,
				       "room", &window->room,
				       "room_sep", &window->room_sep,
				       "room_join_favorites", &window->room_join_favorites,
				       "edit_context", &window->edit_context,
				       "edit_context_separator", &window->edit_context_separator,
#ifndef USE_HILDON
				       "presence_toolbar", &window->presence_toolbar,
#endif
				       "roster_scrolledwindow", &sw,
#ifdef USE_HILODN
				       "contact_add_button", &window->button_add;
				       "contact_remove_button", &window->button_remove;
#endif
				       NULL);

	empathy_glade_connect (glade,
			      window,
			      "main_window", "destroy", main_window_destroy_cb,
			      "main_window", "configure_event", main_window_configure_event_cb,
			      "chat_quit", "activate", main_window_chat_quit_cb,
			      "chat_new_message", "activate", main_window_chat_new_message_cb,
			      "chat_history", "activate", main_window_chat_history_cb,
			      "room_join_new", "activate", main_window_room_join_new_cb,
			      "room_join_favorites", "activate", main_window_room_join_favorites_cb,
			      "room_manage_favorites", "activate", main_window_room_manage_favorites_cb,
			      "chat_add_contact", "activate", main_window_chat_add_contact_cb,
			      "chat_show_offline", "toggled", main_window_chat_show_offline_cb,
			      "edit", "button-press-event", main_window_edit_button_press_event_cb,
			      "edit_accounts", "activate", main_window_edit_accounts_cb,
			      "edit_personal_information", "activate", main_window_edit_personal_information_cb,
			      "edit_preferences", "activate", main_window_edit_preferences_cb,
			      "help_about", "activate", main_window_help_about_cb,
#ifndef USE_HILDON
			      "help_contents", "activate", main_window_help_contents_cb,
#endif
#ifdef USE_HILDON
			      "contact_add_button", "clicked",  main_window_add_clicked_cb,
			      "contact_remove_button", "clicked", main_window_remove_clicked_cb,
#endif
			      NULL);

	/* Set up connection related widgets. */
	main_window_connection_items_setup (window, glade);
	g_object_unref (glade);

#ifdef USE_HILDON
        hildon_program_add_window (hildon_program_get_instance (), HILDON_WINDOW (window->window));
        hildon_window_set_menu(HILDON_WINDOW(window->window), GTK_MENU(window->main_menu));
        window->errors_label = gtk_label_new(_(""));
        gtk_container_add(GTK_CONTAINER(window->errors_vbox), window->errors_label);
        gtk_widget_show_all(window->errors_vbox);
#endif

	window->mc = empathy_mission_control_new ();
	dbus_g_proxy_connect_signal (DBUS_G_PROXY (window->mc), "AccountStatusChanged",
				     G_CALLBACK (main_window_status_changed_cb),
				     window, NULL);

	/* Set up menu */
	main_window_favorite_chatroom_menu_setup (window);

	gtk_widget_hide (window->edit_context);
	gtk_widget_hide (window->edit_context_separator);

#ifndef USE_HILDON
	window->tooltips = g_object_ref_sink (gtk_tooltips_new ());
	/* Set up presence chooser */
	window->presence_chooser = empathy_presence_chooser_new ();
	gtk_widget_show (window->presence_chooser);
	item = gtk_tool_item_new ();
	gtk_widget_show (GTK_WIDGET (item));
	gtk_container_add (GTK_CONTAINER (item), window->presence_chooser);
	gtk_tool_item_set_is_important (item, TRUE);
	gtk_tool_item_set_expand (item, TRUE);
	gtk_toolbar_insert (GTK_TOOLBAR (window->presence_toolbar), item, -1);

	/* Set up the throbber */
	ebox = gtk_event_box_new ();
	gtk_event_box_set_visible_window (GTK_EVENT_BOX (ebox), FALSE);

	window->throbber = ephy_spinner_new ();
	ephy_spinner_set_size (EPHY_SPINNER (window->throbber), GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_container_add (GTK_CONTAINER (ebox), window->throbber);

	item = gtk_tool_item_new ();
	gtk_container_add (GTK_CONTAINER (item), ebox);
	gtk_widget_show_all (GTK_WIDGET (item));

	gtk_toolbar_insert (GTK_TOOLBAR (window->presence_toolbar), item, -1);

	str = _("Show and edit accounts");
	gtk_tooltips_set_tip (GTK_TOOLTIPS (window->tooltips),
			      ebox, str, str);

	g_signal_connect (ebox,
			  "button-press-event",
			  G_CALLBACK (main_window_throbber_button_press_event_cb),
			  window);
#endif
	/* Set up contact list. */
	empathy_status_presets_get_all ();

	list_iface = EMPATHY_CONTACT_LIST (empathy_contact_manager_new ());
	window->list_store = empathy_contact_list_store_new (list_iface);
	window->list_view = empathy_contact_list_view_new (window->list_store);
	empathy_contact_list_view_set_interactive (window->list_view, TRUE);
	g_object_unref (list_iface);

	gtk_widget_show (GTK_WIDGET (window->list_view));
#ifndef USE_HILDON
	gtk_container_add (GTK_CONTAINER (sw),
			   GTK_WIDGET (window->list_view));
#else
	moko = moko_finger_scroll_new ();
        g_object_set(G_OBJECT(moko), "force_show_hindicator", TRUE, NULL);
	gtk_widget_show(moko);
	gtk_container_add (GTK_CONTAINER (moko),GTK_WIDGET (window->list_view));
	gtk_container_add (GTK_CONTAINER (sw),moko);
#endif

	/* Load user-defined accelerators. */
	main_window_accels_load ();

	/* Set window size. */
	empathy_geometry_load (GEOMETRY_NAME, &x, &y, &w, &h);

	if (w >= 1 && h >= 1) {
		/* Use the defaults from the glade file if we
		 * don't have good w, h geometry.
		 */
		empathy_debug (DEBUG_DOMAIN, "Configuring window default size w:%d, h:%d", w, h);
		gtk_window_set_default_size (GTK_WINDOW (window->window), w, h);
	}

	if (x >= 0 && y >= 0) {
		/* Let the window manager position it if we
		 * don't have good x, y coordinates.
		 */
		empathy_debug (DEBUG_DOMAIN, "Configuring window default position x:%d, y:%d", x, y);
		gtk_window_move (GTK_WINDOW (window->window), x, y);
	}

	conf = empathy_conf_get ();
	
	/* Show offline ? */
#ifdef USE_HILDON
        gconf_ret = empathy_conf_get_bool (conf,
                                           EMPATHY_PREFS_CONTACTS_SHOW_OFFLINE,
                                           &show_offline);
        if (!gconf_ret) {
                show_offline = TRUE;
                empathy_conf_set_bool (conf, EMPATHY_PREFS_CONTACTS_SHOW_OFFLINE,
                                            show_offline);
        }
#else
        empathy_conf_get_bool (conf,
                               EMPATHY_PREFS_CONTACTS_SHOW_OFFLINE,
                               &show_offline);
#endif
	empathy_conf_notify_add (conf,
				EMPATHY_PREFS_CONTACTS_SHOW_OFFLINE,
				main_window_notify_show_offline_cb,
				show_offline_widget);

	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (show_offline_widget),
					show_offline);

	/* Show avatars ? */
	empathy_conf_get_bool (conf,
			      EMPATHY_PREFS_UI_SHOW_AVATARS,
			      &show_avatars);
	empathy_conf_notify_add (conf,
				EMPATHY_PREFS_UI_SHOW_AVATARS,
				(EmpathyConfNotifyFunc) main_window_notify_show_avatars_cb,
				window);
	empathy_contact_list_store_set_show_avatars (window->list_store, show_avatars);

	/* Is compact ? */
	empathy_conf_get_bool (conf,
			      EMPATHY_PREFS_UI_COMPACT_CONTACT_LIST,
			      &compact_contact_list);
	empathy_conf_notify_add (conf,
				EMPATHY_PREFS_UI_COMPACT_CONTACT_LIST,
				(EmpathyConfNotifyFunc) main_window_notify_compact_contact_list_cb,
				window);
	empathy_contact_list_store_set_is_compact (window->list_store, compact_contact_list);

	/* Sort criterium */
	empathy_conf_notify_add (conf,
				EMPATHY_PREFS_CONTACTS_SORT_CRITERIUM,
				(EmpathyConfNotifyFunc) main_window_notify_sort_criterium_cb,
				window);
	main_window_notify_sort_criterium_cb (conf,
					      EMPATHY_PREFS_CONTACTS_SORT_CRITERIUM,
					      window);

	main_window_update_status (window);

	return window->window;
}

static void
main_window_destroy_cb (GtkWidget         *widget,
			EmpathyMainWindow *window)
{
	/* Save user-defined accelerators. */
	main_window_accels_save ();

	dbus_g_proxy_disconnect_signal (DBUS_G_PROXY (window->mc), "AccountStatusChanged",
					G_CALLBACK (main_window_status_changed_cb),
					window);

	if (window->size_timeout_id) {
		g_source_remove (window->size_timeout_id);
	}

	g_list_free (window->widgets_connected);
	g_list_free (window->widgets_disconnected);

#ifndef USE_HILDON
	g_object_unref (window->tooltips);
#endif
	g_object_unref (window->mc);
	g_object_unref (window->list_store);

	g_free (window);
}

static void
main_window_favorite_chatroom_menu_setup (EmpathyMainWindow *window)
{
	GList *chatrooms, *l;

	window->chatroom_manager = empathy_chatroom_manager_new ();
	chatrooms = empathy_chatroom_manager_get_chatrooms (window->chatroom_manager, NULL);
	window->room_menu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (window->room));

	for (l = chatrooms; l; l = l->next) {
		main_window_favorite_chatroom_menu_add (window, l->data);
	}

	if (!chatrooms) {
		gtk_widget_hide (window->room_sep);
	}

	gtk_widget_set_sensitive (window->room_join_favorites, chatrooms != NULL);

	g_signal_connect (window->chatroom_manager, "chatroom-added",
			  G_CALLBACK (main_window_favorite_chatroom_menu_added_cb),
			  window);
	g_signal_connect (window->chatroom_manager, "chatroom-removed",
			  G_CALLBACK (main_window_favorite_chatroom_menu_removed_cb),
			  window);

	g_list_free (chatrooms);
}

static void
main_window_favorite_chatroom_menu_added_cb (EmpathyChatroomManager *manager,
					     EmpathyChatroom        *chatroom,
					     EmpathyMainWindow     *window)
{
	main_window_favorite_chatroom_menu_add (window, chatroom);
	gtk_widget_show (window->room_sep);
	gtk_widget_set_sensitive (window->room_join_favorites, TRUE);
}

static void
main_window_favorite_chatroom_menu_removed_cb (EmpathyChatroomManager *manager,
					       EmpathyChatroom        *chatroom,
					       EmpathyMainWindow     *window)
{
	GtkWidget *menu_item;

	menu_item = g_object_get_data (G_OBJECT (chatroom), "menu_item");

	g_object_set_data (G_OBJECT (chatroom), "menu_item", NULL);
	gtk_widget_destroy (menu_item);

	main_window_favorite_chatroom_menu_update (window);
}

static void
main_window_favorite_chatroom_menu_activate_cb (GtkMenuItem    *menu_item,
						EmpathyChatroom *chatroom)
{
	main_window_favorite_chatroom_join (chatroom);
}

static void
main_window_favorite_chatroom_menu_update (EmpathyMainWindow *window)
{
	GList *chatrooms;

	chatrooms = empathy_chatroom_manager_get_chatrooms (window->chatroom_manager, NULL);

	if (chatrooms) {
		gtk_widget_show (window->room_sep);
	} else {
		gtk_widget_hide (window->room_sep);
	}

	gtk_widget_set_sensitive (window->room_join_favorites, chatrooms != NULL);
	g_list_free (chatrooms);
}

static void
main_window_favorite_chatroom_menu_add (EmpathyMainWindow *window,
					EmpathyChatroom    *chatroom)
{
	GtkWidget   *menu_item;
	const gchar *name;

	if (g_object_get_data (G_OBJECT (chatroom), "menu_item")) {
		return;
	}

	name = empathy_chatroom_get_name (chatroom);
	menu_item = gtk_menu_item_new_with_label (name);

	g_object_set_data (G_OBJECT (chatroom), "menu_item", menu_item);
	g_signal_connect (menu_item, "activate",
			  G_CALLBACK (main_window_favorite_chatroom_menu_activate_cb),
			  chatroom);

	gtk_menu_shell_insert (GTK_MENU_SHELL (window->room_menu),
			       menu_item, 3);

	gtk_widget_show (menu_item);
}

static void
main_window_favorite_chatroom_join (EmpathyChatroom *chatroom)
{
	MissionControl *mc;
	McAccount      *account;
	const gchar    *room;

	mc = empathy_mission_control_new ();
	account = empathy_chatroom_get_account (chatroom);
	room = empathy_chatroom_get_room (chatroom);

	empathy_debug (DEBUG_DOMAIN, "Requesting channel for '%s'", room);

	mission_control_request_channel_with_string_handle (mc,
							    account,
							    TP_IFACE_CHANNEL_TYPE_TEXT,
							    room,
							    TP_HANDLE_TYPE_ROOM,
							    NULL, NULL);	
	g_object_unref (mc);
}

static void
main_window_chat_quit_cb (GtkWidget         *widget,
			  EmpathyMainWindow *window)
{
	gtk_main_quit ();
}

static void
main_window_chat_new_message_cb (GtkWidget         *widget,
				 EmpathyMainWindow *window)
{
	//empathy_new_message_dialog_show (GTK_WINDOW (window->window));
}

static void
main_window_chat_history_cb (GtkWidget         *widget,
			     EmpathyMainWindow *window)
{
	empathy_log_window_show (NULL, NULL, FALSE, GTK_WINDOW (window->window));
}

static void
main_window_room_join_new_cb (GtkWidget         *widget,
			      EmpathyMainWindow *window)
{
	empathy_new_chatroom_dialog_show (GTK_WINDOW (window->window));
}

static void
main_window_room_join_favorites_cb (GtkWidget         *widget,
				    EmpathyMainWindow *window)
{
	GList *chatrooms, *l;

	chatrooms = empathy_chatroom_manager_get_chatrooms (window->chatroom_manager, NULL);
	for (l = chatrooms; l; l = l->next) {
		main_window_favorite_chatroom_join (l->data);
	}
	g_list_free (chatrooms);
}

static void
main_window_room_manage_favorites_cb (GtkWidget         *widget,
				      EmpathyMainWindow *window)
{
	empathy_chatrooms_window_show (GTK_WINDOW (window->window));
}

static void
main_window_chat_add_contact_cb (GtkWidget         *widget,
				 EmpathyMainWindow *window)
{
	empathy_new_contact_dialog_show (GTK_WINDOW (window->window));
}

static void
main_window_chat_show_offline_cb (GtkCheckMenuItem  *item,
				  EmpathyMainWindow *window)
{
	gboolean current;

	current = gtk_check_menu_item_get_active (item);

	empathy_conf_set_bool (empathy_conf_get (),
			      EMPATHY_PREFS_CONTACTS_SHOW_OFFLINE,
			      current);

	/* Turn off sound just while we alter the contact list. */
	// FIXME: empathy_sound_set_enabled (FALSE);
	empathy_contact_list_store_set_show_offline (window->list_store, current);
	//empathy_sound_set_enabled (TRUE);
}

static gboolean
main_window_edit_button_press_event_cb (GtkWidget         *widget,
					GdkEventButton    *event,
					EmpathyMainWindow *window)
{
	EmpathyContact *contact;
	gchar         *group;

	if (!event->button == 1) {
		return FALSE;
	}

	group = empathy_contact_list_view_get_selected_group (window->list_view);
	if (group) {
		GtkMenuItem *item;
		GtkWidget   *label;
		GtkWidget   *submenu;

		item = GTK_MENU_ITEM (window->edit_context);
		label = gtk_bin_get_child (GTK_BIN (item));
		gtk_label_set_text (GTK_LABEL (label), _("Group"));

		gtk_widget_show (window->edit_context);
		gtk_widget_show (window->edit_context_separator);

		submenu = empathy_contact_list_view_get_group_menu (window->list_view);
		gtk_menu_item_set_submenu (item, submenu);

		g_free (group);

		return FALSE;
	}

	contact = empathy_contact_list_view_get_selected (window->list_view);
	if (contact) {
		GtkMenuItem *item;
		GtkWidget   *label;
		GtkWidget   *submenu;

		item = GTK_MENU_ITEM (window->edit_context);
		label = gtk_bin_get_child (GTK_BIN (item));
		gtk_label_set_text (GTK_LABEL (label), _("Contact"));

		gtk_widget_show (window->edit_context);
		gtk_widget_show (window->edit_context_separator);

		submenu = empathy_contact_list_view_get_contact_menu (window->list_view,
								     contact);
		gtk_menu_item_set_submenu (item, submenu);

		g_object_unref (contact);

		return FALSE;
	}

	gtk_widget_hide (window->edit_context);
	gtk_widget_hide (window->edit_context_separator);

	return FALSE;
}

static void
main_window_edit_accounts_cb (GtkWidget         *widget,
			      EmpathyMainWindow *window)
{
	empathy_accounts_dialog_show (GTK_WINDOW (window->window));
}

static void
main_window_edit_personal_information_cb (GtkWidget         *widget,
					  EmpathyMainWindow *window)
{
	//empathy_vcard_dialog_show (GTK_WINDOW (window->window));
}

static void
main_window_edit_preferences_cb (GtkWidget         *widget,
				 EmpathyMainWindow *window)
{
	empathy_preferences_show (GTK_WINDOW (window->window));
}

static void
main_window_help_about_cb (GtkWidget         *widget,
			   EmpathyMainWindow *window)
{
	empathy_about_dialog_new (GTK_WINDOW (window->window));
}

#ifndef USE_HILDON
static void
main_window_help_contents_cb (GtkWidget         *widget,
			      EmpathyMainWindow *window)
{
	//empathy_help_show ();
}

static gboolean
main_window_throbber_button_press_event_cb (GtkWidget         *throbber_ebox,
					    GdkEventButton    *event,
					    EmpathyMainWindow *window)
{
	if (event->type != GDK_BUTTON_PRESS ||
	    event->button != 1) {
		return FALSE;
	}

	empathy_accounts_dialog_show (GTK_WINDOW (window->window));

	return FALSE;
}
#endif

static void
main_window_status_changed_cb (MissionControl                  *mc,
			       TelepathyConnectionStatus        status,
			       McPresence                       presence,
			       TelepathyConnectionStatusReason  reason,
			       const gchar                     *unique_name,
			       EmpathyMainWindow               *window)
{
	main_window_update_status (window);
}

static void
main_window_update_status (EmpathyMainWindow *window)
{
	GList *accounts, *l;
	guint  connected = 0;
	guint  connecting = 0;
	guint  disconnected = 0;
#ifdef USE_HILDON
	guint  enabled = 0;
#endif

	/* Count number of connected/connecting/disconnected accounts */
	accounts = mc_accounts_list_by_enabled (TRUE);	
	for (l = accounts; l; l = l->next) {
		McAccount *account;
		guint      status;

		account = l->data;
#ifdef USE_HILDON
                enabled++;
#endif

		status = mission_control_get_connection_status (window->mc,
								account,
								NULL);

		if (status == 0) {
			connected++;
		} else if (status == 1) {
			connecting++;
		} else if (status == 2) {
			disconnected++;
		}

#ifndef USE_HILDON
		g_object_unref (account);
#endif
	}
#ifndef USE_HILDON
	g_list_free (accounts);
#else
        mc_accounts_list_free(accounts);
#endif

#ifndef USE_HILDON
	/* Update the spinner state */
	if (connecting > 0) {
		ephy_spinner_start (EPHY_SPINNER (window->throbber));
	} else {
		ephy_spinner_stop (EPHY_SPINNER (window->throbber));
	}
#else
        if (enabled > 0) {
                if (connecting > 0) {
                        gtk_label_set_text(GTK_LABEL(window->errors_label),
                                                _("Account connecting..."));
                } else if (connected > 0){
                        gtk_label_set_text(GTK_LABEL(window->errors_label),
                                                "");
                } else {
                        gtk_label_set_text(GTK_LABEL(window->errors_label),
                                                _("Connect fail, check network and account setting"));
                }
        } else {
                gtk_label_set_text(GTK_LABEL(window->errors_label),
                                                _("No account enabled"));
        }
#endif

	/* Update widgets sensibility */
	for (l = window->widgets_connected; l; l = l->next) {
		gtk_widget_set_sensitive (l->data, (connected > 0));
	}

	for (l = window->widgets_disconnected; l; l = l->next) {
		gtk_widget_set_sensitive (l->data, (disconnected > 0));
	}
}

/*
 * Accels
 */
static void
main_window_accels_load (void)
{
	gchar *filename;

	filename = g_build_filename (g_get_home_dir (), ".gnome2", PACKAGE_NAME, ACCELS_FILENAME, NULL);
	if (g_file_test (filename, G_FILE_TEST_EXISTS)) {
		empathy_debug (DEBUG_DOMAIN, "Loading from:'%s'", filename);
		gtk_accel_map_load (filename);
	}

	g_free (filename);
}

static void
main_window_accels_save (void)
{
	gchar *dir;
	gchar *file_with_path;

	dir = g_build_filename (g_get_home_dir (), ".gnome2", PACKAGE_NAME, NULL);
	g_mkdir_with_parents (dir, S_IRUSR | S_IWUSR | S_IXUSR);
	file_with_path = g_build_filename (dir, ACCELS_FILENAME, NULL);
	g_free (dir);

	empathy_debug (DEBUG_DOMAIN, "Saving to:'%s'", file_with_path);
	gtk_accel_map_save (file_with_path);

	g_free (file_with_path);
}

static void
main_window_connection_items_setup (EmpathyMainWindow *window,
				    GladeXML          *glade)
{
	GList         *list;
	GtkWidget     *w;
	gint           i;
	const gchar *widgets_connected[] = {
		"room",
		"chat_new_message",
		"chat_add_contact",
		"edit_personal_information"
	};
	const gchar *widgets_disconnected[] = {
	};

	for (i = 0, list = NULL; i < G_N_ELEMENTS (widgets_connected); i++) {
		w = glade_xml_get_widget (glade, widgets_connected[i]);
		list = g_list_prepend (list, w);
	}

	window->widgets_connected = list;

	for (i = 0, list = NULL; i < G_N_ELEMENTS (widgets_disconnected); i++) {
		w = glade_xml_get_widget (glade, widgets_disconnected[i]);
		list = g_list_prepend (list, w);
	}

	window->widgets_disconnected = list;
}

static gboolean
main_window_configure_event_timeout_cb (EmpathyMainWindow *window)
{
	gint x, y, w, h;

	gtk_window_get_size (GTK_WINDOW (window->window), &w, &h);
	gtk_window_get_position (GTK_WINDOW (window->window), &x, &y);

	empathy_geometry_save (GEOMETRY_NAME, x, y, w, h);

	window->size_timeout_id = 0;

	return FALSE;
}

static gboolean
main_window_configure_event_cb (GtkWidget         *widget,
				GdkEventConfigure *event,
				EmpathyMainWindow *window)
{
	if (window->size_timeout_id) {
		g_source_remove (window->size_timeout_id);
	}

	window->size_timeout_id = g_timeout_add (500,
					       (GSourceFunc) main_window_configure_event_timeout_cb,
					       window);

	return FALSE;
}

static void
main_window_notify_show_offline_cb (EmpathyConf  *conf,
				    const gchar *key,
				    gpointer     check_menu_item)
{
	gboolean show_offline;

	if (empathy_conf_get_bool (conf, key, &show_offline)) {
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (check_menu_item),
						show_offline);
	}
}

static void
main_window_notify_show_avatars_cb (EmpathyConf        *conf,
				    const gchar       *key,
				    EmpathyMainWindow *window)
{
	gboolean show_avatars;

	if (empathy_conf_get_bool (conf, key, &show_avatars)) {
		empathy_contact_list_store_set_show_avatars (window->list_store,
							    show_avatars);
	}
}

static void
main_window_notify_compact_contact_list_cb (EmpathyConf        *conf,
					    const gchar       *key,
					    EmpathyMainWindow *window)
{
	gboolean compact_contact_list;

	if (empathy_conf_get_bool (conf, key, &compact_contact_list)) {
		empathy_contact_list_store_set_is_compact (window->list_store,
							  compact_contact_list);
	}
}

static void
main_window_notify_sort_criterium_cb (EmpathyConf        *conf,
				      const gchar       *key,
				      EmpathyMainWindow *window)
{
	gchar *str = NULL;

	if (empathy_conf_get_string (conf, key, &str) && str) {
		GType       type;
		GEnumClass *enum_class;
		GEnumValue *enum_value;

		type = empathy_contact_list_store_sort_get_type ();
		enum_class = G_ENUM_CLASS (g_type_class_peek (type));
		enum_value = g_enum_get_value_by_nick (enum_class, str);
		g_free (str);

		if (enum_value) {
			empathy_contact_list_store_set_sort_criterium (window->list_store, 
								      enum_value->value);
		}
	}
}

#ifdef USE_HILDON
static void
main_window_add_clicked_cb (GtkWidget         *button,
                            EmpathyMainWindow *window)
{
	empathy_new_contact_dialog_show (GTK_WINDOW (window->window));
	return;
}

static void
main_window_remove_clicked_cb (GtkWidget         *button,
                               EmpathyMainWindow *window)
{
	EmpathyContactListView *view;
	EmpathyContact         *contact;
	GtkWindow	       *parent;
        EmpathyContactList     *list;
	GtkWidget              *msg;
        gint                    response = GTK_RESPONSE_NONE;

        parent = GTK_WINDOW (window->window);
	view = window->list_view;
        contact = empathy_contact_list_view_get_selected (view);
	if (!contact)
		return;

        msg = gtk_message_dialog_new (parent, GTK_DIALOG_MODAL,
                                      GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE, "Really Remove This Contact?") ;

        if (NULL != msg) {
                gtk_window_set_title (GTK_WINDOW (msg), _("Contact Remove")) ;
                gtk_dialog_add_buttons (GTK_DIALOG (msg), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_REMOVE, GTK_RESPONSE_YES, NULL) ;
                response = gtk_dialog_run (GTK_DIALOG (msg)) ;
                gtk_widget_destroy (msg) ;
        }

        if (GTK_RESPONSE_YES == response) {
                list = empathy_contact_list_store_get_list_iface (window->list_store);
                empathy_contact_list_remove (list, contact,
                                            _("Sorry, I don't want you in my contact list anymore."));
        }
	return;
}
#endif
