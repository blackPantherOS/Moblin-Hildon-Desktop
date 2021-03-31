/*
 * This plugin based on the example dummy plugin from the Maemo
 * maemo-af-desktop repository.  Original copyright declaration:
 * Lucas Rocha <lucas.rocha@nokia.com>
 * Copyright 2006 Nokia Corporation.
 *
 * Modifcations transform the example to an embedded mozilla home applet:
 * Bob Spencer <bob.spencer@intel.com>
 * Copyright 2007 Intel Corporation
 *
 * Contributors:
 * Michael Frey <michael.frey@pepper.com>
 * Rusty Lynch <rusty.lynch@intel.com>
 * Horace Li <horace.li@intel.com>
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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/resource.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glib-object.h>

#include "mobile-basic-home-plugin.h"
#include "gtkmozembed.h"

#include <libhildondesktop/libhildondesktop.h>
#include <libhildonwm/hd-wm.h>
#include <libosso.h>
#include <hildon/hildon-banner.h>
#include <hildon/hildon-note.h>

#define GMENU_I_KNOW_THIS_IS_UNSTABLE
#include <gmenu-tree.h>

#define FILE_URL_PREFIX "file://"
#define PROFILE_DIR "/usr/share/mobile-basic-flash"
#define HOME_HTML   "home.html"

#define PROFILE_NAME "mozapplet"
#define MARQUEE_HEIGHT 52
#define DESKTOP_FILE_SUFFIX ".desktop"
#define DESKTOP_GROUP "Desktop Entry"

#define DESKTOP_DIR    "/usr/share/mobile-basic-flash/applications"
#define CP_PLUGINS_DIR "/usr/share/applications/hildon-control-panel"

//TBD: use theme pattern to get fonts
#define PIXMAP_DIR    "/usr/share/pixmaps"

//TBD: use gconf header to replace all gconf defines
#define BKGD_PATH             "/desktop/moblin/background"
#define BKGD_FILE             BKGD_PATH "/picture_filename"
#define BKGD_PIC_OPTIONS      BKGD_PATH "/picture_options"
#define BKGD_PRIMARY_COLOR    BKGD_PATH "/primary_color"

#define INTERFACE_PATH        "/desktop/gnome/interface"
#define GTK_THEME             INTERFACE_PATH "/gtk_theme"
#define ICON_THEME            INTERFACE_PATH "/icon_theme"
#define FONT_NAME             INTERFACE_PATH "/font_name"

//gconf location of custom flash movie, and hide/show marquee
#define HILDON_DESKTOP_GCONF_PATH "/desktop/hildon"
#define MARQUEE_KEY     HILDON_DESKTOP_GCONF_PATH "/marquee/hide"
#define HOMEPAGE_KEY    HILDON_DESKTOP_GCONF_PATH "/htmlhomeplugin/homepage"
#define FLASHMOVIE_KEY  HILDON_DESKTOP_GCONF_PATH "/htmlhomeplugin/flashmoviename"
#define ONLYSHOWIN_FILTER_KEY  HILDON_DESKTOP_GCONF_PATH "/htmlhomeplugin/onlyshowin_filter"
#define ONLYSHOWIN_IGNORE_KEY  HILDON_DESKTOP_GCONF_PATH "/htmlhomeplugin/onlyshowin_ignore"

#define MARQUEE_GCONF_PATH "/apps/marquee-plugins"
#define ACTIVE_CAT_KEY MARQUEE_GCONF_PATH "/active"
#define CATEGORIES_KEY MARQUEE_GCONF_PATH "/categories"

#define UNKNOWN_ICON_NAME     "unknown"

#define DialogDestroy 111

#define MB_HOME_PLUGIN_GET_PRIVATE(object) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((object), MOBILE_BASIC_TYPE_HOME_PLUGIN, MobileBasicHomePluginPrivate))

HD_DEFINE_PLUGIN (MobileBasicHomePlugin, mobile_basic_home_plugin, HILDON_DESKTOP_TYPE_HOME_ITEM);

static GtkWidget* launch_banner = NULL;

typedef struct {
	int index;
	gchar *plugin;
	gchar *name;
	gchar *icon;
	gchar *exec;
	gchar *service;
	gchar *filename;
	gchar *id;
	gchar *cat;
} application_entry_t;

struct _plugin_context_t {
        GtkWidget *container;
        GList *app_list;
        osso_context_t *osso_context;
        GConfClient *client;
        const char *active;
};


extern void init_window_helper (plugin_context_t *context);
static GdkFilterReturn mobile_basic_home_x_event_handler (GdkXEvent *xevent, GdkEvent *event, gpointer data);
static void home_screen_changed (HildonDesktopHomeItem *item);
static void start_plugin(plugin_context_t *, gchar*);

void hide_banner(void);
void net_stop_cb (GtkMozEmbed *embed, gpointer data);
void js_status_cb (GtkMozEmbed *embed, gpointer data);
gint compare_items(gconstpointer a, gconstpointer b);
static gboolean is_app_in_ignore_list (GSList *ignore_list, gchar *app);

// escape " and '
static gchar *strescape_quotes (gchar *src) {
    int len = strlen(src);
    gchar *tmp = malloc ((len*2)+1);
    int j=0;
    for (int i=0; i<len; i++,j++) {
        if (src[i]=='\'' || src[i]=='"') {
            tmp[j++]='\\';
        }            
        tmp[j]=src[i];
    }
    tmp[j]='\0';
    return tmp;
}

static void update_background(plugin_context_t *context) {
	gchar *pic, *opt, *color;

	pic = gconf_client_get_string (context->client,BKGD_FILE,NULL);
	opt = gconf_client_get_string (context->client,BKGD_PIC_OPTIONS,NULL);
	color = gconf_client_get_string (context->client,BKGD_PRIMARY_COLOR,NULL);

	//if a picture is selected (not "no Wallpaper"), make sure it is valid
	if ((pic != NULL) &&
	    (!g_utf8_validate(pic, -1, NULL) || !g_file_test(pic, G_FILE_TEST_EXISTS))) {
		pic = g_filename_from_utf8 (pic, -1, NULL,NULL, NULL);
	}

        //escape quotes
        if (pic) {
            gchar *tmp = pic;
            pic = strescape_quotes (tmp);
            g_free (tmp);
        }

	gchar *url = g_strdup_printf("javascript:setBackground(['%s', '%s', '%s'])", 
				     pic, opt, color);

	gtk_moz_embed_load_url(GTK_MOZ_EMBED(context->container), url);
	g_free(url);

	if (pic) {
            g_free (pic);
	}
       	if (opt) {
            g_free (opt);
	}
	if (color) {
            g_free (color);
	}
}


static void update_theme(plugin_context_t *context) {

	gchar *theme, *icon_theme, *font;
	theme =      gconf_client_get_string (context->client, GTK_THEME, NULL);
	icon_theme = gconf_client_get_string (context->client, ICON_THEME, NULL);
	font =       gconf_client_get_string (context->client, FONT_NAME, NULL);

	//a little hack -- Human is default now, but it doesn't exist. 
	if (!theme || (!strcmp (theme, "Human"))) {
		theme = g_strdup ("mobilebasic");
	}
	if (!icon_theme) {
		icon_theme = g_strdup ("hicolor");
        }
	gchar *url = g_strdup_printf("javascript:setThemeValues(['%s', '%s', '%s'])", 
				     theme, icon_theme, font);
	gtk_moz_embed_load_url(GTK_MOZ_EMBED(context->container), url);
	g_free(url);

	if (theme) {
		g_free (theme);
	}
       	if (icon_theme) {
		g_free (icon_theme);
	}
	if (font) {
		g_free (font);
	}
}

static void dump_application(gpointer data, gpointer ignore)
{
	application_entry_t *item = (application_entry_t *)data;
	g_print("app[%i] = %s\n", item->index, item->exec);
}

static void __add_application(plugin_context_t *c, application_entry_t *i)
{
	gchar *url = g_strdup_printf("javascript:addApp([%i, '%s', '%s'])",
				     i->index, 
				     i->name, 
				     i->icon);
	gtk_moz_embed_load_url(GTK_MOZ_EMBED(c->container), url);
	g_free(url);
	return;
}

static void add_application(gpointer data, gpointer payload)
{
	application_entry_t *item = (application_entry_t *)data;
	plugin_context_t *c = (plugin_context_t *)payload;

	if (!g_ascii_strcasecmp("All", c->active)) {
		if (g_ascii_strcasecmp(item->cat, "Preferences"))
			__add_application(c, item);
	}
	else {
		if (!g_ascii_strcasecmp(item->cat, c->active))
			__add_application(c, item);
	}
	return;
}

static gboolean init_program(plugin_context_t *c)
{
	gchar *url;
       
	update_background(c);
	update_theme(c);
	url = g_strdup_printf("javascript:launchDesktop()"); 
	gtk_moz_embed_load_url(GTK_MOZ_EMBED(c->container), url);
	g_free(url);

	return FALSE;
}

void hide_banner (void)
{
	if (launch_banner != NULL) {
		gtk_widget_destroy(launch_banner);
                launch_banner = NULL;
	}
}

static void show_banner (const gchar *msg)
{
	// make sure we are not already showing a launchBanner
	hide_banner();

        // now show a new launch banner
        launch_banner = hildon_banner_show_animation(NULL, NULL, msg);
        gtk_window_set_position(GTK_WINDOW (launch_banner), GTK_WIN_POS_CENTER_ALWAYS);
}

//this function parses a full exec string and returns the first portion
//before the space
static gchar *parse_exec(const gchar *exec_name)
{
  if (exec_name == NULL) {
    return NULL;
  }

  gchar *space = strchr(exec_name, ' ');
  if (space) {
    gchar *cmd = g_strdup (exec_name);
    cmd[space - exec_name] = 0;
    return cmd;
  }

  return g_strdup (exec_name);
}

//find a running app with same name/exec path as item passed in
static HDWMEntryInfo* find_running_app(application_entry_t *item)
{
  // check if app is already running, if so show it..
  GList *applications_list = NULL;
  HDWMApplication *app = NULL;
  HDWMEntryInfo *app_info = NULL;
  HDWM *hdwm = hd_wm_get_singleton();

  // get list of running applications, the app_info for the running
  // app is based on what is defined in /usr/share/applications/*.desktop
  // Specifically, the StartupWMClass attribute needs to be correct
  // in the desktop file or else the app_info won't be correct in this list
  applications_list = hd_wm_get_applications (hdwm);

  guint len = g_list_length(applications_list);
  printf ("find_running_app #running apps=%i\n", len);

  for (int i = 0; i < len; i++) {

    app = HD_WM_APPLICATION(g_list_nth_data(applications_list, i));
    const gchar *app_name = hd_wm_application_get_name(app);
    //get the first part of the exec before the space
    gchar *exec_name = parse_exec(hd_wm_application_get_exec(app));
    const gchar *class_name = hd_wm_application_get_class_name(app);

    g_debug("searching appName=%s execName=%s className=%s appInfo=%p\n", 
	    app_name, exec_name, class_name, 
	    HD_WM_ENTRY_INFO(hd_wm_application_get_active_window(app)));
    
    gchar *item_exec_name = parse_exec(item->exec);

    // look for match based on exec command or name
    if ((exec_name != NULL && (!g_ascii_strcasecmp(item_exec_name, exec_name))) ||
        (app_name != NULL && (!g_ascii_strcasecmp(item->name, app_name))))
        {

        app_info = HD_WM_ENTRY_INFO(hd_wm_application_get_active_window(app));
        g_debug("found already running returning app_name=%s app_exec=%s app_info=%p\n", 
		item->name, item_exec_name, app_info);

        g_free(exec_name);
        g_free(item_exec_name);

        return app_info;
    }

    g_free(exec_name);
    g_free(item_exec_name);

  }

  g_print("could not find match for app=%s\n", item->name);

  return NULL;
}


//Called to launch the application
//static void start_app(const gchar *app)
static void launch_app(plugin_context_t *c, application_entry_t *item)
{
	gchar *program = NULL;
	GError *error = NULL;
	gint argc;
	gchar **argv;
	GPid child_pid;
	gchar *space;

	if (item->plugin) {
		start_plugin(c, item->plugin);
		return;
	}

        //Show banner during startup.  
        //Banner is hidden in callback: window_stack_change_cb
        gchar* msg = g_strdup_printf("Starting %s...", item->name);
        show_banner(msg);

	if (item->service) {
		g_print("Kicking the %s service\n", item->service);
		hd_wm_top_service (item->service);
		return;
	} else {
		HDWMEntryInfo *running_app = find_running_app(item);
		if (running_app) {
			hd_wm_top_item(running_app);
			return;
		}
	}

	// otherwise launch a new instance
	space = strchr(item->exec, ' ');
	if (space) {
		gchar *cmd = g_strdup (item->exec);
		cmd[space - item->exec] = 0;
		gchar *exc = g_find_program_in_path (cmd);
		
		program = g_strconcat (exc, space, NULL);
		
		g_free (exc);
		g_free (cmd);
	} else {
		program = g_find_program_in_path (item->exec);
	}

	if (!program) {
		g_warning("Attempt to exec invalid entry: %s", item->exec);
		hide_banner();
	  return;
	}

	if (g_shell_parse_argv (program, &argc, &argv, &error)) {
	  g_spawn_async (
			 /* Child's current working directory,
			    or NULL to inherit parent's */
			 NULL,
			 /* Child's argument vector. [0] is the path of
			    the program to execute */
			 argv,
			 /* Child's environment, or NULL to inherit
			    parent's */
			 NULL,
			 /* Flags from GSpawnFlags */
			 (GSpawnFlags)0,
			 /* Function to run in the child just before
			    exec() */
			 NULL,
			 /* User data for child_setup */
			 NULL,
			 /* Return location for child process ID 
			    or NULL */
			 &child_pid,
			 /* Return location for error */
			 &error);
	}

	if (error) {
		g_warning ("Others_menu_activate_app: failed to execute %s: %s.",
			   program, error->message);
		g_clear_error (&error);
		hide_banner();
	} else {
		int priority;
		errno = 0;
		gchar *oom_filename;
		int fd;
		
		/* If the child process inherited desktop's high 
		 * priority, give child default priority */
		priority = getpriority (PRIO_PROCESS, child_pid);
	  
		if (!errno && priority < 0) {
			setpriority (PRIO_PROCESS, child_pid, 0);
		}
		
		/* Unprotect from OOM */
		oom_filename = g_strdup_printf ("/proc/%i/oom_adj",
						child_pid);
		fd = open (oom_filename, O_WRONLY);
		g_free (oom_filename);
		
		if (fd >= 0) {
			write (fd, "0", sizeof (char));
			close (fd);
		}
	}

	// Hide banner after 15s if not already hidden
	//g_timeout_add (15*1000, force_banner_hide, NULL);
}

