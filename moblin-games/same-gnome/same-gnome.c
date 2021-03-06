/* -*- mode: C; indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; -*- */

/* same-gnome.c : Same Game
 *
 * Copyright (c) 2004 by Callum McKenzie
 *
 */

#include <config.h>

#include <stdlib.h>

#include <glib/gi18n.h>

#include <gtk/gtk.h>

#ifdef HAVE_GNOME
#include <gnome.h>
#endif /* HAVE_GNOME */

#include <libgames-support/games-conf.h>
#include <libgames-support/games-gridframe.h>
#include <libgames-support/games-stock.h>
#include <libgames-support/games-scores.h>

#include "same-gnome.h"

#include "drawing.h"
#include "game.h"
#include "input.h"
#include "ui.h"

#define DEFAULT_CUSTOM_WIDTH 15
#define DEFAULT_CUSTOM_HEIGHT 10
#define MINIMUM_CUSTOM_WIDTH 4
#define MINIMUM_CUSTOM_HEIGHT 4
#define MAXIMUM_CUSTOM_HEIGHT 100
#define MAXIMUM_CUSTOM_WIDTH 100

gchar *localthemedir;

gchar *theme;
gint game_size = UNSET;

/* Keep this in sync with the enum above. */
gint board_sizes[MAX_SIZE][3] = { {-1, -1, -1}
,				/* This is a dummy entry. */
{-1, -1, -1}
,				/* Space for the custom size. */
{15, 10, 3}
,
{30, 20, 4}
,
{45, 30, 4}
};

const GamesScoresCategory scorecats[] = { {"Small", N_("Small")},
{"Medium", N_("same-gnome|Medium")},
{"Large", N_("Large")},
GAMES_SCORES_LAST_CATEGORY
};

static const GamesScoresDescription scoredesc = { scorecats,
  "Small",
  "same-gnome",
  GAMES_SCORES_STYLE_PLAIN_DESCENDING
};

GamesScores *highscores;
gint board_width;
gint board_height;
gint board_ncells;
gint ncolours;

gint window_width;
gint window_height;

static void
initialise_options (gint requested_size, gchar * requested_theme)
{
  gint intvalue;

  game_size = SMALL;
  theme = DEFAULT_THEME;

  if (requested_size != -1)
    game_size = requested_size;
  if (requested_theme != NULL)
    theme = requested_theme;

  localthemedir = g_build_filename (g_get_user_data_dir (),
				    "/gnome-games/same-gnome/themes/",
				    THEME_VERSION, NULL);

  intvalue = games_conf_get_integer (NULL, KEY_CUSTOM_WIDTH, NULL);
  if (intvalue == 0)
    intvalue = DEFAULT_CUSTOM_WIDTH;
  board_sizes[CUSTOM][0] = CLAMP (intvalue, MINIMUM_CUSTOM_WIDTH,
				  MAXIMUM_CUSTOM_WIDTH);
  intvalue = games_conf_get_integer (NULL, KEY_CUSTOM_HEIGHT, NULL);
  if (intvalue == 0)
    intvalue = DEFAULT_CUSTOM_HEIGHT;
  board_sizes[CUSTOM][1] = CLAMP (intvalue, MINIMUM_CUSTOM_HEIGHT,
				  MAXIMUM_CUSTOM_HEIGHT);

  if (requested_size != -1) {
    game_size = requested_size - 1 + SMALL;
    clear_savegame ();
  } else {
    game_size = games_conf_get_integer (NULL, KEY_SIZE, NULL);
    if (game_size == 0)
      game_size = DEFAULT_GAME_SIZE;
  }

  /* FIXME: This doesn't work for a custom size. */
  game_size = CLAMP (game_size, SMALL, MAX_SIZE - 1);
  set_sizes (game_size);

  if (requested_theme != NULL)
    theme = requested_theme;
  else
    theme = games_conf_get_string_with_default (NULL, KEY_THEME, DEFAULT_THEME);

  /* An invalid theme will be picked up at load time. Although we can
   * guarantee that theme != NULL. */
}

int
main (int argc, char *argv[])
{
  GOptionContext *context;
  gchar *requested_theme = NULL;
  gint requested_size = -1;

  const GOptionEntry options[] = {
    {"theme", 't', 0, G_OPTION_ARG_STRING, &requested_theme,
     N_("Set the theme"), N_("NAME")},
    {"scenario", 's', 0, G_OPTION_ARG_STRING, &requested_theme,
     N_("For backwards compatibility"), N_("NAME")},
    {"size", 'z', 0, G_OPTION_ARG_INT, &requested_size,
     N_("Game size (1=small, 3=large)"), N_("NUMBER")},
    {NULL, '\0', 0, 0, NULL, NULL, NULL}
  };

#ifdef HAVE_GNOME
  GnomeClient *client;
  GnomeProgram *program;
#else
  gboolean retval;
  GError *error = NULL;
#endif

#if defined(HAVE_GNOME) || defined(HAVE_RSVG_GNOMEVFS)
  /* If we're going to use gnome-vfs, we need to init threads before
   * calling any glib functions.
   */
  g_thread_init (NULL);
#endif

  setgid_io_init ();

  /* Initialise i18n. */
  bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
  textdomain (GETTEXT_PACKAGE);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

  context = g_option_context_new (NULL);
#if GLIB_CHECK_VERSION (2, 12, 0)
  g_option_context_set_translation_domain (context, GETTEXT_PACKAGE);
#endif

  g_option_context_add_main_entries (context, options, GETTEXT_PACKAGE);

#ifdef HAVE_GNOME
  program =
    gnome_program_init (APPNAME, VERSION, LIBGNOMEUI_MODULE, argc, argv,
			GNOME_PARAM_APP_DATADIR, DATADIR,
			GNOME_PARAM_GOPTION_CONTEXT, context, NULL);
#else
  g_option_context_add_group (context, gtk_get_option_group (TRUE));

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_option_context_free (context);
  if (!retval) {
    g_print ("%s", error->message);
    g_error_free (error);
    exit (1);
  }
#endif /* HAVE_GNOME */

  g_set_application_name (_(APPNAME_LONG));

  games_conf_initialise (APPNAME);

  games_stock_init ();

  highscores = games_scores_new (&scoredesc);

  initialise_options (requested_size, requested_theme);

  build_gui ();

  /* FIXME: This should be one alternative of an if statement, the other
   * alternative should be to load an old game. */
  if (!load_game ())
    new_game ();

  gtk_main ();

  games_conf_shutdown ();

#ifdef HAVE_GNOME
  g_object_unref (program);
#endif /* HAVE_GNOME */

  return 0;
}
