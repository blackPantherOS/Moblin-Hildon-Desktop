/*
 * Copyright (C) 2007 Intel
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Authored by Neil Jagdish Patel <njp@o-hand.com>
 *
 */

#include <glib.h>

#include <stdio.h>
#include <string.h>

#include <gconf/gconf.h>
#include <gconf/gconf-client.h>

#include "launcher-animation-linear.h"
#include "launcher-behave.h"
#include "launcher-menu.h"
#include "launcher-item.h"
#include "launcher-spinner.h"
#include "launcher-minimap-manager.h"

G_DEFINE_TYPE (LauncherAnimationLinear, launcher_animation_linear, LAUNCHER_TYPE_ANIMATION)

#define LAUNCHER_ANIMATION_LINEAR_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE(obj, \
        LAUNCHER_TYPE_ANIMATION_LINEAR, LauncherAnimationLinearPrivate))

/* GConf keys */
#define COL_DIR "/apps/desktop-launcher"
#define TXT_COL COL_DIR "/title_color" /* int colors[3] (0-255 each) */
#define COM_COL COL_DIR "/comment_color"


struct _LauncherAnimationLinearPrivate
{
  LauncherMenu *menu;
  GList *list;

  ClutterActor *active_item;
  ClutterActor *app_group;
  ClutterActor *spinner;
  
  ClutterActor *comment;
  ClutterTimeline *comm_time;
  ClutterAlpha *comm_alpha;
  ClutterBehaviour *comm_behave;
  gboolean fade_out;

  ClutterTimeline *timeline;
  ClutterAlpha *alpha;
  ClutterBehaviour *behave;

  gboolean needle;

  gint startx;
  gint endx;

  /* Item startup */
  gboolean launching;
  ClutterActor * last_active;
  ClutterTimeline *spin_time;
  ClutterEffectTemplate *spin_temp;
  ClutterTimeline *act_time;
  ClutterAlpha *act_alpha;
  ClutterBehaviour *act_behave;
  gulong i_started;
  gulong i_finished;
  gint i_x;
  gint i_y;

  /* List changing */
  ClutterTimeline *list_time;
  ClutterEffectTemplate *list_temp;

  /* GConf color settings */
  ClutterColor title_col;
  ClutterColor comment_col;
};

enum
{
  PROP_0,
  PROP_LIST
};

enum
{
  ACTIVE_ITEM_CHANGED,
  LAUNCH_STARTED,
  LAUNCH_FINISHED,

  LAST_SIGNAL
};
static guint _linear_signals[LAST_SIGNAL] = {0};

static void
on_timeline_completed (ClutterTimeline *timeline, LauncherAnimationLinear*anim);


/*
 * This function makes sure that all the items are at the correct state in 
 * regards of their position on the screen. We ignore items that are not
 * visible to the user. For the ones that are, we calculate a scale depending
 * on their distance from the center of the stage, and then set their
 * scale and opacity in relation to that.
 */
static void
ensure_layout (LauncherAnimationLinear *anim)
{
  LauncherAnimationLinearPrivate *priv;
  GList *l;
  gint leftmost, rightmost, center, groupx;

  g_return_if_fail (LAUNCHER_IS_ANIMATION_LINEAR (anim));
  priv = anim->priv;
   
  groupx = clutter_actor_get_x (priv->app_group);
  
  /* Ignore actors which are before 'leftmost' and further than 'rightmost' */
  leftmost = -1 * LAUNCHER_ITEM_WIDTH ();
  rightmost = CLUTTER_STAGE_WIDTH () + LAUNCHER_ITEM_WIDTH ();
  center = CLUTTER_STAGE_WIDTH ()/2;

  for (l = priv->list; l; l= l->next)
  {
    ClutterActor *item = CLUTTER_ACTOR (l->data);
    gint x, y;
    gfloat scale, percent;

    if (!CLUTTER_IS_ACTOR (item))
      return;
        
    x = clutter_actor_get_x (item);
    x += groupx;
    y = clutter_actor_get_y (item);

    if (x < leftmost || x > rightmost)
      continue;
    
    if (x < center)
    {
      gint dist = 0; /* To avoid using minus numbers in division */

      if (x < 0)
        dist = -1 * x;
      percent = (x + dist+1.0)/(center+dist+1.0); /* 1.0 is to avoid zero div */
      scale = 0.3 + (0.7 * percent);
    }
    else
    {
      percent = (x - (CLUTTER_STAGE_WIDTH () /2.0))
                / (CLUTTER_STAGE_WIDTH () /2.0);

      scale = 1.0 - (0.7 * percent);
    }

    clutter_actor_set_scale (item, scale, scale);
    clutter_actor_set_opacity (item, 100 + (155 * scale));
    
    if (scale > 0.9)
      clutter_actor_set_opacity (launcher_item_get_label (LAUNCHER_ITEM (item)),
                                 255 * scale);
    else
      clutter_actor_set_opacity (launcher_item_get_label (LAUNCHER_ITEM (item)),
                                 50 * scale);

    if (scale > 0.97 && priv->active_item != item)
    {
      priv->active_item = item;
      g_signal_emit (anim, _linear_signals[ACTIVE_ITEM_CHANGED], 0, item);
    }
  }
}