static void start_app_from_index(plugin_context_t *c, int index) 
{
	GList *l;
	application_entry_t *item = NULL;

	g_print("Starting app %i\n", index);
	
	l = g_list_nth(c->app_list, index);
	if (!l) {
		g_warning("start_app::Invalid application index");
		g_list_foreach(c->app_list, dump_application, c->container);
		return;
	}

	item = (application_entry_t *)l->data;

	launch_app(c, item);
}


//start the application from a path string (command path and arguments)
//NOTE: this does not look up the application from the .desktop file
//but directly uses the command passed in for execution
static void start_app_from_path(plugin_context_t *c, gchar *path)
{
        // make an app entry and set the exec property
        application_entry_t *item = (application_entry_t *) g_malloc0(sizeof(application_entry_t));

	item->exec = path;
	item->name = path;

	launch_app(c, item);
}

//start an application based on it's name as defined in the .desktop file
static void start_app_from_name(plugin_context_t *c, gchar* appName) 
{
	application_entry_t *item = NULL;

	guint len = g_list_length(c->app_list);
	gboolean foundApp = FALSE;

	for (int i = 0; i < len; i++) {
		item = (application_entry_t *) g_list_nth_data(c->app_list, i);
		// compare name passed in with app name in list
		if(!g_ascii_strcasecmp(item->name, appName)) {
			foundApp = TRUE;
			break;
		}
	}
	
	if (!foundApp) {
		g_warning("start_app_from_name::couldn't find app with name=%s", appName);
		g_list_foreach(c->app_list, dump_application, c->container);
		return;
	}

	launch_app(c, item);
}

