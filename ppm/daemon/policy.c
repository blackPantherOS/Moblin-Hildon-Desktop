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
#include <ctype.h>
#include <glib.h>
#include <dbus/dbus-glib-bindings.h>
#include "dbus-bind.h"
#include "linuxppm.h"

static GList *layers;
static GList *modes;
/*
 * Find the first separator char in string line
 * the separator should be one or more space or tab.
 * return the pointer of the first separator
 * or NULL if there isn't any.
 */
static char * find_separator( char * line)
{
	char *c;
	c = line;
	while (*c != 0) {
		if (isspace(*c))
			return c;
		c++;
	}
	return NULL;
}

/*
 * Read a specific policy description file and add it to the
 * list of layers under "name".
 */
static void parse_policy_file(char *filename, char *name)
{
	FILE *file;
	char line[4096];
	struct policy_line *pl;
	struct policy_layer *layer;

	layer = malloc(sizeof(struct policy_layer));
	assert(layer!=NULL);
	memset(layer, 0, sizeof(struct policy_layer));

	strcpy(layer->name, name);
	layer->priority = strtoull(name, NULL, 10);

	layers = g_list_append(layers, layer);

	file = fopen(filename, "r");
	while (!feof(file)) {
		char *c, *c2;
		if (fgets(line, 4095, file)==NULL)
			break;
		if (line[0]=='#')
			continue;

		pl = malloc(sizeof(struct policy_line));
		if (!pl)
			continue;
		memset(pl, 0, sizeof(struct policy_line));

                c = find_separator(line);
        	if (!c)
                    continue;

                *c=0;
		strcpy(pl->class, line);
		c++;
		while (isspace(*c) ) c++;
		c2=c;

                c = find_separator(c);
                if (!c)
                    continue;

                *c = 0;
		strcpy(pl->command, c2);
		c++;
		while (isspace(*c)) c++;
		c2=strchr(c,'\n');
		if (c2) *c2=0;
		strcpy(pl->value, c);
		layer->lines = g_list_append(layer->lines, pl);
	}
	fclose(file);
}

/*
 * Read a specific mode description file and add it to the
 * list of modes under "name".
 */
static void parse_mode_file(char *filename, char *name)
{
	FILE *file;
	char line[4096];
	struct mode_line *pl;
	struct policy_mode *mode;
	char *str, *save_ptr, *token;

	mode = malloc(sizeof(struct policy_mode));
	assert(mode!=NULL);
	memset(mode, 0, sizeof(struct policy_mode));

	strcpy(mode->name, name);
	mode->active = 0;

	modes = g_list_append(modes, mode);

	file = fopen(filename, "r");
	while (!feof(file)) {
		char *c, *c2;
		if (fgets(line, 4095, file)==NULL)
			break;
		if (line[0]=='#')
			continue;

		if (strstr(line ,"flags")) {
			token = strtok_r(line, " =\t\n", &save_ptr);
			for (str = NULL; ;) {
				token = strtok_r(str, " =\t\n", &save_ptr);
				if (!token)
					break;
				if (strcmp(token, "invisible") == 0)
					mode->hide = TRUE;
				else if (strcmp(token, "default") == 0)
					mode->active = TRUE;
				PRINTF("add flags:%s: \n", token);
			}
			continue;
		}

		if (strstr(line ,"events")) {
			token = strtok_r(line, " =\t\n", &save_ptr);
			if (!token)
				break;
			token = strtok_r(NULL, " =\t\n", &save_ptr);
			if (!token)
				break;

			strcpy(mode->event_name, token);
			PRINTF("add event:%s: \n", token);
			break;
			continue;
		}

		pl = malloc(sizeof(struct mode_line));
		if (!pl)
			continue;
		memset(pl, 0, sizeof(struct mode_line));

		c = line;
                while (isspace(*c)) c++;
                c2=strchr(c,'\n');
                if (c2) *c2=0;
                strcpy(pl->layer_name, c);
		mode->layers = g_list_append(mode->layers, pl);
	}
	fclose(file);
}

