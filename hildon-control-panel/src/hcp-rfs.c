/*
 * This file is part of hildon-control-panel
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <crypt.h>
#include <strings.h>

#include <hildon/hildon-code-dialog.h>
#include <hildon/hildon-banner.h>
#include <hildon/hildon-help.h>
#include <libosso.h>

#include <glib/gi18n.h>

#include "hcp-rfs.h"
#include "hcp-program.h"

#define HCP_CODE_DIALOG_HELP_TOPIC "Features_restorefactorysettings_passwordquerydialog"

#define HCP_RFS_INFOBANNER_OK      _("rfs_bd_ok")
#define HCP_RFS_INFOBANNER_CANCEL  _("rfs_bd_cancel")
#define HCP_RFS_IB_WRONG_LOCKCODE  dgettext("hildon-libs", "secu_info_incorrectcode")

#define HCP_MCE_DBUS_SERVICE       "com.nokia.mce"
#define HCP_MCE_DBUS_REQUEST_IF    "com.nokia.mce.request"
#define HCP_MCE_DBUS_REQUEST_PATH  "/com/nokia/mce/request"
#define HCP_MCE_PASSWORD_VALIDATE  "validate_devicelock_code"

#define HCP_DEFAULT_SALT "$1$JE5Gswee$"

#define HCP_RFC_WARNING_DIALOG_WIDTH 450

/*
 * Asks the user for confirmation, returns TRUE if confirmed
 */
static gboolean hcp_rfs_display_warning (HCPProgram  *program,
                                         const gchar *warning,
                                         const gchar *title,
                                         const gchar *help_topic)
{
  GtkWidget *confirm_dialog;
  GtkWidget *label;
  gint ret;

  confirm_dialog = gtk_dialog_new_with_buttons (
      title,
      GTK_WINDOW (program->window),
      GTK_DIALOG_MODAL, 
      HCP_RFS_INFOBANNER_OK, GTK_RESPONSE_OK,
      HCP_RFS_INFOBANNER_CANCEL, GTK_RESPONSE_CANCEL,
      NULL
      );
  
  gtk_window_set_position (GTK_WINDOW (confirm_dialog),
                           GTK_WIN_POS_NONE);

  hildon_help_dialog_help_enable (GTK_DIALOG (confirm_dialog),
                                  help_topic,
                                  program->osso);

  gtk_dialog_set_has_separator (GTK_DIALOG (confirm_dialog), FALSE);

  label = gtk_label_new (warning);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (confirm_dialog)->vbox), 
                     label);

  gtk_widget_set_size_request (confirm_dialog,
                               HCP_RFC_WARNING_DIALOG_WIDTH,
                               -1);

  gtk_widget_show_all  (confirm_dialog);
  ret = gtk_dialog_run (GTK_DIALOG (confirm_dialog));

  gtk_widget_destroy (GTK_WIDGET (confirm_dialog));

  if (ret == GTK_RESPONSE_CANCEL || 
          ret == GTK_RESPONSE_DELETE_EVENT) {
      return FALSE;
  }

  return TRUE;
}

/*
 * Sends dbus message to MCE to check the lock code
 * Returns 0 if not correct, 1 if correct, -1 if an error occured
 */