//start an application based on it's ID as defined in the .desktop file
static void start_app_from_id(plugin_context_t *c, char* appId) 
{
	application_entry_t *item = NULL;

	guint len = g_list_length(c->app_list);
	gboolean foundApp = FALSE;

	for (int i = 0; i < len; i++) {
		item = (application_entry_t *) g_list_nth_data(c->app_list, i);
		// compare name passed in with app name in list
		if(!g_ascii_strcasecmp(item->name, appId)) {
			foundApp = TRUE;
			break;
		}
	}
	
	if (!foundApp) {
		g_warning("start_app_from_id::couldn't find app with id=%s", appId);
		g_list_foreach(c->app_list, dump_application, c->container);
		return;
	}

	launch_app(c, item);
}

//This function will start a control panel plugin
static void start_plugin(plugin_context_t *c, gchar* pluginName) 
{
        g_print("launching plugin %s\n", pluginName);
	osso_return_t results = osso_cp_plugin_execute(c->osso_context, pluginName, c->container, TRUE);
	if (results == OSSO_OK) {
		g_print("plugin successfully executed\n");
	} else {
		g_print("error occurred executing plugin code=%i\n", results);
 	}
 }

/////////////////////////////////////////////////////////
// After the container HTML file has completed loading,
// loop through each application in the app_list and
// register the application appropriate app data with 
// a JavaScript funciton running in the HTML content.
//
// Once all application data has been registered, then
// trigger the flash content to initialize the desktop
void net_stop_cb (GtkMozEmbed *embed, gpointer data)
{
	plugin_context_t *context = (plugin_context_t *)data;
	g_list_foreach(context->app_list, add_application, data);
	g_idle_add((GSourceFunc)init_program, data);
}

