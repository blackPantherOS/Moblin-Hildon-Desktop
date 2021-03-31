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

#include "dbus-bind.h"
#include "linuxppm.h"
#include "hal.h"

FILE *stderrfp = NULL;
FILE *stdoutfp = NULL;
FILE *testfp = NULL;

static struct option opts[] = {
	{"testmode", 0, 0, 't'},
	{"foreground", 0, 0, 'f'},
	{"version", 0, 0, 'v'},
	{"help", 0, 0, 'h'},
	{NULL, 0, 0, 0},
};

static const char *opts_help[] = {
	"Test mode, enable debug printing."	/* testing mode */
	"Run in the foreground.",		/* foreground */
	"Print version information.",		/* version */
	"Print this message.",			/* help */
};

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

	if(!(dir = opendir(PLUGIN_PATH)))
	{
		EPRINTF("Path not found: %s\n", PLUGIN_PATH);
		return FALSE;
	}
	closedir(dir);
	if(!(dir = opendir(POLICY_PATH)))
	{
		EPRINTF("Path not found: %s\n", POLICY_PATH);
		return FALSE;
	}
	closedir(dir);
	return TRUE;
}

int
main(int argc, char **argv)
{
	int i, foreground = 0;
	GMainLoop *loop;
	FILE *fp;

	stderrfp = stderr;
	stdoutfp = stdout;

	/* parse the command line */
	while((i = getopt_long(argc, argv, "fvht", opts, NULL)) >= 0)
	{
		switch (i) {
		case 't':
			testfp = stdout;
			break;
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
		if((fp = fopen(PPMD_LOGFILE, "w")) == NULL)
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

	/* start the dbus connection */
	if(!cei_server_init())
		return 1;

	load_plugins(PLUGIN_PATH);
	parse_policies(POLICY_PATH);
	parse_modes(MODES_PATH);

	/* at startup just load the default policy */

	activate_layers_of_all_active_modes();
	flatten_policies();
	activate_flatten();

	/* start the HAL connection */
	if (!hal_init())
		exit(EXIT_FAILURE);

	if(!foreground)
		PRINTF("PPM Daemon start (PID %d)\n", getpid());

	g_main_loop_run(loop);

	g_main_loop_unref(loop);
	return 0;
}
