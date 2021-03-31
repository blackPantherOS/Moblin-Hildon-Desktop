/* NetworkManagerDispatcher -- Dispatches messages from NetworkManager
 *
 * Dan Williams <dcbw@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * (C) Copyright 2004 Red Hat, Inc.
 */

#include <glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus-glib.h>
#include <getopt.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

#include "NetworkManager.h"
#include "nm-utils.h"


enum NMDAction
{
	NMD_DEVICE_DONT_KNOW,
	NMD_DEVICE_NOW_INACTIVE,
	NMD_DEVICE_NOW_ACTIVE,
	NMD_DEVICE_GOING_INACTIVE,
	NMD_DEVICE_GOING_ACTIVE,
};
typedef enum NMDAction	NMDAction;


#define NM_SCRIPT_DIR		SYSCONFDIR"/NetworkManager/dispatcher.d"

#define NMD_DEFAULT_PID_FILE	LOCALSTATEDIR"/run/NetworkManagerDispatcher.pid"

static DBusConnection *nmd_dbus_init (void);

/*
 * nmd_permission_check
 *
 * Verify that the given script has the permissions we want.  Specifically,
 * ensure that the file is
 *	- A regular file.
 *	- Owned by root.
 *	- Not writable by the group or by other.
 *	- Not setuid.
 *	- Executable by the owner.
 *
 */
static inline gboolean nmd_permission_check (struct stat *s)
{
	if (!S_ISREG (s->st_mode))
		return FALSE;
	if (s->st_uid != 0)
		return FALSE;
	if (s->st_mode & (S_IWGRP|S_IWOTH|S_ISUID))
		return FALSE;
	if (!(s->st_mode & S_IXUSR))
		return FALSE;
	return TRUE;
}


/*
 * nmd_execute_scripts
 *
 * Call scripts in /etc/NetworkManager.d when devices go down or up
 *
 */
static void nmd_execute_scripts (NMDAction action, char *iface_name)
{
	GDir *		dir;
	const char *	file_name;
	const char *	char_act;

	if (action == NMD_DEVICE_NOW_ACTIVE)
		char_act = "up";
	else if (action == NMD_DEVICE_NOW_INACTIVE)
		char_act = "post-down";
	else if (action == NMD_DEVICE_GOING_ACTIVE)
		char_act = "pre-up";
	else if (action == NMD_DEVICE_GOING_INACTIVE)
		char_act = "down";
	else
		return;

	if (!(dir = g_dir_open (NM_SCRIPT_DIR, 0, NULL)))
	{
		nm_warning ("nmd_execute_scripts(): opendir() could not open '" NM_SCRIPT_DIR "'.  errno = %d", errno);
		return;
	}

	while ((file_name = g_dir_read_name (dir)))
	{
		char *		file_path = g_strdup_printf (NM_SCRIPT_DIR"/%s", file_name);
		struct stat	s;

		if ((file_name[0] != '.') && (stat (file_path, &s) == 0))
		{
			if (nmd_permission_check (&s))
			{
				char *cmd;
				int ret;

				cmd = g_strdup_printf ("%s %s %s", file_path, iface_name, char_act);
				ret = system (cmd);
				if (ret == -1)
					nm_warning ("nmd_execute_scripts(): system() failed with errno = %d", errno);
				g_free (cmd);
			}
		}

		g_free (file_path);
	}

	g_dir_close (dir);
}


/*
 * nmd_get_device_name
 *
 * Queries NetworkManager for the name of a device, specified by a device path
 */
static char * nmd_get_device_name (DBusConnection *connection, char *path)
{
	DBusMessage *	message;
	DBusMessage *	reply;
	DBusError		error;
	char *		dbus_dev_name = NULL;
	char *		dev_name = NULL;

	if (!(message = dbus_message_new_method_call (NM_DBUS_SERVICE, path, NM_DBUS_INTERFACE, "getName")))
	{
		nm_warning ("Couldn't allocate the dbus message");
		return NULL;
	}

	dbus_error_init (&error);
	reply = dbus_connection_send_with_reply_and_block (connection, message, -1, &error);
	dbus_message_unref (message);
	if (dbus_error_is_set (&error))
	{
		nm_warning ("%s raised: %s", error.name, error.message);
		dbus_error_free (&error);
		return NULL;
	}

	/* now analyze reply */
	if (!dbus_message_get_args (reply, NULL, DBUS_TYPE_STRING, &dbus_dev_name, DBUS_TYPE_INVALID))
	{
		nm_warning ("There was an error getting the device name from NetworkManager." );
		dev_name = NULL;
	}
	else
		dev_name = g_strdup (dbus_dev_name);
	
	dbus_message_unref (reply);

	return dev_name;
}

