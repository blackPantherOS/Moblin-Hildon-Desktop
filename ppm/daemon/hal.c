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
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <inttypes.h>

#include <dbus/dbus.h>
#include <glib.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <libhal.h>

#include "linuxppm.h"

static LibHalContext *hal_ctx;
static DBusConnection *dbus_connection;

static void watch_all_devices_with_capability(LibHalContext *ctx, const char *capability) {
	char **devices;
	int num_devices;
	int i;

	devices = libhal_find_device_by_capability(ctx, capability,
			&num_devices, NULL);
	for (i = 0; i < num_devices; i++) {
		PRINTF("Watching properties on device %s\n", devices[i]);
		libhal_device_add_property_watch(ctx, devices[i], NULL);
	}
}

static void query_ac_adapters(LibHalContext *ctx) {
	DBusError error;
	char **devices;
	int num_devices;
	int i;
	gboolean ac_present = FALSE;

	dbus_error_init(&error);

	/* Find out if any AC adapter is plugged in */
	devices = libhal_find_device_by_capability(ctx, "ac_adapter",
			&num_devices, NULL);
	for (i = 0; i < num_devices; i++) {
		if (libhal_device_get_property_bool(ctx, devices[i],
					"ac_adapter.present", &error) > 0 &&
				!dbus_error_is_set(&error))
			ac_present = TRUE;
	}

	/* Notify PPMD that the state may have changed */
	if (ac_present) {
		handle_events("ppm.ac", TRUE);
		handle_events("ppm.battery", FALSE);
	} else {
		handle_events("ppm.battery", TRUE);
		handle_events("ppm.ac", FALSE);
	}
	dbus_error_free(&error);
}

void hal_property_modified(LibHalContext *ctx, const char *udi,
		const char *key, dbus_bool_t is_removed,
		dbus_bool_t is_added) {
	DBusError error;

	dbus_error_init(&error);
	if (libhal_device_query_capability(ctx, udi, "ac_adapter", &error) &&
			!dbus_error_is_set(&error)) {
		query_ac_adapters(ctx);
	}

	dbus_error_free(&error);
}

void hal_device_added(LibHalContext *ctx,  const char *udi) {
	DBusError error;

	dbus_error_init(&error);
	if (libhal_device_query_capability(ctx, udi, "ac_adapter", &error) &&
			!dbus_error_is_set(&error)) {
		libhal_device_add_property_watch(ctx, udi, NULL);
		query_ac_adapters(ctx);
	}
	dbus_error_free(&error);
}

void hal_device_removed(LibHalContext *ctx, const char *udi) {
	DBusError error;

	dbus_error_init(&error);
	if (libhal_device_query_capability(ctx, udi, "ac_adapter", &error) &&
			!dbus_error_is_set(&error)) {
		query_ac_adapters(ctx);
	}
	dbus_error_free(&error);
}

static gboolean dbus_init() {
	DBusError error;

	dbus_error_init(&error);
	dbus_connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
	if(dbus_connection == NULL || dbus_error_is_set(&error)) {
		EPRINTF("Could not connect to Dbus:  %s\n", error.message);
		dbus_error_free(&error);
		return FALSE;
	}

	/* Make sure the program doesn't die if dbus goes down.
	 * TODO Make re-init function. */
	dbus_connection_set_exit_on_disconnect(dbus_connection, FALSE);
	dbus_connection_setup_with_g_main(dbus_connection, NULL);

	dbus_error_free(&error);
	PRINTF("DBus initialized.\n");
	return TRUE;
}

gboolean hal_init() {
	DBusError error;

	if(!dbus_init())
		return FALSE;

	if (!(hal_ctx = libhal_ctx_new())) {
		EPRINTF("Could not create new HAL context.\n");
		return FALSE;
	}
	libhal_ctx_set_dbus_connection(hal_ctx, dbus_connection);

	/* Set callbacks for device removal, addition, and modification */
	libhal_ctx_set_device_property_modified(hal_ctx, hal_property_modified);
	libhal_ctx_set_device_added(hal_ctx, hal_device_added);
	libhal_ctx_set_device_removed(hal_ctx, hal_device_removed);
	PRINTF("Initialized HAL callbacks.\n");

	dbus_error_init(&error);
	if (!libhal_ctx_init(hal_ctx, &error)) {
		EPRINTF("Cannot connect to HAL:  %s\n", error.message);
		dbus_error_free(&error);
		libhal_ctx_free(hal_ctx);
		return FALSE;
	}

	watch_all_devices_with_capability(hal_ctx, "ac_adapter");
	/* Get the initial AC state */
	query_ac_adapters(hal_ctx);
	PRINTF("HAL initialized.\n");
	return TRUE;
}