static gint 
hcp_rfs_check_lock_code (HCPProgram *program, const gchar *code)
{
  gchar *crypted_code;
  osso_return_t ret;
  osso_rpc_t returnvalue;
  gint result;

  crypted_code = crypt  (code, HCP_DEFAULT_SALT);
  crypted_code = rindex (crypted_code, '$') + 1;

  ret = osso_rpc_run_system (program->osso, 
                      HCP_MCE_DBUS_SERVICE,
                      HCP_MCE_DBUS_REQUEST_PATH,
                      HCP_MCE_DBUS_REQUEST_IF,
                      HCP_MCE_PASSWORD_VALIDATE,
                      &returnvalue,
                      DBUS_TYPE_STRING,
                      crypted_code,
                      DBUS_TYPE_STRING,
                      HCP_DEFAULT_SALT,
                      DBUS_TYPE_INVALID);

  switch (ret)
  {
    case OSSO_INVALID:
      g_warning ("Lockcode query call failed: Invalid parameter");
      osso_rpc_free_val (&returnvalue);
      return -1;

    case OSSO_RPC_ERROR:
    case OSSO_ERROR:
    case OSSO_ERROR_NAME:
    case OSSO_ERROR_NO_STATE:
    case OSSO_ERROR_STATE_SIZE:
      if (returnvalue.type == DBUS_TYPE_STRING)
      {
        g_warning ("Lockcode query call failed: %s", returnvalue.value.s);
      }
      else
      {
        g_warning ("Lockcode query call failed: unspecified");
      }
      osso_rpc_free_val (&returnvalue);
      return -1;

    case OSSO_OK:
        break;

    default:
      g_warning ("Lockcode query call failed: unknown"
  	         " error type %d", ret);
      osso_rpc_free_val (&returnvalue);
      return -1;
  }

  if (returnvalue.type != DBUS_TYPE_BOOLEAN)
  {
    g_warning ("Lockcode query call failed: unexpected return "
               "value type %d", returnvalue.type);

    osso_rpc_free_val (&returnvalue);
    return -1;
  }

  result = (gint)returnvalue.value.b;
  osso_rpc_free_val (&returnvalue);
  
  return result;
}

/*
 * Prompts the user for the lock password.
 * Returns TRUE if correct password, FALSE if cancelled
 */
static gboolean 
hcp_rfs_check_lock_code_dialog (HCPProgram *program)
{
  GtkWidget *dialog;
  gint ret;
  gint password_correct = FALSE;

  dialog = hildon_code_dialog_new ();
  
  hildon_help_dialog_help_enable (GTK_DIALOG (dialog),
                                  HCP_CODE_DIALOG_HELP_TOPIC,
                                  program->osso);

  gtk_widget_show_all (dialog);

  gtk_window_set_transient_for (GTK_WINDOW (dialog),
                                GTK_WINDOW (program->window));

  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_NONE);

  while (!password_correct)
  {
    gtk_widget_set_sensitive (dialog, TRUE);
    
    ret = gtk_dialog_run (GTK_DIALOG (dialog));

    gtk_widget_set_sensitive (dialog, FALSE);

    if (ret == GTK_RESPONSE_CANCEL ||
      ret == GTK_RESPONSE_DELETE_EVENT) {
      gtk_widget_destroy (dialog);
      
      return FALSE;
    }

    password_correct = hcp_rfs_check_lock_code ( 
            program,
            hildon_code_dialog_get_code (HILDON_CODE_DIALOG (dialog)));

    if (!password_correct)
    {
      hildon_banner_show_information (dialog,
                                      NULL,
                                      HCP_RFS_IB_WRONG_LOCKCODE);

      hildon_code_dialog_clear_code (HILDON_CODE_DIALOG (dialog));
    }
  }

  gtk_widget_destroy (dialog);

  if (password_correct == -1)
  {
    /* An error occured in the lock code verification query */
    return FALSE;
  }

  return TRUE;
}

static void 
hcp_rfs_launch_script (const gchar *script)
{
  GError *error = NULL;

  if (!g_spawn_command_line_async (script, &error))
  {
    g_warning ("Call to RFS or CUD script failed");

    if (error)
    {
      g_warning (error->message);
      g_error_free (error);
    }
  }
}


gboolean 
hcp_rfs (const gchar *warning, const gchar *title,
         const gchar *script, const gchar *help_topic,
	 gboolean check_lock_code)
{
  if (warning)
  {
    if (!hcp_rfs_display_warning (hcp_program_get_instance (), 
                                  warning, title, help_topic))
    {
      /* User canceled, return */
      return TRUE;
    }
  }
          
  if (!check_lock_code ||
      hcp_rfs_check_lock_code_dialog (hcp_program_get_instance ()))
  {
    /* Password is correct, proceed */
    hcp_rfs_launch_script (script);
    return FALSE;
  }
  else
  {
    /* User cancelled or an error occured. Exit */
    return TRUE;
  }
}
