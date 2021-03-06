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

#ifndef __EMPATHY_IDLE_H__
#define __EMPATHY_IDLE_H__

#include <glib.h>

#include <libmissioncontrol/mission-control.h>

G_BEGIN_DECLS

#define EMPATHY_TYPE_IDLE         (empathy_idle_get_type ())
#define EMPATHY_IDLE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), EMPATHY_TYPE_IDLE, EmpathyIdle))
#define EMPATHY_IDLE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), EMPATHY_TYPE_IDLE, EmpathyIdleClass))
#define EMPATHY_IS_IDLE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), EMPATHY_TYPE_IDLE))
#define EMPATHY_IS_IDLE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), EMPATHY_TYPE_IDLE))
#define EMPATHY_IDLE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), EMPATHY_TYPE_IDLE, EmpathyIdleClass))

typedef struct _EmpathyIdle      EmpathyIdle;
typedef struct _EmpathyIdleClass EmpathyIdleClass;
typedef struct _EmpathyIdlePriv  EmpathyIdlePriv;

struct _EmpathyIdle {
	GObject parent;
};

struct _EmpathyIdleClass {
	GObjectClass parent_class;
};

GType        empathy_idle_get_type            (void) G_GNUC_CONST;
EmpathyIdle *empathy_idle_new                 (void);
McPresence   empathy_idle_get_state           (EmpathyIdle *idle);
void         empathy_idle_set_state           (EmpathyIdle *idle,
					       McPresence   state);
const gchar *empathy_idle_get_status          (EmpathyIdle *idle);
void         empathy_idle_set_status          (EmpathyIdle *idle,
					       const gchar *status);
McPresence   empathy_idle_get_flash_state     (EmpathyIdle *idle);
void         empathy_idle_set_flash_state     (EmpathyIdle *idle,
					       McPresence   state);
void         empathy_idle_set_presence        (EmpathyIdle *idle,
					       McPresence   state,
					       const gchar *status);
gboolean     empathy_idle_get_auto_away       (EmpathyIdle *idle);
void         empathy_idle_set_auto_away       (EmpathyIdle *idle,
					       gboolean     auto_away);
gboolean     empathy_idle_get_auto_disconnect (EmpathyIdle *idle);
void         empathy_idle_set_auto_disconnect (EmpathyIdle *idle,
					       gboolean     auto_disconnect);

G_END_DECLS

#endif /* __EMPATHY_IDLE_H__ */