/*
 * When a new 'endx' is determined for the animation, we want to  match that
 * up to the closet item, to make sure when the animation finishes, there is
 * an item in the middle of the stage.
 */
static void
find_active_item (LauncherAnimationLinear *anim)
{
  LauncherAnimationLinearPrivate *priv;
  GList *l;
  gint groupx, center, activex;
  ClutterActor *active = NULL;

  g_return_if_fail (LAUNCHER_IS_ANIMATION_LINEAR (anim));
  priv = anim->priv;

  if (priv->needle)
    return;

  /* We take the groups position *after the animation completes*, to find the
   * closest item, and adjust the priv->enx variable to make sure that item
   * is in the center of the stage when the animation completes.
   */
  groupx = priv->endx;
  center = CLUTTER_STAGE_WIDTH () /2;

  if (priv->startx == priv->endx)
  {
   active = priv->active_item;
  }
  else if (priv->startx > priv->endx)
  {
    for (l = priv->list; l; l = l->next)
    {
      ClutterActor *item = CLUTTER_ACTOR (l->data);
      gint x;

      x = clutter_actor_get_x (item);
      x += groupx;

      if (x > center)
      {
        active = item;
        break;
      }
    }
  }
  else
  {
    for (l = priv->list; l; l = l->next)
    {
      ClutterActor *item = CLUTTER_ACTOR (l->data);
      gint x;

      x = clutter_actor_get_x (item);
      x += groupx;

      active = item;
      if (x > center)
      {
        break;
      }
    }
  }
  
  if (!CLUTTER_IS_ACTOR (active))
    active = CLUTTER_ACTOR (priv->list->data);

  activex = clutter_actor_get_x (active);
  activex += groupx;

  if (priv->endx < priv->startx && activex < 0)
  {
    active = g_list_nth_data (priv->list, g_list_length (priv->list)-1);
    activex = clutter_actor_get_x (active);
    activex += groupx;
  }
  
  if (activex < center)
    priv->endx = groupx + (center - activex);
  else
    priv->endx = groupx - (activex-center);
}

/*
 * Started the 'launched' animation whent the item is launched.
 */
static void
on_item_launch_started (LauncherItem *item, LauncherAnimationLinear *anim)
{
  LauncherAnimationLinearPrivate *priv;
  
  g_return_if_fail (LAUNCHER_IS_ANIMATION_LINEAR (anim));
  priv = anim->priv;

  priv->launching = TRUE;
  priv->active_item = CLUTTER_ACTOR (item);

  /* Save the current item state, for restoration later */
  priv->i_x = clutter_actor_get_x (CLUTTER_ACTOR (item));
  priv->i_y = clutter_actor_get_y (CLUTTER_ACTOR (item));

  /* Parent the actor to the stage, so it doesn't fade away */
  clutter_actor_reparent (CLUTTER_ACTOR (item), clutter_stage_get_default ());
  clutter_actor_set_size (CLUTTER_ACTOR (item), 
                          LAUNCHER_ITEM_WIDTH (),
                          LAUNCHER_ITEM_HEIGHT ());
  clutter_actor_set_scale (CLUTTER_ACTOR (item), 1.0, 1.0);
  clutter_actor_set_position (CLUTTER_ACTOR (item), 
                              CLUTTER_STAGE_WIDTH ()/2,
               (CLUTTER_STAGE_HEIGHT ()/2)-LAUNCHER_MINIMAP_MANAGER_HEIGHT ());
  clutter_actor_show (CLUTTER_ACTOR (item));

  clutter_actor_show (priv->spinner);
  launcher_spinner_spin (LAUNCHER_SPINNER (priv->spinner), TRUE);

 
  priv->spin_time = clutter_effect_fade (priv->spin_temp,
                       priv->spinner,
                       0, 255,
                       NULL, NULL);
  priv->spin_time = clutter_effect_fade (priv->spin_temp,
                       priv->app_group,
                       255, 0,
                       NULL, NULL);

  clutter_timeline_start (priv->act_time);
  clutter_timeline_start (priv->spin_time);

 g_signal_emit (anim, _linear_signals[LAUNCH_STARTED], 0, item);
}

/* 
 * When the fade in finishes, restore the actor to its orignal state 
 */
static void
on_fade_in_complete (ClutterActor *app_group, LauncherAnimationLinear *anim)
{
  LauncherAnimationLinearPrivate *priv;

  g_return_if_fail (LAUNCHER_IS_ANIMATION_LINEAR (anim));
  priv = anim->priv;

  g_object_ref (priv->active_item);
  clutter_container_remove_actor (CLUTTER_CONTAINER (
                                                clutter_stage_get_default()),
                                  priv->active_item);
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->app_group), 
                               priv->active_item);
  g_object_unref (priv->active_item);
  clutter_actor_set_position (priv->active_item, priv->i_x, priv->i_y);

  clutter_actor_show (priv->active_item);

  ensure_layout (anim);
}