/////////////////////////////////////////////////////////
// The Flash content (via the HTML container) will trigger
// actions in the native container by updating the HTML
// status field.  To do this we implement a very primitive
// text based protocol consisting of:
// "command:arg1:arg2:..."
//
// Current commands:
// run:index    Start the application associated with index
// log:msg      Write a string to the log
void js_status_cb (GtkMozEmbed *embed, gpointer data)
{
	char *message;
	gchar** tokens;
	plugin_context_t *c = (plugin_context_t *)data;

	message = gtk_moz_embed_get_js_status(embed);
	if (message) {
		tokens = g_strsplit(message, ":", 2); //(examples: "run_id:5" "run_app:myspace /mydocs/photos")
		if (tokens[0] == NULL || tokens[1] == NULL) {
			return;
		}

                if(!g_ascii_strcasecmp(tokens[0],"run_id")) {
			start_app_from_id(c, tokens[1]);

                } else if (!g_ascii_strcasecmp(tokens[0],"run_index")) {   //run: deprecated, use run_id or run_app
			start_app_from_index(c, atoi(tokens[1]));

                } else if (!g_ascii_strcasecmp(tokens[0],"run_path")) {
			start_app_from_path(c, tokens[1]);

                } else if (!g_ascii_strcasecmp(tokens[0],"run_name")) {
			start_app_from_name(c, tokens[1]);

                } else if (!g_ascii_strcasecmp(tokens[0],"launchPlugin")) {
			start_plugin(c, tokens[1]);

 		} else if (!g_ascii_strncasecmp(tokens[0],"log", 3)) {
			g_print("LOG: %s\n", tokens[1]);
 		}

		g_strfreev(tokens);
		g_free(message);
	}
}

