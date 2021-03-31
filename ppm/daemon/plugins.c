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


/* Load and handle micropolicy plugins */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <glib.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <dirent.h>

#include "linuxppm.h"

static GList *plugins;


/*
 * this function is called by the plugins to register themselves with the micropolicy engine
 */

void register_plugin(char *name, mp_message_func *msg_func)
{
	struct micropolicy *policy;
	policy = malloc(sizeof(struct micropolicy));
	assert(policy!=NULL);

	memset(policy, 0, sizeof(struct micropolicy));
	strcpy(policy->name, name);
	policy->send_message = msg_func;
	plugins = g_list_append(plugins, policy);
	PRINTF("Registered plugin: %s \n", name);
}

void load_plugins(char *path)
{
	DIR *dir;
	struct dirent *entry;
	char filename[PATH_MAX];

	dir = opendir(path);
	if (!dir)
		return;
	do {
		void *handle;
		mp_init_func *func;
		entry = readdir(dir);
		if (!entry)
			break;
		/* skip reserved names etc starting with a .  */
		if (entry->d_name[0]=='.')
			continue;
		sprintf(filename,"%s/%s", path, entry->d_name);
		handle = dlopen(filename, RTLD_NOW);
		if (!handle) {
			PRINTF("Invalid plugin: %s \n", filename);
			continue;
		}
		func = dlsym(handle, "start_plugin");
		if (!func) {
			PRINTF("Invalid plugin, no start_plugin: %s \n", filename);
			continue;
		}
		/* now call the initialization function of the plugin */
		func();

	} while (entry != NULL);
	closedir(dir);
}

void send_message(char *class, char *command, char *option)
{
	GList *list;
	struct micropolicy *policy;

	PRINTF("Sending message to %s, command is %s option is %s \n", class, command, option);

	list = g_list_first(plugins);
	while (list) {
		policy = list->data;
		list = g_list_next(list);
		if (policy->send_message)
			policy->send_message(class, command, option);
	}
}