/*
 * nmd reinit_dbus
 *
 * Reconnect to the system message bus if the connection was dropped.
 *
 */
static gboolean nmd_reinit_dbus (gpointer user_data)
{
	if (nmd_dbus_init ())
	{
		nm_info ("Successfully reconnected to the system bus.");
		return FALSE;
	}
	else
		return TRUE;
}

/*
 * nmd_dbus_filter
 *
 * Handles dbus messages from NetworkManager, dispatches device active/not-active messages
 */
static DBusHandlerResult nmd_dbus_filter (DBusConnection *connection, DBusMessage *message, void *user_data)
{
	const char	*object_path;
	DBusError		 error;
	char			*dev_object_path = NULL;
	gboolean		 handled = FALSE;
	NMDAction		 action = NMD_DEVICE_DONT_KNOW;

	dbus_error_init (&error);
	object_path = dbus_message_get_path (message);

	if (dbus_message_is_signal (message, DBUS_INTERFACE_LOCAL, "Disconnected"))
	{
		dbus_connection_unref (connection);
		connection = NULL;
		g_timeout_add (3000, nmd_reinit_dbus, NULL);
		handled = TRUE;
	}

	if (dbus_message_is_signal (message, NM_DBUS_INTERFACE, "DeviceNoLongerActive"))
		action = NMD_DEVICE_GOING_INACTIVE;
	else if (dbus_message_is_signal (message, NM_DBUS_INTERFACE, "DeviceNowActive"))
		action = NMD_DEVICE_NOW_ACTIVE;
	else if (dbus_message_is_signal (message, NM_DBUS_INTERFACE, "DeviceActivating"))
		action = NMD_DEVICE_GOING_ACTIVE;
	else if (dbus_message_is_signal (message, NM_DBUS_INTERFACE, "DeviceDeactivated"))
		action = NMD_DEVICE_NOW_INACTIVE;

	if (action != NMD_DEVICE_DONT_KNOW)
	{
		if (dbus_message_get_args (message, &error, DBUS_TYPE_OBJECT_PATH, &dev_object_path, DBUS_TYPE_INVALID))
		{
			char *	dev_iface_name = NULL;

			dev_object_path = nm_dbus_unescape_object_path (dev_object_path);
			if (dev_object_path)
				dev_iface_name = nmd_get_device_name (connection, dev_object_path);

			if (dev_object_path && dev_iface_name)
			{
				nm_info ("Device %s (%s) is now %s.", dev_object_path, dev_iface_name,
						(action == NMD_DEVICE_NOW_INACTIVE ? "going down" :
						(action == NMD_DEVICE_NOW_ACTIVE ? "up" :
						(action == NMD_DEVICE_GOING_INACTIVE ? "going down" :
						(action == NMD_DEVICE_GOING_ACTIVE ? "going up" : "error")))));

				nmd_execute_scripts (action, dev_iface_name);
			}

			g_free (dev_object_path);
			g_free (dev_iface_name);

			handled = TRUE;
		}
	}

	return (handled ? DBUS_HANDLER_RESULT_HANDLED : DBUS_HANDLER_RESULT_NOT_YET_HANDLED);
}

/*
 * nmd_dbus_init
 *
 * Initialize a connection to NetworkManager
 */
