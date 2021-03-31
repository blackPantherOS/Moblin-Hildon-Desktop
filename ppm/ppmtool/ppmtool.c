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
#include <glib.h>
#include <dirent.h>
#include "ppmtool.h"
#include "dbus-bind.h"
#include "ppmtool-interface.h"

#define	MAX_MODES	10

#define PPMTOOL_NONE		0
#define PPMTOOL_ACTIVATE	1
#define PPMTOOL_APPEND_ACTIVE	2
#define PPMTOOL_DEACTIVATE	3

int num_modes = 0;
struct mode_t *modes = NULL;

struct cmddef {
	const char *name;
	const char *arg;
};

static struct option opts[] = {
	{"list", 0, 0, 'l'},
	{"activate", 1, 0, 'a'},
	{"version", 0, 0, 'v'},
	{"gtk", 0, 0, 'g'},
	{"help", 0, 0, 'h'},
	{NULL, 0, 0, 0},
};

static const char *opts_help[] = {
	"list all available ppm modes.",	/* list modes */
	"activate the passed-in mode.",		/* change modes */
	"Print version information.",		/* version */
	"gtk dispaly as gtk app.",		/* run as gtk app */
	"Print this message.",			/* help */
};

static void
print_usage(char *name)
{
	struct option *opt;
	const char **hlp;
	int max, size;

	PRINTF("Usage: %s [OPTIONS]\n", name);
	max = 0;
	for (opt = opts; opt->name; opt++) {
		size = strlen(opt->name);
		if (size > max) max = size;
	}
	for (opt = opts, hlp = opts_help; opt->name; opt++, hlp++) {
		PRINTF("  -%c, --%s", opt->val, opt->name);
		size = strlen(opt->name);
		for (; size < max; size++)
			PRINTF(" ");
		PRINTF("  %s\n", *hlp);
	}

}

void print_active_modes()
{
	int i;

	PRINTF("\nCurrently available modes:\n");
	if(cei_query_modes(&modes, &num_modes)) {
		for(i = 0; i < num_modes; i++)
			PRINTF("Mode %d: %s%s\n", i, modes[i].name,
					((modes[i].active == 1)?" (active)":""));
	}
}

void print_prompt(int action)
{
	print_active_modes();
	PRINTF("\nType mode numbers (separated by spaces) to ");
	switch (action) {
	case PPMTOOL_ACTIVATE:
		PRINTF("activate ");
		break;
	case PPMTOOL_APPEND_ACTIVE:
		PRINTF("append active ");
		break;
	case PPMTOOL_DEACTIVATE:
		PRINTF("deactivate ");
		break;
	}
	PRINTF("modes,\nor type q to quit\n");
	PRINTF("or type b to go back to prev menu\n");
	PRINTF("> ");
}

void print_main_menu()
{

        PRINTF("\nOptions:\n");
	PRINTF("1. change active modes\n");
	PRINTF("2. append to active modes\n");
	PRINTF("3. deactivate modes\n");
	PRINTF("\nType option numbers ,\nor type q to quit\n");
	PRINTF("> ");
}

/* Must be called after global varible modes is set */
int valid_mode(char * mode) {
	int i;
	for(i = 0; i < num_modes; i++)
		if(strcmp(modes[i].name, mode) == 0)
			return 1;
	return 0;
}

