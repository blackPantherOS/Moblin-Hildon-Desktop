/*
 *  ui.c - general user interface code.
 *	part of galculator
 *  	(c) 2002-2006 Simon Floery (chimaira@users.sf.net)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include "galculator.h"
#include "ui.h"
#include "display.h"
#include "math_functions.h"
#include "config_file.h"
#include "general_functions.h"
#include "callbacks.h"

#include <gtk/gtk.h>
#include <glade/glade.h>

GladeXML	*main_window_xml=NULL, *dispctrl_xml=NULL, *button_box_xml=NULL, 
		*prefs_xml=NULL, *about_dialog_xml=NULL;
char		dec_point[2];
GtkListStore	*prefs_constant_store=NULL, *prefs_user_function_store=NULL;

static void set_disp_ctrl_object_data ();
static void set_all_dispctrl_buttons_property (GFunc func, gpointer data);
static void set_all_normal_buttons_property (GFunc func, gpointer data);
static void set_table_child_callback (gpointer data, gpointer user_data);

/* active_buttons. bit mask, in which modes the corresponding button is active.
 * assume TRUE for all other bases/modes!
 */

s_active_buttons active_buttons[] = {\
	{"button_2", ~(AB_BIN)}, \
	{"button_3", ~(AB_BIN)}, \
	{"button_4", ~(AB_BIN)}, \
	{"button_5", ~(AB_BIN)}, \
	{"button_6", ~(AB_BIN)}, \
	{"button_7", ~(AB_BIN)}, \
	{"button_8", ~(AB_BIN | AB_OCT)}, \
	{"button_9", ~(AB_BIN | AB_OCT)}, \
	{"button_a", ~(AB_DEC | AB_BIN | AB_OCT)}, \
	{"button_b", ~(AB_DEC | AB_BIN | AB_OCT)}, \
	{"button_c", ~(AB_DEC | AB_BIN | AB_OCT)}, \
	{"button_d", ~(AB_DEC | AB_BIN | AB_OCT)}, \
	{"button_e", ~(AB_DEC | AB_BIN | AB_OCT)}, \
	{"button_f", ~(AB_DEC | AB_BIN | AB_OCT)}, \
	{"button_ee", AB_DEC}, \
	{"button_sin", AB_DEC}, \
	{"button_cos", AB_DEC}, \
	{"button_tan", AB_DEC}, \
	{"button_point", ~(AB_BIN | AB_OCT | AB_HEX)}, \
	{"button_sign", ~(AB_BIN | AB_OCT | AB_HEX)}, \
	{NULL}\
};

/* glade_file_open. opens a new .glade file, checks if this open was successful
 * and returns *GladeXML.
 */

static GladeXML *glade_file_open (char *filename, 
				char *root_widget, 
				gboolean fatal)
{
	GladeXML	*xml;
	
	xml = glade_xml_new (filename, root_widget, NULL);
	
	if (xml == NULL) {
		fprintf (stderr, _("[%s] Couldn't load %s. This file is necessary \
to build galculator's user interface. Make sure you did a make install and the file \
is accessible!\n"), PACKAGE, filename);
		if (fatal == TRUE) exit(EXIT_FAILURE);
	}
	return xml;
}

/* apply_object_data. with gtk (better gobject) we can store additional
 * information for a widget (resp gobject). This information could be the
 * operation sign for src/calc_basic.c. This function sets such object data.
 * Is called from set_scientific_object_data resp set_basic_object_data.
 */

static void apply_object_data (s_operation_map operation_map[],
			s_gfunc_map gfunc_map[],
			s_function_map function_map[])
{
	int 		counter;
	gpointer	*func;
	GObject		*object;
	
	counter = 0;
	while (operation_map[counter].button_name != NULL) {
		object = G_OBJECT (glade_xml_get_widget (button_box_xml, 
			operation_map[counter].button_name));
		g_object_set_data (object, "operation",
			GINT_TO_POINTER(operation_map[counter].operation));
		g_object_set_data (object, "display_string",
			operation_map[counter].display_string);
		counter++;
	}
	
	counter = 0;
	while (gfunc_map[counter].button_name != NULL) {
		object = G_OBJECT (glade_xml_get_widget (button_box_xml, 
			gfunc_map[counter].button_name));
 		g_object_set_data (object, "display_string", gfunc_map[counter].display_string);
		g_object_set_data (object, "func", gfunc_map[counter].func);
		counter++;
	};
	
	counter = 0;
	while (function_map[counter].button_name != NULL) {
		func = (void *) malloc (sizeof (function_map[counter].func));
		memcpy (func, function_map[counter].func, sizeof (function_map[counter].func));
		object = G_OBJECT (glade_xml_get_widget (button_box_xml, 
			function_map[counter].button_name));
		g_object_set_data (object, "display_names", function_map[counter].display_names);
		g_object_set_data (object, "func", func);	
		counter++;
	};
}

/* set_scientific_object_data. Here, we set the information that is saved in
 * apply_object_data. For scientific mode.
 */

