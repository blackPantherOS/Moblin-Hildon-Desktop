/*
 * Copyright (C) 2007 Neil J. Patel
 * Copyright (C) 2007 OpenedHand Ltd
 *
 * Author: Neil J. Patel  <njp@o-`hand.com>
 */

#include <config.h>
#include <glib.h>
#include <clutter/clutter.h>
#include <math.h>

#ifndef _HAVE_LAUNCHER_SPINNER_H
#define _HAVE_LAUNCHER_SPINNER_H


G_BEGIN_DECLS

#define LAUNCHER_TYPE_SPINNER launcher_spinner_get_type()

#define LAUNCHER_SPINNER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
        LAUNCHER_TYPE_SPINNER, \
	      LauncherSpinner))

#define LAUNCHER_SPINNER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass),\
	      LAUNCHER_TYPE_SPINNER, \
	      LauncherSpinnerClass))

#define LAUNCHER_IS_SPINNER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
	      LAUNCHER_TYPE_SPINNER))

#define LAUNCHER_IS_SPINNER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
        LAUNCHER_TYPE_SPINNER))

#define LAUNCHER_SPINNER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),\
        LAUNCHER_TYPE_SPINNER, \
        LauncherSpinnerClass))

#define LAUNCHER_SPINNER_SIZE() (CLUTTER_STAGE_HEIGHT()/2)

typedef struct _LauncherSpinner LauncherSpinner;
typedef struct _LauncherSpinnerClass LauncherSpinnerClass;
typedef struct _LauncherSpinnerPrivate LauncherSpinnerPrivate;

struct _LauncherSpinner
{
  ClutterGroup         parent;
	
  /* private */
  LauncherSpinnerPrivate   *priv;
};

struct _LauncherSpinnerClass 
{
  /*< private >*/
  ClutterGroupClass parent_class;
}; 

GType launcher_spinner_get_type (void) G_GNUC_CONST;

ClutterActor* 
launcher_spinner_new (void);

void
launcher_spinner_spin (LauncherSpinner *spinner, gboolean spin);

G_END_DECLS

#endif
