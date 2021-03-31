/*
 * Copyright (C) 2007  Intel  Ltd
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; version 2 of the License
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include <glade/glade.h>
#include <glade/glade-build.h>
#include <gtk/gtk.h>
#include "moko-finger-scroll.h"

GladeXML *g_window;

#define TYPE_DOUBLE 0
#define TYPE_INT 1
#define TYPE_UINIT 2
#define TYPE_BOOL 3

static void set_spinbutton_value(GladeXML *window,  char * id, char *prop, int type)
{
	GtkWidget * widget;
	widget = glade_xml_get_widget(window, id);
	gdouble value;
	
	GtkWidget *moko;

	moko = 	glade_xml_get_widget(window, "moko1");
	if(moko == NULL)
	{
		printf("can't get moko \n");
		return;
	}

	if(widget)
	{	
		gint value_int;
		guint value_uint;

		value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
		value_int =value;
		printf(" prop %s  value %f\n", prop, value);

		if(type == TYPE_DOUBLE )
			g_object_set(G_OBJECT(moko), prop,  value, NULL);
		else
			g_object_set(G_OBJECT(moko), prop,  value_int, NULL);
	

	}else
	{
		printf("failure get widget %s \n", id);
	}
	
} 

static void set_choose_value(GladeXML *window,  char * id, char *prop, int type)
{
	GtkWidget * widget;
	widget = glade_xml_get_widget(window, id);
	
	GtkWidget *moko;

	moko = 	glade_xml_get_widget(window, "moko1");
	if(moko == NULL)
	{
		printf("can't get moko \n");
		return;
	}

	if(widget)
	{	
		gint value_int;
		value_int = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
	
		if(value_int >= 0)
		{	
			if(type == TYPE_BOOL)
			{
				gboolean b;
				b=value_int? TRUE:FALSE;
				g_object_set(G_OBJECT(moko), prop, b, NULL);

			}else
			{
				g_object_set(G_OBJECT(moko), prop, value_int, NULL);
			}
			printf(" prop %s  value %d\n", prop, value_int);
		}
		

	}else
	{
		printf("failure get widget %s \n", id);
	}
}

void on_hindicator_mode_changed(GtkEditable *editable, gpointer user_data)
{
	printf(" %s\n", __FUNCTION__);
	set_choose_value(g_window, "hindicator_mode", "hindicator_mode", TYPE_INT);
}

void on_velocity_max_changed(GtkEditable *editable, gpointer user_data)
{
	printf(" %s\n", __FUNCTION__);
	set_spinbutton_value(g_window, "velocity_max", "velocity_max", TYPE_DOUBLE);

}

void on_vindicator_mode_changed(GtkEditable *editable, gpointer user_data)
{
	printf(" %s\n", __FUNCTION__);
	set_choose_value(g_window, "vindicator_mode", "vindicator_mode", TYPE_INT);
}


void on_deceleration_changed(GtkEditable *editable, gpointer user_data)
{
	printf(" %s\n", __FUNCTION__);
	set_spinbutton_value(g_window,"deceleration", "deceleration", TYPE_DOUBLE);

}
void on_enable_changed(GtkEditable *editable, gpointer user_data)
{
	printf(" %s\n", __FUNCTION__);
	set_choose_value(g_window, "enable", "enabled", TYPE_BOOL);

}
void on_velocity_min_changed(GtkEditable *editable, gpointer user_data)
{
	printf(" %s\n", __FUNCTION__);
	set_spinbutton_value(g_window,"velocity_min", "velocity_min", TYPE_DOUBLE);
}

void on_sps_changed(GtkEditable *editable, gpointer user_data)
{
	printf(" %s\n", __FUNCTION__);
	set_spinbutton_value(g_window,"sps", "sps",  TYPE_INT);

}
void on_spring_speed_changed(GtkEditable *editable, gpointer user_data)
{
	printf(" %s\n", __FUNCTION__);
	set_spinbutton_value(g_window,"spring_speed", "spring_speed", TYPE_DOUBLE);
}
void on_mode_changed(GtkEditable *editable, gpointer user_data)
{
	printf(" %s\n", __FUNCTION__);
	set_choose_value(g_window, "mode", "mode", TYPE_INT);
}
void on_indicator_width_changed(GtkEditable *editable, gpointer user_data)
{
	printf(" %s\n", __FUNCTION__);
	set_spinbutton_value(g_window, "indicator-width", "indicator-width",  TYPE_INT);

}
static GtkWidget * glade_moko_finger_scroll_new (GladeXML *xml, GType type, GladeWidgetInfo *info)
{
	GtkWidget *moko = moko_finger_scroll_new();
	gtk_widget_show(moko);
	return moko;
}

enum
{
	COL_NAME=0,
	COL_AGE,
	NUM_COLS
};

GtkTreeModel *
create_and_fill_model(void)
{
	int i=100;
	GtkListStore 	*store;
	GtkTreeIter		iter;

	store = gtk_list_store_new(NUM_COLS, G_TYPE_STRING, G_TYPE_INT);

	for(i=0;i<500;i++)
	{
		gtk_list_store_append (store, &iter);
		gtk_list_store_set(store, &iter, COL_NAME, "ABC", COL_AGE, i, -1);

	}
	return GTK_TREE_MODEL(store);
}
static void update_spinbutton_value(GladeXML *window,  gchar * id, gdouble value)
{
	GtkWidget * widget;
	widget = glade_xml_get_widget(window, id);
	if(widget)
	{
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), value);

	}else
	{
		printf("failure get widget %s \n", id);
	}
	
} 

static void update_choose_value(GladeXML *window,  gchar * id, int value)
{
	GtkWidget * widget;
	widget = glade_xml_get_widget(window, id);
	if(widget)
	{
		gtk_combo_box_set_active(GTK_COMBO_BOX(widget), value);

	}else
	{
		printf("failure get widget %s \n", id);
	}
}


static void update_property_value(GladeXML *window)
{
	gboolean enable;
	gint	 vindicator_mode;	
	gint	 hindicator_mode;
	gboolean mode;
	gdouble	 velocity_min;
	gdouble	 velocity_max;
	gdouble   deceleration;
	gdouble spring_speed;
	guint	sps;  //min 0 max 15
	guint	indicator_width;
	
	GtkWidget *moko;
	GtkWidget *con;

	moko = 	glade_xml_get_widget(window, "moko1");
	if(moko == NULL)
	{
		printf("can't get moko \n");
		return;
	}

	g_object_get(G_OBJECT(moko),
			 "enabled", &enable, 
			 "vindicator_mode", &vindicator_mode,
			 "hindicator_mode", &hindicator_mode,
			 "mode",&mode,
			 "velocity_min", &velocity_min,
			 "velocity_max", &velocity_max,
			 "deceleration", &deceleration,
			"spring_speed", &spring_speed,
			"sps", &sps,
			NULL);

	gtk_widget_style_get (moko, "indicator-width", &indicator_width,NULL);
	
	update_spinbutton_value(window, "deceleration", deceleration);
	update_spinbutton_value(window, "spring_speed", spring_speed);
	update_spinbutton_value(window, "indicator-width", indicator_width);
	update_spinbutton_value(window, "sps", sps);
	update_spinbutton_value(window, "velocity_max", velocity_max);
	update_spinbutton_value(window, "velocity_min", velocity_min);			
		
	update_choose_value(window, "mode",mode);
	update_choose_value(window, "enable", enable);
	update_choose_value(window, "vindicator_mode", vindicator_mode);
	update_choose_value(window, "hindicator_mode", hindicator_mode);					
}
int main(int argc, char ** argv)
{
	GladeXML * window;
	GtkWidget *tree;
	
	gtk_init(&argc, & argv);
	
	glade_register_widget(MOKO_TYPE_FINGER_SCROLL, 
							glade_moko_finger_scroll_new, 
							glade_standard_build_children, NULL);

	g_window=window = glade_xml_new("moko.glade", NULL, NULL);
	
	tree = glade_xml_get_widget(window, "treeview1");
	if(tree)
	{
		gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW	(tree), -1, "Name", gtk_cell_renderer_text_new(), "text",COL_NAME,NULL);
		gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW	(tree), -1, "Age", gtk_cell_renderer_text_new(), "text",COL_AGE, NULL);
		 gtk_tree_view_set_model(GTK_TREE_VIEW	(tree), create_and_fill_model());
	}	

	update_property_value(window);

	glade_xml_signal_autoconnect(window);

	
	gtk_main();
	
	return 0;
}