static gint layer_sort(gconstpointer A, gconstpointer B)
{
	struct policy_layer *a, *b;
	a = (struct policy_layer*)A;
	b = (struct policy_layer*)B;
	return a->priority - b->priority;
}

/*
 * Do a numeric sort of the policy layers based on priority
 */
static void sort_policies(void)
{
	layers = g_list_sort(layers, layer_sort);
}

static void print_policies(void)
{
	GList *list;
	struct policy_layer *layer;
	list = g_list_first(layers);
	while (list) {
		layer = list->data;
		list = g_list_next(list);
		PRINTF(" Layer %i -- %s \n", layer->priority, layer->name);
	}
}

static void print_policy_modes(void)
{
	GList *list;
	struct policy_mode *mode;
	list = g_list_first(modes);
	while (list) {
		mode = list->data;
		list = g_list_next(list);
		PRINTF(" Mode %s \n", mode->name);
	}
}

void get_modes(GPtrArray ** mode_list, int *count)
{
	GList *list;
	struct policy_mode *mode;
	int i = 0;
	int active;
	GValue *value;
	gboolean ret;

	print_policies();
	*count = g_list_length(modes);

	*mode_list = g_ptr_array_sized_new(*count);

	list = g_list_first(modes);
	for(i = 0; (i < (*count))&&(list != NULL);) {
		mode = list->data;
		list = g_list_next(list);

		if (mode->hide == TRUE)
			continue;

		/* allocate memory and initialize it to structure type */
		value  = g_new0(GValue, 1);
		g_value_init(value, PPM_DBUS_STRUCTURE);

		/* fill up the struct values */
		g_value_take_boxed(value,
		   dbus_g_type_specialized_construct(PPM_DBUS_STRUCTURE));

		if (mode->active)
			active = 1;
		else
			active = 0;

		ret = dbus_g_type_struct_set(value, 0,
		   mode->name, 1, active , -1);

		if (ret == TRUE) {
			g_ptr_array_add(*mode_list, g_value_get_boxed(value));
			i++;
		}
		g_free(value);
	}

	*count = i;
}

/*
 * Read the various policy files from disk
 * add fill the "layers" list with a sorted
 * range of policy entries.
 */
void parse_policies(char *pathname)
{
	DIR *dir;
	struct dirent *entry;

	dir = opendir(pathname);
	if (!dir)
		return;
	do {
		char filename[PATH_MAX];
		entry = readdir(dir);
		if (!entry)
			break;
		if (entry->d_name[0]=='.')
			continue;
		sprintf(filename, "%s/%s", pathname, entry->d_name);
		parse_policy_file(filename, entry->d_name);

	} while (entry);
	closedir(dir);
	sort_policies();
	print_policies();
}

/*
 * Read the various modes files from disk
 * add fill the "modes" list.
 */
void parse_modes(char *pathname)
{
	DIR *dir;
	struct dirent *entry;

	dir = opendir(pathname);
	if (!dir)
		return;
	do {
		char filename[PATH_MAX];
		entry = readdir(dir);
		if (!entry)
			break;
		if (entry->d_name[0]=='.')
			continue;
		sprintf(filename, "%s/%s", pathname, entry->d_name);
		parse_mode_file(filename, entry->d_name);

	} while (entry);
	closedir(dir);
	print_policy_modes();
}



static GList *policy_classes;

static struct policy_classes *find_class(char *name)
{
	GList *list;
	struct policy_classes *class;
	list = g_list_first(policy_classes);
	while (list) {
		struct policy_classes *class;
		class = list->data;
		list = g_list_next(list);
		if (strcmp(class->name, name)==0)
			return class;
	}
	/* doesn't exist yet */
	class = malloc(sizeof(struct policy_classes));
	assert(class!=NULL);
	memset(class, 0, sizeof(struct policy_classes));
	strcpy(class->name, name);
	policy_classes = g_list_append(policy_classes, class);
	return class;
}

static struct policy_property *find_property(char *name, struct policy_classes *class)
{
	GList *list;
	struct policy_property *property;
	list = g_list_first(class->properties);
	while (list) {
		property = list->data;
		list = g_list_next(list);
		if (strcmp(property->name, name)==0)
			return property;
	}
	/* doesn't exist yet */
	property = malloc(sizeof(struct policy_property));
	assert(property!=NULL);
	memset(property, 0, sizeof(struct policy_property));
	strcpy(property->name, name);
	class->properties = g_list_append(class->properties, property);
	return property;
}