gint compare_items(gconstpointer a, gconstpointer b)
{
	gchar *f1 = ((application_entry_t *)a)->filename;
	gchar *f2 = ((application_entry_t *)b)->filename;

	return g_strcasecmp(f1, f2);
}

static gboolean is_app_in_ignore_list (GSList *ignore_list, gchar *appconf)
{
  //get isolated name of .desktop  (no path, no extension)
  gchar *app = strrchr(appconf, '/') + 1;
  int pos = strlen(app) - strlen(".desktop");
  if (pos > 0 && !g_ascii_strcasecmp(app + pos, ".desktop")) {
      app[pos]='\0';
  }
  //see if app is in the list of apps to show, regardless of onlyshowin flag
  GSList *list = NULL;
  for (list = ignore_list; list; list = g_slist_next(list)) {
    if (!g_ascii_strcasecmp (app, (gchar *)list->data)) {
        return TRUE;
    }
  }
  return FALSE;
}

static GList* application_list_buildup (GList *list, GMenuTreeDirectory *directory)
{
  static gint index = 0;
  GSList *entry_list, *iter;
  GConfClient *client;
  GSList *onlyshowin_ignore = NULL;
  gboolean onlyshowin_filter = FALSE;

  client = gconf_client_get_default ();
  if (!client) {
    return NULL;
  }

  //Setup two filters: 
  // onlyshowin_filter will not show apps for .desktop
  //  files that don't have OnlyShowIn=GNOME;Mobile
  // onlyshowin_ignore will show an app regardless of onlyshowin
  onlyshowin_filter = gconf_client_get_bool (client,
                                             ONLYSHOWIN_FILTER_KEY,
                                             NULL);
  if (onlyshowin_filter) {
      onlyshowin_ignore = gconf_client_get_list (client,
                                                 ONLYSHOWIN_IGNORE_KEY,
                                                 GCONF_VALUE_STRING,
                                                 NULL);
  }

  entry_list = gmenu_tree_directory_get_contents (directory);

  for (iter = entry_list; iter; iter = iter->next)
  {
    GMenuTreeItem *item = GMENU_TREE_ITEM (iter->data);

    switch (gmenu_tree_item_get_type (item))
    {
      case GMENU_TREE_ITEM_DIRECTORY:
        list = application_list_buildup (list, GMENU_TREE_DIRECTORY (item));
        break;
      case GMENU_TREE_ITEM_ENTRY:
      {
        GMenuTreeEntry *entry = GMENU_TREE_ENTRY (item);
        GKeyFile *key_file = NULL;
        gchar *desktop_file = NULL;
        gchar **onlyshowin = NULL;
        gboolean show_app = FALSE;
        struct stat buf;

        desktop_file = g_strdup (gmenu_tree_entry_get_desktop_file_path (entry));
        if (!((stat(desktop_file, &buf) == 0) &&
            S_ISREG(buf.st_mode) &&
            g_str_has_suffix(desktop_file, DESKTOP_FILE_SUFFIX)))
        {
          g_free (desktop_file);
          break;
        }

        key_file = g_key_file_new();

        if ( FALSE == g_key_file_load_from_file (key_file, desktop_file,
               G_KEY_FILE_NONE, NULL))
        {
          g_key_file_free (key_file);
          g_free (desktop_file);
          break;
        }

        //  If gconf key ONLYSHOWIN_FILTER_KEY == True, check for 
        //   OnlyShowIn=GNOME;Mobile in .desktop file
        //  If not, skip app.  If no gconf key, show all apps regardless
        if (onlyshowin_filter && 
            !is_app_in_ignore_list (onlyshowin_ignore, desktop_file)) {
          onlyshowin = g_key_file_get_string_list (key_file,
                                                   DESKTOP_GROUP,
                                                   "OnlyShowIn",
                                                   NULL,
                                                   NULL);
          if (onlyshowin) {
              for (gint j = 0; onlyshowin[j]; j++) {
                  if (!g_ascii_strcasecmp (onlyshowin[j], "MOBILE")) {
                      show_app = TRUE;
                      break;
                  }
              }
              g_strfreev (onlyshowin);
          }
        } else {
            show_app = TRUE;
        }
        if (!show_app) {
            g_key_file_free (key_file);
            g_free (desktop_file);
            break;
        }
        
        // Add the application
        GtkIconTheme *theme = NULL;
        GtkIconInfo *info = NULL;
        application_entry_t *i = NULL;
        GConfClient *client = gconf_client_get_default ();
        GSList *dir_list = NULL;

        i = (application_entry_t *) g_malloc0 (sizeof(application_entry_t));

        i->exec = g_strdup (gmenu_tree_entry_get_exec (entry));
        if (!i->exec) {
            g_free (i);
            continue;
        }

        theme = gtk_icon_theme_get_default();
        gtk_icon_theme_prepend_search_path(theme, PIXMAP_DIR);

        i->index = index;
        i->name = g_strdup (gmenu_tree_entry_get_name (entry));
        i->cat = g_strdup (gmenu_tree_directory_get_name (directory));
        i->filename = g_strdup (gmenu_tree_entry_get_desktop_file_id (entry));
        i->service = g_key_file_get_string (key_file,
                                            DESKTOP_GROUP,
                                            DESKTOP_ENTRY_SERVICE_FIELD,
                                            NULL);

        gchar *t = g_strdup (gmenu_tree_entry_get_icon (entry));
        //if there is an extension, strip it.  simple logic will catch 90% (.jpg, .png)
        int pos = strlen(t)-4;  //extension length = 4
        if (pos>0 && t[pos]=='.') {
            t[pos] = '\0';
        }            
        info = gtk_icon_theme_lookup_icon (theme, t, 48, GTK_ICON_LOOKUP_NO_SVG);
        if (!info) {
            //let's try the unknown icon
            info = gtk_icon_theme_lookup_icon (theme, UNKNOWN_ICON_NAME, 48, GTK_ICON_LOOKUP_NO_SVG);
        }
        if (info) {
            i->icon = g_strdup (gtk_icon_info_get_filename (info));
        } else {
            i->icon = g_strdup ("null");
        }
        g_free (t);

        if (client) {
            GSList *node;
            dir_list = gconf_client_get_list (client,
                                              CATEGORIES_KEY,
                                              GCONF_VALUE_STRING,
                                              NULL);
            for (node = dir_list; node; node = node->next) {
                if (!g_ascii_strcasecmp ((gchar *)node->data, i->cat))
                    break;
            }

            if (!node) {
                dir_list = g_slist_append (dir_list, i->cat);
                gconf_client_set_list (client,
                                       CATEGORIES_KEY,
                                       GCONF_VALUE_STRING,
                                       dir_list, NULL);
            }
        }

        list = g_list_append (list, i);
        index ++;

        g_key_file_free (key_file);
        g_free (desktop_file);
        break;
      }
      default:
        break;
    }
    gmenu_tree_item_unref (item);
  }
  g_slist_free (entry_list);

  return list;
}

