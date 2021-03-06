/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2005-2007 Imendio AB
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
 * Authors: Martyn Russell <martyn@imendio.com>
 *          Xavier Claessens <xclaesse@gmail.com>
 */

#include <config.h>

#include <string.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <glib/gi18n.h>
#include <dbus/dbus-glib.h>

#include <libmissioncontrol/mc-account.h>
#include <libmissioncontrol/mc-profile.h>
#include <libmissioncontrol/mission-control.h>
#include <libmissioncontrol/mc-account-monitor.h>
#include <libtelepathy/tp-constants.h>

#include <libempathy/empathy-debug.h>
#include <libempathy/empathy-utils.h>
#include <libempathy-gtk/empathy-ui-utils.h>

#include "empathy-accounts-dialog.h"
#include "empathy-profile-chooser.h"
#include "empathy-account-widget-generic.h"
#include "empathy-account-widget-jabber.h"
#include "empathy-account-widget-msn.h"
#include "empathy-account-widget-salut.h"

#define DEBUG_DOMAIN "AccountDialog"

/* Flashing delay for icons (milliseconds). */
#define FLASH_TIMEOUT 500

typedef struct {
	GtkWidget        *window;
#ifdef USE_HILDON
        GtkWindow        *parent;
#endif

	GtkWidget        *alignment_settings;

	GtkWidget        *vbox_details;
	GtkWidget        *frame_no_account;
	GtkWidget        *label_no_account;
	GtkWidget        *label_no_account_blurb;

	GtkWidget        *treeview;

	GtkWidget        *button_add;
	GtkWidget        *button_remove;
	GtkWidget        *button_connect;

	GtkWidget        *frame_new_account;
	GtkWidget        *combobox_profile;
	GtkWidget        *hbox_type;
	GtkWidget        *button_create;
	GtkWidget        *button_back;

	GtkWidget        *image_type;
	GtkWidget        *label_name;
	GtkWidget        *settings_widget;

	gboolean          connecting_show;
	guint             connecting_id;
	gboolean          account_changed;

	MissionControl   *mc;
	McAccountMonitor *monitor;
} EmpathyAccountsDialog;

enum {
	COL_NAME,
	COL_STATUS,
	COL_ACCOUNT_POINTER,
	COL_COUNT
};

static void       accounts_dialog_setup                     (EmpathyAccountsDialog            *dialog);
static void       accounts_dialog_update_account            (EmpathyAccountsDialog            *dialog,
							     McAccount                       *account);
static void       accounts_dialog_model_setup               (EmpathyAccountsDialog            *dialog);
static void       accounts_dialog_model_add_columns         (EmpathyAccountsDialog            *dialog);
static void       accounts_dialog_model_select_first        (EmpathyAccountsDialog            *dialog);
static void       accounts_dialog_model_pixbuf_data_func    (GtkTreeViewColumn               *tree_column,
							     GtkCellRenderer                 *cell,
							     GtkTreeModel                    *model,
							     GtkTreeIter                     *iter,
							     EmpathyAccountsDialog            *dialog);
static McAccount *accounts_dialog_model_get_selected        (EmpathyAccountsDialog            *dialog);
static void       accounts_dialog_model_set_selected        (EmpathyAccountsDialog            *dialog,
							     McAccount                       *account);
static gboolean   accounts_dialog_model_remove_selected     (EmpathyAccountsDialog            *dialog);
static void       accounts_dialog_model_selection_changed   (GtkTreeSelection                *selection,
							     EmpathyAccountsDialog            *dialog);
static void       accounts_dialog_add_account               (EmpathyAccountsDialog            *dialog,
							     McAccount                       *account);
static void       accounts_dialog_account_added_cb          (McAccountMonitor                *monitor,
							     gchar                           *unique_name,
							     EmpathyAccountsDialog            *dialog);
static void       accounts_dialog_account_removed_cb        (McAccountMonitor                *monitor,
							     gchar                           *unique_name,
							     EmpathyAccountsDialog            *dialog);
static gboolean   accounts_dialog_row_changed_foreach       (GtkTreeModel                    *model,
							     GtkTreePath                     *path,
							     GtkTreeIter                     *iter,
							     gpointer                         user_data);
static gboolean   accounts_dialog_flash_connecting_cb       (EmpathyAccountsDialog            *dialog);
static void       accounts_dialog_status_changed_cb         (MissionControl                  *mc,
							     TelepathyConnectionStatus        status,
							     McPresence                       presence,
							     TelepathyConnectionStatusReason  reason,
							     const gchar                     *unique_name,
							     EmpathyAccountsDialog            *dialog);
static void       accounts_dialog_button_create_clicked_cb  (GtkWidget                       *button,
							     EmpathyAccountsDialog            *dialog);
static void       accounts_dialog_button_back_clicked_cb    (GtkWidget                       *button,
							     EmpathyAccountsDialog            *dialog);