/*
 * Stop the launched animation and return to a normal state once the startup of
 * the item is finished.
 */
static void
on_item_launch_finished (LauncherItem *item, LauncherAnimationLinear *anim)
{
  LauncherAnimationLinearPrivate *priv;
  
  g_return_if_fail (LAUNCHER_IS_ANIMATION_LINEAR (anim));
  priv = anim->priv;

  priv->launching = FALSE;

  launcher_spinner_spin (LAUNCHER_SPINNER (priv->spinner), FALSE);

  clutter_timeline_stop (priv->spin_time);
  g_object_unref (priv->spin_time);
    
  priv->spin_time = clutter_effect_fade (priv->spin_temp,
                       priv->spinner,
                       255, 0,
                       NULL, NULL);
  priv->spin_time = clutter_effect_fade (priv->spin_temp,
                       priv->app_group,
                       clutter_actor_get_opacity (priv->app_group), 255,
                       (ClutterEffectCompleteFunc)on_fade_in_complete, 
                       anim);
 clutter_timeline_start (priv->spin_time);

 g_signal_emit (anim, _linear_signals[LAUNCH_FINISHED], 0, item);
}

/*
 *  On a standard 'clicked' event, we want to find the item that was clicked.
 *  If it is the active icon, then we launch it, otherwise we move the clicked
 *  item to the centre.
 */
static void
launcher_animation_linear_clicked (LauncherAnimationLinear *anim,
                                   gint                     x,
                                   gint                     y)
{
  LauncherAnimationLinearPrivate *priv;
  ClutterActor *stage = clutter_stage_get_default ();
  ClutterActor *clicked = NULL;
  ClutterActor *parent = NULL;

  g_return_if_fail (LAUNCHER_IS_ANIMATION_LINEAR (anim));
  priv = anim->priv;

  clicked = clutter_stage_get_actor_at_pos (CLUTTER_STAGE (stage), x, y);

  if (!CLUTTER_IS_ACTOR (clicked) || CLUTTER_IS_STAGE (clicked))
    return;

  /* Find the corresponding LauncherItem */
  parent = clutter_actor_get_parent (clicked);
  while (parent)
   {
     if (LAUNCHER_IS_ITEM (parent))
       break;
     parent = clutter_actor_get_parent (parent);
   }
  if (!LAUNCHER_IS_ITEM (parent))
    return;

  /* If it is the current active item, activate it. Otherwise move to that
   * item.
   */
  if (parent == priv->active_item)
  {
    if (priv->last_active)
    {
      g_signal_handler_disconnect (priv->active_item, priv->i_started);
      g_signal_handler_disconnect (priv->active_item, priv->i_finished);
    }
    priv->active_item = parent;
    priv->i_started = g_signal_connect (parent, "item-started",
                                        G_CALLBACK (on_item_launch_started),
                                        anim);
    priv->i_finished = g_signal_connect (parent, "item-finished",
                                         G_CALLBACK (on_item_launch_finished),
                                         anim);   
    launcher_item_activate (LAUNCHER_ITEM (parent));
  }
  else
  {
    launcher_animation_set_active_item (LAUNCHER_ANIMATION (anim),
                                        LAUNCHER_ITEM (parent));
  }
}

/*
 * Handler for stage events.
 * If a button_press is received, we reset the time and starting position
 * variables. Then (and while the button is pressed), we track motion events as
 * 1:1 movement of the carousel. Upon release, we start a timeline to create
 * the kinetic scrolling effect, it's effect depending on the distance covered
 * by the pointer.
 *
 * The button_press event will immediatly stop the currently playing timeline.
 * TODO: We should use the 'time' as a factor to all of this, i.e. short 
 * bursts have a greater effect than longer scrolls.
 */