static void set_scientific_object_data ()
{
	s_operation_map	operation_map[] = {
		{"button_pow", "^", '^'},
		{"button_lsh", "<<", '<'},
		{"button_mod", "MOD", 'm'},
		{"button_and", "AND", '&'},
		{"button_or", "OR", '|'},
		{"button_xor", "XOR", 'x'},
		{"button_enter", "=", '='},
		{"button_plus", "+", '+'},
		{"button_minus", "-", '-'},
		{"button_mult", "*", '*'},
		{"button_div", "/", '/'},
		{"button_percent", "%", '%'},
		{"button_f1", "(", '('},	/* paropen or swapxy */
		{"button_f2", ")", ')'},	/* parclose or rolldn */
		{NULL}
	};
	
	s_gfunc_map gfunc_map[] = {
		{"button_sign", "-", display_result_toggle_sign},
		{"button_ee", "e", display_append_e},
		{"button_f1", "(", gfunc_f1},	/* paropen or swapxy */
		{"button_f2", ")", gfunc_f2},	/* parclose or rolldn */
		{NULL}
	};
	
	/* declare this one static as we need the display_names throughout */
	
	static s_function_map function_map[] = {
		{"button_sin", {"sin(", "asin(", "sinh(", "asinh("}, {sin_wrapper, asin_wrapper, sinh, asinh}},
		{"button_cos", {"cos(", "acos(", "cosh(", "acosh("}, {cos_wrapper, acos_wrapper, cosh, acosh}},
		{"button_tan", {"tan(", "atan(", "tanh(", "atanh("}, {tan_wrapper, atan_wrapper, tanh, atanh}},
		{"button_log", {"log(", "10^", "log(", "log("}, {log10, pow10y, log10, log10}},
		{"button_ln", {"ln(", "e^", "ln(", "ln("}, {log, exp, log, log}},
		{"button_sq", {"^2", "sqrt(", "^2", "^2"}, {powx2, sqrt, powx2, powx2}},
		{"button_sqrt", {"sqrt(", "^2", "sqrt(", "sqrt("}, {sqrt, powx2, sqrt, sqrt}},
		{"button_fac", {"!", "!", "!", "!"}, {factorial, factorial, factorial, factorial}},
		{"button_cmp", {"~", "~", "~", "~"}, {cmp, cmp, cmp, cmp}},
		{NULL}
	};

	apply_object_data (operation_map, gfunc_map, function_map);
}

/* set_basic_object_data. Here, we set the information that is saved in
 * apply_object_data. For basic mode.
 */

static void set_basic_object_data ()
{
	s_operation_map	operation_map[] = {
		{"button_enter", "=", '='},
		{"button_plus", "+", '+'},
		{"button_minus", "-", '-'},
		{"button_mult", "*", '*'},
		{"button_div", "/", '/'},
		{"button_percent", "%", '%'},
		{"button_f1", "(", '('},	/* paropen or swapxy */
		{"button_f2", ")", ')'},	/* parclose or rolldn */
		{NULL}
	};
	
	s_gfunc_map gfunc_map[] = {
		{"button_sign", "-", display_result_toggle_sign},
		{"button_f1", "(", gfunc_f1},	/* paropen or swapxy */
		{"button_f2", ")", gfunc_f2},	/* parclose or rolldn */
		{NULL}
	};
	
	s_function_map function_map[] = {
		{"button_sqrt", {"sqrt", "^2", "sqrt", "sqrt"}, {sqrt, powx2, sqrt, sqrt}},
		{NULL}
	};
	
	apply_object_data (operation_map, gfunc_map, function_map);
}

/* set_disp_ctrl_object_data. Here, we set the information that is saved in
 * apply_object_data. For display control buttons.
 */

static void set_disp_ctrl_object_data ()
{
	int	counter=0;
	
	s_gfunc_map map[] = {\
		{"button_clr", NULL, clear},\
		{"button_backspace", NULL, backspace},\
		{"button_allclr", NULL, all_clear},\
		{NULL}\
	};

	while (map[counter].button_name != NULL) {
		g_object_set_data (G_OBJECT (glade_xml_get_widget (
			dispctrl_xml, map[counter].button_name)),
			"display_string", map[counter].display_string);
		g_object_set_data (G_OBJECT (glade_xml_get_widget (
			dispctrl_xml, map[counter].button_name)),
			"func", map[counter].func);
		counter++;
	};
}

/* ui_pack_from_xml. This is a very special function. But we need it at least
 * three times. takes child_name from child_xml and adds it to box at index.
 * signals are connected and accel_group of accel_child_name is attached to
 * box's toplevel window.
 */

