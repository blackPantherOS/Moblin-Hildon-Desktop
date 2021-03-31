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

#define	MAX_MODES	10
FILE *stdoutfp = NULL;
FILE *stderrfp = NULL;
FILE *testfp = NULL;

void test_message(char *class, char *command, char *option)
{
	printf("%s %s %s\n", class, command, option);
}

void start_plugin(void)
{
	register_plugin("test", test_message);
}

/* Usage: ppmtest mode... */
int main(int argc, char **argv) {
	gchar *modes_to_activate[MAX_MODES];
	int index = 0;

	stderrfp = stderr;
	stdoutfp = fopen("/dev/null", "w");
	testfp = stdoutfp;

	if (argc < 2) {
		printf("ERROR: Must call ppmtest with one or more modes.\n");
		exit(0);
	}
	for (index = 1; index < argc; index++)
		modes_to_activate[index-1] = argv[index];
	modes_to_activate[index-1] = NULL;

	/* Only install our test plugin. */
	start_plugin();
	parse_policies(TEST_POLICY_PATH);
	/* Note that parse_modes will activate the default mode. */
	parse_modes(TEST_MODES_PATH);

	/* cei_activate_modes will disable the default mode
	 * before activating the ones we tell it to. */
	cei_activate_modes(NULL, modes_to_activate, NULL);
	exit(1);
}