static void       accounts_dialog_button_connect_clicked_cb (GtkWidget                       *button,
							     EmpathyAccountsDialog            *dialog);
static void       accounts_dialog_button_add_clicked_cb     (GtkWidget                       *button,
							     EmpathyAccountsDialog            *dialog);
static void       accounts_dialog_remove_response_cb        (GtkWidget                       *dialog,
							     gint                             response,
							     McAccount                       *account);
static void       accounts_dialog_button_remove_clicked_cb  (GtkWidget                       *button,
							     EmpathyAccountsDialog            *dialog);
#ifdef USE_HILDON
static gboolean   accounts_dialog_delete_event_cb           (GtkWidget               *widget,
                                                             GdkEvent                *event,
                                                             EmpathyAccountsDialog   *dialog);
static gboolean   accounts_dialog_close_confirm             (EmpathyAccountsDialog *dialog);
static void       accounts_dialog_button_close_clicked_cb   (GtkWidget              *button,
                                                             EmpathyAccountsDialog  *dialog);
#endif
static void       accounts_dialog_treeview_row_activated_cb (GtkTreeView                     *tree_view,
							     GtkTreePath                     *path,
							     GtkTreeViewColumn               *column,
							     EmpathyAccountsDialog            *dialog);
#ifndef USE_HILDON
static void       accounts_dialog_response_cb               (GtkWidget                       *widget,
							     gint                             response,
							     EmpathyAccountsDialog            *dialog);
#endif
static void       accounts_dialog_destroy_cb                (GtkWidget                       *widget,
							     EmpathyAccountsDialog            *dialog);
#ifdef USE_HILDON
EmpathyAccountsDialog *accounts_dialog = NULL;
#endif

static void
accounts_dialog_setup (EmpathyAccountsDialog *dialog)
{
	GtkTreeView  *view;
	GtkListStore *store;
	GtkTreeIter   iter;
	GList        *accounts, *l;

	view = GTK_TREE_VIEW (dialog->treeview);
	store = GTK_LIST_STORE (gtk_tree_view_get_model (view));

	accounts = mc_accounts_list ();

	for (l = accounts; l; l = l->next) {
		McAccount                 *account;
		const gchar               *name;
		TelepathyConnectionStatus  status;

		account = l->data;

		name = mc_account_get_display_name (account);
		if (!name) {
			continue;
		}

		status = mission_control_get_connection_status (dialog->mc, account, NULL);

		gtk_list_store_insert_with_values (store, &iter,
						   -1,
						   COL_NAME, name,
						   COL_STATUS, status,
						   COL_ACCOUNT_POINTER, account,
						   -1);

		accounts_dialog_status_changed_cb (dialog->mc,
						   status,
						   MC_PRESENCE_UNSET,
						   TP_CONN_STATUS_REASON_NONE_SPECIFIED,
						   mc_account_get_unique_name (account),
						   dialog);

		g_object_unref (account);
	}

	g_list_free (accounts);
}

static void
accounts_dialog_update_connect_button (EmpathyAccountsDialog *dialog)
{
	McAccount   *account;
	GtkWidget   *image;
	const gchar *stock_id;
	const gchar *label;

	account = accounts_dialog_model_get_selected (dialog);
	
	gtk_widget_set_sensitive (dialog->button_connect, account != NULL);
#ifdef USE_HILDON
        empathy_accounts_dialog_update_connect_button_sensitive (account);
#else
        gtk_widget_set_sensitive (dialog->button_connect, account != NULL);
#endif

	if (account && mc_account_is_enabled (account)) {
		label = _("Disable");
		stock_id = GTK_STOCK_DISCONNECT;
	} else {
		label = _("Enable");
		stock_id = GTK_STOCK_CONNECT;
	}

	image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_BUTTON);

	gtk_button_set_label (GTK_BUTTON (dialog->button_connect), label);
	gtk_button_set_image (GTK_BUTTON (dialog->button_connect), image);
}

