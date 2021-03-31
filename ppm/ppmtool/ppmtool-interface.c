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
 *      Tariq Shureih  <tariq.shureih@intel.com>
 *      Arjan van de Ven <arjan@linux.intel.com>
 *      Mohamed Abbas <mohamed.abbas@intel.com>
 *      Sarah Sharp <sarah.a.sharp@intel.com>
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <getopt.h>
#include <time.h>
#include <sys/poll.h>
#include <grp.h>
#include <signal.h>
#include <glib.h>
#include <dirent.h>


#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "ppmtool.h"
#include "dbus-bind.h"
#include "ppmtool-interface.h"

#define PPMTOOL_HOOKUP_OBJECT_NO_REF(main_app,widget,name) \
	g_object_set_data(G_OBJECT (main_app), name, widget)

enum
{
	COL_MODE = 0,
	COL_ACTIVE,
	NUMBER_COL
} ;


extern int num_modes;
extern struct mode_t *modes;

static char* selected_mode = NULL;
static GtkWidget *PPM_tool = NULL;

/*
 * this function will be called on user selecting a mode
 */
static gboolean mode_selection_func(GtkTreeSelection *selection,
				    GtkTreeModel *model,
				    GtkTreePath *path,
				    gboolean currently_sel,
				    gpointer user_data)
{
	GtkTreeIter iter;

	if (gtk_tree_model_get_iter(model, &iter, path)) {
		if (!currently_sel) {
			gchar *name;

			gtk_tree_model_get(model, &iter, COL_MODE, &name, -1);
			if (selected_mode) {
				g_free(selected_mode);
				selected_mode = NULL;
			}
			selected_mode = g_strdup(name);
			g_free(name);
		}
	}

	return TRUE; 
}

/*
 * fill the list of all available mode we got from ppmd
 */
static void fill_mode_list(GtkListStore *store)
{
	int i;
	GtkTreeIter iter;

	gtk_list_store_clear(store);

	for(i = 0; i < num_modes; i++) {
		gtk_list_store_append (store, &iter);
		gtk_list_store_set(store, &iter,
				   COL_MODE, modes[i].name, -1);
		if (modes[i].active == 1) {
			gtk_list_store_set(store, &iter,
					   COL_ACTIVE, "Active", -1);
			if (!selected_mode)
				selected_mode = g_strdup(modes[i].name);
				
		} else
			gtk_list_store_set(store, &iter,
					   COL_ACTIVE, "Non-active", -1);
		if (selected_mode && !strcmp(modes[i].name, selected_mode)) {
			GtkTreeSelection *sel = NULL;
			GtkWidget *widget = NULL;

			widget = GTK_WIDGET(g_object_get_data(
					    G_OBJECT(PPM_tool), "modes"));
			if (widget)
				sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
			if (sel)
				gtk_tree_selection_select_iter(sel ,&iter);
		}
	}
}

/*
 * call ppmd with the selected mode
 */
static int select_mew_mode()
{
	char* modes_to_activate[2];
	int i;

	if (!selected_mode)
		return -1;

	modes_to_activate[0] = selected_mode;
	modes_to_activate[1] = NULL;
	cei_activate_modes(modes_to_activate);

	for(i = 0; i < num_modes; i++) {
		g_free(modes[i].name);
		modes[i].name = NULL;
	}
	free(modes);
	modes = NULL;
	if(!cei_query_modes(&modes, &num_modes)) {
		return -1;
	}

	g_free(selected_mode);
	selected_mode = NULL;
	return 0;
}


/*
 * the following functions are click events handler
 */
static void on_apply_button_clicked(GtkButton *button, gpointer user_data)
{
	GtkListStore *store;
	GtkWidget *widget = NULL;

	select_mew_mode();
	
	store = GTK_LIST_STORE(g_object_get_data(G_OBJECT(user_data), "store"));

	if (store)
		fill_mode_list(store);

	widget = GTK_WIDGET(g_object_get_data(G_OBJECT(user_data), "modes"));

	if (widget)
		gtk_widget_show (widget);

	widget = GTK_WIDGET(g_object_get_data(G_OBJECT(user_data), "active_mode"));
	if (selected_mode && widget)
		gtk_label_set_text(GTK_LABEL(widget), selected_mode);
	else
		gtk_label_set_text(GTK_LABEL(widget), "No active mode");

}


