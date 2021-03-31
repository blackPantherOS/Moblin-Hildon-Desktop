/*
#    Copyright (c) 2007 Intel Corporation
#
#    This program is free software; you can redistribute it and/or modify it
#    under the terms of the GNU General Public License as published by the Free
#    Software Foundation; version 2 of the License
#
#    This program is distributed in the hope that it will be useful, but
#    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
#    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
#    for more details.
#
#    You should have received a copy of the GNU General Public License along
#    with this program; if not, write to the Free Software Foundation, Inc., 59
#    Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#include <stdlib.h>
#include <gtk/gtk.h>
#include <hildon/hildon-program.h>
#include <hildon/hildon-window.h>
#include <time.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <config.h>
#include <libosso.h>

typedef struct {
  GtkWidget *button1,
    *button2,
    *button3,
    *button4;
  gint32 lastClicked;
} buttons;

gint32 level, step;
gint32  width, height; /* width and height of screen */ 
GArray *seqArr;
GtkWidget *levelLbl;
Display *display;

static void start_sequence(GtkWidget *widget, buttons *buttonPkg)
{
  GtkWidget *image;
  GdkPixbuf *pixbuf;
  gint32 i;
  gint32 randNum;
  GdkColor color;
  struct timespec ts;
  GRand *numGenerator;

  /*Display sequence if user advances to a new level*/
  if(step == 0)
    {
      numGenerator = g_rand_new();
      /* The time for the button to flash */
      ts.tv_sec = 0;
      ts.tv_nsec = 200000000;
  
      nanosleep(&ts, NULL);
      randNum = g_rand_int_range(numGenerator,1,5);
      g_array_append_val(seqArr,randNum);
      for(i=0; i<=level; i++)
	{
	  nanosleep(&ts, NULL);
	  switch(g_array_index(seqArr, gint32, i))
	    {
	    case 1:	
	      pixbuf = gdk_pixbuf_new_from_file_at_size(PIXMAPDIR "/yellow-down.png",
							width, height, NULL);
	      image = gtk_image_new_from_pixbuf(pixbuf);
	      
	      gtk_button_set_image(GTK_BUTTON(buttonPkg->button1), image);
	      /*forces the widget to raw itself*/
	      while (g_main_context_iteration(NULL, FALSE));
	      nanosleep(&ts, NULL);
	      pixbuf = gdk_pixbuf_new_from_file_at_size(PIXMAPDIR "/yellow-up.png",
							width, height, NULL);
	      image = gtk_image_new_from_pixbuf(pixbuf);
	      gtk_button_set_image(GTK_BUTTON(buttonPkg->button1), image);
	      break;
	    case 2:	gdk_color_parse("red", &color);
	      pixbuf = gdk_pixbuf_new_from_file_at_size(PIXMAPDIR "/red-down.png",
							width, height, NULL);
	      image = gtk_image_new_from_pixbuf(pixbuf);
	      gtk_button_set_image(GTK_BUTTON(buttonPkg->button2), image);
	      /*forces the widget to raw itself*/
	      while (g_main_context_iteration(NULL, FALSE));
	      nanosleep(&ts, NULL);
	      pixbuf = gdk_pixbuf_new_from_file_at_size(PIXMAPDIR "/red-up.png",
							width, height, NULL);
	      image = gtk_image_new_from_pixbuf(pixbuf);
	      gtk_button_set_image(GTK_BUTTON(buttonPkg->button2), image);
	      break;
	    case 3:	gdk_color_parse("blue", &color);
	      pixbuf = gdk_pixbuf_new_from_file_at_size(PIXMAPDIR "/blue-down.png",
							width, height, NULL);
	      image = gtk_image_new_from_pixbuf(pixbuf);
	      gtk_button_set_image(GTK_BUTTON(buttonPkg->button3), image);
	      /*forces the widget to raw itself*/
	      while (g_main_context_iteration(NULL, FALSE));
	      nanosleep(&ts, NULL);
	      pixbuf = gdk_pixbuf_new_from_file_at_size(PIXMAPDIR "/blue-up.png",
							width, height, NULL);
	      image = gtk_image_new_from_pixbuf(pixbuf);
	      gtk_button_set_image(GTK_BUTTON(buttonPkg->button3), image);
	      break;
	    case 4:	gdk_color_parse("#009900", &color);
	      pixbuf = gdk_pixbuf_new_from_file_at_size(PIXMAPDIR "/green-down.png",
							width, height, NULL);
	      image = gtk_image_new_from_pixbuf(pixbuf);
	      gtk_button_set_image(GTK_BUTTON(buttonPkg->button4), image);
	      /*forces the widget to raw itself*/
	      while (g_main_context_iteration(NULL, FALSE));
	      nanosleep(&ts, NULL);
	      pixbuf = gdk_pixbuf_new_from_file_at_size(PIXMAPDIR "/green-up.png",
							width, height, NULL);
	      image = gtk_image_new_from_pixbuf(pixbuf);
	      gtk_button_set_image(GTK_BUTTON(buttonPkg->button4), image);
	      break;
	    }
	  while (g_main_context_iteration(NULL, FALSE));
   
	}
    }
}