/////////////////////////////////////////////////////////
// Read the system desktop entries and build a list of
// applications that the Flash content should expose in
// its interface.

static GList *__parse_desktop_files(GList *list, const char *name)
{
  GMenuTreeDirectory *root = NULL;
  GMenuTree *tree = NULL;

  tree = gmenu_tree_lookup (name, GMENU_TREE_FLAGS_NONE);

  if (NULL == tree)
  {
    g_debug ("No application is available at the momemnt\n");
    return list;
  }

  root = gmenu_tree_get_root_directory (tree);

  list = application_list_buildup (list, root);
  gmenu_tree_item_unref (root);

  return list;
}

//TBD:  Get list from /usr/share/applications/<pkg>  MobileId
static void build_app_list(plugin_context_t *c)
{
	c->app_list = __parse_desktop_files(c->app_list, "applications.menu");
	c->app_list = __parse_desktop_files(c->app_list, "settings.menu");
	
	c->app_list = g_list_sort(c->app_list, compare_items);
	for (int i=0; i<g_list_length(c->app_list); i++) {
		GList *l = g_list_nth(c->app_list, i);
		application_entry_t *item = (application_entry_t *)l->data;
		item->index = i;
	}
	
	return;
}


//gconf notification callback (registered in init below)
static void background_changed (GConfClient *client, guint id,
				GConfEntry *entry,
				plugin_context_t *c) {
	//ignoring input: update all background properites
	update_background(c);
}

