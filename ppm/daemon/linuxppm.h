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


#ifndef __INCLUDE_GUARD_LINUXPPM_H_
#define __INCLUDE_GUARD_LINUXPPM_H_

#include <glib.h>
#include "config.h"
#define PRINTF(args...)  fprintf(stdoutfp, args)
#define DPRINTF(args...) if(testfp) fprintf(testfp, args)
#define EPRINTF(args...) fprintf(stderrfp, args)
#define MEMCHECK(ptr) if(ptr == NULL) EPRINTF("Out of Memory\n"), exit(EXIT_FAILURE);

#define PPM_DBUS_STRUCTURE (dbus_g_type_get_struct("GValueArray",	\
						   G_TYPE_STRING,	\
						   G_TYPE_INT,		\
						   G_TYPE_INVALID))

extern int testmode;
extern FILE *stdoutfp;
extern FILE *stderrfp;
extern FILE *testfp;

typedef void (mp_message_func)(char *class, char *command, char *option);
typedef void (mp_init_func)(void);

struct micropolicy {
	char	name[80];
	mp_message_func	*send_message;
};


struct policy_line {
	char class[80];
	char command[80];
	char value[255];
};

struct policy_layer {
	int priority;
	int active;
	gboolean hide;
	char name[80];
	GList *lines;
};

struct policy_property {
	char name[80];
	char value[160];
        char oldvalue[255];
};
struct policy_classes {
	char name[80];
	GList *properties;
};

struct mode_line {
	char layer_name[80];
};

struct policy_mode {
	int active;
	gboolean hide;
	char name[80];
	char event_name[160];
	GList *layers;
};

/* functions for use by micropolicy plugins */
void register_plugin(char *name, mp_message_func *msg_func);


/* functions internal to the PPM */
void load_plugins(char *path);
void send_message(char *class, char *command, char *option);
void parse_policies(char *pathname);
void activate_flatten(void);
void flatten_policies(void);
void find_active(void);
void deactivate_current(void);
void activate_layer(char *name);
void deactivate_layer(char *name);
struct policy_mode *find_mode(char *name);
void get_modes(GPtrArray **, int *);
void deactivate_current_layers(void);
void activate_mode(char *name);
void deactivate_mode(char *name);
void activate_layers_of_all_active_modes(void);
void parse_modes(char *pathname);
void handle_events(char *event_name, gboolean activate);

#endif