static void
launcher_animation_linear_handle_event (LauncherAnimation *anim, 
                                        ClutterEvent      *event)
{
#define TIMEOUT 300
#define SPEED_FACTOR 2
  LauncherAnimationLinearPrivate *priv;
  static gint startx = 0;
  static gint lastx = 0;
  static guint32 start_time = 0;

  g_return_if_fail (LAUNCHER_IS_ANIMATION_LINEAR (anim));
  priv = LAUNCHER_ANIMATION_LINEAR (anim)->priv;

  if (clutter_timeline_is_playing (priv->act_time))
    return;
  if (clutter_timeline_is_playing (priv->spin_time))
    return;
  
  if (event->type == CLUTTER_BUTTON_PRESS)
  {
    if (!startx)
      startx = lastx = event->button.x;
    start_time = event->button.time;

    if (clutter_timeline_is_playing (priv->timeline))
      clutter_timeline_stop (priv->timeline);
    
    /* Fade out the comment */
    priv->fade_out = TRUE;
    clutter_timeline_start (priv->comm_time);
  }
  else if (event->type == CLUTTER_BUTTON_RELEASE)
  {
    gint endx = clutter_actor_get_x (priv->app_group);
    guint32 time = event->button.time - start_time;
    
    if (time > TIMEOUT)
    {
      priv->endx = endx;
    }
    else if (event->button.x > startx)
    {
      /* 
       * The mouse from left to right, so we have to *add* pixels to the
       * current app_group position to make it move to the right
       */
      endx += (event->motion.x - startx) * SPEED_FACTOR;
      priv->endx = endx;
    }
    else if (event->button.x < startx)
    {
      /* 
       * The mouse from right to left, so we have to *minus* pixels to the
       * current app_group position to make it move to the left
       */
      endx -= (startx - event->button.x) * SPEED_FACTOR;
      priv->endx = endx;
    }
    else
    {
      /* If the click was fast, treat it as a standard 'clicked' event */
      if (time < TIMEOUT)
        launcher_animation_linear_clicked (LAUNCHER_ANIMATION_LINEAR (anim), 
                                           event->button.x, event->button.y);
      startx = lastx = 0;
      return;
     }
    priv->startx = clutter_actor_get_x (priv->app_group);
    find_active_item (LAUNCHER_ANIMATION_LINEAR (anim));

   /* Now start our timeline */
    if (clutter_timeline_is_playing (priv->timeline))
      clutter_timeline_rewind (priv->timeline);
    else
      clutter_timeline_start (priv->timeline);
    
    /*g_print ("travelled from %d to %d. newx = %d\n", 
               startx, event->button.x, priv->endx);*/
    startx =  lastx = 0;
  }
  else if (event->type == CLUTTER_MOTION)
  {
    gint offset;

    if (!startx)
      return;
    if (event->motion.x > lastx)
    {
      offset = event->motion.x - lastx;
    }
    else
    {
      offset = lastx - event->motion.x;
      offset *= -1;
    }
    lastx = event->motion.x;
    clutter_actor_set_position (priv->app_group,
                                clutter_actor_get_x (priv->app_group) + offset,
                                clutter_actor_get_y (priv->app_group));
    ensure_layout (LAUNCHER_ANIMATION_LINEAR (anim));
    clutter_actor_queue_redraw (priv->app_group);
  }  
  else if (event->type == CLUTTER_KEY_RELEASE)
  {
    gint offset = 0;

    switch (event->key.keyval)
    {
      case CLUTTER_Return:
      case CLUTTER_KP_Enter:
      case CLUTTER_ISO_Enter:
        launcher_animation_linear_clicked (LAUNCHER_ANIMATION_LINEAR (anim),
                                           CLUTTER_STAGE_WIDTH ()/2,
                                           CLUTTER_STAGE_HEIGHT ()/2);
        break;
      case CLUTTER_Left:
      case CLUTTER_KP_Left:
        offset += LAUNCHER_ITEM_WIDTH () * 1.5;
        break;
      case CLUTTER_Right:
      case CLUTTER_KP_Right:
        offset -= LAUNCHER_ITEM_WIDTH () ;
        break;
      default:
        break;
    }
    if (offset)
    {
      /* Fade out the comment */
      priv->fade_out = TRUE;
      clutter_timeline_start (priv->comm_time);

      /* Move one item to the left or right */
      priv->startx = clutter_actor_get_x (priv->app_group);
      priv->endx = priv->startx + offset;
      find_active_item (LAUNCHER_ANIMATION_LINEAR (anim));
      
      if (!clutter_timeline_is_playing (priv->timeline))
        clutter_timeline_start (priv->timeline);
      else
        clutter_timeline_rewind (priv->timeline);
    }
    start_time = 0;
  }
}



/*
 * This is the animation function which is called at every iteration of the
 * timeline. It uses the alpha_value variable to calculate a position along
 * the starting x and ending x as a matter of time, and then moves to that
 * position. When alpha_value == CLUTTER_ALPHA_MAX_ALPHA, we are at the end
 * of the timeline, and subsequently reached our destination (priv->endx).
 */
static void
alpha_func (ClutterBehaviour *behave,
            guint32           alpha_value,
            LauncherAnimationLinear *anim)
{
  LauncherAnimationLinearPrivate *priv;
  gfloat factor;
  gint newx;
  gint curx;
  gint offset;

  g_return_if_fail (LAUNCHER_IS_ANIMATION_LINEAR (anim));
  priv = anim->priv;

  factor = (gfloat)alpha_value / CLUTTER_ALPHA_MAX_ALPHA;

  curx = clutter_actor_get_x (priv->app_group);
  /*curx = priv->startx;*/

  if (curx > priv->endx)
  {
    offset = curx - priv->endx;
    offset *= factor;
    
    newx = curx - offset;
  }
  else
  {
    offset = priv->endx - curx;
    offset *= factor;
    
    newx = curx + offset;
  }

  g_object_set (priv->app_group, "x", newx, NULL);

  ensure_layout (anim);

  if (factor > 0.6 && priv->fade_out)
    on_timeline_completed (NULL, anim);

  clutter_actor_queue_redraw (priv->app_group);
}