static void
accounts_dialog_update_account (EmpathyAccountsDialog *dialog,
				McAccount            *account)
{
	if (dialog->settings_widget) {
		gtk_widget_destroy (dialog->settings_widget);
		dialog->settings_widget = NULL;
	}

	if (!account) {
		GtkTreeView  *view;
		GtkTreeModel *model;

		gtk_widget_show (dialog->frame_no_account);
		gtk_widget_hide (dialog->vbox_details);

		gtk_widget_set_sensitive (dialog->button_connect, FALSE);
		gtk_widget_set_sensitive (dialog->button_remove, FALSE);

		view = GTK_TREE_VIEW (dialog->treeview);
		model = gtk_tree_view_get_model (view);

		if (gtk_tree_model_iter_n_children (model, NULL) > 0) {
			gtk_label_set_markup (GTK_LABEL (dialog->label_no_account),
					      _("<b>No Account Selected</b>"));
			gtk_label_set_markup (GTK_LABEL (dialog->label_no_account_blurb),
					      _("To add a new account, you can click on the "
						"'Add' button and a new entry will be created "
						"for you to start configuring.\n"
						"\n"
						"If you do not want to add an account, simply "
						"click on the account you want to configure in "
						"the list on the left."));
		} else {
			gtk_label_set_markup (GTK_LABEL (dialog->label_no_account),
					      _("<b>No Accounts Configured</b>"));
			gtk_label_set_markup (GTK_LABEL (dialog->label_no_account_blurb),
					      _("To add a new account, you can click on the "
						"'Add' button and a new entry will be created "
						"for you to start configuring."));
		}
	} else {
		McProfile *profile;
		const gchar *config_ui;

		gtk_widget_hide (dialog->frame_no_account);
		gtk_widget_show (dialog->vbox_details);

		profile = mc_account_get_profile (account);
		config_ui = mc_profile_get_configuration_ui (profile);
		g_object_unref (profile);

		if (!empathy_strdiff (config_ui, "jabber")) {
			dialog->settings_widget = 
				empathy_account_widget_jabber_new (account);
		} 
		else if (!empathy_strdiff (config_ui, "msn")) {
			dialog ->settings_widget =
				empathy_account_widget_msn_new (account);
		}
		else if (!empathy_strdiff (config_ui, "local-xmpp")) {
			dialog->settings_widget =
				empathy_account_widget_salut_new (account);
		}
		else {
			dialog->settings_widget = 
				empathy_account_widget_generic_new (account);
		}
		
		gtk_widget_grab_focus (dialog->settings_widget);
	}

	if (dialog->settings_widget) {
		gtk_container_add (GTK_CONTAINER (dialog->alignment_settings),
				   dialog->settings_widget);
	}

	if (account) {
		McProfile *profile;
		gchar     *text;

		profile = mc_account_get_profile (account);
		gtk_image_set_from_icon_name (GTK_IMAGE (dialog->image_type),
					      mc_profile_get_icon_name (profile),
					      GTK_ICON_SIZE_DIALOG);
		/* FIXME: Uncomment once we depend on GTK+ 2.12
		gtk_widget_set_tooltip_text (dialog->image_type,
					     mc_profile_get_display_name (profile));
		*/

		text = g_strdup_printf ("<big><b>%s</b></big>", mc_account_get_display_name (account));
		gtk_label_set_markup (GTK_LABEL (dialog->label_name), text);
		g_free (text);
	}
}

static void
accounts_dialog_model_setup (EmpathyAccountsDialog *dialog)
{
	GtkListStore     *store;
	GtkTreeSelection *selection;

	store = gtk_list_store_new (COL_COUNT,
				    G_TYPE_STRING,     /* name */
				    G_TYPE_UINT,       /* status */
				    MC_TYPE_ACCOUNT);  /* account */

	gtk_tree_view_set_model (GTK_TREE_VIEW (dialog->treeview),
				 GTK_TREE_MODEL (store));

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->treeview));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

	g_signal_connect (selection, "changed",
			  G_CALLBACK (accounts_dialog_model_selection_changed),
			  dialog);

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
					      COL_NAME, GTK_SORT_ASCENDING);

	accounts_dialog_model_add_columns (dialog);

	g_object_unref (store);
}

static void
accounts_dialog_name_edited_cb (GtkCellRendererText   *renderer,
				gchar                 *path,
				gchar                 *new_text,
				EmpathyAccountsDialog *dialog)
{
	McAccount    *account;
	GtkTreeModel *model;
	GtkTreePath  *treepath;
	GtkTreeIter   iter;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->treeview));
	treepath = gtk_tree_path_new_from_string (path);
	gtk_tree_model_get_iter (model, &iter, treepath);
	gtk_tree_model_get (model, &iter,
			    COL_ACCOUNT_POINTER, &account,
			    -1);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter,
			    COL_NAME, new_text,
			    -1);
	gtk_tree_path_free (treepath);

	mc_account_set_display_name (account, new_text);
	g_object_unref (account);
}

static void
accounts_dialog_model_add_columns (EmpathyAccountsDialog *dialog)
{
	GtkTreeView       *view;
	GtkTreeViewColumn *column;
	GtkCellRenderer   *cell;

	view = GTK_TREE_VIEW (dialog->treeview);
	gtk_tree_view_set_headers_visible (view, TRUE);

	/* account name/status */
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Accounts"));

	cell = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, cell, FALSE);
	gtk_tree_view_column_set_cell_data_func (column, cell,
						 (GtkTreeCellDataFunc)
						 accounts_dialog_model_pixbuf_data_func,
						 dialog,
						 NULL);

	cell = gtk_cell_renderer_text_new ();
	g_object_set (cell,
		      "ellipsize", PANGO_ELLIPSIZE_END,
		      "editable", TRUE,
		      NULL);
	gtk_tree_view_column_pack_start (column, cell, TRUE);
	gtk_tree_view_column_add_attribute (column,
					    cell,
					    "text", COL_NAME);
	g_signal_connect (cell, "edited",
			  G_CALLBACK (accounts_dialog_name_edited_cb),
			  dialog);

	gtk_tree_view_column_set_expand (column, TRUE);
	gtk_tree_view_append_column (view, column);
}

