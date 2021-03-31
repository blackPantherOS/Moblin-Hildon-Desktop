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
#include <asm/types.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>

/* work around a bug in debian -- it exposes kernel internal types to userspace */
#define u64 __u64
#define u32 __u32
#define u16 __u16
#define u8 __u8
#include <linux/ethtool.h>
#undef u64
#undef u32
#undef u16
#undef u8

#include "linuxppm.h"

static int radio_allowed = 1;
static int wifi_on = 1;



static void rfkill(int value)
{
	FILE *file;
	int sock;
	struct ifreq ifr;
	struct ethtool_value ethtool;
	struct ethtool_drvinfo driver;

	file = popen("/sbin/iwpriv -a 2> /dev/null", "r");
	if (!file) {
		file = popen("/usr/sbin/iwpriv -a 2> /dev/null", "r");
		if (!file)
			return;
	}
	while (!feof(file)) {
		char line[1024];
		char rfkill_path[PATH_MAX];
		memset(line, 0, 1024);
		if (fgets(line, 1023, file)==NULL)
			break;
		if (strstr(line, "get_power:Power save level")) {
			FILE *rf;
			char *c;
			c = strchr(line, ' ');
			if (c) *c = 0;

			memset(&ifr, 0, sizeof(struct ifreq));
			memset(&ethtool, 0, sizeof(struct ethtool_value));

			sock = socket(AF_INET, SOCK_DGRAM, 0);
			if (sock<0)
				continue;

			strcpy(ifr.ifr_name, line);

			memset(&driver, 0, sizeof(driver));
			driver.cmd = ETHTOOL_GDRVINFO;
		        ifr.ifr_data = (void*) &driver;
		        ioctl(sock, SIOCETHTOOL, &ifr);

			sprintf(rfkill_path,"/sys/bus/pci/devices/%s/rf_kill", driver.bus_info);
			close(sock);
			rf = fopen(rfkill_path, "w");
			if (!rf)
				continue;
			fprintf(rf, "%i", value);
			fclose(rf);
		}
	}
	pclose(file);
}

static void wifi_power(int value)
{
	FILE *file;
	int ret;

	file = popen("/sbin/iwpriv -a 2> /dev/null", "r");
	if (!file) {
		file = popen("/usr/sbin/iwpriv -a 2> /dev/null", "r");
		if (!file)
			return;
	}
	while (!feof(file)) {
		char line[1024];
		char power_path[PATH_MAX];
		memset(line, 0, 1024);
		if (fgets(line, 1023, file)==NULL)
			break;
		if (strstr(line, "get_power:Power save level")) {
			char *c;
			c = strchr(line, ' ');
			if (c) *c = 0;

			sprintf(power_path, "/sbin/iwpriv %s set_power %i", line, value);
			ret = system(power_path);
		}
	}
	pclose(file);
}


static void wifi_performance(char *option)
{
	if (!option)
		return;
	if (strcmp(option, "max")==0)
		wifi_power(6);

	if (strcmp(option, "powersave")==0)
		wifi_power(5);
	if (strcmp(option, "cool")==0)
		wifi_power(5);

	if (strcmp(option, "balanced")==0)
		wifi_power(1);
}



static void radio_message(char *command, char *option)
{
	if (strcmp(command, "radio"))
		return;
	if (strcmp(option, "off")==0) {
		radio_allowed = 0;
		rfkill(1);
		return;
	}
	if (strcmp(option, "allowed")==0) {
		/* if we go from no radio->radio and wireless is on, enable the radio */
		if (!radio_allowed && wifi_on)
			rfkill(0);
		radio_allowed = 1;
		return;
	}
}

static void wifi_message(char *command, char *option)
{
	if (strcmp(command, "radio")==0 && strcmp(option, "off")==0) {
		if (wifi_on)
			rfkill(1);
		wifi_on = 0;
		return;
	}
	if (strcmp(command, "radio")==0 && strcmp(option, "on")==0) {
		/* if we go from no wifi->wifi enable the radio */
		if (radio_allowed && !wifi_on)
			rfkill(0);
		wifi_on = 1;
		return;
	}

	if (strcmp(command, "performance")==0) {
		wifi_performance(option);
		return;
	}
}


static void wireless_message(char *class, char *command, char *option)
{
	if (!class || !command)
		return; /* malformed message */

	if (strcmp(class, "wifi")==0)
		wifi_message(command, option);

	if (strcmp(class, "radio")==0)
		radio_message(command, option);
}


void start_plugin(void)
{
	register_plugin("wifi", wireless_message);
}