/*
 * This takes care of fading the comment in & out 
 */
static void
comment_fade_func (ClutterBehaviour *behave,
                   guint32           alpha_value,
                   LauncherAnimationLinear *anim)
{
  LauncherAnimationLinearPrivate *priv;
  gfloat factor;
  
  g_return_if_fail (LAUNCHER_IS_ANIMATION_LINEAR (anim));
  priv = anim->priv;

  factor = (gfloat)alpha_value / CLUTTER_ALPHA_MAX_ALPHA;

  if (priv->fade_out)
    clutter_actor_set_opacity (priv->comment, 100 - (100*factor));
  else
    clutter_actor_set_opacity (priv->comment, 100 * factor);

  clutter_actor_queue_redraw (priv->app_group);
}

/*
 * This animates the active_item in a loop until the item's startup is 
 * completed
 */
static void
active_alpha_func (ClutterBehaviour *behave,
                   guint32           alpha_value,
                   LauncherAnimationLinear *anim)
{
  LauncherAnimationLinearPrivate *priv;
  gfloat factor;
  
  g_return_if_fail (LAUNCHER_IS_ANIMATION_LINEAR (anim));
  priv = anim->priv;

  factor = (gfloat)alpha_value / CLUTTER_ALPHA_MAX_ALPHA;

  clutter_actor_rotate_y (priv->active_item, 360.0 * factor,
                          0,
                          0);
  
  if (factor == 1.0 && !priv->launching)
    clutter_timeline_stop (priv->act_time);
  clutter_actor_queue_redraw (priv->active_item);
}

/*
 * When a new list is added, we need to ensure all icons are int he correct 
 * place and are ready for usage.
 */
static void
add_apps (LauncherAnimationLinear *anim)
{
  LauncherAnimationLinearPrivate *priv;
  GList *l, *list;
  gint x;
  gint y;

  g_return_if_fail (LAUNCHER_IS_ANIMATION_LINEAR (anim));
  priv = anim->priv;

  /* First remove old applications */
  list = clutter_container_get_children (CLUTTER_CONTAINER (priv->app_group));
  for (l = list; l; l = l->next)
  {
    ClutterActor *actor = l->data;

    if (CLUTTER_IS_ACTOR (actor))
      clutter_container_remove_actor (CLUTTER_CONTAINER (priv->app_group),
                                      actor);
  }

  x = (CLUTTER_STAGE_WIDTH () /2) - (LAUNCHER_ITEM_WIDTH ()/2);
  y = (CLUTTER_STAGE_HEIGHT ()/2)- (LAUNCHER_MINIMAP_MANAGER_HEIGHT ());

  for (l = priv->list; l; l = l->next)
  {
    ClutterActor *item = CLUTTER_ACTOR (l->data);

    if (!CLUTTER_ACTOR (item))
      continue;
    clutter_container_add_actor (CLUTTER_CONTAINER (priv->app_group), item);
    clutter_actor_set_position (item, x, y);
    clutter_actor_show (item);
    x += LAUNCHER_ITEM_WIDTH () + (LAUNCHER_ITEM_WIDTH ()/6);
  }
  clutter_actor_show (priv->app_group);

  if (priv->list)
  {
    priv->startx = CLUTTER_STAGE_WIDTH ();
    priv->endx = CLUTTER_STAGE_WIDTH ()/2;
    find_active_item (anim);
    clutter_timeline_start (priv->timeline);
  }
}


static void
on_timeline_completed (ClutterTimeline *timeline, LauncherAnimationLinear*anim)
{
  LauncherAnimationLinearPrivate *priv;
  ClutterActor *actor = NULL;
  const gchar *comment;
  gint x, y, w, h;
  
  g_return_if_fail (LAUNCHER_IS_ANIMATION_LINEAR (anim));
  priv = anim->priv;

  if (clutter_timeline_is_playing (priv->comm_time))
    return;

  actor = clutter_stage_get_actor_at_pos (
                                  CLUTTER_STAGE (clutter_stage_get_default ()),
                                  CLUTTER_STAGE_WIDTH () /2,
                                  CLUTTER_STAGE_HEIGHT ()/2);

  if (!actor)
    return;

  while (!CLUTTER_IS_STAGE (actor))
  {
    if (LAUNCHER_IS_ITEM (actor))
      break;
    actor = clutter_actor_get_parent (actor);
  }

  if (!LAUNCHER_IS_ITEM (actor))
    return;

  comment = launcher_menu_application_get_comment (
        launcher_item_get_application (LAUNCHER_ITEM (actor)));

  clutter_label_set_text (CLUTTER_LABEL (priv->comment), comment);
  clutter_label_set_line_wrap (CLUTTER_LABEL (priv->comment), FALSE);

  w = clutter_actor_get_width (priv->comment);
  h = clutter_actor_get_height (priv->comment);

  x = (CLUTTER_STAGE_WIDTH () /2) - (w/2);
  y = (CLUTTER_STAGE_HEIGHT () * 0.70) - h;
  
  clutter_actor_set_position (priv->comment, x, y);
  
  priv->fade_out = FALSE;
  clutter_timeline_start (priv->comm_time);
}