static void button_click(GtkWidget *widget, gpointer data)
{
  gint32 i,
    response;
  GtkWidget *dialog;

  if(g_array_index(seqArr, gint32, step) == GPOINTER_TO_INT(data))
    {
      if(step < level)
	step+=1;	
      else
	{
	  level+=1;
	  step = 0;
	  gtk_label_set_text(GTK_LABEL(levelLbl),
			     g_strdup_printf ("Level %d", level+1));
	  
	}
    }
  else
    {
      dialog = gtk_message_dialog_new (NULL,
				       GTK_DIALOG_DESTROY_WITH_PARENT,
				       GTK_MESSAGE_QUESTION,
				       GTK_BUTTONS_YES_NO,
				       "You made it to level %d!", level+1);
      gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
					       "Would you like to try again?");
      gtk_window_set_title(GTK_WINDOW(dialog), "Game Over");

      response = gtk_dialog_run(GTK_DIALOG(dialog));
      gtk_widget_destroy(dialog);

      switch (response) {
      case GTK_RESPONSE_YES:
	break;
      case GTK_RESPONSE_NO:
	gtk_main_quit();
	break;
      }

      g_array_remove_range(seqArr,0,level);
      step = 0;
      level = 0; 

      gtk_label_set_text(GTK_LABEL(levelLbl),
			 g_strdup_printf ("Level %d", level+1));
    }
}


static gboolean delete_event( GtkWidget *widget,
			      GdkEvent *event,
			      gpointer data)
{
  gtk_main_quit();
  return FALSE;
}


static gboolean button_change( GtkWidget *widget,
				    GdkEvent *event,
				    gpointer new_image)
{
  GtkWidget *image;
  GdkPixbuf *pixbuf;

  pixbuf = gdk_pixbuf_new_from_file_at_size(new_image, width, height, NULL);
  image = gtk_image_new_from_pixbuf(pixbuf);

  gtk_button_set_image(GTK_BUTTON(widget), image);
  
  while (g_main_context_iteration(NULL, FALSE));

  if(event->type == GDK_BUTTON_RELEASE)
    {
      gtk_button_clicked(GTK_BUTTON(widget));

    }
  return TRUE;
}