/*
 * Create a compound view of the various policies into the policy_classes
 * variable. This is similar to a "flatten visible layers" operation in
 * a program like the gimp.
 */
void flatten_policies(void)
{
	GList *list;
	struct policy_layer *layer;
	list = g_list_first(layers);
	while (list) {
		GList *list2;
		struct policy_line *line;
		layer = list->data;
		list = g_list_next(list);
		if (!layer->active)
			continue;

		list2 = layer->lines;
		while (list2) {
			struct policy_classes *class;
			struct policy_property *property;
			line = list2->data;
			class = find_class(line->class);
			if (!class)
				continue;
			property = find_property(line->command, class);
			if (!property)
				continue;
			strcpy(property->value, line->value);
			list2 = g_list_next(list2);
		}
	}
}

/* List all active layers */
void find_active(void)
{
	GList *list;
	struct policy_layer *layer;
	list = g_list_first(layers);
	while(list) {
		layer = list->data;
		list = g_list_next(list);
		if (layer->active)
			PRINTF("Active Layer: %s\n", layer->name);
	}
}

/* Mark all actives layers as non-active */
void deactivate_current_layers(void)
{
	GList *list;
	struct policy_layer *layer;
	list = g_list_first(layers);
	while(list) {
		layer = list->data;
		list = g_list_next(list);
		if (layer->active) {
			layer->active = 0;
			PRINTF("Layer %s deactivated\n", layer->name);
		}
	}
}

/* Mard all active modes to non-active, skib
 * hidden active one since they were set by event
 * not the user.
 */
void deactivate_current(void)
{
	GList *list;
	struct policy_mode *mode;
	list = g_list_first(modes);
	while(list) {
		mode = list->data;
		list = g_list_next(list);
		if (mode->active && (mode->hide == FALSE))
			mode->active = 0;
	}
	deactivate_current_layers();
}

/* Search for the layer then activate it */
void activate_layer(char *name)
{
	GList *list;
	struct policy_layer *layer;
	list = g_list_first(layers);
	while(list) {
		layer = list->data;
		list = g_list_next(list);
		if (((strcmp(name, layer->name) == 0) && !layer->active)) {
			layer->active = 1;
			PRINTF("Layer %s activated\n", layer->name);
			break;
		}
	}
}


/* Search for the mode then activate it and all its
 * layers in its list.
 */
void activate_mode(char *name)
{
        GList *list;
        struct policy_mode *mode = NULL;
        list = g_list_first(modes);
	struct mode_line *layer = NULL;

        while(list) {
                mode = list->data;
                list = g_list_next(list);
                if (((strcmp(name, mode->name) == 0) && !mode->active)) {
                        mode->active = 1;
			PRINTF("Mode %s activated\n", mode->name);
                        break;
                } else
			mode = NULL;
        }

	if (mode && mode->active) {
		list = g_list_first(mode->layers);
		while (list) {
			layer = list->data;
			list = g_list_next(list);
			activate_layer(layer->layer_name);
		}
	}
}

/* Go through all active modes then activate all
 * the layers listed in the modes list
 */
void activate_layers_of_all_active_modes(void)
{
	GList *list;
	GList *list1;
	struct policy_mode *mode;
	list = g_list_first(modes);
	struct mode_line *layer;

	while(list) {
		mode = list->data;
		list = g_list_next(list);
		if (mode->active) {
			list1 = g_list_first(mode->layers);
			while (list1) {
				layer = list1->data;
				list1 = g_list_next(list1);
				activate_layer(layer->layer_name);
			}
		}
	}
}

/* Search the layers for the given name then
 * marked as non-active.
 */
void deactivate_layer(char *name)
{
	GList *list;
	struct policy_layer *layer;
	list = g_list_first(layers);
	while(list) {
		layer = list->data;
		list = g_list_next(list);
		if (((strcmp(name, layer->name) == 0) && layer->active)) {
			layer->active = 0;
			PRINTF("Layer %s inactivated\n", layer->name);
			break;
		}
	}
}

