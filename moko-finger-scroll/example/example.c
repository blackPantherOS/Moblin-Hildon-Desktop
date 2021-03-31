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


#include  <gtk/gtk.h>
#include "moko-finger-scroll.h"
#include <gdk-pixbuf/gdk-pixbuf.h>

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

GtkTreeModel *
create_and_fill_icon_mode(void)
{
	GtkListStore *store;
	GtkTreeIter		iter;
	GdkPixbuf 	*pix;
	int i;

	store = gtk_list_store_new(2, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, G_TYPE_BOOLEAN);
	pix = gdk_pixbuf_new_from_file("gnome-textfile.png", NULL);
//	printf("%p \n", pix);

	for(i=0;i <500; i++)
	{
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 0, pix ,
										1, "ABC",
										-1);
	}
	return GTK_TREE_MODEL(store);
}

add_tree_view(GtkNotebook *notebook)
{
	GtkTreeModel	*model;	
	GtkWidget		*top;
	GtkWidget 		*tree ,*label,*moko;

	model= create_and_fill_model();		
	tree  = gtk_tree_view_new ();
	gtk_widget_show(tree);
	gtk_tree_view_insert_column_with_attributes(tree, -1, "Name", gtk_cell_renderer_text_new(), "text",COL_NAME,NULL);
	gtk_tree_view_insert_column_with_attributes(tree, -1, "Age", gtk_cell_renderer_text_new(), "text",COL_AGE, NULL);
	gtk_tree_view_set_model(tree, model);

	

	label = gtk_label_new("treeview");
	gtk_widget_show(label);

	moko = moko_finger_scroll_new ();
	gtk_widget_show(moko);
	
	g_object_set(G_OBJECT(moko), "hindicator_mode", MOKO_FINGER_SCROLL_INDICATOR_MODE_ALPHA, NULL);
	g_object_set(G_OBJECT(moko), "vindicator_mode", MOKO_FINGER_SCROLL_INDICATOR_MODE_ALPHA, NULL);

	g_object_set(G_OBJECT(moko), "spring_color", 0xFF0000, NULL);

	gtk_container_add (GTK_CONTAINER (moko),tree);
	gtk_notebook_append_page(notebook, moko, label);
}

add_icon_view(GtkNotebook *notebook)
{
	GtkWidget		*top;
	GtkWidget 		*icon ,*label,*moko;
	GtkTreeModel	*model;	

	model= create_and_fill_icon_mode();		

	icon  = gtk_icon_view_new ();
	gtk_widget_show(icon);
	gtk_icon_view_set_model(icon, model);

	label = gtk_label_new("iconview");
	gtk_widget_show(label);

	  gtk_icon_view_set_pixbuf_column (GTK_ICON_VIEW (icon), 0);
	  gtk_icon_view_set_text_column (GTK_ICON_VIEW (icon), 1);

	moko = moko_finger_scroll_new ();
	gtk_widget_show(moko);

	g_object_set(G_OBJECT(moko), "spring_color", 0x0, NULL);
	
	gtk_container_add (GTK_CONTAINER (moko),icon);
	gtk_notebook_append_page(notebook, moko, label);
}

add_text_view(GtkNotebook *notebook)
{
	GtkWidget 		*text ,*label,*moko;
	GtkTreeModel	*model;	
	GtkTextBuffer    *buffer;
	int i;
	
	buffer = gtk_text_buffer_new(gtk_text_tag_table_new());

	for(i=0;i<500;i++)
		gtk_text_buffer_insert_at_cursor(buffer, "abc\n", 4);

	text = gtk_text_view_new_with_buffer(buffer);

	gtk_widget_show(text);
	
	//g_object_set(G_OBJECT(text), "editable", FALSE);

	label = gtk_label_new("textview");
	gtk_widget_show(label);

	moko = moko_finger_scroll_new ();
	gtk_widget_show(moko);
	
	gtk_container_add (GTK_CONTAINER (moko),text);
	gtk_notebook_append_page(notebook, moko, label);
}




GdkPixbuf *g_pixbuf;
#define IMAGE_FILE "sample.jpg"
double g_rate;