static void setup_button(buttons **buttonPkg, GtkWidget **button, gint32 buttonNum,
		  gpointer downImgLoc, gpointer upImgLoc )
{
  GtkWidget *image;
  GdkPixbuf *pixbuf;

  pixbuf = gdk_pixbuf_new_from_file_at_size(upImgLoc, width, height, NULL);
  image = gtk_image_new_from_pixbuf(pixbuf);
  *button = gtk_button_new();
  gtk_widget_set_name(*button, "nobutton");
  gtk_button_set_image(GTK_BUTTON(*button), image);
  gtk_button_set_focus_on_click(GTK_BUTTON(*button), FALSE);
  gtk_button_set_relief(GTK_BUTTON(*button), GTK_RELIEF_NONE);
  g_signal_connect(G_OBJECT(*button),
		   "clicked", G_CALLBACK(button_click),
		   GINT_TO_POINTER(buttonNum));
  g_signal_connect(G_OBJECT(*button),
		   "clicked", G_CALLBACK(start_sequence), *buttonPkg);
  g_signal_connect(G_OBJECT(*button), 
		   "button_press_event",
		   G_CALLBACK(button_change),
		   downImgLoc);
  g_signal_connect(G_OBJECT(*button),
		   "button_release_event",
		   G_CALLBACK(button_change),
		   upImgLoc);
}

GtkWidget* setup_dialog(buttons **buttonPkg)
{
  GtkWidget *label;
  gchar *message;
  GtkWidget *button;
  GtkWidget *dialog;
  
  dialog = gtk_dialog_new();
  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
  gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_title(GTK_WINDOW(dialog), "Moblin Says");
  label = gtk_label_new("");
  message = g_markup_printf_escaped("<span><b>%s</b></span>\n"
				    "<span size='small'>%s</span>",
				    ("Welcome to Moblin Says!"),
				    ("Press Go to play the game."));
  gtk_label_set_markup(GTK_LABEL(label), message);

  g_free(message);
  gtk_misc_set_padding(GTK_MISC(label), 12,0);
  button = gtk_button_new_with_label("Go!");
  g_signal_connect_swapped(button, "clicked",
			   G_CALLBACK(gtk_widget_hide), 
			   dialog);
  g_signal_connect(button, "clicked", 
		   G_CALLBACK(start_sequence), *buttonPkg);
  g_signal_connect_swapped(button, "clicked",
			   G_CALLBACK(gtk_widget_destroy),
			   dialog);
  
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label,
		    FALSE , FALSE, 10);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, 
			    FALSE, FALSE, 10);
  
  return dialog;
}

static void new_game(gpointer data)
{

  level = 0;
  step = 0;

  if (seqArr)
     g_array_remove_range(seqArr,0,level);
  else
    seqArr = g_array_new(FALSE, FALSE, sizeof(gint32));

  if (levelLbl)
    gtk_label_set_text(GTK_LABEL(levelLbl),
			 g_strdup_printf ("Level %d", level+1));

}

static void attach_menu(HildonWindow *window, buttons **buttonPkg )
{
  GtkWidget *menu;
  GtkWidget *item;

  if(NULL == window)
    return;
  
  menu = gtk_menu_new();
  
  item = gtk_menu_item_new_with_label("New Game");
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  g_signal_connect_swapped(item, "activate", 
			   G_CALLBACK(new_game), NULL);
  g_signal_connect(item, "activate",
		   G_CALLBACK(start_sequence), *buttonPkg);
  item = gtk_menu_item_new_with_label("Quit");
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  g_signal_connect_swapped(item, "activate",
			   G_CALLBACK (gtk_main_quit), window); 
  
  hildon_window_set_menu(window, GTK_MENU(menu)); 
  gtk_widget_show_all(GTK_WIDGET(menu));

}


