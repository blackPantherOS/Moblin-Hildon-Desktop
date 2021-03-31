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


#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <getopt.h>
#include <time.h>
#include <sys/poll.h>
#include <grp.h>
#include <signal.h>
#include <dirent.h>
#include <asm/unistd.h>
#include <glib.h>

#include "dbus-bind.h"
#include <dbus/dbus-glib-lowlevel.h>
#include "cei.h"
#include "linuxppm.h"
#include "hal.h"

FILE *stderrfp = NULL;
FILE *stdoutfp = NULL;
FILE *testfp = NULL;

static struct option opts[] = {
	{"foreground", 0, 0, 'f'},
	{"version", 0, 0, 'v'},
	{"help", 0, 0, 'h'},
	{NULL, 0, 0, 0},
};

static const char *opts_help[] = {
	"Run in the foreground.",		/* foreground */
	"Print version information.",		/* version */
	"Print this message.",			/* help */
};

#define __ignore  __attribute__ ((__unused__))

static void
print_usage(char *name)
{
	struct option *opt;
	const char **hlp;
	int max, size;

	printf("Usage: %s [OPTIONS]\n", name);
	max = 0;
	for (opt = opts; opt->name; opt++) {
		size = strlen(opt->name);
		if (size > max) max = size;
	}
	for (opt = opts, hlp = opts_help; opt->name; opt++, hlp++) {
		printf("  -%c, --%s", opt->val, opt->name);
		size = strlen(opt->name);
		for (; size < max; size++)
			printf(" ");
		printf("  %s\n", *hlp);
	}
}

gboolean
check_paths()
{
DIR *dir;

	if(!(dir = opendir(PLUGIN_PATH_USER)))
	{
		EPRINTF("Path not found: %s\n", PLUGIN_PATH);
		return FALSE;
	}
	closedir(dir);
	return TRUE;
}


static DBusHandlerResult ppm_got_message(DBusConnection __ignore *connection,
					 DBusMessage *message,
					 void __ignore *user_data)
{
	DBusError error;
	char *class = NULL;
	char *name = NULL;
	char *value = NULL;
	/* handle disconnect events first */

	if (dbus_message_is_signal(message, DBUS_ERROR_DISCONNECTED,
		"Disconnected")) {
		EPRINTF("Error dbus disconnect\n");
		exit(EXIT_FAILURE);
	}
        if (dbus_message_is_signal(message, DBUS_CEI_INTERFACE_CMD,
				   "SetdCmd")) {

		dbus_error_init(&error);
		if (dbus_message_get_args(message, &error,
					  DBUS_TYPE_STRING, &class,
					  DBUS_TYPE_STRING, &name,
					  DBUS_TYPE_STRING, &value,
					  DBUS_TYPE_INVALID)) {
			
			EPRINTF("We are recieving command %s %s %s\n",
				 class, name, value);

			send_message(class, name, value);
		}
		dbus_error_free(&error);
		return DBUS_HANDLER_RESULT_HANDLED;
	}
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}


int
main(int argc, char **argv)
{
	int i, foreground = 0;
	GMainLoop *loop;
	FILE *fp;
	DBusConnection *bus = NULL;
	DBusError error;

	stderrfp = stderr;
	stdoutfp = stdout;

	/* parse the command line */
	while((i = getopt_long(argc, argv, "fvht", opts, NULL)) >= 0)
	{
		switch (i) {
		case 'f':
			foreground = 1;
			setbuf(stdout, NULL);
			setbuf(stderr, NULL);
			break;
		case 'v':
			printf("PPM version %d\n", VERSION);
			exit(0);
		case 'h':
			print_usage(argv[0]);
			exit(0);
		default:
			print_usage(argv[0]);
			exit(1);
		}
	}

	if(!foreground)
	{
		if((fp = fopen(PPMD_LOGFILE_USER, "w")) == NULL)
		{
			fprintf(stderr, "Failed to open logfile: %s\n",
				PPMD_LOGFILE);
			exit(EXIT_FAILURE);
		}
		setbuf(fp, NULL);
		stdoutfp = fp;
		stderrfp = fp;
		if(testfp) testfp = fp;
	}

	if(!check_paths())
		exit(EXIT_FAILURE);

	/* daemonize if not in foreground*/
	if (!foreground) {
		switch(fork()) {
		case -1:
			EPRINTF("%s: fork: %s\n", argv[0], strerror(errno));
			exit(EXIT_FAILURE);
		case 0:
			/* child */
			setsid();
			umask(0);
			break;
		default:
			/* parent */
			exit(EXIT_SUCCESS);
		}
	}

	loop = g_main_loop_new(NULL,FALSE);

	g_type_init();
	dbus_error_init(&error);

	bus = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
	if (!bus) {
		EPRINTF("Error dbus systen is not available\n");
		exit(EXIT_FAILURE);
	} 

	dbus_connection_setup_with_g_main(bus, NULL);

	dbus_bus_add_match(bus,
			   "type='signal',"
			   "interface='" DBUS_CEI_INTERFACE_CMD "',"
			   "path='" DBUS_CEI_PATH_CMD "'",
			   &error);
	if (dbus_error_is_set(&error)) {
		PRINTF("Error can not register with PPMD signal: %s: %s", error.name, error.message);
		exit(EXIT_FAILURE);
	}

	dbus_connection_add_filter(bus, ppm_got_message, NULL, NULL);

	load_plugins(PLUGIN_PATH_USER);


	if(!foreground)
		PRINTF("PPM Daemon start (PID %d)\n", getpid());

	g_main_loop_run(loop);

	dbus_bus_remove_match(bus, "type='signal',interface='" DBUS_CEI_INTERFACE_CMD "'", &error);

	dbus_error_free(&error);
	g_main_loop_unref(loop);

	return 0;
}
