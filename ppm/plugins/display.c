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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <asm/types.h>

#include <dbus/dbus.h>
#include <glib.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <libhal.h>

#include "linuxppm.h"


#define HAL_DBUS_SERVICE                        "org.freedesktop.Hal"
#define HAL_DBUS_PATH_MANAGER                   "/org/freedesktop/Hal/Manager"
#define HAL_DBUS_INTERFACE_MANAGER              "org.freedesktop.Hal.Manager"
#define HAL_DBUS_INTERFACE_LAPTOP_PANEL         "org.freedesktop.Hal.Device.LaptopPanel"

#define PPM_MAX_BRIGHTNESS 8
const int available_brightness[PPM_MAX_BRIGHTNESS] = {
	10, 30, 40, 50, 60, 70, 80, 100
};

static DBusGProxy *hal_proxy = NULL;

static DBusGProxy* get_proxy()
{
	DBusGConnection *connection = NULL;
	DBusGProxy *proxy = NULL;
	GError *error = NULL;
	gchar *udi;
	gchar **names = NULL;
	int i;

	if (hal_proxy)
		return hal_proxy;

	if(connection == NULL) {
		connection = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);

		if (error) {
			PRINTF("Error can not connect to dbus:  %s\n",
			error->message);
			g_error_free (error);
			return NULL;
		}
		proxy = dbus_g_proxy_new_for_name(connection,
				HAL_DBUS_SERVICE, HAL_DBUS_PATH_MANAGER,
				HAL_DBUS_INTERFACE_MANAGER);

		if (!proxy) {
			PRINTF ("Error can not find laptop_panel capability\n");
			return NULL;
		}
		dbus_g_proxy_call(proxy, "FindDeviceByCapability", &error,
				  G_TYPE_STRING, "laptop_panel", G_TYPE_INVALID,
				  G_TYPE_STRV, &names, G_TYPE_INVALID);
		if (error) {
			PRINTF ("ERROR can not find laptop_panel in HAL %s\n", error->message);
			g_error_free (error);
			g_object_unref (proxy);
			return NULL;
		}
		if (names == NULL || names[0] == NULL) {
			PRINTF ("Error: can not find  devices with laptop_panel\n");
			if (names) 
				g_free(names);
			g_object_unref (proxy);
			return NULL;
		}
		udi = g_strdup (names[0]);
		for (i=0; names[i]; i++)
			g_free (names[i]);

		g_free (names);
	}

	hal_proxy  = dbus_g_proxy_new_for_name(connection,
					       HAL_DBUS_SERVICE, udi,
					       HAL_DBUS_INTERFACE_LAPTOP_PANEL);

	g_object_unref(proxy);

	g_free(udi);
	if (hal_proxy == NULL) {
		PRINTF("Error Cannot get proxy\n");
		return NULL;
	}

	return hal_proxy;
}

void set_brightness(char *option)
{
	int brightness = 84;
	int temp, i;
	int retval;
	DBusGProxy *proxy = NULL;
	GError *error = NULL;

	proxy = get_proxy();

	if (proxy == NULL) {
		PRINTF("Error we can not get display brightness\n");
		return;
	}

	brightness = atoi(option);
	temp = available_brightness[0];

	for (i=0; i < PPM_MAX_BRIGHTNESS; i++)
		if (brightness >= available_brightness[i])
			temp = available_brightness[i];
		else
			break;

	brightness = temp;

	dbus_g_proxy_call(proxy, "SetBrightness", &error,
			  G_TYPE_INT, (int)brightness,
			  G_TYPE_INVALID, G_TYPE_INT, &retval,
			  G_TYPE_INVALID);

}


static void display_message(char *class, char *command, char *option)
{
	if (!class || !command) {
		PRINTF("Malformed Messages, aborting\n");
		return; /* malformed message */
	}

	if (strcmp(class, "display"))
		return; /* not for us */


	/* if command is brightness we can do basic brightness controles */
	if (strcmp(command, "brightness")== 0 && option)
		set_brightness(option);

}


void start_plugin(void)
{
	register_plugin("display", display_message);
}

