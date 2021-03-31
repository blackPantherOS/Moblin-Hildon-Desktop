/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2006-2007 Imendio AB
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

#include "config.h"

#include <glib/gi18n.h>
#include <gtk/gtkaboutdialog.h>
#include <gtk/gtksizegroup.h>
#include <glade/glade.h>

#include "empathy-about-dialog.h"
#include "empathy-ui-utils.h"

#ifndef USE_HILDON
#define WEB_SITE "http://live.gnome.org/Empathy"
#else
#define WEB_SITE "http://www.moblin.org/projects_chat.html"
#endif

static void about_dialog_activate_link_cb (GtkAboutDialog  *about,
					   const gchar     *link,
					   gpointer         data);

static const char *authors[] = {
	"Mikael Hallendal",
	"Richard Hult",
	"Martyn Russell",
	"Geert-Jan Van den Bogaerde",
	"Kevin Dougherty",
	"Eitan Isaacson",
	"Xavier Claessens",
#ifdef USE_HILDON
	"Peter Zhu", 
#endif
	NULL
};

static const char *documenters[] = {
	NULL
};

static const char *artists[] = {
	"Andreas Nilsson <nisses.mail@home.se>",
	"Vinicius Depizzol <vdepizzol@gmail.com>",
	NULL
};

#ifndef USE_HILDON
static const char *license[] = {
	N_("Empathy is free software; you can redistribute it and/or modify "
	   "it under the terms of the GNU General Public License as published by "
	   "the Free Software Foundation; either version 2 of the License, or "
	   "(at your option) any later version."),
	N_("Empathy is distributed in the hope that it will be useful, "
	   "but WITHOUT ANY WARRANTY; without even the implied warranty of "
	   "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
	   "GNU General Public License for more details."),
	N_("You should have received a copy of the GNU General Public License "
	   "along with Empathy; if not, write to the Free Software Foundation, Inc., "
	   "51 Franklin Street, Fifth Floor, Boston, MA 02110-130159 USA")
};
#else
static const char *license[] = {
	N_(PACKAGE_NAME" is customized for use on MID based on Empathy"),
	N_(PACKAGE_NAME" is free software; you can redistribute it and/or modify "
	   "it under the terms of the GNU General Public License as published by "
	   "the Free Software Foundation; either version 2 of the License, or "
	   "(at your option) any later version."),
	N_(PACKAGE_NAME" is distributed in the hope that it will be useful, "
	   "but WITHOUT ANY WARRANTY; without even the implied warranty of "
	   "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
	   "GNU General Public License for more details."),
	N_("You should have received a copy of the GNU General Public License "
	   "along with Empathy; if not, write to the Free Software Foundation, Inc., "
	   "51 Franklin Street, Fifth Floor, Boston, MA 02110-130159 USA")
};
#endif

static void
about_dialog_activate_link_cb (GtkAboutDialog *about,
			       const gchar    *link,
			       gpointer        data)
{
	empathy_url_show (link);
}

void
empathy_about_dialog_new (GtkWindow *parent)
{
	gchar *license_trans;

	gtk_about_dialog_set_url_hook (about_dialog_activate_link_cb, NULL, NULL);

	license_trans = g_strconcat (_(license[0]), "\n\n",
				     _(license[1]), "\n\n",
				     _(license[2]), "\n\n",
				     NULL);

	gtk_show_about_dialog (parent,
			       "artists", artists,
			       "authors", authors,
#ifndef USE_HILDON
			       "comments", _("An Instant Messaging client for GNOME"),
#else
			       "comments", _("An Instant Messaging client for MID"),
#endif
			       "license", license_trans,
			       "wrap-license", TRUE,
#ifdef USE_HILDON
			       "copyright", "Imendio AB 2002-2007\nCollabora Ltd 2007\nIntel Ltd 2007",
#else
			       "copyright", "Imendio AB 2002-2007\nCollabora Ltd 2007",
#endif
			       "documenters", documenters,
			       "logo-icon-name", "empathy",
			       "translator-credits", _("translator-credits"),
			       "version", PACKAGE_VERSION,
			       "website", WEB_SITE,
			       NULL);

	g_free (license_trans);
}