static void
accounts_dialog_model_select_first (EmpathyAccountsDialog *dialog)
{
	GtkTreeView      *view;
	GtkTreeModel     *model;
	GtkTreeSelection *selection;
	GtkTreeIter       iter;

	/* select first */
	view = GTK_TREE_VIEW (dialog->treeview);
	model = gtk_tree_view_get_model (view);
	
	if (gtk_tree_model_get_iter_first (model, &iter)) {
		selection = gtk_tree_view_get_selection (view);
		gtk_tree_selection_select_iter (selection, &iter);
	} else {
		accounts_dialog_update_account (dialog, NULL);
	}
}

static void
accounts_dialog_model_pixbuf_data_func (GtkTreeViewColumn    *tree_column,
					GtkCellRenderer      *cell,
					GtkTreeModel         *model,
					GtkTreeIter          *iter,
					EmpathyAccountsDialog *dialog)
{
	McAccount                 *account;
	const gchar               *icon_name;
	GdkPixbuf                 *pixbuf;
	TelepathyConnectionStatus  status;

	gtk_tree_model_get (model, iter,
			    COL_STATUS, &status,
			    COL_ACCOUNT_POINTER, &account,
			    -1);

	icon_name = empathy_icon_name_from_account (account);
	pixbuf = empathy_pixbuf_from_icon_name (icon_name, GTK_ICON_SIZE_BUTTON);

	if (pixbuf) {
		if (status == TP_CONN_STATUS_DISCONNECTED ||
		    (status == TP_CONN_STATUS_CONNECTING && 
		     !dialog->connecting_show)) {
			GdkPixbuf *modded_pixbuf;

			modded_pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
							TRUE,
							8,
							gdk_pixbuf_get_width (pixbuf),
							gdk_pixbuf_get_height (pixbuf));

			gdk_pixbuf_saturate_and_pixelate (pixbuf,
							  modded_pixbuf,
							  1.0,
							  TRUE);
			g_object_unref (pixbuf);
			pixbuf = modded_pixbuf;
		}
	}

	g_object_set (cell,
		      "visible", TRUE,
		      "pixbuf", pixbuf,
		      NULL);

	g_object_unref (account);
	if (pixbuf) {
		g_object_unref (pixbuf);
	}
}

static McAccount *
accounts_dialog_model_get_selected (EmpathyAccountsDialog *dialog)
{
	GtkTreeView      *view;
	GtkTreeModel     *model;
	GtkTreeSelection *selection;
	GtkTreeIter       iter;
	McAccount        *account;

	view = GTK_TREE_VIEW (dialog->treeview);
	selection = gtk_tree_view_get_selection (view);

	if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
		return NULL;
	}

	gtk_tree_model_get (model, &iter, COL_ACCOUNT_POINTER, &account, -1);

	return account;
}

static void
accounts_dialog_model_set_selected (EmpathyAccountsDialog *dialog,
				    McAccount            *account)
{
	GtkTreeView      *view;
	GtkTreeSelection *selection;
	GtkTreeModel     *model;
	GtkTreeIter       iter;
	gboolean          ok;

	view = GTK_TREE_VIEW (dialog->treeview);
	model = gtk_tree_view_get_model (view);
	selection = gtk_tree_view_get_selection (view);

	for (ok = gtk_tree_model_get_iter_first (model, &iter);
	     ok;
	     ok = gtk_tree_model_iter_next (model, &iter)) {
		McAccount *this_account;
		gboolean   equal;

		gtk_tree_model_get (model, &iter,
				    COL_ACCOUNT_POINTER, &this_account,
				    -1);

		equal = empathy_account_equal (this_account, account);
		g_object_unref (this_account);

		if (equal) {
			gtk_tree_selection_select_iter (selection, &iter);
			break;
		}
	}
}

static gboolean
accounts_dialog_model_remove_selected (EmpathyAccountsDialog *dialog)
{
	GtkTreeView      *view;
	GtkTreeModel     *model;
	GtkTreeSelection *selection;
	GtkTreeIter       iter;

	view = GTK_TREE_VIEW (dialog->treeview);
	selection = gtk_tree_view_get_selection (view);

	if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
		return FALSE;
	}

	return gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
}