static void ui_pack_from_xml (GtkWidget *box, 
				int index, 
				GladeXML *child_xml, 
				char *child_name,
				char *accel_child_name,
				gboolean expand,
				gboolean fill)
{
	GtkWidget	*child_widget, *accel_child_widget;
	GtkAccelGroup	*accel_group;
	
	/* at first connect signal handlers */
	glade_xml_signal_autoconnect (child_xml);
	/* next, get the "root" child */
	child_widget = glade_xml_get_widget (child_xml, child_name);
	/* we have to add the accel_group of child to the main_window in order
	 * to get working accelerators.
	 */
	accel_child_widget = glade_xml_get_widget (child_xml, accel_child_name);

	accel_group = gtk_accel_group_from_accel_closure ((GClosure *) 
		(gtk_widget_list_accel_closures (accel_child_widget))->data);
	
	gtk_window_add_accel_group ((GtkWindow *) gtk_widget_get_toplevel (box),
		accel_group);

	gtk_box_pack_start ((GtkBox *) box, child_widget, expand, fill, 0);
	gtk_box_reorder_child ((GtkBox *) box, child_widget, index);
	gtk_widget_show (box);
}


/* ui_main_window_create. creates the main_window, containing menu toolbar and
 * the display skeleton. display control buttons and the calculator's buttons
 * are added by the callbacks for scientific resp basic mode.
 */

GtkWidget *ui_main_window_create ()
{
	main_window_xml = glade_file_open (MAIN_GLADE_FILE, "main_window", TRUE);
	/* connect the signals in the interface */
	glade_xml_signal_autoconnect(main_window_xml);
	return glade_xml_get_widget (main_window_xml, "main_window");
}

/* ui_main_window_set_dispctrl. we can't (un-)hide the dispctrl buttons as they
 * have the same key accelerators and thus only one button group would get
 * activated.
 */

void ui_main_window_set_dispctrl (int location)
{
	GtkWidget	*table_dispctrl, *box;
	s_signal_cb	signal_cb;
	
	/* destroy any existing display controls */
	if (dispctrl_xml) {
		table_dispctrl = glade_xml_get_widget (dispctrl_xml, "table_dispctrl");
		if (table_dispctrl) gtk_widget_destroy (table_dispctrl); 
		g_object_unref (G_OBJECT(dispctrl_xml));
		dispctrl_xml = NULL;
	}	
	/* now create the new one at location */
	switch(location) {
		case DISPCTRL_BOTTOM:
			box = glade_xml_get_widget (main_window_xml, "window_vbox");
			dispctrl_xml = glade_file_open (DISPCTRL_BOTTOM_GLADE_FILE, 
				"table_dispctrl", TRUE);
			ui_pack_from_xml (box, 2, dispctrl_xml, "table_dispctrl", 
				"button_clr", TRUE, TRUE);
			break;
		case DISPCTRL_RIGHT:
			box = glade_xml_get_widget (main_window_xml, "display_hbox");
			dispctrl_xml = glade_file_open (DISPCTRL_RIGHT_GLADE_FILE, 
				"table_dispctrl", TRUE);
			ui_pack_from_xml (box, 1, dispctrl_xml, "table_dispctrl",
				"button_clr", FALSE, FALSE);
			break;
		case DISPCTRL_RIGHTV:
			box = glade_xml_get_widget (main_window_xml, "display_hbox");
			dispctrl_xml = glade_file_open (DISPCTRL_RIGHTV_GLADE_FILE, 
				"table_dispctrl", TRUE);
			ui_pack_from_xml (box, 1, dispctrl_xml, "table_dispctrl",
				"button_clr", FALSE, FALSE);
			break;
		default:
			error_message ("Unknown mode in \"ui_main_window_set_dispctrl\"");
	}
	set_disp_ctrl_object_data ();

	/* finally we connect that signal handler */
	
	signal_cb.detailed_signal = g_strdup ("button_press_event");
	signal_cb.callback = (GCallback) on_button_press_event;
	set_all_dispctrl_buttons_property (set_table_child_callback, (gpointer) &signal_cb);
}

/* ui_main_window_buttons_destroy. removes the scientific resp basic mode 
 * buttons. display control buttons are not touched here!
 */

void ui_main_window_buttons_destroy ()
{
	GtkWidget	*box;
	GList		*children;
	
	box = glade_xml_get_widget (main_window_xml, "window_vbox");
	children = gtk_container_get_children ((GtkContainer *)box);
	children = g_list_last (children);
	if (children->data != NULL) gtk_widget_destroy (children->data);
	g_list_free (children);
}

/* ui_main_window_buttons_create. fills main_window with calculator's buttons,
 * paying respect to current mode. dispctrl buttons need to be done extra.
 */