static gboolean
example_button_press (GtkWidget *win,
				    GdkEventButton *event,
				    gpointer user_data)
{
	GdkPixbuf *pixbuf;
	gint width, height, winw, winh;
	GtkImage *image;


//	gtk_widget_size_allocate(win,  &alloc);
	
//	printf("x %d y %d, width; height", alloc.x, alloc.y, alloc.width, alloc.height);

	image = GTK_IMAGE(user_data);
	
	width = gdk_pixbuf_get_width(g_pixbuf);
	height = gdk_pixbuf_get_height (g_pixbuf);

	gdk_drawable_get_size (GTK_WIDGET(win)->parent->window,  &winw, &winh);
	printf("%d %d - %f %f\n", winw, winh, event->x, event->y );

	if(event ->y  > winh/2 )
	{
			g_rate *=0.9;
			pixbuf = gdk_pixbuf_scale_simple (g_pixbuf,width*g_rate, height*g_rate, GDK_INTERP_NEAREST);
			gtk_image_set_from_pixbuf(image, pixbuf);

	}else 
	{
			g_rate *=1.1;
			pixbuf = gdk_pixbuf_scale_simple (g_pixbuf,width*g_rate, height*g_rate, GDK_INTERP_NEAREST);
			gtk_image_set_from_pixbuf(image, pixbuf);

	}
	printf("button_press type %d  button %d %f %f\n" , event->type,  event->button, event->x , event->y);
}

add_image(GtkNotebook *notebook)
{
	GtkWidget 		*image ,*label,*moko;
	GtkTreeModel	*model;	
	GError			*error;
	GtkWidget 		*event;
	GtkAllocation alloc;

	error = NULL;
	g_rate =  1.0;
	g_pixbuf = gdk_pixbuf_new_from_file ("sample.jpg" , &error);
	printf("%p\n", g_pixbuf);
	image = gtk_image_new_from_pixbuf(g_pixbuf);
	gtk_widget_show(image);
		
	event = gtk_event_box_new();
	gtk_widget_show(event);

	label = gtk_label_new("image");
	gtk_widget_show(label);

	printf("gtk_image_get_storage_type %d\n", gtk_image_get_storage_type(image));

	moko = moko_finger_scroll_new ();
	gtk_widget_show(moko);
	
	gtk_container_add (GTK_CONTAINER (event), image);
	moko_finger_scroll_add_with_viewport(moko, event);
	gtk_notebook_append_page(notebook, moko, label);

	g_signal_connect (G_OBJECT (event), "button-press-event",
		G_CALLBACK (example_button_press), image);

	//g_object_set(G_OBJECT(moko), "hindicator_mode", MOKO_FINGER_SCROLL_INDICATOR_MODE_HIDE);
	//g_object_set(G_OBJECT(moko), "vindicator_mode", MOKO_FINGER_SCROLL_INDICATOR_MODE_HIDE);

	g_object_set(G_OBJECT(moko), "hindicator_mode", MOKO_FINGER_SCROLL_INDICATOR_MODE_ALPHA, NULL);
        g_object_set(G_OBJECT(moko), "vindicator_mode", MOKO_FINGER_SCROLL_INDICATOR_MODE_ALPHA, NULL);
}

add_layout_view(GtkNotebook *notebook)
{
	GtkWidget 		*layout ,*label,*moko;
	GtkTreeModel	*model;	
	GError			*error;
	GtkWidget 		*event;
	GtkAllocation alloc;
	GtkTreeView *tree;
	GtkDrawingArea *draw;

	GdkGC *gc;
	GdkColormap *colormap;
	GdkBitmap *window_shape_bitmap;

	GdkColor black;
	GdkColor white;

	layout = gtk_layout_new(NULL,NULL);
	gtk_widget_show(layout);

	label = gtk_label_new("layout");
	gtk_widget_show(label);

	moko = moko_finger_scroll_new ();
	gtk_widget_show(moko);
	
	model= create_and_fill_model();		
	tree  = gtk_tree_view_new ();
	gtk_widget_show(tree);
	gtk_tree_view_insert_column_with_attributes(tree, -1, "Name", gtk_cell_renderer_text_new(), "text",COL_NAME,NULL);
	gtk_tree_view_insert_column_with_attributes(tree, -1, "Age", gtk_cell_renderer_text_new(), "text",COL_AGE, NULL);
	gtk_tree_view_set_model(tree, model);
	
	gtk_layout_put(layout, tree,  10,10);
	gtk_layout_set_size(layout, 500, 600);

	gtk_container_add (GTK_CONTAINER (moko), layout);
	gtk_notebook_append_page(notebook, moko, label);
}
int
main (int argc, char *argv[])
{
	GtkTreeModel	*model;	
	GtkWidget		*top;
	GtkWidget 		*notebook, *tree ,*label,*moko;

	gtk_init (&argc, &argv);
	
	top = gtk_window_new (GTK_WINDOW_TOPLEVEL);	

	g_signal_connect (G_OBJECT (top), "delete-event",  (GCallback) gtk_main_quit, NULL);
	gtk_widget_show_all (top);

	notebook = gtk_notebook_new ();
	gtk_widget_show(notebook);
	gtk_container_add (GTK_CONTAINER (top), notebook);
  	//gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_BOTTOM);
  	
	add_tree_view(notebook);
	add_icon_view(notebook);
	// there are problems working with textview
	add_text_view(notebook);
	add_image(notebook);
	add_layout_view(notebook);
	gtk_main ();
}