static void
accounts_dialog_model_selection_changed (GtkTreeSelection     *selection,
					 EmpathyAccountsDialog *dialog)
{
	McAccount    *account;
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gboolean      is_selection;

	is_selection = gtk_tree_selection_get_selected (selection, &model, &iter);

	gtk_widget_set_sensitive (dialog->button_add, TRUE);
	gtk_widget_set_sensitive (dialog->button_remove, is_selection);
	gtk_widget_set_sensitive (dialog->button_connect, is_selection);

	accounts_dialog_update_connect_button (dialog);

	account = accounts_dialog_model_get_selected (dialog);
	accounts_dialog_update_account (dialog, account);

	if (account) {
		g_object_unref (account);
	}

	/* insure new account frame is hidden when a row is selected*/
	gtk_widget_hide (dialog->frame_new_account);
}

static void
accounts_dialog_add_account (EmpathyAccountsDialog *dialog,
			     McAccount            *account)
{
	TelepathyConnectionStatus  status;
	const gchar               *name;
	GtkTreeView               *view;
	GtkTreeModel              *model;
	GtkListStore              *store;
	GtkTreeIter                iter;
	gboolean                   ok;

	view = GTK_TREE_VIEW (dialog->treeview);
	model = gtk_tree_view_get_model (view);
	store = GTK_LIST_STORE (model);

	for (ok = gtk_tree_model_get_iter_first (model, &iter);
	     ok;
	     ok = gtk_tree_model_iter_next (model, &iter)) {
		McAccount *this_account;
		gboolean   equal;

		gtk_tree_model_get (model, &iter,
				    COL_ACCOUNT_POINTER, &this_account,
				    -1);

		equal =  empathy_account_equal (this_account, account);
		g_object_unref (this_account);

		if (equal) {
			return;
		}
	}

	status = mission_control_get_connection_status (dialog->mc, account, NULL);
	name = mc_account_get_display_name (account);

	g_return_if_fail (name != NULL);

	empathy_debug (DEBUG_DOMAIN, "Adding new account: %s", name);

	gtk_list_store_insert_with_values (store, &iter,
					   -1,
					   COL_NAME, name,
					   COL_STATUS, status,
					   COL_ACCOUNT_POINTER, account,
					   -1);
}

static void
accounts_dialog_account_added_cb (McAccountMonitor     *monitor,
				  gchar                *unique_name,
				  EmpathyAccountsDialog *dialog)
{
	McAccount *account;

	account = mc_account_lookup (unique_name);
	accounts_dialog_add_account (dialog, account);
	g_object_unref (account);
}

static void
accounts_dialog_account_removed_cb (McAccountMonitor     *monitor,
				    gchar                *unique_name,
				    EmpathyAccountsDialog *dialog)
{
	McAccount *account;

	account = mc_account_lookup (unique_name);

	accounts_dialog_model_set_selected (dialog, account);
	accounts_dialog_model_remove_selected (dialog);

	g_object_unref (account);
}

static gboolean
accounts_dialog_row_changed_foreach (GtkTreeModel *model,
				     GtkTreePath  *path,
				     GtkTreeIter  *iter,
				     gpointer      user_data)
{
	gtk_tree_model_row_changed (model, path, iter);

	return FALSE;
}

static gboolean
accounts_dialog_flash_connecting_cb (EmpathyAccountsDialog *dialog)
{
	GtkTreeView  *view;
	GtkTreeModel *model;

	dialog->connecting_show = !dialog->connecting_show;

	view = GTK_TREE_VIEW (dialog->treeview);
	model = gtk_tree_view_get_model (view);

	gtk_tree_model_foreach (model, accounts_dialog_row_changed_foreach, NULL);

	return TRUE;
}