/* Implemented methods */
static void
on_list_fade_complete (ClutterActor *group, LauncherAnimationLinear *anim)
{
  LauncherAnimationLinearPrivate *priv;
  const gchar *comment;
  ClutterActor *actor;
  gint x, y, w, h;

  g_return_if_fail (LAUNCHER_IS_ANIMATION_LINEAR (anim));
  priv = anim->priv;

  add_apps (LAUNCHER_ANIMATION_LINEAR (anim));

  clutter_effect_fade (priv->list_temp,
                       priv->app_group,
                       0, 255,
                       NULL, NULL);

  actor = clutter_group_get_nth_child (CLUTTER_GROUP (priv->app_group), 0);
  comment = launcher_menu_application_get_comment (
        launcher_item_get_application (LAUNCHER_ITEM (actor)));

  clutter_label_set_text (CLUTTER_LABEL (priv->comment), comment);
  clutter_label_set_line_wrap (CLUTTER_LABEL (priv->comment), FALSE);

  w = clutter_actor_get_width (priv->comment);
  h = clutter_actor_get_height (priv->comment);

  x = (CLUTTER_STAGE_WIDTH () /2) - (w/2);
  y = (CLUTTER_STAGE_HEIGHT () * 0.70) - h;
  
  clutter_actor_set_position (priv->comment, x, y);
}

static void
launcher_animation_linear_set_list (LauncherAnimation *anim, 
                                    GList             *list)
{
  LauncherAnimationLinearPrivate *priv;

  if (!LAUNCHER_IS_ANIMATION_LINEAR (anim)) return;

  priv = LAUNCHER_ANIMATION_LINEAR (anim)->priv;
  priv->list = list;

  clutter_effect_fade (priv->list_temp,
                       priv->app_group,
                       255, 0,
                       (ClutterEffectCompleteFunc)on_list_fade_complete,
                       (gpointer)anim);
}


/* 
 * We need to find the position of the item in the group, then set priv->endx
 * so that item would eb in the center of the screen (active).
 */
static void
launcher_animation_linear_set_active_item (LauncherAnimation *anim,
                                           LauncherItem      *item)
{
  LauncherAnimationLinearPrivate *priv;
  gint x;

  g_return_if_fail (LAUNCHER_IS_ANIMATION_LINEAR (anim));
  g_return_if_fail (LAUNCHER_IS_ITEM (item));
  priv = LAUNCHER_ANIMATION_LINEAR (anim)->priv;

  if (clutter_actor_get_parent (CLUTTER_ACTOR (item)) != priv->app_group)
    return;

  x = clutter_actor_get_x (CLUTTER_ACTOR (item));
  x *= -1;
  x += CLUTTER_STAGE_WIDTH ()/2;

  priv->startx = clutter_actor_get_x (priv->app_group);
  priv->endx = x;

  priv->fade_out = TRUE;
  clutter_timeline_start (priv->comm_time);
  if (clutter_timeline_is_playing (priv->timeline))
    clutter_timeline_rewind (priv->timeline);
  else
    clutter_timeline_start (priv->timeline);
}

static void
parse_colors (GSList *colors, ClutterColor *color)
{
  if (g_slist_length (colors) != 3)
  {
    color->red = 255;
    color->green = 255;
    color->blue = 255;
    color->alpha = 255;
  } 
  else
  {
    color->red = GPOINTER_TO_INT (g_slist_nth_data (colors, 0));
    color->green = GPOINTER_TO_INT (g_slist_nth_data (colors, 1));
    color->blue = GPOINTER_TO_INT (g_slist_nth_data (colors, 2));
    color->alpha = 255;
  }  
}

