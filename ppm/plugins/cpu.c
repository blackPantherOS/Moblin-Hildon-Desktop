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

static void select_governor(char *gov)
{
	DIR *dir;
	struct dirent *dirent;
	FILE *file;
	char filename[PATH_MAX];

	PRINTF("Selecting governor %s \n", gov);

	dir = opendir("/sys/devices/system/cpu");
	if (!dir)
		return;

	while ((dirent = readdir(dir))) {
		if (dirent->d_name[0]=='.' || strlen(dirent->d_name)<3)
			continue;
		sprintf(filename, "/sys/devices/system/cpu/%s/cpufreq/scaling_governor", dirent->d_name);
		file = fopen(filename, "w");
		if (!file)
			continue;
		fprintf(file, "%s", gov);
		fclose(file);
	}

	closedir(dir);
}

static void set_ondemand(int bias, int threshold)
{
	DIR *dir;
	struct dirent *dirent;
	FILE *file;
	char filename[PATH_MAX];

	dir = opendir("/sys/devices/system/cpu");
	if (!dir)
		return;

	while ((dirent = readdir(dir))) {
		if (dirent->d_name[0]=='.' || strlen(dirent->d_name)<3)
			continue;
		sprintf(filename, "/sys/devices/system/cpu/%s/cpufreq/ondemand/powersave_bias", dirent->d_name);
		file = fopen(filename, "w");
		if (!file)
			continue;
		fprintf(file, "%i", bias);
		fclose(file);
		sprintf(filename, "/sys/devices/system/cpu/%s/cpufreq/ondemand/up_threshold", dirent->d_name);
		file = fopen(filename, "w");
		if (!file)
			continue;
		fprintf(file, "%i", threshold);
		fclose(file);
	}

	closedir(dir);
}


static void select_performance(char *option)
{
	PRINTF("cpu performance option %s \n", option);
	if (strcmp(option, "max")==0) {
		select_governor("performance");
		return;
	}
	if (strcmp(option, "balanced")==0) {
		select_governor("ondemand");
		set_ondemand(0, 50);
		return;
	}
	if (strcmp(option, "maxbattery")==0) {
		select_governor("ondemand");
		set_ondemand(5, 80);
		return;
	}
	if (strcmp(option, "cool")==0) {
		select_governor("powersave");
		return;
	}
}

void cpu_message(char *class, char *command, char *option)
{
	if (!class || !command)
		return; /* malformed message */
	if (strcmp(class, "cpu"))
		return; /* not for us */

	if (strcmp(command, "performance")==0 && option)
		select_performance(option);
}


void start_plugin(void)
{
	register_plugin("cpu", cpu_message);
}
