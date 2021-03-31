/*
 *  menu.c
 *  This file is part of Mousepad
 *
 *  Copyright (C) 2006 Benedikt Meurer <benny@xfce.org>
 *  Copyright (C) 2005 Erik Harrison
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

#include <gdk/gdkkeysyms.h>
#include "mousepad.h"

static GtkWidget *cut_menu_item;
static GtkWidget *copy_menu_item;
static GtkWidget *paste_menu_item;
static GtkWidget *delete_menu_item;

gboolean menu_toggle_paste_item(void)
{
	gtk_widget_set_sensitive(
		paste_menu_item,
		gtk_clipboard_wait_is_text_available(
			gtk_clipboard_get(GDK_SELECTION_CLIPBOARD)));
/*g_print("CLIPBOARD_CHECKED!\n"); */
	
	return FALSE;
}

void menu_toggle_clipboard_item(gboolean selected)
{
	gtk_widget_set_sensitive(cut_menu_item, selected);
	gtk_widget_set_sensitive(copy_menu_item, selected);
	gtk_widget_set_sensitive(delete_menu_item, selected);
/*	menu_toggle_paste_item(); */
}

static gchar *menu_translate(const gchar *path, gpointer data)
{
	gchar *retval;
	
	retval = (gchar *)g_strdup(path);
	return retval;
}

static const GtkItemFactoryEntry menu_items[] =
{
	{ "/_File", NULL,
		NULL, 0, "<Branch>" },
	{ "/File/_New", "<control>N",
		G_CALLBACK(cb_file_new_window), 0, "<StockItem>", GTK_STOCK_NEW },
#if 0
	{ "/File/New _Window", "<shift><control>N",
		G_CALLBACK(cb_file_new), 1 },
#endif
	{ "/File/_Open...", "<control>O",
		G_CALLBACK(cb_file_open), 0, "<StockItem>", GTK_STOCK_OPEN },
#if GTK_CHECK_VERSION(2,10,0)
  { "/File/Open _Recent", NULL,
    NULL, 0, "<Item>", NULL, },
#endif
	{ "/File/---", NULL,
		NULL, 0, "<Separator>" },
	{ "/File/_Save", "<control>S",
		G_CALLBACK(cb_file_save), 0, "<StockItem>", GTK_STOCK_SAVE },
	{ "/File/Save _As...", NULL,
		G_CALLBACK(cb_file_save_as), 0, "<StockItem>", GTK_STOCK_SAVE_AS },
	{ "/File/---", NULL,
		NULL, 0, "<Separator>" },
	{ "/File/_Print...", "<control>P",
		G_CALLBACK(cb_file_print), 0, "<StockItem>", GTK_STOCK_PRINT },
	{ "/_Edit",	 NULL,
		NULL, 0, "<Branch>" },
	{ "/Edit/_Undo", "<control>Z",
		G_CALLBACK(cb_edit_undo), 0, "<StockItem>", GTK_STOCK_UNDO },
	{ "/Edit/_Redo", "<control>Y",
		G_CALLBACK(cb_edit_redo), 0, "<StockItem>", GTK_STOCK_REDO },
	{ "/Edit/---", NULL,
		NULL, 0, "<Separator>" },
	{ "/Edit/Cu_t", "<control>X",
		G_CALLBACK(cb_edit_cut), 0, "<StockItem>", GTK_STOCK_CUT },
	{ "/Edit/_Copy", "<control>C",
		G_CALLBACK(cb_edit_copy), 0, "<StockItem>", GTK_STOCK_COPY },
	{ "/Edit/_Paste", "<control>V",
		G_CALLBACK(cb_edit_paste), 0, "<StockItem>", GTK_STOCK_PASTE },
	{ "/Edit/_Delete", NULL,
		G_CALLBACK(cb_edit_delete), 0, "<StockItem>", GTK_STOCK_DELETE },
	{ "/Edit/---", NULL,
		NULL, 0, "<Separator>" },
	{ "/Edit/Select _All", "<control>A",
		G_CALLBACK(cb_edit_select_all), 0 },
	{ "/_Search",	 NULL,
		NULL, 0, "<Branch>" },
	{ "/Search/_Find...", "<control>F",
		G_CALLBACK(cb_search_find), 0, "<StockItem>", GTK_STOCK_FIND },
	{ "/Search/Find _Next", "F3",
		G_CALLBACK(cb_search_find_next), 0 },
	{ "/Search/Find _Previous", "<shift>F3",
		G_CALLBACK(cb_search_find_prev), 0 },
	{ "/Search/_Replace...", "<control>H",
		G_CALLBACK(cb_search_replace), 0, "<StockItem>", GTK_STOCK_FIND_AND_REPLACE },
	{ "/Search/---", NULL,
		NULL, 0, "<Separator>" },
	{ "/Search/_Jump To...", "<control>J",
		G_CALLBACK(cb_search_jump_to), 0, "<StockItem>", GTK_STOCK_JUMP_TO },
	{ "/_Options", NULL,
		NULL, 0, "<Branch>" },
	{ "/Options/_Font...", NULL,
		G_CALLBACK(cb_option_font), 0, "<StockItem>", GTK_STOCK_SELECT_FONT },
	{ "/Options/_Word Wrap", NULL,
		G_CALLBACK(cb_option_word_wrap), 0, "<CheckItem>" },
	{ "/Options/_Line Numbers", NULL,
		G_CALLBACK(cb_option_line_numbers), 0, "<CheckItem>" },
	{ "/Options/---", NULL,
		NULL, 0, "<Separator>" },
	{ "/Options/_Auto Indent", NULL,
		G_CALLBACK(cb_option_auto_indent), 0, "<CheckItem>" },
	{ "/_About", "F1",
		G_CALLBACK(cb_help_about), 0, "<Item>" },
        { "/---", NULL,
                NULL, 0, "<Separator>" },
        { "/_Quit", "<control>Q",
                G_CALLBACK(cb_file_quit), 0, "<Item>" },
};