static void
accounts_dialog_status_changed_cb (MissionControl                  *mc,
				   TelepathyConnectionStatus        status,
				   McPresence                       presence,
				   TelepathyConnectionStatusReason  reason,
				   const gchar                     *unique_name,
				   EmpathyAccountsDialog            *dialog)
{
	GtkTreeView      *view;
	GtkTreeSelection *selection;
	GtkTreeModel     *model;
	GtkTreeIter       iter;
	gboolean          ok;
	McAccount        *account;
	GList            *accounts, *l;
	gboolean          found = FALSE;
	
	/* Update the status in the model */
	view = GTK_TREE_VIEW (dialog->treeview);
	selection = gtk_tree_view_get_selection (view);
	model = gtk_tree_view_get_model (view);
	account = mc_account_lookup (unique_name);

	empathy_debug (DEBUG_DOMAIN, "Status changed for account %s: "
		      "status=%d presence=%d",
		      unique_name, status, presence);

	for (ok = gtk_tree_model_get_iter_first (model, &iter);
	     ok;
	     ok = gtk_tree_model_iter_next (model, &iter)) {
		McAccount *this_account;
		gboolean   equal;

		gtk_tree_model_get (model, &iter,
				    COL_ACCOUNT_POINTER, &this_account,
				    -1);

		equal = empathy_account_equal (this_account, account);
		g_object_unref (this_account);

		if (equal) {
			GtkTreePath *path;

			gtk_list_store_set (GTK_LIST_STORE (model), &iter,
					    COL_STATUS, status,
					    -1);

			path = gtk_tree_model_get_path (model, &iter);
			gtk_tree_model_row_changed (model, path, &iter);
			gtk_tree_path_free (path);

			break;
		}
	}

	g_object_unref (account);

	/* Start to flash account if status is connecting */
	if (status == TP_CONN_STATUS_CONNECTING) {
		if (!dialog->connecting_id) {
			dialog->connecting_id = g_timeout_add (FLASH_TIMEOUT,
							       (GSourceFunc) accounts_dialog_flash_connecting_cb,
							       dialog);
		}

		return;
	}

	/* Stop to flash if no account is connecting */
	accounts = mc_accounts_list ();
	for (l = accounts; l; l = l->next) {
		McAccount *this_account;

		this_account = l->data;

		if (mission_control_get_connection_status (mc, this_account, NULL) == TP_CONN_STATUS_CONNECTING) {
			found = TRUE;
			break;
		}

		g_object_unref (this_account);
	}
	g_list_free (accounts);

	if (!found && dialog->connecting_id) {
		g_source_remove (dialog->connecting_id);
		dialog->connecting_id = 0;
	}

	gtk_widget_show (dialog->window);
}

static void
accounts_dialog_button_create_clicked_cb (GtkWidget             *button,
					  EmpathyAccountsDialog  *dialog)
{
	McProfile   *profile;
	McAccount   *account;
	const gchar *str;

	/* Update widgets */
	gtk_widget_show (dialog->vbox_details);
	gtk_widget_hide (dialog->frame_no_account);
	gtk_widget_hide (dialog->frame_new_account);

	profile = empathy_profile_chooser_get_selected (dialog->combobox_profile);

	/* Create account */
	account = mc_account_create (profile);

	str = mc_account_get_unique_name (account);
	mc_account_set_display_name (account, str);

	accounts_dialog_add_account (dialog, account);
	accounts_dialog_model_set_selected (dialog, account);

	g_object_unref (account);
	g_object_unref (profile);
}

static void
accounts_dialog_button_back_clicked_cb (GtkWidget             *button,
					EmpathyAccountsDialog  *dialog)
{
	McAccount *account;

	gtk_widget_hide (dialog->vbox_details);
	gtk_widget_hide (dialog->frame_no_account);
	gtk_widget_hide (dialog->frame_new_account);

	gtk_widget_set_sensitive (dialog->button_add, TRUE);

	account = accounts_dialog_model_get_selected (dialog);
	accounts_dialog_update_account (dialog, account);
}

static void
accounts_dialog_button_connect_clicked_cb (GtkWidget            *button,
					   EmpathyAccountsDialog *dialog)
{
	McAccount *account;
	gboolean   enable;

	account = accounts_dialog_model_get_selected (dialog);
	enable = (!mc_account_is_enabled (account));
	mc_account_set_enabled (account, enable);
	accounts_dialog_update_connect_button (dialog);

	g_object_unref (account);
}

static void
accounts_dialog_button_add_clicked_cb (GtkWidget             *button,
				       EmpathyAccountsDialog *dialog)
{
	GtkTreeView      *view;
	GtkTreeSelection *selection;

	view = GTK_TREE_VIEW (dialog->treeview);
	selection = gtk_tree_view_get_selection (view);
	gtk_tree_selection_unselect_all (selection);

	gtk_widget_set_sensitive (dialog->button_add, FALSE);
	gtk_widget_hide (dialog->vbox_details);
	gtk_widget_hide (dialog->frame_no_account);
	gtk_widget_show (dialog->frame_new_account);

	gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->combobox_profile), 0);
	gtk_widget_grab_focus (dialog->combobox_profile);
}

static void
accounts_dialog_remove_response_cb (GtkWidget *dialog,
				    gint       response,
				    McAccount *account)
{
	if (response == GTK_RESPONSE_YES) {
		mc_account_delete (account);
	}

	gtk_widget_destroy (dialog);
}

