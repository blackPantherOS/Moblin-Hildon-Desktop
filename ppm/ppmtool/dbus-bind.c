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
#include <stdlib.h>
#include <ctype.h>
#include <glib.h>
#include <dbus/dbus-glib-bindings.h>
#include "cei.h"
#include "ppmtool.h"
#include "dbus-bind.h"
#include "cei_client.h"

static DBusGProxy *proxy = NULL;

gboolean
cei_activate_modes(gchar **active_modes)
{
	GError *error = NULL;

	if(!DBUS_CEI_ACTIVATE_MODES(proxy, (const gchar **)active_modes, &error))
	{
		if(error) {
			EPRINTF("active_modes error: %s\n",
				error->message);
			g_error_free(error);
		} else {
			EPRINTF("active_modes error\n");
		}
		return FALSE;
	}
	return TRUE;
}

gboolean
cei_append_active_modes(gchar **active_modes)
{
	GError *error = NULL;

	if(!DBUS_CEI_APPEND_ACTIVE_MODES(proxy, (const gchar **)active_modes, &error))
	{
		if(error) {
			EPRINTF("active_modes error: %s\n",
				error->message);
			g_error_free(error);
		} else {
			EPRINTF("active_modes error\n");
		}
		return FALSE;
	}
	return TRUE;
}

gboolean
cei_deactivate_modes(gchar **deactive_modes)
{
	GError *error = NULL;

	if(!DBUS_CEI_DEACTIVATE_MODES(proxy, (const gchar **)deactive_modes, &error))
	{
		if(error) {
			EPRINTF("deactive_modes error: %s\n",
				error->message);
			g_error_free(error);
		} else {
			EPRINTF("deactive_modes error\n");
		}
		return FALSE;
	}
	return TRUE;
}

gboolean
cei_query_modes(struct mode_t **modes, int *policycount)
{
	GError *error = NULL;
	int i;
	GPtrArray* mode_list;
	GValueArray *value_array;
	GValue *value;
        if(!DBUS_CEI_QUERY_MODES(proxy, &mode_list,
                policycount, &error))
        {
                if(error) {
                        EPRINTF("query_policies error: %s\n",
                                error->message);
                        g_error_free(error);
                } else {
                        EPRINTF("query_policies error\n");
                }
                return FALSE;
        }
        *modes = (struct mode_t *)malloc(sizeof(struct mode_t)*(*policycount));
        for(i = 0; i < mode_list->len; i++)
        {
		value_array = (GValueArray *) g_ptr_array_index (mode_list, i);

		value = g_value_array_get_nth(value_array, 0);
		(*modes)[i].name = g_value_dup_string(value);
		g_value_unset(value);

		value = g_value_array_get_nth(value_array, 1);
		(*modes)[i].active = g_value_get_int(value);
		g_value_unset(value);

		g_value_array_free(value_array);
        }
	g_ptr_array_free(mode_list, TRUE);

        return TRUE;
}

gboolean
cei_client_init(void)
{
	DBusGConnection *connection;
	GError *error = NULL;

	g_type_init();
	connection = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
	if( connection == NULL)
	{
		PRINTF("Fail to open connection to bus:%s\n",error->message);
		return FALSE;
	}

	proxy = dbus_g_proxy_new_for_name(connection,
		DBUS_CEI_SERVICE,
		DBUS_CEI_PATH,
		DBUS_CEI_SERVICE);
	if( proxy == NULL)
	{
		PRINTF("ERROR: PPM daemon not running\n");
		return FALSE;
	}
	return TRUE;
}

gboolean
cei_client_exit(void)
{

	if (proxy)
		g_object_unref (proxy);

	proxy = NULL;
	return TRUE;
}