static DBusConnection *nmd_dbus_init (void)
{
	DBusConnection *connection = NULL;
	DBusError		 error;

	/* connect to NetworkManager service on the system bus */
	dbus_error_init (&error);
	connection = dbus_bus_get (DBUS_BUS_SYSTEM, &error);
	if (connection == NULL)
	{
		nm_warning ("nmd_dbus_init(): could not connect to the message bus.  dbus says: '%s'", error.message);
		dbus_error_free (&error);
		return (NULL);
	}

	dbus_connection_set_exit_on_disconnect (connection, FALSE);
	dbus_connection_setup_with_g_main (connection, NULL);

	if (!dbus_connection_add_filter (connection, nmd_dbus_filter, NULL, NULL))
		return (NULL);

	dbus_bus_add_match (connection,
				"type='signal',"
				"interface='" NM_DBUS_INTERFACE "',"
				"sender='" NM_DBUS_SERVICE "',"
				"path='" NM_DBUS_PATH "'", &error);
	if (dbus_error_is_set (&error))
		return (NULL);

	return (connection);
}

/*
 * nmd_print_usage
 *
 * Prints program usage.
 *
 */
static void nmd_print_usage (void)
{
	fprintf (stderr, "\n" "usage : NetworkManagerDispatcher [--no-daemon] [--pid-file=<file>] [--help]\n");
	fprintf (stderr,
		"\n"
		"        --no-daemon        Do not daemonize\n"
		"        --pid-file=<path>  Specify the location of a PID file\n"
		"        --help             Show this information and exit\n"
		"\n"
		"NetworkManagerDispatcher listens for device messages from NetworkManager\n"
		"and runs scripts in " NM_SCRIPT_DIR "\n"
		"\n");
}


static void
write_pidfile (const char *pidfile)
{
 	char pid[16];
	int fd;
 
	if ((fd = open (pidfile, O_CREAT|O_WRONLY|O_TRUNC, 00644)) < 0)
	{
		nm_warning ("Opening %s failed: %s", pidfile, strerror (errno));
		return;
	}
 	snprintf (pid, sizeof (pid), "%d", getpid ());
	if (write (fd, pid, strlen (pid)) < 0)
		nm_warning ("Writing to %s failed: %s", pidfile, strerror (errno));
	if (close (fd))
		nm_warning ("Closing %s failed: %s", pidfile, strerror (errno));
}


/*
 * main
 *
 */
int main (int argc, char *argv[])
{
	gboolean		become_daemon = TRUE;
	GMainLoop *	loop  = NULL;
	DBusConnection	*connection = NULL;
	char *		pidfile = NULL;
	char *		user_pidfile = NULL;

	/* Parse options */
	while (1)
	{
		int c;
		int option_index = 0;
		const char *opt;

		static struct option options[] = {
			{"no-daemon",	0, NULL, 0},
			{"pid-file",	1, NULL, 0},
			{"help",		0, NULL, 0},
			{NULL,		0, NULL, 0}
		};

		c = getopt_long (argc, argv, "", options, &option_index);
		if (c == -1)
			break;

		switch (c)
		{
			case 0:
				opt = options[option_index].name;
				if (strcmp (opt, "help") == 0)
				{
					nmd_print_usage ();
					return 0;
				}
				else if (strcmp (opt, "no-daemon") == 0)
					become_daemon = FALSE;
				else if (strcmp (opt, "pid-file") == 0)
					user_pidfile = g_strdup (optarg);
				else
				{
					nmd_print_usage ();
					return 1;
				}
				break;

			default:
				nmd_print_usage ();
				return 1;
				break;
		}
	}

	openlog("NetworkManagerDispatcher", (become_daemon) ? LOG_CONS : LOG_CONS | LOG_PERROR, (become_daemon) ? LOG_DAEMON : LOG_USER);

	if (become_daemon)
	{
		if (daemon (FALSE, FALSE) < 0)
		{
	     	nm_warning ("NetworkManagerDispatcher could not daemonize: %s", strerror (errno));
		     exit (1);
		}

		pidfile = user_pidfile ? user_pidfile : NMD_DEFAULT_PID_FILE;
		write_pidfile (pidfile);
	}

	g_type_init ();
	if (!g_thread_supported ())
		g_thread_init (NULL);

	/* Connect to the NetworkManager dbus service and run the main loop */
	if ((connection = nmd_dbus_init ()))
	{
		loop = g_main_loop_new (NULL, FALSE);
		g_main_loop_run (loop);
	}

	/* Clean up pidfile */
	if (pidfile)
		unlink (pidfile);
	g_free (user_pidfile);

	return 0;
}
