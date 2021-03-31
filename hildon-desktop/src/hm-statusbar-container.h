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

#ifndef STATUSBAR_CONTAINER_H
#define STATUSBAR_CONTAINER_H

#include <glib-object.h>
#include <libhildondesktop/tasknavigator-item.h>
#include <libhildonwm/hd-wm.h>

G_BEGIN_DECLS

typedef struct _StatusbarContainer StatusbarContainer;
typedef struct _StatusbarContainerClass StatusbarContainerClass;
typedef struct _StatusbarContainerPrivate StatusbarContainerPrivate;

#define HM_TYPE_STATUSBAR_CONTAINER         (statusbar_container_get_type ())
#define STATUSBAR_CONTAINER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HM_TYPE_STATUSBAR_CONTAINER, StatusbarContainer))
#define STATUSBAR_CONTAINER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  HM_TYPE_STATUSBAR_CONTAINER, StatusbarContainerClass))
#define IS_STATUSBAR_CONTAINER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HM_TYPE_STATUSBAR_CONTAINER))
#define IS_STATUSBAR_CONTAINER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  HM_TYPE_STATUSBAR_CONTAINER))
#define STATUSBAR_CONTAINER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  HM_TYPE_STATUSBAR_CONTAINER, StatusbarContainerClass))

struct _StatusbarContainer
{
   TaskNavigatorItem tnitem;
   StatusbarContainerPrivate *priv;
};

struct _StatusbarContainerClass
{
   TaskNavigatorItemClass parent_class;
};

GType  statusbar_container_get_type  (void);

G_END_DECLS

#endif //STATUSBAR_CONTAINER_H
