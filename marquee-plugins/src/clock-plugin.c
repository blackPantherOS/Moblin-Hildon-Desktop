/*
 * Copyright (C) 2007 Intel Corporation
 *
 * Author:  Xu Bo <bo.b.xu@intel.com>
 *          Horace Li <horace.li@intel.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <gtk/gtkcalendar.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>

#include "clock-plugin.h"

#define MAX_READ_SIZE 50
#define CONFIG_FILE "/etc/clock-plugin.conf"
#define DEFAULT_TIME_FORMAT "\%a \%b \%d, \%r\%p"
#define GTK_RESPONSE_EDIT 1

#define CLOCK_PLUGIN_GET_PRIVATE(object) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((object), CLOCK_TYPE_PLUGIN, ClockPluginPrivate))

HD_DEFINE_PLUGIN (ClockPlugin, clock_plugin, TASKNAVIGATOR_TYPE_ITEM);

struct _ClockPluginPrivate
{
   GtkWidget *btn;
   GtkWidget *time_lb;
   GtkWidget *cal_dlg;
   gchar time_format[20];
   guint timeout_id;
   gint panel_height;
};

void clock_plugin_screen_changed (TaskNavigatorItem *item);
static void clock_plugin_finalize (GObject *object);

static gboolean clock_plugin_update_time (gpointer data)
{
  ClockPlugin *clock_plugin = (ClockPlugin *)data;
  ClockPluginPrivate *priv = CLOCK_PLUGIN_GET_PRIVATE (clock_plugin);
  struct timeval  time_val;
  struct tm      *tm_time;
  gchar          *tz_str;
  gchar		time_str[60];
	
  //reset timezone
  tz_str = g_strdup (g_getenv ("TZ"));
  g_unsetenv ("TZ");
  if (tz_str)
  {
    g_setenv ("TZ", tz_str, FALSE);
    g_free (tz_str);
  }
  gettimeofday (&time_val, NULL);
  tm_time = localtime (&time_val.tv_sec);
  strftime (time_str, G_N_ELEMENTS (time_str), priv->time_format, tm_time);

  gtk_label_set_text((GtkLabel*)priv->time_lb, time_str);

  if (priv->timeout_id)
    g_source_remove (priv->timeout_id);
  //update per second
  priv->timeout_id = g_timeout_add (1000, clock_plugin_update_time, clock_plugin);
  return FALSE;
}

// Display the pop-up calendar
static void on_cal_btn_clicked (GtkWidget *widget, ClockPlugin *clock_plugin)
{
  ClockPluginPrivate *priv = CLOCK_PLUGIN_GET_PRIVATE (clock_plugin);
  if (priv->cal_dlg == NULL)
  {
    GtkWidget *gtk_cal = gtk_calendar_new ();

    //set calendar display options and packing
    gtk_calendar_set_display_options (GTK_CALENDAR (gtk_cal),
                                      GTK_CALENDAR_SHOW_HEADING |
                                      GTK_CALENDAR_SHOW_DAY_NAMES |
                                      GTK_CALENDAR_SHOW_WEEK_NUMBERS);

    priv->cal_dlg = gtk_dialog_new ();
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG(priv->cal_dlg)->vbox), gtk_cal, TRUE, TRUE, 0);

    gtk_dialog_set_has_separator (GTK_DIALOG (priv->cal_dlg), FALSE);

    //set decorations, needs realizing first
    gtk_widget_realize (priv->cal_dlg);
    gdk_window_set_decorations (priv->cal_dlg->window, GDK_DECOR_BORDER);

    //get the window position for further setting
    gint dlg_x, dlg_y;
    gtk_window_get_position (GTK_WINDOW (priv->cal_dlg), &dlg_x, &dlg_y);

    gtk_widget_show_all (priv->cal_dlg);

    //set the window position
    gtk_window_move (GTK_WINDOW (priv->cal_dlg), dlg_x, priv->panel_height+2);
  }
  else
  {
    gtk_widget_destroy (priv->cal_dlg);
    priv->cal_dlg = NULL;
  }
}

static void clock_plugin_set_up_time_format (ClockPlugin *clock_plugin, gint scn_width)
{
  ClockPluginPrivate *priv;
  struct stat config_stat;
  FILE *fd_config;
  gchar readbuf[MAX_READ_SIZE], *param;
  gboolean format_inited = 0;

  priv = CLOCK_PLUGIN_GET_PRIVATE (clock_plugin);

  // if screen width is less than 1024 pixels, we will simplify the time format
  // so that it can be shown up completely.
  if (scn_width >= 1024)
  {
    //init time format
    if (stat(CONFIG_FILE, &config_stat) < 0) {
      //no config file, use default format
      strcpy(priv->time_format, DEFAULT_TIME_FORMAT);
    }
    else {
      fd_config = fopen(CONFIG_FILE,"r");
      while (fgets(readbuf, MAX_READ_SIZE, fd_config) != NULL) {
        //should have a "Format" field
        if (strncmp(readbuf, "Format:", 7) == 0) {
          param = readbuf+7;
          g_print ("*******\n\n\nClock param: %s\n\n\n******", param);
          if(strncmp(param, "Simple", 6) == 0) {
            //use simple format
            strcpy(priv->time_format,"\%T \%p");
          } else if(strncmp(param, "Full", 4) == 0) {
            //use full format, default format
            strcpy(priv->time_format,"\%a \%b \%d, \%R \%p");
          } else {
            strncpy(priv->time_format, param, strlen(param)-1);
            //strip extra "0" and lower am/pm
          }
          format_inited = 1;
          break;  //found format, that's all we need
        }
      }
      //if no "Format" field
      if (format_inited == 0) {
        //use a default format
        strcpy(priv->time_format, DEFAULT_TIME_FORMAT);
      }
      fclose(fd_config);
    }
  }
  else
    strcpy(priv->time_format,"\%l:\%M\%P");
}

static void clock_plugin_init (ClockPlugin *clock_plugin)
{
  ClockPluginPrivate *priv;
  GtkWidget *btn, *time_lb;
  gint scn_width = 800, plugin_width;

  priv = CLOCK_PLUGIN_GET_PRIVATE (clock_plugin);

  priv->panel_height = plugins_get_marquee_panel_height ();
  btn = priv->btn = gtk_button_new();

  //size of clock depending on size of window
  GdkScreen *screen = gtk_widget_get_screen (GTK_WIDGET(clock_plugin));
  if (screen != NULL) {
    scn_width = gdk_screen_get_width (screen);
  }

  if (scn_width > 600)
  {
    plugin_width = scn_width - 104 * (4 + scn_width / 320);

    gtk_widget_set_size_request (GTK_WIDGET(clock_plugin), plugin_width, priv->panel_height);
  }

  time_lb = priv->time_lb = gtk_label_new("");

  //set the font size smaller
  PangoAttribute *pa_size = pango_attr_size_new (17000);
  pa_size->start_index = 0;
  pa_size->end_index = -1;
  PangoAttribute *pa_font = pango_attr_family_new ("Sans");
  pa_font->start_index = 0;
  pa_font->end_index = -1;
  PangoAttribute *pa_color = pango_attr_foreground_new (0xE2E1,0xE2E1,0xE2E1);
  pa_color->start_index = 0;
  pa_color->end_index = -1;
  PangoAttrList *pl = pango_attr_list_new();
  pango_attr_list_insert(pl,pa_size);
  pango_attr_list_insert(pl,pa_font);
  pango_attr_list_insert(pl,pa_color);

  gtk_label_set_attributes ((GtkLabel*)time_lb, pl);
  pango_attr_list_unref (pl);

  // add time_lb
  gtk_container_add (GTK_CONTAINER (btn), time_lb);
  gtk_container_add (GTK_CONTAINER (clock_plugin), btn);

  if (scn_width > 600)
    gtk_widget_show_all (GTK_WIDGET(clock_plugin));
  else
    gtk_widget_hide_all (GTK_WIDGET(clock_plugin));

  clock_plugin_set_up_time_format (clock_plugin, scn_width);
  clock_plugin_update_time(clock_plugin);

  //when clicked, launch time/date setting
  priv->cal_dlg = NULL;
  g_signal_connect (btn, "clicked", G_CALLBACK (on_cal_btn_clicked), clock_plugin);
}

void clock_plugin_screen_changed (TaskNavigatorItem *item)
{
  ClockPlugin *clock_plugin = (ClockPlugin *)item;
  GdkScreen *screen = gdk_screen_get_default ();
  gint width, height, scn_width;

  scn_width = gdk_screen_get_width (screen);
  if (scn_width > 600)
  {
    GList *children, *iter;
    width = scn_width - 104 * (4 + scn_width / 320);
    height = plugins_get_marquee_panel_height ();

    children = gtk_container_get_children (GTK_CONTAINER(clock_plugin));
    for (iter = children ; iter ; iter = g_list_next (iter))
    {
      if (GTK_IS_WIDGET (iter->data))
      {
        gtk_widget_set_size_request (GTK_WIDGET(iter->data), width, height);
      }
    }

    gtk_widget_set_size_request (GTK_WIDGET(clock_plugin), width, height);
    gtk_widget_show_all (GTK_WIDGET(clock_plugin));

    clock_plugin_set_up_time_format (clock_plugin, scn_width);
    clock_plugin_update_time(clock_plugin);
  }
  else
    gtk_widget_hide_all (GTK_WIDGET(clock_plugin));
}

static void clock_plugin_class_init (ClockPluginClass *class)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (class);
  TaskNavigatorItemClass *item_class = TASKNAVIGATOR_ITEM_CLASS (class);

  item_class->screen_changed = clock_plugin_screen_changed;
  object_class->finalize = clock_plugin_finalize;

  g_type_class_add_private (object_class, sizeof (ClockPluginPrivate));
}

static void clock_plugin_finalize (GObject *object)
{
  ClockPlugin *clock_plugin = CLOCK_PLUGIN (object);
  ClockPluginPrivate *priv = CLOCK_PLUGIN_GET_PRIVATE (clock_plugin);	
  if (priv->timeout_id)
    g_source_remove (priv->timeout_id);
  G_OBJECT_CLASS (clock_plugin_parent_class)->finalize (object);
}
