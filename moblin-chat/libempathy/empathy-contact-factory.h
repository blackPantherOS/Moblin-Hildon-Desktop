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

#ifndef __EMPATHY_CONTACT_FACTORY_H__
#define __EMPATHY_CONTACT_FACTORY_H__

#include <glib.h>

#include <libmissioncontrol/mc-account.h>

#include "empathy-contact.h"

G_BEGIN_DECLS

#define EMPATHY_TYPE_CONTACT_FACTORY         (empathy_contact_factory_get_type ())
#define EMPATHY_CONTACT_FACTORY(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), EMPATHY_TYPE_CONTACT_FACTORY, EmpathyContactFactory))
#define EMPATHY_CONTACT_FACTORY_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), EMPATHY_TYPE_CONTACT_FACTORY, EmpathyContactFactoryClass))
#define EMPATHY_IS_CONTACT_FACTORY(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), EMPATHY_TYPE_CONTACT_FACTORY))
#define EMPATHY_IS_CONTACT_FACTORY_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), EMPATHY_TYPE_CONTACT_FACTORY))
#define EMPATHY_CONTACT_FACTORY_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), EMPATHY_TYPE_CONTACT_FACTORY, EmpathyContactFactoryClass))

typedef struct _EmpathyContactFactory      EmpathyContactFactory;
typedef struct _EmpathyContactFactoryClass EmpathyContactFactoryClass;
typedef struct _EmpathyContactFactoryPriv  EmpathyContactFactoryPriv;

struct _EmpathyContactFactory {
	GObject parent;
};

struct _EmpathyContactFactoryClass {
	GObjectClass parent_class;
};

GType                  empathy_contact_factory_get_type         (void) G_GNUC_CONST;
EmpathyContactFactory *empathy_contact_factory_new              (void);
EmpathyContact *       empathy_contact_factory_get_user         (EmpathyContactFactory *factory,
								 McAccount             *account);
EmpathyContact *       empathy_contact_factory_get_from_id      (EmpathyContactFactory *factory,
								 McAccount             *account,
								 const gchar           *id);
EmpathyContact *       empathy_contact_factory_get_from_handle  (EmpathyContactFactory *factory,
								 McAccount             *account,
								 guint                  handle);
GList *                empathy_contact_factory_get_from_handles (EmpathyContactFactory *factory,
								 McAccount             *account,
								 GArray                *handles);
void                   empathy_contact_factory_set_name         (EmpathyContactFactory *factory,
								 EmpathyContact        *contact,
								 const gchar           *name);

G_END_DECLS

#endif /* __EMPATHY_CONTACT_FACTORY_H__ */
