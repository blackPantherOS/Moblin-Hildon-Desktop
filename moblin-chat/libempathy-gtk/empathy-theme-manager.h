/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2005-2007 Imendio AB
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
 * Authors: Richard Hult <richard@imendio.com>
 */

#ifndef __EMPATHY_THEME_MANAGER_H__
#define __EMPATHY_THEME_MANAGER_H__

#include <glib-object.h>

#include "empathy-chat-view.h"

G_BEGIN_DECLS

#define EMPATHY_TYPE_THEME_MANAGER         (empathy_theme_manager_get_type ())
#define EMPATHY_THEME_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), EMPATHY_TYPE_THEME_MANAGER, EmpathyThemeManager))
#define EMPATHY_THEME_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), EMPATHY_TYPE_THEME_MANAGER, EmpathyThemeManagerClass))
#define EMPATHY_IS_THEME_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), EMPATHY_TYPE_THEME_MANAGER))
#define EMPATHY_IS_THEME_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), EMPATHY_TYPE_THEME_MANAGER))
#define EMPATHY_THEME_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), EMPATHY_TYPE_THEME_MANAGER, EmpathyThemeManagerClass))

typedef struct _EmpathyThemeManager      EmpathyThemeManager;
typedef struct _EmpathyThemeManagerClass EmpathyThemeManagerClass;

struct _EmpathyThemeManager {
	GObject      parent;
};

struct _EmpathyThemeManagerClass {
	GObjectClass parent_class;
};

GType               empathy_theme_manager_get_type            (void) G_GNUC_CONST;
EmpathyThemeManager *empathy_theme_manager_get                 (void);
const gchar **      empathy_theme_manager_get_themes          (void);
void                empathy_theme_manager_apply               (EmpathyThemeManager *manager,
							      EmpathyChatView     *view,
							      const gchar        *theme);
void                empathy_theme_manager_apply_saved         (EmpathyThemeManager *manager,
							      EmpathyChatView     *view);
void                empathy_theme_manager_update_show_avatars (EmpathyThemeManager *manager,
							      EmpathyChatView     *view,
							      gboolean            show);

G_END_DECLS

#endif /* __EMPATHY_THEME_MANAGER_H__ */