int
main(int argc, char **argv)
{
	int i;
	int index = 0;
	char ch[100];
	int list = 0;
	int change_modes = 0;
	char* modes_to_activate[MAX_MODES];
	int action = PPMTOOL_NONE;
	gboolean ret;
	gboolean win_mode = FALSE;

	setbuf(stdout, NULL);


	if(!cei_client_init())
		return 1;

	if(!cei_query_modes(&modes, &num_modes)) {
		cei_client_exit();
		return 1;
	}

	while((i = getopt_long(argc, argv, "lvhag:", opts, NULL)) >= 0)
	{
		switch (i) {
		case 'a':
			change_modes = 1;

			if(valid_mode(optarg)) {
				printf("Activating mode(s): %s", optarg);
				modes_to_activate[index++] = optarg;
			}

			/* check end of args for more modes */
			for(; optind < argc; optind++) {
				if(argv[optind][0] != '-' && valid_mode(argv[optind])) {
					modes_to_activate[index++] = argv[optind];
					printf(", %s", argv[optind]);
				}
			}
			printf("\n");
			modes_to_activate[index] = NULL;
			break;
		case 'l':
			list = 1;
			break;
		case 'v':
			PRINTF("PPM version %lf\n", VERSION);
			cei_client_exit();
			exit(0);
		case 'h':
			print_usage(argv[0]);
			cei_client_exit();
			exit(0);
		case 'g':
			win_mode = TRUE;
			break;
		default:
			print_usage(argv[0]);
			cei_client_exit();
			exit(1);
		}
	}

	if(list)
	{
		print_active_modes();
		cei_client_exit();
		return 0;
	}

	if (win_mode) {
		show_ppm(argc, argv);
		if (modes) {
			for(i = 0; i < num_modes; i++) {
				g_free(modes[i].name);
                        	modes[i].name = NULL;
                	}
                	free(modes);
                	modes = NULL;
		}
		cei_client_exit();
		return 0;
	}

	if(change_modes && index) {
		int ret = cei_activate_modes(modes_to_activate);
		cei_client_exit();
		return ret;
	}

	printf("\n");
	print_active_modes();

	while(1) {
		char *token, *str, *save_ptr;

		if (action == PPMTOOL_NONE) {
			print_main_menu();

                	if (fgets(ch, 100, stdin) == NULL) continue;
                	if(ch[0] == 'q' || ch[0] == 'Q') {
                        	cei_client_exit();
                        	return 0;
                	}

			token = strtok_r(ch, " \t\n", &save_ptr);
			if (!token)
				continue;

			if(sscanf(token, "%d", &action) == 1) {
				if ((action < PPMTOOL_ACTIVATE) ||
				    (action > PPMTOOL_DEACTIVATE)) {
					action = PPMTOOL_NONE;
					continue;
				}

			} else
				continue;
		}

		print_prompt(action);

		if (fgets(ch, 100, stdin) == NULL) continue;
		if(ch[0] == 'q' || ch[0] == 'Q') {
			cei_client_exit();
			return 0;
		} else if(ch[0] == 'b' || ch[0] == 'B') {
			action = PPMTOOL_NONE;
			continue;
		}

		index = 0;
		for(i = 0; i < MAX_MODES; i++)
			modes_to_activate[i] = NULL;

		for (str = &ch[0]; ;str = NULL) {

			token = strtok_r(str, " \t\n", &save_ptr);
			if (!token)
				break;

			if((sscanf(token, "%d", &i) == 1) &&
				(i >= 0) && (i < num_modes)) {

				PRINTF("ppmtool: activating mode %d - %s...\n",
					i, modes[i].name);

				modes_to_activate[index] = modes[i].name;
				index++;
			}
		}
		modes_to_activate[index] = NULL;
		if (index) {
			switch (action) {
			case PPMTOOL_ACTIVATE :
				ret = cei_activate_modes(modes_to_activate);
				break;
                        case PPMTOOL_APPEND_ACTIVE :
                                ret = cei_append_active_modes(modes_to_activate);
                                break;
                        case PPMTOOL_DEACTIVATE :
                                ret = cei_deactivate_modes(modes_to_activate);
                                break;
			default :
				ret = TRUE;
				break;
			}
			if (ret == FALSE) {
				cei_client_exit();
				return 1;
			}
		}

		for(i = 0; i < num_modes; i++) {
			g_free(modes[i].name);
			modes[i].name = NULL;
		}
		free(modes);
		modes = NULL;
		if(!cei_query_modes(&modes, &num_modes)) {
			cei_client_exit();
			return 1;
		}

	}

	cei_client_exit();
	return 0;
}
