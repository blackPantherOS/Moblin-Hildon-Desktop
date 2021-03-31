/*
 * Copyright (C) 2007 Neil J. Patel
 * Copyright (C) 2007 OpenedHand Ltd
 *
 * Author: Neil J. Patel  <njp@o-hand.com>
 */

#include "launcher-spinner.h"

#include "launcher-behave.h"	

G_DEFINE_TYPE (LauncherSpinner, launcher_spinner, CLUTTER_TYPE_GROUP);

#define LAUNCHER_SPINNER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
        LAUNCHER_TYPE_SPINNER, LauncherSpinnerPrivate))
	
static GdkPixbuf		*spinner_pixbuf = NULL;

struct _LauncherSpinnerPrivate
{
  ClutterActor *texture;

  ClutterTimeline		*timeline;
  ClutterAlpha		*alpha;
  ClutterBehaviour 	*behave;
};


/* Starts the timeline */
void
launcher_spinner_spin (LauncherSpinner *spinner, gboolean spin)
{
  LauncherSpinnerPrivate *priv;
	
  g_return_if_fail (LAUNCHER_IS_SPINNER (spinner));
  priv = LAUNCHER_SPINNER_GET_PRIVATE (spinner);
  
  if (spin)
	  clutter_timeline_start (priv->timeline);
  else
	  clutter_timeline_stop (priv->timeline);
}


/* Spins the spinner texture on its z-axis */
static void
launcher_spinner_alpha_func (ClutterBehaviour *behave,
		       	   guint	     alpha_value,
		           gpointer	     data)
{
  LauncherSpinnerPrivate *priv;
  gfloat factor;
  gfloat angle;
	
  g_return_if_fail (LAUNCHER_IS_SPINNER (data));
  priv = LAUNCHER_SPINNER_GET_PRIVATE (data);
	
  /* First we calculate the factor (how far we are along the timeline
  between 0-1 */
  factor = (gfloat)alpha_value / CLUTTER_ALPHA_MAX_ALPHA;
	
  /* Calculate the angle */
  angle = factor * 360.0;
	
  /* Set the new angle */
  clutter_actor_rotate_z (CLUTTER_ACTOR (priv->texture), angle, 
                          (LAUNCHER_SPINNER_SIZE () /2),
                          (LAUNCHER_SPINNER_SIZE () /2));

	clutter_actor_queue_redraw (CLUTTER_ACTOR(data));	
} 		
		
/* GObject Stuff */

static void 
launcher_spinner_dispose (GObject *object)
{
  LauncherSpinner         *self = LAUNCHER_SPINNER(object);
  LauncherSpinnerPrivate  *priv;  

  priv = self->priv;
  
  G_OBJECT_CLASS (launcher_spinner_parent_class)->dispose (object);
}

static void 
launcher_spinner_finalize (GObject *object)
{
  G_OBJECT_CLASS (launcher_spinner_parent_class)->finalize (object);
}

static void
launcher_spinner_class_init (LauncherSpinnerClass *klass)
{
  GObjectClass        *gobject_class = G_OBJECT_CLASS (klass);
  ClutterActorClass   *parent_class; 

  parent_class = CLUTTER_ACTOR_CLASS (launcher_spinner_parent_class);

  gobject_class->finalize     = launcher_spinner_finalize;
  gobject_class->dispose      = launcher_spinner_dispose;
	
  g_type_class_add_private (gobject_class, sizeof (LauncherSpinnerPrivate));
}

static void
launcher_spinner_init (LauncherSpinner *spinner)
{
  LauncherSpinnerPrivate *priv;
  priv = LAUNCHER_SPINNER_GET_PRIVATE (spinner);

  if (spinner_pixbuf == NULL) 
  {
    spinner_pixbuf = gdk_pixbuf_new_from_file_at_scale (
                  PKGDATADIR "/spinner.svg",
                  LAUNCHER_SPINNER_SIZE (),
                  LAUNCHER_SPINNER_SIZE (),
                  TRUE,
                  NULL);
	}
	if (spinner_pixbuf)
  {
		priv->texture = clutter_texture_new_from_pixbuf (spinner_pixbuf);
    clutter_container_add_actor (CLUTTER_CONTAINER (spinner), 
                                 priv->texture);
    clutter_actor_set_position (priv->texture, 0, 0);
    clutter_actor_set_size (priv->texture,  
                            LAUNCHER_SPINNER_SIZE (),
                            LAUNCHER_SPINNER_SIZE ());
    clutter_actor_show (priv->texture);
  }
	else
		g_print ("Could not load spinner\n");		
  
  priv->timeline = clutter_timeline_new (40, 40);
  clutter_timeline_set_loop (priv->timeline, TRUE);
  priv->alpha = clutter_alpha_new_full (priv->timeline,
                                        clutter_ramp_inc_func,
                                        NULL, NULL);
  priv->behave = launcher_behave_new (priv->alpha,
	                                    launcher_spinner_alpha_func,
                                      (gpointer)spinner);
}

ClutterActor*
launcher_spinner_new (void)
{
  ClutterActor         *spinner;

  spinner = g_object_new (LAUNCHER_TYPE_SPINNER, 
                          NULL);


 	return CLUTTER_ACTOR (spinner);
}

