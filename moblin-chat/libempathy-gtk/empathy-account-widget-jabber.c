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

#include "config.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#include <libmissioncontrol/mc-profile.h>

#include <libempathy/empathy-utils.h>
#ifdef USE_HILDON
#include <libempathy/empathy-debug.h>
#endif

#include "empathy-account-widget-jabber.h"
#include "empathy-ui-utils.h"

#ifdef USE_HILDON
#include "empathy-accounts-dialog.h"
#define DEBUG_DOMAIN "JabberAccount"
#endif

#define PORT_WITHOUT_SSL 5222
#define PORT_WITH_SSL 5223


typedef struct {
	McAccount *account;

	GtkWidget *vbox_settings;
	GtkWidget *button_forget;
	GtkWidget *entry_id;
	GtkWidget *entry_password;
	GtkWidget *entry_resource;
	GtkWidget *entry_server;
	GtkWidget *spinbutton_port;
	GtkWidget *checkbutton_ssl;
#ifdef USE_HILDON
        GtkWidget *entry_https_proxy_server;
        GtkWidget *entry_https_proxy_port;
#endif
} EmpathyAccountWidgetJabber;

static gboolean account_widget_jabber_entry_focus_cb           (GtkWidget                 *widget,
								GdkEventFocus             *event,
								EmpathyAccountWidgetJabber *settings);
#ifdef USE_HILDON
static gboolean account_widget_jabber_proxy_focus_cb           (GtkWidget                 *widget,
                                                                GdkEventFocus             *event,
                                                                EmpathyAccountWidgetJabber *settings);
#endif
static void     account_widget_jabber_entry_changed_cb         (GtkWidget                 *widget,
								EmpathyAccountWidgetJabber *settings);
static void     account_widget_jabber_checkbutton_toggled_cb   (GtkWidget                 *widget,
								EmpathyAccountWidgetJabber *settings);
static void     account_widget_jabber_value_changed_cb         (GtkWidget                 *spinbutton,
								EmpathyAccountWidgetJabber *settings);
static void     account_widget_jabber_button_forget_clicked_cb (GtkWidget                 *button,
								EmpathyAccountWidgetJabber *settings);
static void     account_widget_jabber_destroy_cb               (GtkWidget                 *widget,
								EmpathyAccountWidgetJabber *settings);
static void     account_widget_jabber_setup                    (EmpathyAccountWidgetJabber *settings);

static gboolean
account_widget_jabber_entry_focus_cb (GtkWidget                 *widget,
				      GdkEventFocus             *event,
				      EmpathyAccountWidgetJabber *settings)
{
	const gchar *param;
	const gchar *str;

	if (widget == settings->entry_password) {
		param = "password";
	}
	else if (widget == settings->entry_resource) {
		param = "resource";
	}
	else if (widget == settings->entry_server) {
		param = "server";
	}
	else if (widget == settings->entry_id) {
		param = "account";
	} else {
		return FALSE;
	}

	str = gtk_entry_get_text (GTK_ENTRY (widget));
	if (G_STR_EMPTY (str)) {
		gchar *value = NULL;

		mc_account_get_param_string (settings->account, param, &value);
		gtk_entry_set_text (GTK_ENTRY (widget), value ? value : "");
		g_free (value);
	} else {
		mc_account_set_param_string (settings->account, param, str);
#ifdef USE_HILDON
                if (mc_account_is_enabled (settings->account) && widget == settings->entry_id) {
                        mc_account_set_enabled (settings->account, FALSE);
                        mc_account_set_enabled (settings->account, TRUE);
                }
#endif
	}
#ifdef USE_HILDON
        if (widget == settings->entry_password ||
            widget == settings->entry_id)
                empathy_accounts_dialog_update_connect_button_sensitive(settings->account);
#endif

	return FALSE;
}


#ifdef USE_HILDON
static gboolean
account_widget_jabber_proxy_focus_cb (GtkWidget                  *widget,
                                      GdkEventFocus              *event,
                                      EmpathyAccountWidgetJabber *settings)
{
        const gchar *param;
        const gchar *str;
        gint   port;

        str = gtk_entry_get_text (GTK_ENTRY (widget));

        if (widget == settings->entry_https_proxy_server) {
                param = "https-proxy-server";
                if (G_STR_EMPTY(str))
                        mc_account_unset_param (settings->account, "https-proxy-server");
                else
                        mc_account_set_param_string (settings->account, param, str);
        }
        else if (widget == settings->entry_https_proxy_port) {
                param = "https-proxy-port";
                port  = G_STR_EMPTY(str) ? 0 : atoi(str);
                if (port)
                        mc_account_set_param_int (settings->account, param, port);
                else
                        mc_account_unset_param (settings->account, "https-proxy-port");
        }

        return FALSE;
}
#endif