static void update_categories(plugin_context_t *context)
{
	gtk_moz_embed_load_url(GTK_MOZ_EMBED(context->container), 
			       "javascript:clearApps()");
	net_stop_cb(NULL, context);
}

//gconf notification callback (for theme and font)
static void gconf_interface_changed (GConfClient *client, guint id,
				GConfEntry *entry,
				plugin_context_t *c) 
{
	update_theme(c);
}

static void active_cat_changed (GConfClient *client, guint id,
				GConfEntry *entry,
				plugin_context_t *c) 
{
	const char *gkey = gconf_entry_get_key(entry);
	GConfValue *gvalue = gconf_entry_get_value(entry);

	if (!c || !gkey || !gvalue)
		return;
	
	if (g_str_equal(gkey, ACTIVE_CAT_KEY)) {
		c->active = gconf_value_get_string(gvalue);
		update_categories(c);
	}
}

static void mobile_basic_home_plugin_init (MobileBasicHomePlugin *home_plugin)
{
	plugin_context_t *context = (plugin_context_t *)g_malloc0(
		sizeof(plugin_context_t));

	home_plugin->context = context;
	context->container = gtk_moz_embed_new();
	
	//setup context object
	context->client = gconf_client_get_default ();
	context->osso_context = osso_initialize("HomePlugin", "1.0", FALSE, NULL);

	gboolean hide_marquee = gconf_client_get_bool (context->client,
						       MARQUEE_KEY,
						       NULL);

	gchar *homepage_name = gconf_client_get_string (context->client,
							HOMEPAGE_KEY,
							NULL);

	gchar *custom_movie_name = gconf_client_get_string (context->client,
							    FLASHMOVIE_KEY,
							    NULL);

	context->active = gconf_client_get_string(context->client,
						  ACTIVE_CAT_KEY,
						  NULL);

        //initialize category list key
        gconf_client_set_list (context->client,
                               CATEGORIES_KEY,
                               GCONF_VALUE_STRING,
                               NULL, NULL);

        //use libwnck to dismiss banner when window or dialog appear
        init_window_helper (context);

	//create application list
	context->app_list = NULL;
	build_app_list(context);

	gtk_moz_embed_set_profile_path(PROFILE_DIR, PROFILE_NAME);
  
	//auto-size plugin to fill screen
	int scn_width = 800;
	int scn_height = 480;
	GdkScreen *screen = gtk_widget_get_screen ((GtkWidget*)home_plugin);
	if (screen != NULL) {
		scn_width = gdk_screen_get_width(screen);
		scn_height = gdk_screen_get_height(screen);
	}

	if (hide_marquee) {
		gtk_widget_set_size_request ((GtkWidget*)home_plugin, scn_width, scn_height);
	} else {
		gtk_widget_set_size_request ((GtkWidget*)home_plugin, scn_width, scn_height-MARQUEE_HEIGHT);
	}


	// default to home.html if no homepage specified in gconf
	gchar *url = NULL;
	if (homepage_name == NULL || !g_ascii_strcasecmp(homepage_name, HOME_HTML)) {
		url = g_strdup_printf("%s%s/%s", FILE_URL_PREFIX, PROFILE_DIR, HOME_HTML);
	} else {
		// make query string
		// format query string to pass to html file url
		gchar *marquee_qs = g_strdup_printf("hideMarquee=%s", (hide_marquee ? "true" : "false"));

		gchar *flash_movie_qs = NULL;
		if (custom_movie_name) {
			flash_movie_qs = g_strdup_printf("movieName=%s", custom_movie_name);
		}
		
		gchar *queryString = NULL;
		if (flash_movie_qs) {
			queryString = g_strconcat("?",
						  marquee_qs,
						  "&",
						  flash_movie_qs,
						  NULL);
		} else {
			queryString = g_strconcat("?",
						  marquee_qs,
						  NULL);
		}

		url = g_strdup_printf("%s%s/%s%s", FILE_URL_PREFIX,
				      PROFILE_DIR, 
				      homepage_name,
				      queryString);

		g_print("loading custom home plugin url=%s\n", url);
		g_free(marquee_qs);
		if (flash_movie_qs) {
			g_free(flash_movie_qs);
		}
		g_free(queryString);
	}
 
 	gtk_moz_embed_load_url(GTK_MOZ_EMBED(context->container), url);
	if (url) {
		g_free(url);
	}

	gtk_widget_show_all(context->container);
	gtk_container_add (GTK_CONTAINER (home_plugin), context->container);
	gtk_signal_connect(GTK_OBJECT(context->container), "js_status",
			   GTK_SIGNAL_FUNC(js_status_cb), (gpointer)context);
	gtk_signal_connect(GTK_OBJECT(context->container), "net_stop",
			   GTK_SIGNAL_FUNC(net_stop_cb), (gpointer)context);


	gconf_client_add_dir (context->client, BKGD_PATH,
			      GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);
	gconf_client_notify_add (context->client,
				 BKGD_PATH,
				 (GConfClientNotifyFunc) background_changed,
				 context, NULL, NULL);

	//notification when theme changes (send up)
	gconf_client_add_dir (context->client, INTERFACE_PATH,
			      GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);
	gconf_client_notify_add (context->client,
				 INTERFACE_PATH,
				 (GConfClientNotifyFunc) gconf_interface_changed,
				 context, NULL, NULL);

	gconf_client_add_dir (context->client, MARQUEE_GCONF_PATH,
			      GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);
	gconf_client_notify_add (context->client,
				 MARQUEE_GCONF_PATH,
				 (GConfClientNotifyFunc) active_cat_changed,
				 context, NULL, NULL);
}