#ifdef USE_HILDON
static gboolean
accounts_dialog_close_confirm(EmpathyAccountsDialog *dialog)
{
        GList *accounts_enabled;
        GtkWidget *msg;
        gint response = GTK_RESPONSE_NONE;
        gboolean confirm;

        accounts_enabled = mc_accounts_list_by_enabled (TRUE);
        if ( !accounts_enabled) {
                msg = gtk_message_dialog_new (GTK_WINDOW(dialog->window), GTK_DIALOG_MODAL,
                                GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE, "No Enabled Accounts, Really Quit?") ;
                if (NULL != msg) {
                        gtk_window_set_title (GTK_WINDOW (msg), _("Close Confirm")) ;
                        gtk_dialog_add_buttons (GTK_DIALOG (msg), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_CLOSE, GTK_RESPONSE_YES, NULL) ;
                        response = gtk_dialog_run (GTK_DIALOG (msg)) ;
                        gtk_widget_destroy (msg) ;
                }

                if (GTK_RESPONSE_YES == response) {
                        confirm = TRUE;
                        gtk_window_iconify (dialog->parent);
                } else
                        confirm = FALSE;
        } else
                confirm = TRUE;
        mc_accounts_list_free (accounts_enabled);
        return confirm;

}

static gboolean
accounts_dialog_delete_event_cb (GtkWidget               *widget,
                                 GdkEvent                *event,
                                 EmpathyAccountsDialog   *dialog)
{
        return !accounts_dialog_close_confirm(dialog);
}

static void
accounts_dialog_button_close_clicked_cb (GtkWidget             *button,
                                         EmpathyAccountsDialog *dialog)
{
        if (accounts_dialog_close_confirm (dialog)) {
                gtk_widget_destroy (dialog->window);
        }
}

#endif

static void
accounts_dialog_button_remove_clicked_cb (GtkWidget            *button,
					  EmpathyAccountsDialog *dialog)
{
	McAccount *account;
	GtkWidget *message_dialog;

	account = accounts_dialog_model_get_selected (dialog);

	if (!mc_account_is_complete (account)) {
		accounts_dialog_model_remove_selected (dialog);
		return;
	}
	message_dialog = gtk_message_dialog_new
		(GTK_WINDOW (dialog->window),
		 GTK_DIALOG_DESTROY_WITH_PARENT,
		 GTK_MESSAGE_QUESTION,
		 GTK_BUTTONS_NONE,
		 _("You are about to remove your %s account!\n"
		   "Are you sure you want to proceed?"),
		 mc_account_get_display_name (account));

	gtk_message_dialog_format_secondary_text
		(GTK_MESSAGE_DIALOG (message_dialog),
		 _("Any associated conversations and chat rooms will NOT be "
		   "removed if you decide to proceed.\n"
		   "\n"
		   "Should you decide to add the account back at a later time, "
		   "they will still be available."));

	gtk_dialog_add_button (GTK_DIALOG (message_dialog),
			       GTK_STOCK_CANCEL, 
			       GTK_RESPONSE_NO);
	gtk_dialog_add_button (GTK_DIALOG (message_dialog),
			       GTK_STOCK_REMOVE, 
			       GTK_RESPONSE_YES);

	g_signal_connect (message_dialog, "response",
			  G_CALLBACK (accounts_dialog_remove_response_cb),
			  account);

	gtk_widget_show (message_dialog);
}

static void
accounts_dialog_treeview_row_activated_cb (GtkTreeView          *tree_view,
					   GtkTreePath          *path,
					   GtkTreeViewColumn    *column,
					   EmpathyAccountsDialog *dialog)
{

	accounts_dialog_button_connect_clicked_cb (dialog->button_connect,
						   dialog);
}

#ifndef USE_HILDON
static void
accounts_dialog_response_cb (GtkWidget            *widget,
			     gint                  response,
			     EmpathyAccountsDialog *dialog)
{
	gtk_widget_destroy (widget);
}
#endif

static void
accounts_dialog_destroy_cb (GtkWidget            *widget,
			    EmpathyAccountsDialog *dialog)
{
	GList *accounts, *l;

	/* Disconnect signals */
	g_signal_handlers_disconnect_by_func (dialog->monitor,
					      accounts_dialog_account_added_cb,
					      dialog);
	g_signal_handlers_disconnect_by_func (dialog->monitor,
					      accounts_dialog_account_removed_cb,
					      dialog);
	dbus_g_proxy_disconnect_signal (DBUS_G_PROXY (dialog->mc),
					"AccountStatusChanged",
					G_CALLBACK (accounts_dialog_status_changed_cb),
					dialog);

	/* Delete incomplete accounts */
	accounts = mc_accounts_list ();
	for (l = accounts; l; l = l->next) {
		McAccount *account;

		account = l->data;
		if (!mc_account_is_complete (account)) {
			/* FIXME: Warn the user the account is not complete
			 *        and is going to be removed. */
			mc_account_delete (account);
		}

		g_object_unref (account);
	}
	g_list_free (accounts);

	if (dialog->connecting_id) {
		g_source_remove (dialog->connecting_id);
	}
#ifdef USE_HILDON
        accounts = NULL;
        accounts = mc_accounts_list_by_enabled (TRUE);
        if (accounts)
                empathy_window_present (dialog->parent, TRUE);
        g_list_free (accounts);
#endif

	g_object_unref (dialog->mc);
	g_object_unref (dialog->monitor);
	
	g_free (dialog);
#ifdef USE_HILDON
        accounts_dialog = NULL;
#endif
}

