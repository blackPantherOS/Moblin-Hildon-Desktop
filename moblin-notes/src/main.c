/*
 *  main.c
 *  This file is part of Mousepad
 *
 *  Copyright (C) 2006 Benedikt Meurer <benny@xfce.org>
 *  Copyright (C) 2004 Tarot Osuji
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "mousepad.h"

static gint rpc_handler (const gchar *interface, const gchar *method, GArray *arguments, gpointer data, osso_rpc_t *retval)
{
        g_return_val_if_fail (method, OSSO_ERROR);

        if ((!strcmp (method, "top_application"))) {
                gtk_window_present (GTK_WINDOW (data));
                retval->type = DBUS_TYPE_INT32;
                retval->value.i = 0;
                return OSSO_OK;
        }

        retval->type = DBUS_TYPE_INT32;
        retval->value.i = -1;
        return OSSO_ERROR;
}

static void create_new_process(gchar *filename)
{
	StructData *sd = g_malloc(sizeof(StructData));
	FileInfo *fi;
	osso_context_t  *osso;

        sd->conf.width = 600;
        sd->conf.height = 400;
        sd->conf.fontname = g_strdup("Monospace 12");
        sd->conf.wordwrap = FALSE;
        sd->conf.linenumbers = FALSE;
        sd->conf.autoindent = FALSE;
        sd->conf.charset = NULL;

	sd->mainwin = create_main_window(sd);
	gtk_widget_show_all(sd->mainwin->window);

        osso = osso_initialize(OSSO_NOTES_SERVICE, OSSO_NOTES_VERSION, TRUE, NULL);
        osso_rpc_set_default_cb_f (osso, (osso_rpc_cb_f *) rpc_handler, sd->mainwin->window);
	
	fi = g_malloc(sizeof(FileInfo));
	fi->filename = filename;
	fi->charset = NULL;
	fi->lineend = 0;
	if (sd->conf.charset)
		fi->manual_charset = sd->conf.charset;
	else
		fi->manual_charset = NULL;
	sd->fi = fi;
 	if (sd->fi->filename) {
		if (file_open_real(sd->mainwin->textview, sd->fi))
			cb_file_new(sd);
		else {
			set_main_window_title(sd);
			undo_init(sd->mainwin->textview, sd->mainwin->textbuffer, sd->mainwin->menubar);
		}
	} else
		cb_file_new(sd);
	
/*	dnd_init(sd->mainwin->window); */
	dnd_init(sd->mainwin->textview);
	keyevent_init(sd->mainwin->textview);

	sd->search.string_find = NULL;
	sd->search.string_replace = NULL;
	
	gtk_main();
	
/*	gtk_widget_destroy(sd->mainwin->window);
	g_free(sd->fi);
	g_free(sd->mainwin);
	g_free(sd->conf.fontname);
	g_free(sd); */
	osso_deinitialize(osso);
}

gint main(gint argc, gchar *argv[])
{
	gchar *filename = NULL;
	gchar **strs;
	
	//xfce_textdomain(PACKAGE, LOCALEDIR, "UTF-8");
	
/*	if (getenv("G_BROKEN_FILENAMES") == NULL)
		if (strcmp(get_default_charset(), "UTF-8") != 0)
			setenv("G_BROKEN_FILENAMES", "1", 0);
*/	

	gtk_init(&argc, &argv);
	
	if (argv[1]) {
		if (g_strstr_len(argv[1], 5, "file:")) {
			filename = g_filename_from_uri(argv[1], NULL, NULL);
			if (g_strrstr(filename, " ")) {
				strs = g_strsplit(filename, " ", -1);
				g_free(filename);
				filename = g_strjoinv("\\ ", strs);
				g_strfreev(strs);
			}
		} else {
		/*	if  (!g_path_is_absolute(argv[1]))
				filename = g_build_filename(g_get_current_dir(), argv[1], NULL);
			else */
				filename = g_strdup(argv[1]);
		}
	}
	
	create_new_process(filename);
	return 0;
}