void ui_main_window_buttons_create (int mode)
{
	GtkWidget	*box, *button;
	struct lconv 	*locale_settings;
	s_signal_cb	signal_cb;
	
	if (mode == BASIC_MODE) {
		if (button_box_xml) g_object_unref (G_OBJECT(button_box_xml));
		button_box_xml = glade_file_open (BASIC_GLADE_FILE, "button_box", TRUE);
		box = glade_xml_get_widget (main_window_xml, "window_vbox");
		ui_pack_from_xml (box, 4, button_box_xml, "button_box", 
			"button_1", TRUE, TRUE);
		set_basic_object_data (button_box_xml);
	} else if (mode == SCIENTIFIC_MODE) {
		if (button_box_xml) g_object_unref (G_OBJECT(button_box_xml));
		button_box_xml = glade_file_open (SCIENTIFIC_GLADE_FILE, "button_box", TRUE);
		box = glade_xml_get_widget (main_window_xml, "window_vbox");
		ui_pack_from_xml (box, 4, button_box_xml, "button_box", 
			"button_1", TRUE, TRUE);
		set_scientific_object_data (button_box_xml);
	} else error_message ("Unknown mode in \"ui_main_window_buttons_create\"");
	signal_cb.detailed_signal = g_strdup ("button_press_event");
	signal_cb.callback = (GCallback) on_button_press_event;
	set_all_normal_buttons_property (set_table_child_callback, (gpointer) &signal_cb);
	
	/* update "decimal point" button to locale's decimal point */
	dec_point[0] = DEFAULT_DEC_POINT;
	locale_settings = localeconv();
	if (strlen (locale_settings->decimal_point) != 1) {
		fprintf (stderr, _("[%s] length of decimal point (in locale) \
is not supported: >%s<\nYou might face problems when using %s! %s\n)"), 
		PACKAGE, locale_settings->decimal_point, PROG_NAME, BUG_REPORT);
	} else dec_point[0] = locale_settings->decimal_point[0];
	dec_point[1] = '\0';
	gtk_button_set_label ((GtkButton *) glade_xml_get_widget (
		button_box_xml, "button_point"), dec_point);
	/* disable mr and m+ button if there is nothing to display */
	button = glade_xml_get_widget (button_box_xml, "button_MR");
	gtk_widget_set_sensitive (button, memory.len > 0);
	button = glade_xml_get_widget (button_box_xml, "button_Mplus");
	gtk_widget_set_sensitive (button, memory.len > 0);
}

/* set_table_child_callback. Function argument for set_all_*_buttons_property.
 * sets the size.
 */

static void set_table_child_callback (gpointer data, gpointer user_data)
{
	s_signal_cb	*signal_cb;
	GtkTableChild	*table_child;
	
	table_child = data;
	signal_cb = user_data;
	g_signal_connect (table_child->widget, signal_cb->detailed_signal, 
		signal_cb->callback, NULL);
}

/* set_table_child_size. Function argument for set_all_buttons_property.
 * sets the size.
 */

static void set_table_child_size (gpointer data, gpointer user_data)
{
	GtkRequisition	*size;
	GtkTableChild	*table_child;
	
	size = user_data;				/* dereference */
	table_child = data;
	gtk_widget_set_size_request (table_child->widget, size->width, size->height);
}

/* set_table_child_font. Function argument for set_all_buttons_property.
 * sets the font of buttons. We have to set the label of the button!
 */

static void set_table_child_font (gpointer data, gpointer user_data)
{
	PangoFontDescription	*font;
	GtkTableChild		*table_child;
	GtkWidget		*w=NULL;
	
	font = user_data;				/* dereference */
	table_child = data;
	
	if (GTK_IS_BIN (table_child->widget)) 
		w = gtk_bin_get_child ((GtkBin *)(table_child->widget));
	/* if it's a normal button, w is now the label we want to font-change.
	 * if it's a popup button, we have to get the most left child first.
	 */
	if (GTK_IS_BOX (w)) w = ((GtkBoxChild *)((((GtkBox *)w)->children)->data))->widget;
	if (GTK_IS_LABEL(w)) gtk_widget_modify_font (w, font);
	/* else do nothing */
}

/* set_all_dispctrl_buttons_property. calls func with argument data for 
 * every button of the display control section.
 */

static void set_all_dispctrl_buttons_property (GFunc func, gpointer data)
{
	GtkTable	*table;

	/* at first the display control table. always there; somehow */
	table = (GtkTable *) glade_xml_get_widget (dispctrl_xml, 
		"table_dispctrl");
	
	/* dispctrl_right has an extra table for cosmetic reasons. */
	if (GTK_IS_TABLE (((GtkTableChild *)table->children->data)->widget))
		table = (GtkTable *) ((GtkTableChild *)table->children->data)->widget;
	g_list_foreach (table->children, func, data);
}

/* set_all_dispctrl_buttons_property. calls func with argument data for 
 * every button of the "calculator" section.
 */

static void set_all_normal_buttons_property (GFunc func, gpointer data)
{
	GtkTable	*table;
	
	/* now depending on mode the remaining buttons */
	if (prefs.mode == BASIC_MODE) {
		table = (GtkTable *) glade_xml_get_widget (button_box_xml, 
			"table_buttons");
		g_list_foreach (table->children, func, data);
	
	} else if (prefs.mode == SCIENTIFIC_MODE) {
		table = (GtkTable *) glade_xml_get_widget (button_box_xml, 
			"table_standard_buttons");
		g_list_foreach (table->children, func, data);
		table = (GtkTable *) glade_xml_get_widget (button_box_xml, 
			"table_bin_buttons");
		g_list_foreach (table->children, func, data);
		table = (GtkTable *) glade_xml_get_widget (button_box_xml, 
			"table_func_buttons");
		g_list_foreach (table->children, func, data);
	}
	else error_message ("Unknown mode in \"set_all_buttons_property\"");
}