static void
on_comment_color_changed (GConfClient            *client,
                          guint                   cid,
                          GConfEntry             *entry,
                          LauncherAnimationLinear *anim)
{
  LauncherAnimationLinearPrivate*priv;
  GConfValue *value = NULL;
  GSList *colors;
  ClutterColor *color;
  
  g_return_if_fail (LAUNCHER_IS_ANIMATION_LINEAR (anim));
  priv = anim->priv;
  color = &priv->comment_col;

  value = gconf_entry_get_value (entry);
  
  colors = gconf_value_get_list (value);
  
  if (g_slist_length (colors) != 3)
  {
    color->red = 255;
    color->green = 255;
    color->blue = 255;
    color->alpha = 255;
  } 
  else
  {
    color->red = gconf_value_get_int (g_slist_nth_data (colors, 0));
    color->green = gconf_value_get_int (g_slist_nth_data (colors, 1));
    color->blue = gconf_value_get_int (g_slist_nth_data (colors, 2));
    color->alpha = 255;
  }
  
  clutter_label_set_color (CLUTTER_LABEL (priv->comment), &priv->comment_col);
  clutter_actor_set_opacity (priv->comment, 100);
}
static void
on_title_color_changed (GConfClient            *client,
                        guint                   cid,
                        GConfEntry             *entry,
                        LauncherAnimationLinear *anim)
{
  LauncherAnimationLinearPrivate*priv;
  GConfValue *value = NULL;
  GSList *colors;
  ClutterColor *color;
  GList *i;

  g_return_if_fail (LAUNCHER_IS_ANIMATION_LINEAR (anim));
  priv = anim->priv;
  color = &priv->title_col;

  value = gconf_entry_get_value (entry);
  
  colors = gconf_value_get_list (value);
  
  if (g_slist_length (colors) != 3)
  {
    color->red = 255;
    color->green = 255;
    color->blue = 255;
    color->alpha = 255;
  } 
  else
  {
    color->red = gconf_value_get_int (g_slist_nth_data (colors, 0));
    color->green = gconf_value_get_int (g_slist_nth_data (colors, 1));
    color->blue = gconf_value_get_int (g_slist_nth_data (colors, 2));
    color->alpha = 255;
  }
  
  for (i = launcher_menu_get_applications (priv->menu); i; i = i->next)
  {
    LauncherItem *item = i->data;

    clutter_label_set_color (CLUTTER_LABEL (launcher_item_get_label (item)),
                             &priv->title_col);
  }
}
/* GObject functions */
static void
launcher_animation_linear_get_property (GObject    *object,
                                        guint       prop_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  LauncherAnimationLinearPrivate *priv;

  g_return_if_fail (LAUNCHER_IS_ANIMATION_LINEAR (object));
  priv = LAUNCHER_ANIMATION_LINEAR (object)->priv;

  switch (prop_id)
  {
    case PROP_LIST:
      g_value_set_pointer (value, priv->list);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
launcher_animation_linear_set_property (GObject      *object,
                                        guint         prop_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  LauncherAnimationLinearPrivate *priv;

  g_return_if_fail (LAUNCHER_IS_ANIMATION_LINEAR (object));
  priv = LAUNCHER_ANIMATION_LINEAR (object)->priv;

  switch (prop_id)
  {
    case PROP_LIST:
      priv->list = g_value_get_pointer (value);
      add_apps (LAUNCHER_ANIMATION_LINEAR (object));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}


static void
launcher_animation_linear_dispose (GObject *object)
{
  G_OBJECT_CLASS (launcher_animation_linear_parent_class)->dispose (object);
}

static void
launcher_animation_linear_finalize (GObject *anim)
{
  LauncherAnimationLinearPrivate *priv;
  
  g_return_if_fail (LAUNCHER_IS_ANIMATION_LINEAR (anim));
  priv = LAUNCHER_ANIMATION_LINEAR (anim)->priv;


  G_OBJECT_CLASS (launcher_animation_linear_parent_class)->finalize (anim);
}


static void
launcher_animation_linear_class_init (LauncherAnimationLinearClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  LauncherAnimationClass *anim_class = LAUNCHER_ANIMATION_CLASS (klass);

  obj_class->finalize = launcher_animation_linear_finalize;
  obj_class->dispose = launcher_animation_linear_dispose; 
  obj_class->get_property = launcher_animation_linear_get_property;
  obj_class->set_property = launcher_animation_linear_set_property;

  anim_class->handle_event = launcher_animation_linear_handle_event;
  anim_class->set_list = launcher_animation_linear_set_list;  
  anim_class->set_active_item = launcher_animation_linear_set_active_item;

  /* Class properties */
  g_object_class_install_property (obj_class,
    PROP_LIST,
    g_param_spec_pointer ("list",
      "List",
      "A GList of applications",
      G_PARAM_CONSTRUCT|G_PARAM_READWRITE));

  /* Class signals */
  _linear_signals[ACTIVE_ITEM_CHANGED] = 
    g_signal_new ("active-item-changed",
                  G_OBJECT_CLASS_TYPE (obj_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LauncherAnimationLinearClass, 
                                   active_item_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1, LAUNCHER_TYPE_ITEM);

  _linear_signals[LAUNCH_STARTED] = 
    g_signal_new ("launch-started",
                  G_OBJECT_CLASS_TYPE (obj_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LauncherAnimationLinearClass, 
                                   launch_started),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1, LAUNCHER_TYPE_ITEM);

  _linear_signals[LAUNCH_FINISHED] = 
    g_signal_new ("launch-finished",
                  G_OBJECT_CLASS_TYPE (obj_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LauncherAnimationLinearClass, 
                                   launch_finished),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1, LAUNCHER_TYPE_ITEM);

  g_type_class_add_private (obj_class, sizeof(LauncherAnimationLinearPrivate));

}

static void
launcher_animation_linear_init (LauncherAnimationLinear *anim)
{
  LauncherAnimationLinearPrivate *priv;
  ClutterColor comment_col = {0x00, 0x00, 0x00, 0xff};
  gchar *font;
  ClutterActor *stage = clutter_stage_get_default ();
  GConfClient *client = gconf_client_get_default ();
  GSList *colors;
  GList *i;
  
  priv = anim->priv = LAUNCHER_ANIMATION_LINEAR_GET_PRIVATE (anim);

  priv->menu = launcher_menu_get_default ();
  priv->active_item = 0;
  priv->startx = CLUTTER_STAGE_WIDTH ();
  priv->endx = CLUTTER_STAGE_WIDTH ()/2;
  priv->i_started = priv->i_finished = 0;

  /* The spinner */
  priv->spinner = launcher_spinner_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), priv->spinner);
  clutter_actor_set_size (priv->spinner, LAUNCHER_SPINNER_SIZE (),
                          LAUNCHER_SPINNER_SIZE ());
  clutter_actor_set_position (priv->spinner,
         (CLUTTER_STAGE_WIDTH ()/2)-(LAUNCHER_SPINNER_SIZE ()/2),
       (CLUTTER_STAGE_HEIGHT ()/2) + (LAUNCHER_SPINNER_SIZE ()*0.05));
  clutter_actor_rotate_x (priv->spinner, 70.0, 0, 0);
  clutter_actor_hide (priv->spinner);
  
  /* The comment that appears below the active item */
  font = g_strdup_printf ("Sans %d", LAUNCHER_ITEM_HEIGHT ()/9);
  priv->comment = clutter_label_new_full (font, " ", &comment_col);
  clutter_container_add_actor (CLUTTER_CONTAINER (clutter_stage_get_default()),
                               priv->comment);
  clutter_actor_set_opacity (priv->comment, 100);
  clutter_actor_show (priv->comment);
  g_free (font);
  
  /* This is the group we add items to */
  priv->app_group = clutter_group_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (clutter_stage_get_default()),
                               priv->app_group);
  clutter_actor_set_position (priv->app_group, 0, 0);

  priv->timeline = clutter_timeline_new (80, 40);
  priv->alpha = clutter_alpha_new_full (priv->timeline,
                                        clutter_sine_inc_func,
                                        NULL, NULL);
  priv->behave = launcher_behave_new (priv->alpha, 
                                      (LauncherBehaveAlphaFunc)alpha_func,
                                      (gpointer)anim);

  priv->comm_time = clutter_timeline_new (30, 60);
  priv->comm_alpha = clutter_alpha_new_full (priv->comm_time,
                                        clutter_sine_inc_func,
                                        NULL, NULL);
  priv->comm_behave = launcher_behave_new (priv->comm_alpha, 
                                     (LauncherBehaveAlphaFunc)comment_fade_func,
                                      (gpointer)anim);

  priv->spin_time = clutter_timeline_new (60, 40);
  priv->spin_temp = clutter_effect_template_new (priv->spin_time, 
                                                 clutter_sine_inc_func); 
  g_object_ref (priv->spin_temp);
 
  priv->act_time = clutter_timeline_new (60, 45);
  clutter_timeline_set_loop (priv->act_time, TRUE);
  priv->act_alpha = clutter_alpha_new_full (priv->act_time,
                                        clutter_ramp_inc_func,
                                        NULL, NULL);
  priv->act_behave = launcher_behave_new (priv->act_alpha, 
                                     (LauncherBehaveAlphaFunc)active_alpha_func,
                                      (gpointer)anim);

  priv->list_time = clutter_timeline_new (40, 60);
  priv->list_temp = clutter_effect_template_new (priv->list_time, 
                                                 clutter_sine_inc_func);


  /* Load up the gconf color values */
  gconf_client_add_dir (client, COL_DIR, GCONF_CLIENT_PRELOAD_NONE, NULL);

  colors = gconf_client_get_list (client, TXT_COL,GCONF_VALUE_INT, NULL);
  parse_colors (colors, &priv->title_col);    
  g_slist_free (colors);
  for (i = launcher_menu_get_applications (priv->menu); i; i = i->next)
  {
    LauncherItem *item = i->data;

    clutter_label_set_color (CLUTTER_LABEL (launcher_item_get_label (item)),
                             &priv->title_col);
  }
  gconf_client_notify_add (client, TXT_COL,
                           (GConfClientNotifyFunc)on_title_color_changed,
                           (gpointer)anim,
                           NULL, NULL);
  
  colors = gconf_client_get_list (client, COM_COL, GCONF_VALUE_INT, NULL);
  parse_colors (colors, &priv->comment_col);
  clutter_label_set_color (CLUTTER_LABEL (priv->comment), &priv->comment_col);
  clutter_actor_set_opacity (priv->comment, 100);  
  gconf_client_notify_add (client, COM_COL,
                           (GConfClientNotifyFunc)on_comment_color_changed,
                           (gpointer)anim,
                           NULL, NULL);

}

LauncherAnimation *
launcher_animation_linear_new (void)
{
  LauncherAnimation *anim;

  anim = g_object_new (LAUNCHER_TYPE_ANIMATION_LINEAR,
                       NULL);
  return anim;
}

