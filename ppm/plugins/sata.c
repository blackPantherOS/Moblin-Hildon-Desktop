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
#include <dirent.h>

#include "linuxppm.h"


static int hotplug = 1;
static int powersave = 0;

static void set_ALPM(int enable)
{
	DIR *dir;
	struct dirent *dirent;
	char filename[PATH_MAX];
	FILE *file;

	dir = opendir("/sys/class/scsi_host");
	if (!dir)
		return;
	do {
		dirent = readdir(dir);
		if (!dirent)
			break;
		if (dirent->d_name[0]=='.')
			continue;
		sprintf(filename,"/sys/class/scsi_host/%s/link_power_management", dirent->d_name);
		file = fopen(filename, "w");
		if (!file)
			continue;
		if (enable)
			fprintf(file, "min_power");
		else
			fprintf(file, "max_performance");
		fclose(file);

	} while (dirent);
	closedir(dir);
}

void sata_message(char *class, char *command, char *option)
{
	if (!class || !command || !option)
		return; /* malformed message */

	if (!(strcmp(class,"storage")==0) && !(strcmp(class,"sata")==0) && (!strcmp(class,"disk")==0))
		return; /* not for us */
	if (strcmp(command, "hotplug")==0) {
		if (strcmp(option,"on")==0) {
			set_ALPM(0);
			hotplug = 1;
		}
		if (strcmp(option,"off")==0) {
			set_ALPM(powersave);
			hotplug = 0;
		}
	}
	if (strcmp(class,"sata")==0 && strcmp(command, "performance")==0) {
		if (strcmp(option,"powersave")==0 && !hotplug) {
			set_ALPM(1);
			powersave = 1;
		}
		if (strcmp(option,"max")==0) {
			set_ALPM(0);
			powersave = 0;
		}
	}

}


void start_plugin(void)
{
	register_plugin("sata", sata_message);
}