GtkWidget *
empathy_accounts_dialog_show (GtkWindow *parent)
{
	static EmpathyAccountsDialog *dialog = NULL;
	GladeXML                    *glade;
	GtkWidget                   *bbox;
	GtkWidget                   *button_close;

	if (dialog) {
		gtk_window_present (GTK_WINDOW (dialog->window));
		return dialog->window;
	}

	dialog = g_new0 (EmpathyAccountsDialog, 1);

	glade = empathy_glade_get_file ("empathy-accounts-dialog.glade",
				       "accounts_dialog",
				       NULL,
				       "accounts_dialog", &dialog->window,
				       "vbox_details", &dialog->vbox_details,
				       "frame_no_account", &dialog->frame_no_account,
				       "label_no_account", &dialog->label_no_account,
				       "label_no_account_blurb", &dialog->label_no_account_blurb,
				       "alignment_settings", &dialog->alignment_settings,
				       "dialog-action_area", &bbox,
				       "treeview", &dialog->treeview,
				       "frame_new_account", &dialog->frame_new_account,
				       "hbox_type", &dialog->hbox_type,
				       "button_create", &dialog->button_create,
				       "button_back", &dialog->button_back,
				       "image_type", &dialog->image_type,
				       "label_name", &dialog->label_name,
				       "button_add", &dialog->button_add,
				       "button_remove", &dialog->button_remove,
				       "button_connect", &dialog->button_connect,
				       "button_close", &button_close,
				       NULL);

	empathy_glade_connect (glade,
			      dialog,
			      "accounts_dialog", "destroy", accounts_dialog_destroy_cb,
#ifndef USE_HILDON
                              "accounts_dialog", "response", accounts_dialog_response_cb,
#else
                              "accounts_dialog", "delete-event", accounts_dialog_delete_event_cb,
#endif
			      "button_create", "clicked", accounts_dialog_button_create_clicked_cb,
			      "button_back", "clicked", accounts_dialog_button_back_clicked_cb,
			      "treeview", "row-activated", accounts_dialog_treeview_row_activated_cb,
			      "button_connect", "clicked", accounts_dialog_button_connect_clicked_cb,
			      "button_add", "clicked", accounts_dialog_button_add_clicked_cb,
			      "button_remove", "clicked", accounts_dialog_button_remove_clicked_cb,
#ifdef USE_HILDON
                              "button_close", "clicked", accounts_dialog_button_close_clicked_cb,
#endif
			      NULL);

	g_object_add_weak_pointer (G_OBJECT (dialog->window), (gpointer) &dialog);

	g_object_unref (glade);

	/* Create profile chooser */
	dialog->combobox_profile = empathy_profile_chooser_new ();
	gtk_box_pack_end (GTK_BOX (dialog->hbox_type),
			  dialog->combobox_profile,
			  TRUE, TRUE, 0);
	gtk_widget_show (dialog->combobox_profile);

	/* Set up signalling */
	dialog->mc = empathy_mission_control_new ();
	dialog->monitor = mc_account_monitor_new ();

	/* FIXME: connect account-enabled/disabled too */
	g_signal_connect (dialog->monitor, "account-created",
			  G_CALLBACK (accounts_dialog_account_added_cb),
			  dialog);
	g_signal_connect (dialog->monitor, "account-deleted",
			  G_CALLBACK (accounts_dialog_account_removed_cb),
			  dialog);
	dbus_g_proxy_connect_signal (DBUS_G_PROXY (dialog->mc), "AccountStatusChanged",
				     G_CALLBACK (accounts_dialog_status_changed_cb),
				     dialog, NULL);

	accounts_dialog_model_setup (dialog);
	accounts_dialog_setup (dialog);
	accounts_dialog_model_select_first (dialog);

#ifndef USE_HILDON
        if (parent) {
                gtk_window_set_transient_for (GTK_WINDOW (dialog->window),
                                              GTK_WINDOW (parent));
        }
#else
        dialog->parent = parent;
        accounts_dialog = dialog;
#endif
	
	gtk_widget_show (dialog->window);

	return dialog->window;
}

#ifdef USE_HILDON
gboolean
empathy_accounts_dialog_is_running(void)
{
        if (accounts_dialog)
                return TRUE;
        else
                return FALSE;
}

void
empathy_accounts_dialog_update_connect_button_sensitive (McAccount *account)
{

        if (!accounts_dialog)
                return;
        if (account == NULL)
                return gtk_widget_set_sensitive (accounts_dialog->button_connect, FALSE);
        return gtk_widget_set_sensitive (accounts_dialog->button_connect,
                                                mc_account_is_complete(account));
}
#endif

