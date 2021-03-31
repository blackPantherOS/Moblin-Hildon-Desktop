/*
 * This plugin based on the example dummy plugin from the Maemo
 * maemo-af-desktop repository.  Original copyright declaration:
 * Lucas Rocha <lucas.rocha@nokia.com>
 * Copyright 2006 Nokia Corporation.
 *
 * Modifcations transform the example to an embedded mozilla home applet:
 * Bob Spencer <bob.spencer@intel.com>
 * Copyright 2007 Intel Corporation
 *
 * Contributors:
 * Michael Frey <michael.frey@pepper.com>
 * Rusty Lynch <rusty.lynch@intel.com>
 * Bill Filler <bill.filler@canonical.com> 
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

#ifndef MOBILE_APP_HOME_PLUGIN_H
#define MOBILE_APP_HOME_PLUGIN_H

#include <glib-object.h>
#include <libhildondesktop/hildon-desktop-home-item.h>
#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

G_BEGIN_DECLS

typedef struct {
	gchar name[100];
	gchar value[100];
} ue_event;

typedef struct _MobileBasicHomePlugin MobileBasicHomePlugin;
typedef struct _MobileBasicHomePluginClass MobileBasicHomePluginClass;
typedef struct _MobileBasicHomePluginPrivate MobileBasicHomePluginPrivate;
typedef struct _plugin_context_t plugin_context_t;

#define MOBILE_BASIC_TYPE_HOME_PLUGIN            (mobile_basic_home_plugin_get_type ())
#define MOBILE_BASIC_HOME_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MOBILE_BASIC_TYPE_HOME_PLUGIN, MobileBasicHomePlugin))
#define MOBILE_BASIC_HOME_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  MOBILE_BASIC_TYPE_HOME_PLUGIN, MobileBasicHomePluginClass))
#define MOBILE_BASIC_IS_HOME_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MOBILE_BASIC_TYPE_HOME_PLUGIN))
#define MOBILE_BASIC_IS_HOME_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  MOBILE_BASIC_TYPE_HOME_PLUGIN))
#define MOBILE_BASIC_HOME_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  MOBILE_BASIC_TYPE_HOME_PLUGIN, MobileBasicHomePluginClass))

struct _MobileBasicHomePlugin 
{
	HildonDesktopHomeItem item;
	MobileBasicHomePluginPrivate *priv;
	plugin_context_t *context;
};

struct _MobileBasicHomePluginClass 
{
	HildonDesktopHomeItemClass parent_class;
};

GType  mobile_basic_home_plugin_get_type  (void);

G_END_DECLS

#endif //MOBILE_APP_HOME_PLUGIN_H

