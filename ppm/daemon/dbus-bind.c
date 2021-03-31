/*
 * This file is part of the Linux Power Policy Manager
 *
 * Copyright © 2007 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the terms of the
 * GNU General Public License version 2 (the "GPL"), in which case the
 * provisions of the GPL are applicable instead of the above.  If you wish to
 * allow the use of your version of this file only under the terms of the GPL
 * and not to allow others to use your version of this file under the MIT
 * license, indicate your decision by deleting the provisions above and replace
 * them with the notice and other provisions required by the GPL.  If you do
 * not delete the provisions above, a recipient may use your version of this
 * file under either the GPL or the MIT license.
 *
 * Authors:
 * 	Tariq Shureih  <tariq.shureih@intel.com>
 * 	Arjan van de Ven <arjan@linux.intel.com>
 * 	Mohamed Abbas <mohamed.abbas@intel.com>
 * 	Sarah Sharp <sarah.a.sharp@intel.com>
 *
 */


#include <stdio.h>
#include <string.h>
#include <glib.h>
#include "cei.h"
#include "dbus-bind.h"
#include "cei_server.h"
//#include "xmlparser.h"
#include "linuxppm.h"

#define CEI_TYPE (cei_get_type ())
G_DEFINE_TYPE(CEI, cei, G_TYPE_OBJECT);

#define POLICY_NODE "policy"

static DBusGConnection *connection = NULL;
void cei_init(CEI *server)
{
}

void cei_class_init(CEIClass *cei_class)
{
	dbus_g_object_type_install_info(G_TYPE_FROM_CLASS(cei_class),
		&dbus_glib_cei_object_info);
}

gboolean
cei_server_init(void)
{
	GError *error = NULL;
	DBusGProxy *proxy;
	unsigned int request_ret;
	CEI *server;

	g_type_init();

	connection = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
	if (connection == NULL) {
		g_warning("Unable to connect to dbus because: %s", error->message);
		g_error_free(error);
		return FALSE;
	}

	proxy = dbus_g_proxy_new_for_name(connection,
		DBUS_SERVICE_DBUS,
		DBUS_PATH_DBUS,
		DBUS_INTERFACE_DBUS);

	if (!org_freedesktop_DBus_request_name (proxy, DBUS_CEI_SERVICE, 0, &request_ret, &error))
	{
		g_warning("Unable to register service: %s", error->message);
		g_error_free(error);
		return FALSE;
	}

	if (request_ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
		g_warning("Another process has claimed this service: %s",
			DBUS_CEI_SERVICE);
		return FALSE;
	}

	server = (CEI *)g_object_new(CEI_TYPE, NULL);
	dbus_g_connection_register_g_object(connection, DBUS_CEI_PATH, G_OBJECT(server));

	return TRUE;
}

/* notify about the new command */
void cei_send_cmd(char *class, char *command, char *value)
{
	DBusMessage *message;

	if (!connection)
		return;

	message = dbus_message_new_signal(DBUS_CEI_PATH_CMD,
					  DBUS_CEI_INTERFACE_CMD, "SetdCmd");

	if (!message || (!dbus_message_append_args(message,
				       DBUS_TYPE_STRING, &class,
				       DBUS_TYPE_STRING, &command,
				       DBUS_TYPE_STRING, &value,
				       DBUS_TYPE_INVALID))) {
		return;
	}

	dbus_connection_send(dbus_g_connection_get_connection(connection), message, NULL);
	dbus_message_unref(message);
}

gboolean cei_query_modes(CEI *obj, GPtrArray ** modes, int *policycount, GError ** error)
{
	if((policycount == NULL)||(modes == NULL))
		return FALSE;

	PRINTF("cei_query_policies\n");

	get_modes(modes, policycount);

	PRINTF("end\n");
	return TRUE;
}