static void
account_widget_jabber_entry_changed_cb (GtkWidget                 *widget,
					EmpathyAccountWidgetJabber *settings)
{
	if (widget == settings->entry_password) {
		const gchar *str;

		str = gtk_entry_get_text (GTK_ENTRY (widget));
		gtk_widget_set_sensitive (settings->button_forget, !G_STR_EMPTY (str));
	}
}

static void  
account_widget_jabber_checkbutton_toggled_cb (GtkWidget                 *widget,
					      EmpathyAccountWidgetJabber *settings)
{
	if (widget == settings->checkbutton_ssl) {
		gint     port = 0;
		gboolean old_ssl;

		mc_account_get_param_int (settings->account, "port", &port);
		old_ssl = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

		if (old_ssl) {
			if (port == PORT_WITHOUT_SSL) {
				port = PORT_WITH_SSL;
			}
		} else {
			if (port == PORT_WITH_SSL) {
				port = PORT_WITHOUT_SSL;
			}
		}
		
		mc_account_set_param_int (settings->account, "port", port);
		mc_account_set_param_boolean (settings->account, "old-ssl", old_ssl);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (settings->spinbutton_port), port);
	}
}

static void
account_widget_jabber_value_changed_cb (GtkWidget                 *spinbutton,
					EmpathyAccountWidgetJabber *settings)
{
	if (spinbutton == settings->spinbutton_port) {
		gdouble value;

		value = gtk_spin_button_get_value (GTK_SPIN_BUTTON (spinbutton));
		mc_account_set_param_int (settings->account, "port", (gint) value);
	}
}

static void
account_widget_jabber_button_forget_clicked_cb (GtkWidget                 *button,
						EmpathyAccountWidgetJabber *settings)
{
	mc_account_set_param_string (settings->account, "password", "");
	gtk_entry_set_text (GTK_ENTRY (settings->entry_password), "");
}

static void
account_widget_jabber_destroy_cb (GtkWidget                 *widget,
				  EmpathyAccountWidgetJabber *settings)
{
	g_object_unref (settings->account);
	g_free (settings);
}

static void
account_widget_jabber_setup (EmpathyAccountWidgetJabber *settings)
{
	gint      port = 0;
	gchar    *id = NULL;
	gchar    *resource = NULL;
	gchar    *server = NULL;
	gchar    *password = NULL;
	gboolean  old_ssl = FALSE;
#ifdef USE_HILDON
        gint      proxy_port = 0;
        gchar    *proxy_server = NULL;
        gchar     buf[11] = {'\0'};
#endif

	mc_account_get_param_int (settings->account, "port", &port);
	mc_account_get_param_string (settings->account, "account", &id);
	mc_account_get_param_string (settings->account, "resource", &resource);
	mc_account_get_param_string (settings->account, "server", &server);
	mc_account_get_param_string (settings->account, "password", &password);
	mc_account_get_param_boolean (settings->account, "old-ssl", &old_ssl);
#ifdef USE_HILDON
        mc_account_get_param_string (settings->account, "https-proxy-server", &proxy_server);
        mc_account_get_param_int (settings->account, "https-proxy-port", &proxy_port);
#endif

	if (!id) {
		McProfile   *profile;
		const gchar *server;

		profile = mc_account_get_profile (settings->account);
		server = mc_profile_get_default_account_domain (profile);
		if (server) {
			id = g_strconcat ("user@", server, NULL);
		}
		g_object_unref (profile);
	}

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (settings->checkbutton_ssl), old_ssl);
	gtk_entry_set_text (GTK_ENTRY (settings->entry_id), id ? id : "");
	gtk_entry_set_text (GTK_ENTRY (settings->entry_password), password ? password : "");
	gtk_entry_set_text (GTK_ENTRY (settings->entry_resource), resource ? resource : "");
	gtk_entry_set_text (GTK_ENTRY (settings->entry_server), server ? server : "");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (settings->spinbutton_port), port);