/* set_all_buttons_property. calls func with argument data for every button.
 */

static void set_all_buttons_property (GFunc func, gpointer data)
{
	set_all_dispctrl_buttons_property (func, data);
	set_all_normal_buttons_property (func, data);
}

/* set_all_buttons_size. gateway for set_all_buttons_property.
 */

void set_all_buttons_size (int width, int height)
{
	GtkRequisition	size;
	
	size.width = width;
	size.height = height;
	set_all_buttons_property (set_table_child_size, (gpointer) &size);
}

/* set_all_buttons_font. gateway for set_all_buttons_property.
 */

void set_all_buttons_font (char *font_string)
{
	PangoFontDescription	*pango_font;

	pango_font = pango_font_description_from_string (font_string);
	set_all_buttons_property (set_table_child_font, pango_font);
}

gboolean button_deactivation (gpointer data)
{
	GtkToggleButton 	*b;
	
	b = (GtkToggleButton*) data;
	gtk_toggle_button_set_active (b, FALSE);
	return FALSE;	
}

void button_activation (GtkToggleButton *b)
{
	g_timeout_add (100, button_deactivation, (gpointer) b);
}

void update_active_buttons (int number_base, int notation_mode)
{
	int		counter=0;
	GtkWidget	*current_button;
	unsigned int	state;
	
	/* state = (1 << number_base) | (1 << (notation_mode + NR_NUMBER_BASES)); */
	state = 1 << number_base;
	while (active_buttons[counter].button_name != NULL) {
		current_button = glade_xml_get_widget (button_box_xml, 
			active_buttons[counter].button_name);
		if (current_button == NULL) {
			counter++;
			continue;
		}
		gtk_widget_set_sensitive (current_button, 
			(active_buttons[counter].mask & state) == state);
		counter++;
	}
}

void update_dispctrl()
{
	/* just put one here and hide it afterwards. we need the button
			for working key accelerators. */
	if (prefs.mode == BASIC_MODE) 
		ui_main_window_set_dispctrl (DISPCTRL_BOTTOM);
	else if (current_status.notation == CS_RPN)
		ui_main_window_set_dispctrl (DISPCTRL_RIGHTV);
	else ui_main_window_set_dispctrl (DISPCTRL_RIGHT);
	set_widget_visibility (dispctrl_xml, "table_dispctrl", 
		prefs.vis_dispctrl);
}

/*
 * (un) hide a widget
 */ 

void set_widget_visibility (GladeXML *xml, char *widget_name, gboolean visible)
{
	GtkWidget	*widget;
	
	widget = glade_xml_get_widget (xml, widget_name);
	if (visible) gtk_widget_show_all (widget);
	else gtk_widget_hide_all (widget);
}

GtkWidget *ui_font_dialog_create (char *title, GtkButton *button)
{
	GladeXML	*font_xml;
	GtkWidget	*font_dialog;
	
	font_xml = glade_file_open (FONT_GLADE_FILE, "font_dialog", FALSE);
	glade_xml_signal_autoconnect(font_xml);
	font_dialog = glade_xml_get_widget (font_xml, "font_dialog");
	
	gtk_window_set_title ((GtkWindow *) font_dialog, title);
	gtk_font_selection_dialog_set_font_name ((GtkFontSelectionDialog *) font_dialog, \
		gtk_button_get_label (button));
	gtk_widget_hide (((GtkFontSelectionDialog *)font_dialog)->apply_button);
	gtk_widget_show (font_dialog);

	return font_dialog;
}

GtkWidget *ui_color_dialog_create (char *title, GtkButton *button)
{
	GladeXML	*color_xml;
	GtkWidget	*color_dialog, *da;
	GdkColor	color;
	GtkRcStyle	*style;
	GtkBox		*box;
	GtkBin		*vp;
	
	box = (GtkBox *)gtk_bin_get_child ((GtkBin *) button);
	vp = (GtkBin *) (((GtkBoxChild *)g_list_nth_data (box->children, 1))->widget);
	da = gtk_bin_get_child(vp);
	
	style = gtk_widget_get_modifier_style (da);
	color = style->fg[GTK_STATE_NORMAL];
	
	color_xml = glade_file_open (COLOR_GLADE_FILE, "color_dialog", FALSE);
	glade_xml_signal_autoconnect(color_xml);
	color_dialog = glade_xml_get_widget (color_xml, "color_dialog");
	
	gtk_window_set_title ((GtkWindow *) color_dialog, title);
	gtk_color_selection_set_current_color ((GtkColorSelection *)((GtkColorSelectionDialog *)color_dialog)->colorsel, \
		&color);
	gtk_widget_hide (((GtkColorSelectionDialog *)color_dialog)->help_button);
	gtk_widget_show (color_dialog);
	return color_dialog;
}