static gint nmenu_items = sizeof(menu_items) / sizeof(GtkItemFactoryEntry);

GtkWidget *create_menu_bar(GtkWidget *window, StructData *sd)
{
        GtkAccelGroup *accel_group;
        GtkItemFactory *ifactory;
        GtkWidget *main_menu;
#if GTK_CHECK_VERSION(2,10,0)
        GtkRecentFilter *recent_filter;
        GtkWidget *recent_menu_item;
        GtkWidget *recent_menu;
#endif

        accel_group = gtk_accel_group_new();
        ifactory = gtk_item_factory_new(GTK_TYPE_MENU, "<main>", accel_group);
        gtk_item_factory_set_translate_func(ifactory, menu_translate, NULL, NULL);
        gtk_item_factory_create_items(ifactory, nmenu_items, (GtkItemFactoryEntry*) menu_items, sd);
        gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);

        /* hidden keybinds */
        gtk_accel_group_connect(
                accel_group, GDK_W, GDK_CONTROL_MASK, 0,
                g_cclosure_new_swap(G_CALLBACK(cb_file_new), sd, NULL));
/*      gtk_widget_add_accelerator(
                gtk_item_factory_get_widget(ifactory, "/File/New"),
                "activate", accel_group, GDK_W, GDK_CONTROL_MASK, 0); */
        gtk_widget_add_accelerator(
                gtk_item_factory_get_widget(ifactory, "/File/Save As..."),
                "activate", accel_group, GDK_S, GDK_SHIFT_MASK | GDK_CONTROL_MASK, 0);
/*      gtk_widget_add_accelerator(
                gtk_item_factory_get_widget(ifactory, "<main>/File/Quit"),
                "activate", accel_group, GDK_Escape, 0, 0); */
        gtk_widget_add_accelerator(
                gtk_item_factory_get_widget(ifactory, "/Edit/Redo"),
                "activate", accel_group, GDK_Z, GDK_SHIFT_MASK | GDK_CONTROL_MASK, 0);
        gtk_widget_add_accelerator(
                gtk_item_factory_get_widget(ifactory, "/Search/Find Next"),
                "activate", accel_group, GDK_G, GDK_CONTROL_MASK, 0);
        gtk_widget_add_accelerator(
                gtk_item_factory_get_widget(ifactory, "/Search/Find Previous"),
                "activate", accel_group, GDK_G, GDK_SHIFT_MASK | GDK_CONTROL_MASK, 0);
        gtk_widget_add_accelerator(
                gtk_item_factory_get_widget(ifactory, "/Search/Replace..."),
                "activate", accel_group, GDK_R, GDK_CONTROL_MASK, 0);

        gtk_widget_set_sensitive(
                gtk_item_factory_get_widget(ifactory, "/Search/Find Next"),
                FALSE);
        gtk_widget_set_sensitive(
                gtk_item_factory_get_widget(ifactory, "/Search/Find Previous"),
                FALSE);

        cut_menu_item = gtk_item_factory_get_widget(ifactory, "/Edit/Cut");
        copy_menu_item = gtk_item_factory_get_widget(ifactory, "/Edit/Copy");
        paste_menu_item = gtk_item_factory_get_widget(ifactory, "/Edit/Paste");
        delete_menu_item = gtk_item_factory_get_widget(ifactory, "/Edit/Delete");
        menu_toggle_clipboard_item(FALSE);

        /* planned functions */
/*      gtk_widget_set_sensitive(
                gtk_item_factory_get_widget(ifactory, "<main>/Options/Auto Indent"),
                FALSE);
        gtk_item_factory_delete_item(ifactory, "/File/New Window"); */

#if GTK_CHECK_VERSION(2,10,0)
        /* add the recent chooser menu */
        recent_menu = gtk_recent_chooser_menu_new();
        recent_filter = gtk_recent_filter_new();
        gtk_recent_filter_add_application(recent_filter, "Moblin Notes Text Editor");
        gtk_recent_chooser_add_filter(GTK_RECENT_CHOOSER(recent_menu), recent_filter);
        gtk_recent_chooser_set_local_only(GTK_RECENT_CHOOSER(recent_menu), TRUE);
        gtk_recent_chooser_set_limit(GTK_RECENT_CHOOSER(recent_menu), 10);
        gtk_recent_chooser_set_show_tips(GTK_RECENT_CHOOSER(recent_menu), TRUE);
        g_signal_connect_swapped(G_OBJECT(recent_menu), "item-activated", G_CALLBACK(cb_file_open_recent), sd);

        recent_menu_item = gtk_item_factory_get_widget(ifactory, "/File/Open Recent");
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(recent_menu_item), recent_menu);
#endif

        main_menu = gtk_item_factory_get_widget(ifactory, "<main>");
	hildon_window_set_menu(HILDON_WINDOW(window), GTK_MENU(main_menu));
	return main_menu;
}