#ifdef USE_HILDON       
        gtk_entry_set_text (GTK_ENTRY (settings->entry_https_proxy_server), 
			    G_STR_EMPTY(proxy_server) ? "" : proxy_server);
        if (proxy_port)
                g_snprintf(buf, sizeof(buf), "%d", proxy_port);
        gtk_entry_set_text (GTK_ENTRY (settings->entry_https_proxy_port), buf);
#endif

	gtk_widget_set_sensitive (settings->button_forget, !G_STR_EMPTY (password));

	g_free (id);
	g_free (resource);
	g_free (server);
	g_free (password);
}

GtkWidget *
empathy_account_widget_jabber_new (McAccount *account)
{
	EmpathyAccountWidgetJabber *settings;
	GladeXML                  *glade;
	GtkSizeGroup              *size_group;
	GtkWidget                 *label_id, *label_password;
	GtkWidget                 *label_server, *label_resource, *label_port;
#ifdef USE_HILDON
        GtkWidget                 *label_https_proxy_server;
	GtkWidget		  *label_https_proxy_port;
#endif


	settings = g_new0 (EmpathyAccountWidgetJabber, 1);
	settings->account = g_object_ref (account);

	glade = empathy_glade_get_file ("empathy-account-widget-jabber.glade",
				       "vbox_jabber_settings",
				       NULL,
				       "vbox_jabber_settings", &settings->vbox_settings,
				       "button_forget", &settings->button_forget,
				       "label_id", &label_id,
				       "label_password", &label_password,
				       "label_resource", &label_resource,
				       "label_server", &label_server,
				       "label_port", &label_port,
				       "entry_id", &settings->entry_id,
				       "entry_password", &settings->entry_password,
				       "entry_resource", &settings->entry_resource,
				       "entry_server", &settings->entry_server,
				       "spinbutton_port", &settings->spinbutton_port,
				       "checkbutton_ssl", &settings->checkbutton_ssl,
#ifdef USE_HILDON
                                       "entry_https_proxy_server", &settings->entry_https_proxy_server,
                                       "entry_https_proxy_port", &settings->entry_https_proxy_port,
                                       "label_https_proxy_server", &label_https_proxy_server,
                                       "label_https_proxy_port", &label_https_proxy_port,
#endif
				       NULL);

	account_widget_jabber_setup (settings);

	empathy_glade_connect (glade, 
			      settings,
			      "vbox_jabber_settings", "destroy", account_widget_jabber_destroy_cb,
			      "button_forget", "clicked", account_widget_jabber_button_forget_clicked_cb,
			      "entry_password", "changed", account_widget_jabber_entry_changed_cb,
			      "spinbutton_port", "value-changed", account_widget_jabber_value_changed_cb,
			      "entry_id", "focus-out-event", account_widget_jabber_entry_focus_cb,
			      "entry_password", "focus-out-event", account_widget_jabber_entry_focus_cb,
			      "entry_resource", "focus-out-event", account_widget_jabber_entry_focus_cb,
			      "entry_server", "focus-out-event", account_widget_jabber_entry_focus_cb,
			      "checkbutton_ssl", "toggled", account_widget_jabber_checkbutton_toggled_cb,
#ifdef USE_HILDON
                              "entry_https_proxy_server", "focus-out-event", account_widget_jabber_proxy_focus_cb,
                              "entry_https_proxy_port", "focus-out-event", account_widget_jabber_proxy_focus_cb,
#endif
			      NULL);

	g_object_unref (glade);

	/* Set up remaining widgets */
	size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

	gtk_size_group_add_widget (size_group, label_id);
	gtk_size_group_add_widget (size_group, label_password);
	gtk_size_group_add_widget (size_group, label_resource);
	gtk_size_group_add_widget (size_group, label_server);
	gtk_size_group_add_widget (size_group, label_port);
#ifdef USE_HILDON
        gtk_size_group_add_widget (size_group, label_https_proxy_server);
        gtk_size_group_add_widget (size_group, label_https_proxy_port);
#endif


	g_object_unref (size_group);

  	gtk_editable_select_region (GTK_EDITABLE (settings->entry_id), 0, -1);
  	gtk_widget_grab_focus (settings->entry_id);

	gtk_widget_show (settings->vbox_settings);

	return settings->vbox_settings;
}