/* menu code - e.g. used for the constant popup menu */

void position_menu (GtkMenu *menu, 
		gint *x, 
		gint *y, 
		gboolean *push_in, 
		gpointer user_data)
{
	/* this code is taken from GTK 2.2.1 source, therefore credits go there.
	 *  gtk+-2.0.6/gtk/gtkoptionmenu.c (function gtk_option_menu_position)
	 * modified to fit our button menu widget.
	 */
	GtkWidget *child;
	GtkWidget *widget;
	GtkRequisition requisition;
	GList *children;
	gint screen_width;
	gint menu_xpos;
	gint menu_ypos;
	gint menu_width;
	
	g_return_if_fail (GTK_IS_BUTTON (user_data));
	
	widget = GTK_WIDGET (user_data);
	
	gtk_widget_get_child_requisition (GTK_WIDGET (menu), &requisition);
	menu_width = requisition.width;
	
	/* i guess we don't need the "active" stuff from the original positioning
		code. we don't have any active items
	 */
	 
	gdk_window_get_origin (widget->window, &menu_xpos, &menu_ypos);
	
	menu_xpos += widget->allocation.x;
	menu_ypos += widget->allocation.y + widget->allocation.height / 2 - 2;
	
	children = GTK_MENU_SHELL(menu)->children;
	while (children) {
		child = children->data;
		if (GTK_WIDGET_VISIBLE (child))	{
			gtk_widget_get_child_requisition (child, &requisition);
			menu_ypos -= requisition.height;
		}
		children = children->next;
	}
	
	/*screen_width = gdk_screen_get_width (gtk_widget_get_screen (widget));*/
	screen_width = gdk_screen_width ();
	
	if (menu_xpos < 0) menu_xpos = 0;
	else if ((menu_xpos + menu_width) > screen_width)
		menu_xpos -= ((menu_xpos + menu_width) - screen_width);
	
	*x = menu_xpos;
	*y = menu_ypos;
	*push_in = TRUE;
}

GtkWidget *ui_user_functions_menu_create (s_user_function *user_function, GCallback user_function_handler)
{
	GtkWidget	*menu, *child;
	int		counter=0;
	char		*label;
	
	menu = gtk_menu_new();
	while (user_function[counter].name != NULL) {
		label = g_strdup_printf ("%s(%s) = %s", user_function[counter].name, 
			user_function[counter].variable, 
			user_function[counter].expression);
		child = gtk_menu_item_new_with_label(label);
		g_free (label);
		gtk_menu_shell_append ((GtkMenuShell *) menu, child);
		gtk_widget_show (child);
		g_signal_connect (G_OBJECT (child), "activate", user_function_handler, GINT_TO_POINTER(counter));
		counter++;
	}
	return menu;
}

GtkWidget *ui_constants_menu_create (s_constant *constant, GCallback const_handler)
{
	GtkWidget	*menu, *child;
	int		counter=0;
	char		*label;
	
	menu = gtk_menu_new();
	while (constant[counter].name != NULL) {
		label = g_strdup_printf ("%s: %s (%s)", constant[counter].name, constant[counter].value, constant[counter].desc);
		child = gtk_menu_item_new_with_label(label);
		g_free (label);
		gtk_menu_shell_append ((GtkMenuShell *) menu, child);
		gtk_widget_show (child);
		g_signal_connect (G_OBJECT (child), "activate", const_handler, constant[counter].value);
		counter++;
	}
	return menu;
}

GtkWidget *ui_memory_menu_create (s_array memory, GCallback memory_handler, char *last_item)
{
	GtkWidget	*menu, *child;
	int		counter=0;
	char		*label;
	
	menu = gtk_menu_new();
	for (counter = 0; counter < memory.len; counter++) {
		label = g_strdup_printf ("%f", memory.data[counter]);
		child = gtk_menu_item_new_with_label(label);
		g_free (label);
		gtk_menu_shell_append ((GtkMenuShell *) menu, child);
		gtk_widget_show (child);
		g_signal_connect (G_OBJECT (child), "activate", memory_handler, GINT_TO_POINTER(counter));
	}
	if (last_item != NULL) {
		label = g_strdup (last_item);
		child = gtk_menu_item_new_with_label(label);
		g_free (label);
		gtk_menu_shell_append ((GtkMenuShell *) menu, child);
		gtk_widget_show (child);
		g_signal_connect (G_OBJECT (child), "activate", memory_handler, GINT_TO_POINTER(counter));
	}
	return menu;
}

GtkWidget *ui_right_mouse_menu_create ()
{
	GtkWidget	*menu, *menu_item;
	
	menu = gtk_menu_new();
	
	menu_item = gtk_check_menu_item_new_with_mnemonic (_("Show _menu bar"));
	gtk_check_menu_item_set_active ((GtkCheckMenuItem *) menu_item, prefs.show_menu);
	gtk_menu_shell_append ((GtkMenuShell *) menu, menu_item);
	gtk_widget_show (menu_item);
	g_signal_connect (G_OBJECT (menu_item), "activate", (GCallback) on_show_menubar1_activate, NULL);
	
	return menu;
}