/* Search all modes for the given name then
 * marked as non-active.
 */
void deactivate_mode(char *name)
{
	GList *list;
	struct policy_mode *mode;
	list = g_list_first(modes);
	while(list) {
		mode = list->data;
		list = g_list_next(list);
		if (((strcmp(name, mode->name) == 0) && mode->active)) {
			mode->active = 0;
			PRINTF("Mode %s deactivated\n", mode->name);
			break;
		}
	}
}

/* Search for a given mode. */
struct policy_mode *find_mode(char *name)
{
	GList *list;
	struct policy_mode *mode = NULL;
	list = g_list_first(modes);
	while(list) {
		mode = list->data;
		list = g_list_next(list);
		if ((strcmp(mode->name, name)) == 0) {
			break;
		} else
			mode = NULL;
	}
	return mode;
}

/* Search all mode for the one than handle the given dbus
 * event, Then activate/deactivate it.
 */
void handle_events(char *event_name, gboolean activate)
{
	struct policy_mode *mode = NULL;
	GList *list;

	PRINTF("handle event %s\n", event_name);
        list = g_list_first(modes);
        while(list) {
                mode = list->data;
                list = g_list_next(list);
                if ((strcmp(mode->event_name, event_name)) == 0) {
                        break;
                } else
                        mode = NULL;
        }

	if (mode) {
		deactivate_current_layers();

		if (activate) {
			activate_mode(mode->name);
		} else {
			deactivate_mode(mode->name);
		}

		activate_layers_of_all_active_modes();
		flatten_policies();
		activate_flatten();
		find_active();
	}

}

/*
 * This function looks at the flattened data and sends out
 * messages for the values that changed since the previous flatten.
 *
 * This function needs to be called every time any layer changes
 * from active to inactive or the other way around.
 */
void activate_flatten(void)
{
	GList *list;
	struct policy_classes *class;
	list = g_list_first(policy_classes);
	while (list) {
		GList *list2;
		struct policy_property *property;
		class = list->data;
		list = g_list_next(list);

		list2 = g_list_first(class->properties);
		while (list2) {
			property = list2->data;
			list2 = g_list_next(list2);
			if (strcmp(property->value, property->oldvalue)) {
				send_message(class->name, property->name,
					     property->value);
				/* send signal */
				cei_send_cmd(class->name, property->name,
					     property->value);
			}
			strcpy(property->oldvalue, property->value);
		}

	}
}

gboolean
select_modes(gchar **active_modes, gboolean append_only)
{
	struct policy_mode *mode;

	if(active_modes == NULL) return FALSE;

	find_active();

	if (append_only == FALSE)
		deactivate_current();

	while (*active_modes) {
		mode = find_mode(*active_modes);
		if(mode == NULL) {
			EPRINTF("Invalid mode: %s\n", *active_modes);
			return FALSE;
		}
		activate_mode(mode->name);
		active_modes++;
	}
	activate_layers_of_all_active_modes();
	flatten_policies();
	activate_flatten();
	find_active();

	return TRUE;
}

gboolean
cei_activate_modes(CEI *obj, gchar **active_modes, GError **error)
{

	return select_modes(active_modes, FALSE);
}

gboolean
cei_append_active_modes(CEI *obj, gchar **active_modes, GError **error)
{

        return select_modes(active_modes, TRUE);
}

gboolean
cei_deactivate_modes(CEI *obj, gchar **deactive_modes, GError **error)
{
	struct policy_mode *mode;

	if(deactive_modes == NULL) return FALSE;

	find_active();
	deactivate_current_layers();

	while (*deactive_modes) {
		mode = find_mode(*deactive_modes);
		if(mode == NULL) {
			EPRINTF("Invalid mode: %s\n", *deactive_modes);
			return FALSE;
		}
		deactivate_mode(mode->name);
		deactive_modes++;
	}
	activate_layers_of_all_active_modes();
	flatten_policies();
	activate_flatten();
	find_active();


	return TRUE;
}
