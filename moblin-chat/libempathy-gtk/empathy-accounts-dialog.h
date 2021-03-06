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
 */

#ifndef __EMPATHY_ACCOUNTS_DIALOG_H__
#define __EMPATHY_ACCOUNTS_DIALOG_H__

#include <gtk/gtkwidget.h>
#ifdef USE_HILDON
#include <libmissioncontrol/mc-account.h>
#endif

G_BEGIN_DECLS

GtkWidget *empathy_accounts_dialog_show (GtkWindow *parent);
#ifdef USE_HILDON
gboolean   empathy_accounts_dialog_is_running(void);
void       empathy_accounts_dialog_update_connect_button_sensitive(McAccount *account);
#endif

G_END_DECLS

#endif /* __EMPATHY_ACCOUNTS_DIALOG_H__ */