GtkWidget *ui_pref_dialog_create ()
{
	int			counter=0;
	GtkWidget		*w, *prefs_dialog;
	GtkTreeIter   		iter;
	GtkWidget		*tree_view;
	GtkCellRenderer 	*renderer;
	GtkTreeViewColumn 	*column;
	GtkTreeSelection	*select;
	GtkSizeGroup		*sgroup;
	s_prefs_entry		*prefs_list;
	
	
	prefs_xml = glade_file_open (PREFS_GLADE_FILE, "prefs_dialog", FALSE);
	glade_xml_signal_autoconnect(prefs_xml);
	
	prefs_dialog = glade_xml_get_widget (prefs_xml, "prefs_dialog");
	
	gtk_window_set_title ((GtkWindow *)prefs_dialog, \
		g_strdup_printf (_("%s Preferences"), PACKAGE));
	/* preferences -> gui */
	prefs_list = config_file_get_prefs_list();
	while (prefs_list[counter].key != NULL) {
		if (prefs_list[counter].set_handler != NULL) {
			prefs_list[counter].set_handler (prefs_xml,
				prefs_list[counter].widget_name,
				prefs_list[counter].variable);
		}
		counter++;
	}
	
	w = glade_xml_get_widget (prefs_xml, "prefs_button_font_label");
	gtk_widget_set_sensitive (w, prefs.custom_button_font);
	
	w = glade_xml_get_widget (prefs_xml, "prefs_button_font");
	gtk_widget_set_sensitive (w, prefs.custom_button_font);
	
	w = glade_xml_get_widget (prefs_xml, "prefs_bin_length");
	gtk_widget_set_sensitive (w, prefs.bin_fixed);

	/* make user defined constants list. */
	
	prefs_constant_store = gtk_list_store_new (NR_CONST_COLUMNS, 
		G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	counter = 0;
	while (constant[counter].name != NULL) {
		gtk_list_store_append (prefs_constant_store, &iter);
		gtk_list_store_set (prefs_constant_store, &iter, 
			CONST_NAME_COLUMN, constant[counter].name, 
			CONST_VALUE_COLUMN, constant[counter].value, 
			CONST_DESC_COLUMN, constant[counter].desc,
			-1);
		counter++;
	}
	tree_view = glade_xml_get_widget (prefs_xml, "constant_treeview");
	gtk_tree_view_set_model ((GtkTreeView *) tree_view, 
		GTK_TREE_MODEL (prefs_constant_store));
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Name", renderer, 
		"text", CONST_NAME_COLUMN, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);
	column = gtk_tree_view_column_new_with_attributes ("Value", renderer, 
		"text", CONST_VALUE_COLUMN, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);
	column = gtk_tree_view_column_new_with_attributes ("Description", renderer, 
		"text", CONST_DESC_COLUMN, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);
	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view));
	gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
	g_signal_connect (G_OBJECT (select), "changed",
                  G_CALLBACK (const_list_selection_changed_cb),
                  NULL);
	
	/* make user defined function list */

	prefs_user_function_store = gtk_list_store_new (NR_UFUNC_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	counter = 0;
	while (user_function[counter].name != NULL) {
		gtk_list_store_append (prefs_user_function_store, &iter);
		gtk_list_store_set (prefs_user_function_store, &iter, 
			UFUNC_NAME_COLUMN, user_function[counter].name, 
			UFUNC_VARIABLE_COLUMN, user_function[counter].variable, 
			UFUNC_EXPRESSION_COLUMN, user_function[counter].expression,
			-1);
		counter++;
	}
	tree_view = glade_xml_get_widget (prefs_xml, "user_function_treeview");
	gtk_tree_view_set_model ((GtkTreeView *) tree_view, 
		GTK_TREE_MODEL (prefs_user_function_store));
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Name", renderer, 
		"text", UFUNC_NAME_COLUMN, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);
	column = gtk_tree_view_column_new_with_attributes ("Variable", renderer, 
		"text", UFUNC_VARIABLE_COLUMN, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);
	column = gtk_tree_view_column_new_with_attributes ("Expression", renderer, 
		"text", UFUNC_EXPRESSION_COLUMN, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);
	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view));
	gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
	g_signal_connect (G_OBJECT (select), "changed",
                  G_CALLBACK (user_function_list_selection_changed_cb),
                  NULL);	
	
	/* pack widget of same size into GtkSizeGroup */

	sgroup = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget (sgroup, 
		glade_xml_get_widget (prefs_xml, "prefs_button_font_label"));
	gtk_size_group_add_widget (sgroup,
		glade_xml_get_widget (prefs_xml, "prefs_button_width_label"));
	gtk_size_group_add_widget (sgroup,
		glade_xml_get_widget (prefs_xml, "prefs_button_height_label"));
	
	sgroup = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget (sgroup, 
		glade_xml_get_widget (prefs_xml, "prefs_const_add_button"));
	gtk_size_group_add_widget (sgroup,
		glade_xml_get_widget (prefs_xml, "prefs_const_update_button"));
	gtk_size_group_add_widget (sgroup,
		glade_xml_get_widget (prefs_xml, "prefs_const_delete_button"));
	gtk_size_group_add_widget (sgroup,
		glade_xml_get_widget (prefs_xml, "prefs_const_clear_button"));
	
	sgroup = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget (sgroup, 
		glade_xml_get_widget (prefs_xml, "prefs_func_add_button"));
	gtk_size_group_add_widget (sgroup,
		glade_xml_get_widget (prefs_xml, "prefs_func_update_button"));
	gtk_size_group_add_widget (sgroup,
		glade_xml_get_widget (prefs_xml, "prefs_func_delete_button"));
	gtk_size_group_add_widget (sgroup,
		glade_xml_get_widget (prefs_xml, "prefs_func_clear_button"));

	gtk_widget_show (prefs_dialog);
	return prefs_dialog;
}