int main(int argc, char *argv[] )
{
  HildonProgram *program;
  HildonWindow *window;
  GtkWidget *vbox1;
  GtkWidget *vbox2;
  GtkWidget *hbox1;
  GtkWidget *hbox2;
  GtkWidget *hbox3;
  GtkWidget *dialog;
  buttons abuttonPkg;
  buttons *buttonPkg;
  osso_context_t *osso_context;
  
  /* Add custom gtkrc file to be parsed */
  gtk_rc_add_default_file(PROGDIR "/customgtkrc");

  /* Checks screen resolution and scales button to fit*/
  display = XOpenDisplay( NULL );
  width = DisplayWidth( display, DefaultScreen(display)) / 2 - 10;
  height = DisplayHeight( display, DefaultScreen(display)) / 2 - 85;
  
  buttonPkg = &abuttonPkg;

  gtk_init (&argc, &argv);
  osso_context = osso_initialize("org.moblin.moblinsays", "0.1", TRUE, NULL);

  if(NULL == osso_context)
    return OSSO_ERROR;
  /** Initilize new game **/
  new_game(NULL);

  program = hildon_program_get_instance();
  window = HILDON_WINDOW(hildon_window_new());
  
  hildon_program_add_window(program, window);
  g_set_application_name("moblinsays");

  g_signal_connect (G_OBJECT(window), "delete_event",
		    G_CALLBACK(delete_event), NULL);

  /*sets the border width of the window*/
  gtk_container_set_border_width(GTK_CONTAINER(window), 10);

  vbox1 = gtk_vbox_new(FALSE, 0);
  vbox2 = gtk_vbox_new(FALSE, 0);
  hbox1 = gtk_hbox_new(FALSE, 0);
  hbox2 = gtk_hbox_new(FALSE, 0);
  hbox3 = gtk_hbox_new(FALSE, 0);

  gtk_container_add(GTK_CONTAINER (window), vbox1);
  gtk_box_pack_start(GTK_BOX(vbox1), vbox2, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(vbox1), hbox1, TRUE, TRUE, 0);

  gtk_box_pack_start(GTK_BOX(vbox1), hbox2, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(vbox1), hbox3, TRUE, TRUE, 0);

  /** BUTTON1 **/  
  setup_button(&buttonPkg, &buttonPkg->button1, 1,
	       PIXMAPDIR "/yellow-down.png", PIXMAPDIR "/yellow-up.png");
  gtk_box_pack_start(GTK_BOX(hbox1), buttonPkg->button1, 
		     FALSE, TRUE, 0);
  gtk_widget_show(buttonPkg->button1);


  /** BUTTON2 **/
  setup_button(&buttonPkg, &buttonPkg->button2, 2,
	       PIXMAPDIR "/red-down.png", PIXMAPDIR "/red-up.png");
  gtk_box_pack_start(GTK_BOX(hbox1), buttonPkg->button2, 
		     FALSE, TRUE, 0);
  gtk_widget_show(buttonPkg->button2);
		

  /** BUTTON 3**/  
  setup_button(&buttonPkg, &buttonPkg->button3, 3,
	       PIXMAPDIR "/blue-down.png", PIXMAPDIR "/blue-up.png");
  gtk_box_pack_start(GTK_BOX(hbox2), buttonPkg->button3, 
		     FALSE, TRUE, 0);
  gtk_widget_show(buttonPkg->button3);


  /** BUTTON4 **/  
  setup_button(&buttonPkg, &buttonPkg->button4, 4,
	       PIXMAPDIR "/green-down.png", PIXMAPDIR "/green-up.png");
  gtk_box_pack_start(GTK_BOX(hbox2), buttonPkg->button4, 
		     FALSE, TRUE, 0);
  gtk_widget_show(buttonPkg->button4);


  /** Go Button **/
  dialog = setup_dialog(&buttonPkg);
  gtk_widget_show_all(dialog);
		     

  /** Level Label **/
  levelLbl = gtk_label_new(g_strdup_printf ("Level %d", level+1));
  gtk_box_pack_start(GTK_BOX(vbox2), levelLbl, TRUE, TRUE, 0);
  gtk_widget_show(levelLbl);


  /** Menu **/
  attach_menu(window, &buttonPkg);
  gtk_widget_show_all(GTK_WIDGET(window));

  gtk_main();

  if(osso_context)
  {
    osso_deinitialize(osso_context);
    osso_context = NULL;
  }

  return 0;
}