static void on_cancel_button_clicked(GtkButton *button, gpointer user_data)
{
	gtk_widget_destroy(GTK_WIDGET(user_data));
}


static void on_ok_button_clicked(GtkButton *button, gpointer user_data)
{

	select_mew_mode();
	gtk_widget_destroy(GTK_WIDGET(user_data));
}


static void ppmtool_close_app(GtkButton *button, gpointer user_data)
{
	int i;

	if (selected_mode)
		g_free(selected_mode);
	selected_mode = NULL;

	if (modes) {
		for(i = 0; i < num_modes; i++) {
			g_free(modes[i].name);
			modes[i].name = NULL;
		}
		free(modes);
	}
	modes = NULL;
	num_modes = 0;
	gtk_main_quit();
}


/*
 * create ppmtool window app
 */
static GtkWidget* create_PPM_tool(void)
{
	GtkWidget *dialog_vbox1;
	GtkWidget *fixed1;
	GtkWidget *label7;
	GtkWidget *active_mode;
	GtkWidget *scrolledwindow3;
	GtkWidget *modes;
	GtkWidget *dialog_action_area1;
	GtkWidget *applybutton1;
	GtkWidget *cancelbutton1;
	GtkWidget *okbutton1;
	GtkListStore  *store;
	GtkCellRenderer     *renderer;
	GtkTreeSelection  *selection;

	/* create dialog window */
	PPM_tool = gtk_dialog_new();
	gtk_widget_set_size_request(PPM_tool, 450, 380);
	gtk_window_set_title (GTK_WINDOW (PPM_tool), ("PPM tool"));
	gtk_window_set_position(GTK_WINDOW (PPM_tool), GTK_WIN_POS_CENTER);
	gtk_window_set_modal(GTK_WINDOW (PPM_tool), TRUE);
	gtk_window_set_type_hint(GTK_WINDOW (PPM_tool), GDK_WINDOW_TYPE_HINT_DIALOG);

	/* add all widgets */
	dialog_vbox1 = GTK_DIALOG(PPM_tool)->vbox;
	gtk_widget_show(dialog_vbox1);

	fixed1 = gtk_fixed_new();
	gtk_widget_show(fixed1);
	gtk_box_pack_start(GTK_BOX (dialog_vbox1), fixed1, TRUE, TRUE, 0);
	gtk_widget_set_size_request(fixed1, 0, 30);

	label7 = gtk_label_new(("Active mode:"));
	gtk_widget_show(label7);
	gtk_fixed_put(GTK_FIXED (fixed1), label7, 0, 8);
	gtk_widget_set_size_request(label7, 145, 20);

	active_mode = gtk_label_new("");
	gtk_widget_show(active_mode);
	gtk_fixed_put(GTK_FIXED (fixed1), active_mode, 145, 8);
	gtk_widget_set_size_request(active_mode, 288, 20);

	scrolledwindow3 = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolledwindow3);
	gtk_box_pack_start(GTK_BOX (dialog_vbox1), scrolledwindow3, TRUE,
			    TRUE, 0);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledwindow3), GTK_SHADOW_IN);

	modes = gtk_tree_view_new();
	gtk_container_add(GTK_CONTAINER (scrolledwindow3), modes);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW (modes),
						     -1,      
						     "Mode",  
						     renderer,
						     "text", COL_MODE,
						     NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW (modes),
						     -1,      
						    "State",  
						    renderer,
						    "text", COL_ACTIVE,
						    NULL);


	store = gtk_list_store_new(NUMBER_COL, G_TYPE_STRING, G_TYPE_STRING);

	fill_mode_list(store);

	gtk_tree_view_set_model(GTK_TREE_VIEW (modes), GTK_TREE_MODEL (store));
	g_object_unref(GTK_TREE_MODEL (store));
	gtk_widget_show(modes);

	dialog_action_area1 = GTK_DIALOG(PPM_tool)->action_area;
	gtk_widget_show(dialog_action_area1);

	gtk_button_box_set_layout(GTK_BUTTON_BOX (dialog_action_area1),
				   GTK_BUTTONBOX_END);

	applybutton1 = gtk_button_new_from_stock("gtk-apply");
	gtk_widget_show(applybutton1);
	gtk_dialog_add_action_widget(GTK_DIALOG (PPM_tool), applybutton1,
				      GTK_RESPONSE_APPLY);
	GTK_WIDGET_SET_FLAGS(applybutton1, GTK_CAN_DEFAULT);
	gtk_button_set_focus_on_click(GTK_BUTTON (applybutton1), FALSE);

	cancelbutton1 = gtk_button_new_from_stock("gtk-cancel");
	gtk_widget_show(cancelbutton1);
	gtk_dialog_add_action_widget(GTK_DIALOG (PPM_tool), cancelbutton1,
				      GTK_RESPONSE_CANCEL);
	GTK_WIDGET_SET_FLAGS(cancelbutton1, GTK_CAN_DEFAULT);

	okbutton1 = gtk_button_new_from_stock ("gtk-ok");
	gtk_widget_show (okbutton1);
	gtk_dialog_add_action_widget(GTK_DIALOG (PPM_tool), okbutton1,
				      GTK_RESPONSE_OK);
	GTK_WIDGET_SET_FLAGS(okbutton1, GTK_CAN_DEFAULT);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(modes));

	gtk_tree_selection_set_select_function(selection, mode_selection_func,
					       NULL, NULL);


	if (selected_mode)
		gtk_label_set_text(GTK_LABEL(active_mode), selected_mode);

	/* hook widget events */
	g_signal_connect((gpointer) applybutton1, "clicked",
			 G_CALLBACK (on_apply_button_clicked),
			 PPM_tool);
	g_signal_connect((gpointer) cancelbutton1, "clicked",
			 G_CALLBACK (on_cancel_button_clicked),
			 PPM_tool);
	g_signal_connect((gpointer) okbutton1, "clicked",
			 G_CALLBACK (on_ok_button_clicked),
			 PPM_tool);
	g_signal_connect(GTK_OBJECT (PPM_tool), "destroy",
			 G_CALLBACK (ppmtool_close_app),
			 PPM_tool);

	PPMTOOL_HOOKUP_OBJECT_NO_REF (PPM_tool, PPM_tool, "PPM_tool");
	PPMTOOL_HOOKUP_OBJECT_NO_REF (PPM_tool, dialog_vbox1, "dialog_vbox1");
	PPMTOOL_HOOKUP_OBJECT_NO_REF (PPM_tool, fixed1, "fixed1");
	PPMTOOL_HOOKUP_OBJECT_NO_REF (PPM_tool, label7, "label7");
	PPMTOOL_HOOKUP_OBJECT_NO_REF (PPM_tool, active_mode, "active_mode");
	PPMTOOL_HOOKUP_OBJECT_NO_REF (PPM_tool, scrolledwindow3, "scrolledwindow3");
	PPMTOOL_HOOKUP_OBJECT_NO_REF (PPM_tool, modes, "modes");
	PPMTOOL_HOOKUP_OBJECT_NO_REF (PPM_tool, dialog_action_area1, "dialog_action_area1");
	PPMTOOL_HOOKUP_OBJECT_NO_REF (PPM_tool, applybutton1, "applybutton1");
	PPMTOOL_HOOKUP_OBJECT_NO_REF (PPM_tool, cancelbutton1, "cancelbutton1");
	PPMTOOL_HOOKUP_OBJECT_NO_REF (PPM_tool, okbutton1, "okbutton1");
	PPMTOOL_HOOKUP_OBJECT_NO_REF (PPM_tool, store, "store");

	return PPM_tool;
}



int show_ppm(int argc, char **argv)
{
	GtkWidget *dialog1;

	gtk_set_locale();
	gtk_init(&argc, &argv);
	
	dialog1 = create_PPM_tool();

	gtk_widget_show(dialog1);
	gtk_main();
	return 0;
}