GtkWidget *ui_about_dialog_create()
{	
	about_dialog_xml = glade_file_open (ABOUT_GLADE_FILE, 
		"about_dialog", FALSE);
	glade_xml_signal_autoconnect(about_dialog_xml);
	return glade_xml_get_widget (about_dialog_xml, "about_dialog");
}

void ui_formula_entry_activate ()
{
	GtkWidget	*formula_entry;
	
	formula_entry = glade_xml_get_widget (main_window_xml, "formula_entry");
	gtk_widget_activate(formula_entry);
}

void ui_formula_entry_set (G_CONST_RETURN gchar *text)
{
	GtkWidget	*formula_entry;

	if (text == NULL) return;
	formula_entry = glade_xml_get_widget (main_window_xml, "formula_entry");
	gtk_entry_set_text ((GtkEntry *) formula_entry, text);
}

void ui_formula_entry_insert (G_CONST_RETURN gchar *text)
{
	GtkWidget	*formula_entry;
	int		position;
	
	if (text == NULL) return;
	formula_entry = glade_xml_get_widget (main_window_xml, "formula_entry");
	position = gtk_editable_get_position ((GtkEditable *) formula_entry);
	gtk_editable_insert_text ((GtkEditable *) formula_entry, text, -1,
                                             &position);
	gtk_editable_set_position ((GtkEditable *) formula_entry, position);
}

void ui_formula_entry_backspace ()
{
	GtkWidget	*formula_entry;
	
	formula_entry = glade_xml_get_widget (main_window_xml, "formula_entry");
	gtk_editable_delete_text ((GtkEditable *) formula_entry, 
		strlen(gtk_entry_get_text((GtkEntry *) formula_entry)) - 1, -1);
}

/* ui_formula_entry_state. if color == NULL looks like we get default. this is
 * what we want.
 */

void ui_formula_entry_state (gboolean error)
{
	GtkWidget		*formula_entry;
	GdkColor		*color=NULL;
	
	formula_entry = glade_xml_get_widget (main_window_xml, "formula_entry");
	if (error) {
		color = (GdkColor *) malloc(sizeof(GdkColor));
		gdk_color_parse ("red", color);
	}
	gtk_widget_modify_text (formula_entry, 0, color);
	gtk_widget_modify_text (formula_entry, 1, color);
	gtk_widget_modify_text (formula_entry, 2, color);
	gtk_widget_modify_text (formula_entry, 3, color);
	gtk_widget_modify_text (formula_entry, 4, color);
	if (color) g_free(color);
}

void ui_button_set_pan ()
{
	set_button_label_and_tooltip (button_box_xml, "button_enter", 
		_("="), _("Enter"));
	set_button_label_and_tooltip (button_box_xml, "button_pow", 
		_("x^y"), _("Power"));
	set_button_label_and_tooltip (button_box_xml, "button_f1", 
		_("("), _("Open Bracket"));
	set_button_label_and_tooltip (button_box_xml, "button_f2", 
		_(")"), _("Close Bracket"));
}

void ui_button_set_rpn ()
{
	set_button_label_and_tooltip (button_box_xml, "button_enter", 
		_("ENT"), _("Enter"));
	set_button_label_and_tooltip (button_box_xml, "button_pow", 
		_("y^x"), _("Power"));
	set_button_label_and_tooltip (button_box_xml, "button_f1", 
		_("x<>y"), _("swap current number with top of stack"));
	set_button_label_and_tooltip (button_box_xml, "button_f2", 
		_("roll"), _("roll down stack"));	
}

void ui_relax_fmod_buttons ()
{
	GtkWidget	*tbutton;
	
	tbutton = glade_xml_get_widget (button_box_xml, "button_inv");
	gtk_toggle_button_set_active ((GtkToggleButton *) tbutton, FALSE);
	tbutton = glade_xml_get_widget (button_box_xml, "button_hyp");
	gtk_toggle_button_set_active ((GtkToggleButton *) tbutton, FALSE);
}