static GdkFilterReturn mobile_basic_home_x_event_handler (GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
  HildonDesktopHomeItem *item = (HildonDesktopHomeItem *)data;
  MobileBasicHomePlugin *home_plugin = (MobileBasicHomePlugin *)item;
  static gint counter = 0;

  g_print ("xevent type %d\n", ((XEvent*)xevent)->type);
  switch (((XEvent*)xevent)->type)
  {
    case DialogDestroy:
      update_categories (home_plugin->context);
      if (counter++ > 2)
      {
        counter = 0;
        gdk_window_remove_filter (GTK_WIDGET (home_plugin)->window, mobile_basic_home_x_event_handler, home_plugin);
      }
      break;
    default:
      break;
  }
  return GDK_FILTER_CONTINUE;
}

static void home_screen_changed (HildonDesktopHomeItem *item)
{
  MobileBasicHomePlugin *home_plugin = (MobileBasicHomePlugin *)item;
  gint width = 800, height = 480;
  gint prev_width, prev_height;
  GdkScreen *screen;
  GdkWindow *win_wrapper = GTK_WIDGET (home_plugin)->window;

  screen = gtk_widget_get_screen (GTK_WIDGET(home_plugin));
  if (screen != NULL) {
    width = gdk_screen_get_width(screen);
    height = gdk_screen_get_height(screen);
  }

  gtk_widget_get_size_request (GTK_WIDGET(home_plugin), &prev_width, &prev_height);
  if (!(width == prev_width && height == prev_height))
  {
    gtk_widget_set_size_request (GTK_WIDGET(home_plugin), width, height);
    gtk_widget_set_size_request (GTK_WIDGET(home_plugin->context->container), width, height);
    gdk_error_trap_push ();

    gdk_window_set_events (win_wrapper, GDK_ALL_EVENTS_MASK);
    gdk_window_add_filter (win_wrapper, mobile_basic_home_x_event_handler, home_plugin);
    gdk_error_trap_pop();
  }
}

static void
mobile_basic_home_plugin_class_init (MobileBasicHomePluginClass *klass)
{
  HildonDesktopHomeItemClass *item_class = HILDON_DESKTOP_HOME_ITEM_CLASS (klass);

  item_class->screen_changed = home_screen_changed;
}
