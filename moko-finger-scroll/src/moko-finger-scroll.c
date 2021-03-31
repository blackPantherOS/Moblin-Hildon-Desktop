/*
 *  libmokoui -- OpenMoko Application Framework UI Library
 *
 *  Authored by Chris Lord <chris@openedhand.com>
 *
 *  Copyright (C) 2006-2007 OpenMoko Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser Public License as published by
 *  the Free Software Foundation; version 2 of the license.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser Public License for more details.
 *
 *  Current Version: $Rev$ ($Date$) [$Author$]
 */

/**
 * SECTION: moko-finger-scroll
 * @short_description: A scrolling widget designed for touch screens
 * @see_also: #GtkScrolledWindow
 *
 * #MokoFingerScroll implements a scrolled window designed to be used with a
 * touch screen interface. The user scrolls the child widget by activating the
 * pointing device and dragging it over the widget.
 *
 */


#include "moko-finger-scroll.h"
#define    MILLION 1000000
#define    DOUBLE_CLICK 500000
#define    DOUBLE_CLK_MOVE 50

//#define DEBUG

#ifdef DEBUG
#define PRINT printf
#else
#define PRINT 1?0:printf
#endif 

G_DEFINE_TYPE (MokoFingerScroll, moko_finger_scroll, GTK_TYPE_EVENT_BOX)
#define FINGER_SCROLL_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MOKO_TYPE_FINGER_SCROLL, MokoFingerScrollPrivate))
typedef struct _MokoFingerScrollPrivate MokoFingerScrollPrivate;
static gboolean moko_finger_scroll_is_normall_padding(MokoFingerScroll *scroll);
static void moko_finger_scroll_redraw (MokoFingerScroll *scroll);

struct _MokoFingerScrollPrivate {
	MokoFingerScrollMode mode;
	gdouble x;
	gdouble y;
	gdouble ex;
	gdouble ey;
	gboolean enabled;
	gboolean clicked;
	guint32 last_time;
	gboolean moved;
	GTimeVal click_start;
	GTimeVal last_click;
	gdouble vmin;
	gdouble vmax;
	gdouble decel;
	gdouble spring_decel;
	guint sps;
	gdouble vel_x;
	gdouble vel_y;
	
	GtkWidget *align;
	gboolean hscroll;
	gboolean vscroll;

	gboolean is_hscrollable;
	gboolean is_vscrollable;
	
	GdkRectangle hscroll_rect;
	GdkRectangle vscroll_rect;
	guint scroll_width;

	GtkAdjustment *hadjust;
	GtkAdjustment *vadjust;
	
	gboolean is_acel;
	gdouble click_x;
	gdouble click_y;

	guint	event_mode;
	gboolean is_first_click;
	gboolean is_double_click;

	MokoFingerScrollIndicatorMode  vindicator_mode;
	MokoFingerScrollIndicatorMode  hindicator_mode;

	gboolean  is_v_spring;
	gboolean  is_h_spring;
	
	guint	v_spring;
	guint 	h_spring;
	
	GdkGC * spring_gc;
	GdkColor spring_color;
	gboolean is_detect_double_click;

	gdouble indicator_alpha;
	gdouble indicator_alpha_current;
	gdouble indicator_alpha_decress_rate;
	gint indicator_off_timeout;
	gint indicator_off_timeout_value;

	guint	indicator_timeout_id;
	guint	idle_id;
};

enum {
	PROP_ENABLED = 1,
	PROP_MODE,
	PROP_VELOCITY_MIN,
	PROP_VELOCITY_MAX,
	PROP_DECELERATION,
	PROP_SPS,
	PROP_VINDICATOR,
	PROP_HINDICATOR,
	PROP_SPRING_SPEED,
 	PROP_IS_DETECT_DOUBLE_CLICK,
	PROP_SPRING_COLOR
};
static gdouble
moko_get_time_delta(GTimeVal *start, GTimeVal *end)
{
	gdouble x, y;
	x=start->tv_sec;
	x*=MILLION;
	x+=start->tv_usec;

	y=end->tv_sec;
	y*=MILLION;
	y+=end->tv_usec;
	
	return y-x;

}

static GdkWindow *
moko_finger_scroll_get_topmost (GdkWindow *window, gint x, gint y,
				gint *x2, gint *y2);

static gboolean
moko_forward_event (MokoFingerScroll *scroll,
				      GdkEventButton *event,
				      gpointer user_data)
{
	MokoFingerScrollPrivate *priv = FINGER_SCROLL_PRIVATE (scroll);
	GtkAdjustment *hadjust, *vadjust;
	gint x, y;
	GdkWindow *child;
	
	g_object_get (G_OBJECT (GTK_BIN (priv->align)->child), "hadjustment",
			&hadjust, "vadjustment", &vadjust, NULL);

	child = moko_finger_scroll_get_topmost (
			GTK_BIN (priv->align)->child->window,
			event->x, event->y, &x, &y);

	event->x = x;
	event->y = y;
		
		/* Set velocity to zero, most widgets don't expect to be
		 * moving while being clicked.
		 */
	//priv->vel_x = 0;
	//	priv->vel_y = 0;

	/* Send synthetic click event */
	((GdkEvent *)event)->any.window = g_object_ref (child);
	//((GdkEvent *)event)->type = GDK_BUTTON_PRESS;
	gdk_event_put ((GdkEvent *)event);
}


static gboolean
moko_forward_motion_event (MokoFingerScroll *scroll,
				      GdkEventMotion *event,
				      gpointer user_data)
{
	MokoFingerScrollPrivate *priv = FINGER_SCROLL_PRIVATE (scroll);
	GtkAdjustment *hadjust, *vadjust;
	gint x, y;
	GdkWindow *child;

	g_object_get (G_OBJECT (GTK_BIN (priv->align)->child), "hadjustment",
			&hadjust, "vadjustment", &vadjust, NULL);

	child = moko_finger_scroll_get_topmost (
			GTK_BIN (priv->align)->child->window,
			event->x, event->y, &x, &y);

	event->x = x;
	event->y = y;
		
		/* Set velocity to zero, most widgets don't expect to be
		 * moving while being clicked.
		 */
	//priv->vel_x = 0;
	//	priv->vel_y = 0;

	/* Send synthetic click event */
	((GdkEvent *)event)->any.window = g_object_ref (child);
	//((GdkEvent *)event)->type = GDK_BUTTON_PRESS;
	gdk_event_put ((GdkEvent *)event);
}

static gboolean
moko_finger_scroll_button_press_cb (MokoFingerScroll *scroll,
				    GdkEventButton *event,
				    gpointer user_data)
{
	MokoFingerScrollPrivate *priv = FINGER_SCROLL_PRIVATE (scroll);

	if ((!priv->enabled) || (priv->clicked) || (event->button != 1) ||
	    (event->time == priv->last_time)) return FALSE;

	g_get_current_time (&priv->click_start);
	priv->last_time = event->time;
	priv->x = event->x;
	priv->y = event->y;
	/* Don't allow a click if we're still moving fast, where fast is
	 * defined as a quarter of our top possible speed.
	 * TODO: Make 'fast' configurable?
	 */
	if ((ABS (priv->vel_x) < (priv->vmax * 0.25)) &&
	    (ABS (priv->vel_y) < (priv->vmax * 0.25)))
		priv->moved = FALSE;
	priv->clicked = TRUE;
	/* Stop scrolling on mouse-down (so you can flick, then hold to stop) */
	priv->vel_x = 0;
	priv->vel_y = 0;
	
	priv->is_acel=FALSE;
	

	priv->is_double_click = FALSE;
	if ( priv->is_first_click && 
		 (moko_get_time_delta(&priv->last_click, &priv->click_start) 
					< DOUBLE_CLICK) &&
         ABS(priv->click_x - event->x ) <  DOUBLE_CLK_MOVE&&
         ABS(priv->click_y - event->y ) <  DOUBLE_CLK_MOVE               
        )
	{
			//PRINT("double click\n");
			priv->is_double_click=TRUE;
			priv->is_first_click =FALSE;
	}
    
        priv->click_x = event->x;
	priv->click_y = event->y;

	priv->is_first_click =TRUE;
	priv->last_click=priv->click_start;

	priv->indicator_alpha_current =  priv->indicator_alpha;
	moko_finger_scroll_redraw(scroll);
	if( priv->indicator_timeout_id )
	{
		g_source_remove(priv->indicator_timeout_id);
		priv->indicator_timeout_id = 0;
	}
	return TRUE;
}

static void
moko_finger_scroll_redraw (MokoFingerScroll *scroll)
{
	MokoFingerScrollPrivate *priv = FINGER_SCROLL_PRIVATE (scroll);

	/* Redraw scroll indicators */
	if (priv->hscroll) {
		if (GTK_WIDGET (scroll)->window) {
			gdk_window_invalidate_rect (GTK_WIDGET (scroll)->window,
				&priv->hscroll_rect, FALSE);
		}
	}
	if (priv->vscroll) {
		if (GTK_WIDGET (scroll)->window) {
			gdk_window_invalidate_rect (GTK_WIDGET (scroll)->window,
				&priv->vscroll_rect, FALSE);
		}
	}
}

static void
moko_finger_scroll_refresh (MokoFingerScroll *scroll)
{
	MokoFingerScrollPrivate *priv = FINGER_SCROLL_PRIVATE (scroll);
	GtkAllocation *allocation = &GTK_WIDGET (scroll)->allocation;
	GtkWidget *widget = GTK_BIN (priv->align)->child;
	gboolean vscroll, hscroll;
	GtkRequisition req;
	guint border;
	
	PRINT("MOKO1:moko_finger_scroll_refresh\n");

	if (!widget) return;
	
	/* Calculate if we need scroll indicators */
	gtk_widget_size_request (widget, NULL);
	hscroll = (priv->hadjust->upper - priv->hadjust->lower >
		priv->hadjust->page_size) ? TRUE : FALSE;
	vscroll = (priv->vadjust->upper - priv->vadjust->lower >
		priv->vadjust->page_size) ? TRUE : FALSE;
	
	priv->is_vscrollable  = vscroll;
	priv->is_hscrollable = hscroll;

	if( priv->hindicator_mode  == MOKO_FINGER_SCROLL_INDICATOR_MODE_SHOW )
		 hscroll = TRUE;
	else if ( priv->hindicator_mode == MOKO_FINGER_SCROLL_INDICATOR_MODE_HIDE )
		hscroll = FALSE;
	
	if( priv->vindicator_mode == MOKO_FINGER_SCROLL_INDICATOR_MODE_SHOW )
		 vscroll = TRUE;
	else if ( priv->vindicator_mode == MOKO_FINGER_SCROLL_INDICATOR_MODE_HIDE )
		vscroll = FALSE;

	/* TODO: Read ltr settings to decide which corner gets scroll
	 * indicators?
	 */
	PRINT("moko_finger_scroll_refresh\n");
	if ((priv->vscroll != vscroll) || (priv->hscroll != hscroll))
	{
		int h, v;
		h = hscroll ? priv->scroll_width: 0;
		v = vscroll ? priv->scroll_width: 0;

		if( priv->hindicator_mode == MOKO_FINGER_SCROLL_INDICATOR_MODE_ALPHA)
			h =0;
		if( priv->vindicator_mode == MOKO_FINGER_SCROLL_INDICATOR_MODE_ALPHA)
			v = 0;

		PRINT("padding h %d v %d\n", h, v);

		//if(moko_finger_scroll_is_normall_padding(scroll) )
			gtk_alignment_set_padding (GTK_ALIGNMENT (priv->align), 0,
				h, 0,v);
	}
	
	/* Store the vscroll/hscroll areas for redrawing */
	if (vscroll) {
		GtkAllocation *allocation = &GTK_WIDGET (scroll)->allocation;
		priv->vscroll_rect.x = allocation->x + allocation->width -
			priv->scroll_width;
		priv->vscroll_rect.y = allocation->y;
		priv->vscroll_rect.width = priv->scroll_width;
		priv->vscroll_rect.height = allocation->height -
			(hscroll ? priv->scroll_width : 0);
	}
	if (hscroll) {
		GtkAllocation *allocation = &GTK_WIDGET (scroll)->allocation;
		priv->hscroll_rect.y = allocation->y + allocation->height -
			priv->scroll_width;
		priv->hscroll_rect.x = allocation->x;
		priv->hscroll_rect.height = priv->scroll_width;
		priv->hscroll_rect.width = allocation->width -
			(vscroll ? priv->scroll_width : 0);
	}
	
	priv->vscroll = vscroll;
	priv->hscroll = hscroll;
	
	moko_finger_scroll_redraw (scroll);
}

static void
moko_finger_scroll_scroll (MokoFingerScroll *scroll, gdouble x, gdouble y,
			   gboolean *sx, gboolean *sy)
{
	/* Scroll by a particular amount (in pixels). Optionally, return if
	 * the scroll on a particular axis was successful.
	 */
	gdouble h, v;
	MokoFingerScrollPrivate *priv = FINGER_SCROLL_PRIVATE (scroll);
	
	if (!GTK_BIN (priv->align)->child) return;
	
	if (priv->hadjust) {
		h = gtk_adjustment_get_value (priv->hadjust) - x;
		if (h > priv->hadjust->upper - priv->hadjust->page_size) {
			if (sx) *sx = FALSE;
			h = priv->hadjust->upper - priv->hadjust->page_size;
		} else if (h < priv->hadjust->lower) {
			if (sx) *sx = FALSE;
			h = priv->hadjust->lower;
		} else if (sx)
			*sx = TRUE;
		gtk_adjustment_set_value (priv->hadjust, h);
	}
	
	if (priv->vadjust) {
		v = gtk_adjustment_get_value (priv->vadjust) - y;
		if (v > priv->vadjust->upper - priv->vadjust->page_size) {
			if (sy) *sy = FALSE;
			v = priv->vadjust->upper - priv->vadjust->page_size;
		} else if (v < priv->vadjust->lower) {
			if (sy) *sy = FALSE;
			v = priv->vadjust->lower;
		} else if (sy)
			*sy = TRUE;
		gtk_adjustment_set_value (priv->vadjust, v);
	}

	moko_finger_scroll_redraw (scroll);
}

static gint moko_finger_scroll_get_h_indicatior_width(MokoFingerScroll *scroll)
{	
	MokoFingerScrollPrivate *priv = FINGER_SCROLL_PRIVATE (scroll);

	if(priv -> hindicator_mode == MOKO_FINGER_SCROLL_INDICATOR_MODE_ALPHA )
	{
		return 0;
	}

	if( priv->hscroll )
	{
		return priv->scroll_width;
	}
	return 0;
}

static gint moko_finger_scroll_get_v_indicatior_width(MokoFingerScroll *scroll)
{	
	MokoFingerScrollPrivate *priv = FINGER_SCROLL_PRIVATE (scroll);
	PRINT("vscroll %d \n", 	priv->vscroll);

	if(priv -> vindicator_mode == MOKO_FINGER_SCROLL_INDICATOR_MODE_ALPHA )
	{
		return 0;
	}


	if( priv->vscroll )
	{
		return priv->scroll_width;
	}
	return 0;
}


static gboolean
moko_finger_scroll_is_h_normall_padding(MokoFingerScroll *scroll)
{
	gint  padding_top;
	gint  padding_bottom;
	gint  padding_left;
	gint  padding_right;
	MokoFingerScrollPrivate *priv = FINGER_SCROLL_PRIVATE (scroll);

	gtk_alignment_get_padding(priv->align, &padding_top, &padding_bottom, &padding_left, &padding_right);
	if( 	padding_left == 0 &&
		padding_right <=  moko_finger_scroll_get_v_indicatior_width(scroll))
		return TRUE;
	return FALSE;
}

static gboolean
moko_finger_scroll_is_v_normall_padding(MokoFingerScroll *scroll)
{
	gint  padding_top;
	gint  padding_bottom;
	gint  padding_left;
	gint  padding_right;
	MokoFingerScrollPrivate *priv = FINGER_SCROLL_PRIVATE (scroll);

	gtk_alignment_get_padding(priv->align, &padding_top, &padding_bottom, &padding_left, &padding_right);
	if( padding_top ==0 &&
		padding_bottom  <= moko_finger_scroll_get_h_indicatior_width(scroll))
		return TRUE;
	return FALSE;
}

static gboolean moko_finger_scroll_is_normall_padding(MokoFingerScroll *scroll)
{
	if(moko_finger_scroll_is_h_normall_padding(scroll) && moko_finger_scroll_is_v_normall_padding(scroll))
		return TRUE;

	return FALSE;
}

static void
moko_finger_scroll_spring(MokoFingerScroll *scroll, double diff_top, double diff_bottom, double diff_left, double diff_right)
{	
	gint  padding_top;
	gint  padding_bottom;
	gint  padding_left;
	gint  padding_right;
	MokoFingerScrollPrivate *priv = FINGER_SCROLL_PRIVATE (scroll);

	 gtk_alignment_get_padding(priv->align, &padding_top, &padding_bottom, &padding_left, &padding_right);
	 
	 PRINT(" padding_top %d   bottom %d left %d right %d\n", padding_top, padding_bottom, padding_left, padding_right);

         padding_top += diff_top;
	 padding_bottom += diff_bottom;
	 padding_left += diff_left;
	 padding_right += diff_right;

	if(padding_top <=1)
		padding_top =0;
	if(padding_left<=1)
		padding_left=0;

	if(padding_bottom<=1+moko_finger_scroll_get_h_indicatior_width(scroll))
		padding_bottom=moko_finger_scroll_get_h_indicatior_width(scroll);

	if(padding_right<=1+moko_finger_scroll_get_v_indicatior_width(scroll))
		padding_right=moko_finger_scroll_get_v_indicatior_width(scroll);

	PRINT(" padding_top %d   bottom %d left %d right %d\n", padding_top, padding_bottom, padding_left, padding_right);

	PRINT("moko_finger_scroll_spring padding_bottom %d diff_bottom %d\n", padding_bottom, diff_bottom);

	gtk_alignment_set_padding(priv->align,   padding_top, padding_bottom, padding_left, padding_right);	
}

static void
moko_finger_scroll_sprint_slow_down(MokoFingerScroll *scroll)
{
	gint  padding_top;
	gint  padding_bottom;
	gint  padding_left;
	gint  padding_right;
	MokoFingerScrollPrivate *priv = FINGER_SCROLL_PRIVATE (scroll);

	 gtk_alignment_get_padding(priv->align, &padding_top, &padding_bottom, &padding_left, &padding_right);
	 
         padding_top  *= priv->spring_decel;
	 padding_bottom  *= priv->spring_decel;
	 padding_left  *= priv->spring_decel;
	 padding_right  *= priv->spring_decel;

	if(padding_top <0)
		padding_top =0;
	if(padding_bottom<moko_finger_scroll_get_h_indicatior_width(scroll))
		padding_bottom=moko_finger_scroll_get_h_indicatior_width(scroll);
	if(padding_right<moko_finger_scroll_get_v_indicatior_width(scroll))
		padding_right=moko_finger_scroll_get_v_indicatior_width(scroll);
	if(padding_left<0)
		padding_left=0;

	gtk_alignment_set_padding(priv->align,   padding_top, padding_bottom, padding_left, padding_right);	

}
static gboolean
moko_finger_scroll_timeout (MokoFingerScroll *scroll)
{
	gboolean sx, sy;
	MokoFingerScrollPrivate *priv = FINGER_SCROLL_PRIVATE (scroll);

	PRINT("moko_finger_scroll_timeout\n");
	
	if ((!priv->enabled) ||
	    (priv->mode == MOKO_FINGER_SCROLL_MODE_PUSH)) return FALSE;
	
	//if(  priv->is_acel == FALSE  )
	//	return TRUE;

	if( (!priv->clicked) && (!moko_finger_scroll_is_normall_padding(scroll)) )
	{
		moko_finger_scroll_sprint_slow_down(scroll);
		return TRUE;
	}
	if (!priv->clicked) {
		/* Decelerate gradually when pointer is raised */
		priv->vel_x *= priv->decel;
		priv->vel_y *= priv->decel;
		if ((ABS (priv->vel_x) < 1.0) && (ABS (priv->vel_y) < 1.0))
		{
			priv->idle_id = 0;
			return FALSE;
		}
	
	}else
		return TRUE;

	moko_finger_scroll_scroll (scroll, priv->vel_x, priv->vel_y, &sx, &sy);
	/* If the scroll on a particular axis wasn't succesful, reset the
	 * initial scroll position to the new mouse co-ordinate. This means
	 * when you get to the top of the page, dragging down works immediately.
	 */
	if (!sx) priv->x = priv->ex;
	if (!sy) priv->y = priv->ey;
	
	return TRUE;
}

static gboolean
moko_finger_scroll_motion_notify_cb (MokoFingerScroll *scroll,
				     GdkEventMotion *event,
				     gpointer user_data)
{
	MokoFingerScrollPrivate *priv = FINGER_SCROLL_PRIVATE (scroll);
	gint dnd_threshold,ix,iy;
	gdouble x, y;

	gint  padding_top;
	gint  padding_bottom;
	gint  padding_left;
	gint  padding_right;
	
	if ((!priv->enabled) || (!priv->clicked)) return FALSE;
	
	/* GTK_POINT_MOTION_HINT_MASK require gtk_widget_get_point is called before next motion event 
	put to queue*/
	gtk_widget_get_pointer(scroll,&ix,&iy);

	/* Only start the scroll if the mouse cursor passes beyond the
	 * DnD threshold for dragging.
	 */
	g_object_get (G_OBJECT (gtk_settings_get_default ()),
		"gtk-dnd-drag-threshold", &dnd_threshold, NULL);
	x = event->x - priv->x;
	y = event->y - priv->y;
	
	if ((!priv->moved) && (
	     (ABS (x) > dnd_threshold) || (ABS (y) > dnd_threshold))) {
		priv->moved = TRUE;
		if (priv->mode != MOKO_FINGER_SCROLL_MODE_PUSH  )
		 {
			priv->idle_id = g_timeout_add ((gint)(1000.0/(gdouble)priv->sps),
				(GSourceFunc)moko_finger_scroll_timeout,
				scroll);
		}
	}
	
	if (priv->moved) {
		
		gdouble dx, dy;
	
		priv->x = event->x;
		priv->y = event->y;

		dx = x; dy=y;
		if(	!moko_finger_scroll_is_h_normall_padding(scroll))
			dx =0;
		
		if(	!moko_finger_scroll_is_v_normall_padding(scroll))
			dy =0;

		if( dx !=0 || dy !=0)
		{
		    switch (priv->mode) {
		    case MOKO_FINGER_SCROLL_MODE_PUSH :
			/* Scroll by the amount of pixels the cursor has moved
			 * since the last motion event.
			 */
			moko_finger_scroll_scroll (scroll, dx, dy, NULL, NULL);
			
			break;
		    case MOKO_FINGER_SCROLL_MODE_ACCEL :
			/* Set acceleration relative to the initial click */
			priv->vel_x = ((dx > 0) ? 1 : -1) *
				(((ABS (dx) /
				 (gdouble)GTK_WIDGET (scroll)->
				 allocation.width) *
				(priv->vmax-priv->vmin)) + priv->vmin);
			priv->vel_y = ((dy > 0) ? 1 : -1) *
				(((ABS (dy) /
				 (gdouble)GTK_WIDGET (scroll)->
				 allocation.height) *
				(priv->vmax-priv->vmin)) + priv->vmin);
			break;
		   
			case MOKO_FINGER_SCROLL_MODE_AUTO:
			//PRINT("auto %f, %f\n", x,y);
			moko_finger_scroll_scroll (scroll, dx, dy, NULL, NULL);
			break;
				
		    default :
			break;
			}
		}
		
		 gtk_alignment_get_padding(priv->align, &padding_top, &padding_bottom, &padding_left, &padding_right);

		if( (priv->vadjust->value)  <=  (priv->vadjust->lower)  ||  padding_top >0 )
		{
			if(priv->is_vscrollable)
				moko_finger_scroll_spring(scroll, y/2,  0, 0 ,0);

		}else if( ( (priv->vadjust->value + priv->vadjust->page_size)  >=  (priv->vadjust->upper) )   
				||   (padding_bottom >moko_finger_scroll_get_h_indicatior_width(scroll))
		 		)
		{
			//PRINT("spring top %d  \n", y);
			if(priv->is_vscrollable)
				moko_finger_scroll_spring(scroll, 0, -y/2, 0 ,0);
			if( y <0)
				moko_finger_scroll_scroll (scroll, x, y, NULL, NULL);
		}
		
		if( (priv->hadjust->value)  <=  (priv->hadjust->lower) || padding_left >0 )
		{
			if(priv->is_hscrollable)
				moko_finger_scroll_spring(scroll, 0,  0, x/2 ,0);

		}else 	if( ((priv->hadjust->value + priv->hadjust->page_size)  >=  (priv->hadjust->upper) )
			|| (padding_right > moko_finger_scroll_get_v_indicatior_width(scroll)) 
		     		)
		{
			if(priv->is_hscrollable)
				moko_finger_scroll_spring(scroll, 0, 0, 0 ,-x/2);
	
			if( x <0)
				moko_finger_scroll_scroll (scroll, x, y, NULL, NULL);
		}
	}
	
	return TRUE;
}

static GdkWindow *
moko_finger_scroll_get_topmost (GdkWindow *window, gint x, gint y,
				gint *tx, gint *ty)
{
	/* Find the GdkWindow at the given point, by recursing from a given
	 * parent GdkWindow. Optionally return the co-ordinates transformed
	 * relative to the child window.
	 */
	gint width, height;
	
	gdk_drawable_get_size (GDK_DRAWABLE (window), &width, &height);
	if ((x < 0) || (x >= width) || (y < 0) || (y >= height)) return NULL;
	
	/*g_debug ("Finding window at (%d, %d) in %p", x, y, window);*/
	
	while (window) {
		gint child_x, child_y;
		GList *c, *children = gdk_window_peek_children (window);
		GdkWindow *old_window = window;
		for (c = children; c; c = c->next) {
			GdkWindow *child = (GdkWindow *)c->data;
			gint wx, wy;
			
			gdk_window_get_geometry (child, &wx, &wy,
				&width, &height, NULL);
			/*g_debug ("Child: %p, (%dx%d+%d,%d)", child,
				width, height, wx, wy);*/
			
			if ((x >= wx) && (x < (wx + width)) &&
			    (y >= wy) && (y < (wy + height))) {
				child_x = x - wx; child_y = y - wy;
				window = child;
			}
		}
		/*g_debug ("\\|/");*/
		if (window == old_window) break;
		
		x = child_x;
		y = child_y;
	}
	
	if (tx) *tx = x;
	if (ty) *ty = y;

	/*g_debug ("Returning: %p", window);*/

	return window;
}

static gboolean
moko_finger_scroll_indicator_timeout (MokoFingerScroll *scroll)
{
	MokoFingerScrollPrivate *priv = FINGER_SCROLL_PRIVATE (scroll);

	PRINT("moko_finger_scroll_indicator_timeout \n");

	if(priv->clicked)
		return TRUE;
	
	if( (!moko_finger_scroll_is_normall_padding(scroll)) )
	{
		priv->indicator_off_timeout_value = priv->indicator_off_timeout;
		priv->indicator_alpha_current = priv->indicator_alpha;
	}
	
	if ((ABS (priv->vel_x) >= 1.0) || (ABS (priv->vel_y) >= 1.0))
	{
		priv->indicator_off_timeout_value = priv->indicator_off_timeout;
		priv->indicator_alpha_current = priv->indicator_alpha;
	}

	priv->indicator_off_timeout_value  -- ;
	PRINT("indicator off value %d\n", priv-> indicator_off_timeout_value );
	if(priv->indicator_off_timeout_value >=0)
		return TRUE;

	priv->indicator_alpha_current *= priv->indicator_alpha_decress_rate;

	moko_finger_scroll_redraw(scroll);
	PRINT("moko_finger_scroll_indicator_timeout \n");
	if(priv->indicator_alpha_current <0.01)
	{
		priv->indicator_timeout_id = 0;
		return FALSE;
	}
	return TRUE;
}

static gboolean
moko_finger_scroll_button_release_cb (MokoFingerScroll *scroll,
				      GdkEventButton *event,
				      gpointer user_data)
{
	MokoFingerScrollPrivate *priv = FINGER_SCROLL_PRIVATE (scroll);
	GTimeVal current;
	gdouble 	delta, speed_x, speed_y;

	if ((!priv->clicked) || (!priv->enabled) || (event->button != 1) ||
	    (event->time == priv->last_time))
		return FALSE;


	if( priv->hindicator_mode == MOKO_FINGER_SCROLL_INDICATOR_MODE_ALPHA || 
		priv->vindicator_mode == MOKO_FINGER_SCROLL_INDICATOR_MODE_ALPHA		
		)
	{
		PRINT("release add indicator out \n");
		priv->indicator_off_timeout_value = priv->indicator_off_timeout;
		priv->indicator_timeout_id = g_timeout_add ((gint)(1000.0/(gdouble)priv->sps),
				(GSourceFunc)moko_finger_scroll_indicator_timeout,
				scroll);
	}

	priv->last_time = event->time;
	g_get_current_time (&current);

	priv->clicked = FALSE;
	priv->is_acel =FALSE;
	
	delta = moko_get_time_delta( &priv->click_start , &current );
	speed_x = event->x -priv->click_x;
	speed_y = event->y -priv->click_y;
	
	speed_x=speed_x*MILLION /delta;
	speed_y=speed_y*MILLION / delta;

	//PRINT("up\n");
	//PRINT("delta %f\n", delta);
	//PRINT("speed %f %f\n", speed_x, speed_y);
	
	priv->vel_x  = speed_x * (gdouble)priv->sps /1000;	
	priv->vel_y  = speed_y * (gdouble)priv->sps /1000;

	if( ABS(priv->vel_x )<20)
	{
		//PRINT("x\n");
		priv->vel_x = 0;
	}
	if(ABS(priv->vel_y )<20)
	{	//PRINT("y\n");	
		priv->vel_y = 0;
	}
	if ((!priv->moved)/* &&
	    (!(current.tv_sec > priv->click_start.tv_sec)) &&
	    (!(current.tv_usec - priv->click_start.tv_usec > 500000))*/) {
		gint x, y;
		GdkEventCrossing *crossing_event;
		GdkWindow *child;
		
		child = moko_finger_scroll_get_topmost (
			GTK_BIN (priv->align)->child->window,
			event->x, event->y, &x, &y);

		if ((!child) || (child == GTK_BIN (priv->align)->child->window))
			return TRUE;

		event->x = x;
		event->y = y;
		
		/* Set velocity to zero, most widgets don't expect to be
		 * moving while being clicked.
		 */
		priv->vel_x = 0;
		priv->vel_y = 0;

		/* Send synthetic enter event */
		crossing_event = (GdkEventCrossing *)
			gdk_event_new (GDK_ENTER_NOTIFY);
		((GdkEventAny *)crossing_event)->type = GDK_ENTER_NOTIFY;
		((GdkEventAny *)crossing_event)->window = g_object_ref (child);
		((GdkEventAny *)crossing_event)->send_event = FALSE;
		crossing_event->subwindow = g_object_ref (child);
		crossing_event->time = event->time;
		crossing_event->x = event->x;
		crossing_event->y = event->y;
		crossing_event->x_root = event->x_root;
		crossing_event->y_root = event->y_root;
		crossing_event->mode = GDK_CROSSING_NORMAL;
		crossing_event->detail = GDK_NOTIFY_UNKNOWN;
		crossing_event->focus = FALSE;
		crossing_event->state = 0;
		gdk_event_put ((GdkEvent *)crossing_event);
		
		/* Send synthetic click (button press/release) event */
		((GdkEventAny *)event)->window = g_object_ref (child);
		((GdkEventAny *)event)->type = GDK_BUTTON_PRESS;
		gdk_event_put ((GdkEvent *)event);
		((GdkEventAny *)event)->window = g_object_ref (child);
		((GdkEventAny *)event)->type = GDK_BUTTON_RELEASE;
		gdk_event_put ((GdkEvent *)event);

		if(priv->is_double_click && priv->is_detect_double_click)
		{
			((GdkEvent *)event)->any.window = g_object_ref (child);
			((GdkEvent *)event)->type = GDK_2BUTTON_PRESS;
			gdk_event_put ((GdkEvent *)event);
			((GdkEvent *)event)->any.window = g_object_ref (child);
			((GdkEvent *)event)->type = GDK_BUTTON_RELEASE;
			gdk_event_put ((GdkEvent *)event);
		
		}
		
		/* Send synthetic leave event */
		((GdkEventAny *)crossing_event)->type = GDK_LEAVE_NOTIFY;
		((GdkEventAny *)crossing_event)->window = g_object_ref (child);
		crossing_event->subwindow = g_object_ref (child);
		crossing_event->window = g_object_ref (child);
		crossing_event->detail = GDK_NOTIFY_UNKNOWN;
		gdk_event_put ((GdkEvent *)crossing_event);
		gdk_event_free ((GdkEvent *)crossing_event);
	}
	
	return TRUE;
}

static gboolean
moko_finger_scroll_draw_spring_area(GtkWidget *widget, GdkEventExpose *event)
{
	MokoFingerScrollPrivate *priv = FINGER_SCROLL_PRIVATE (widget);
	gint  padding_top;
	gint  padding_bottom;
	gint  padding_left;
	gint  padding_right;
	GtkAllocation *allocation = &widget->allocation;

	gtk_alignment_get_padding(priv->align, &padding_top, &padding_bottom, &padding_left, &padding_right);
	if ( moko_finger_scroll_is_normall_padding( MOKO_FINGER_SCROLL(widget) ) )
		return TRUE;
	
	if( priv ->spring_gc  == NULL)
		priv ->spring_gc = gdk_gc_new(GTK_WIDGET(widget)->window);

	PRINT("spring gc %p\n", priv->spring_gc);

	
	gdk_gc_set_foreground(priv->spring_gc,&priv->spring_color);

	if( padding_top > 0 )
	{	
		PRINT("draw top spring\n");
		gdk_draw_rectangle (widget->window,
				priv->spring_gc,
				TRUE, 
				allocation->x, allocation->y,
				allocation->width, padding_top
				);
	}
	
	if( padding_left > 0 )
	{	
		PRINT("draw left spring\n");
		gdk_draw_rectangle (widget->window,
				priv->spring_gc,
				TRUE, 
				allocation->x, allocation->y,
				padding_left, allocation->height
				);
	}		

	if( padding_bottom > moko_finger_scroll_get_h_indicatior_width( MOKO_FINGER_SCROLL(widget)))
	{
		PRINT("draw bottom spring\n");
		gdk_draw_rectangle (widget->window,
				priv->spring_gc,
				TRUE, 
				allocation->x,
				allocation->y+allocation->height -padding_bottom ,
				allocation->width, 
				padding_bottom-
					moko_finger_scroll_get_h_indicatior_width( MOKO_FINGER_SCROLL(widget)) 
				);
	
	}

	if( padding_right > moko_finger_scroll_get_v_indicatior_width( MOKO_FINGER_SCROLL(widget)))
	{
		PRINT("draw right spring\n");
		gdk_draw_rectangle (widget->window,
				priv->spring_gc,
				TRUE, 
				allocation->x+allocation->width -padding_right , 
				allocation->y,
				padding_right-
					moko_finger_scroll_get_v_indicatior_width( MOKO_FINGER_SCROLL(widget)) ,
				allocation->height
				);
	
	}
	
	return TRUE;
}

#define SET_GRAY(color, value ) { color.red =(value); color.green=(value); color.blue = (value);} 
#define SET_COLOR(color, value) { color.red *=(value); color.green*=(value); color.blue *= (value);}  
static gboolean
moko_finger_scroll_draw_shadow_line(GtkWidget *widget, int x1, int y1, int x2, int y2, int dx, int dy)
{
	MokoFingerScrollPrivate *priv = FINGER_SCROLL_PRIVATE (widget);
	gint  padding_top;
	gint  padding_bottom;
	gint  padding_left;
	gint  padding_right;
	GtkAllocation *allocation = &widget->allocation;
	GdkColor color;

	priv->spring_color.red =  ((priv->spring_color.pixel>>16)&(0xFF))*(65535/255);
	priv->spring_color.green = ((priv->spring_color.pixel>>8) &0xFF)*(65535/255);
	priv->spring_color.blue =  (priv->spring_color.pixel&0xFF)*(65535/255);

	PRINT("draw top spring shadow %d %d %d %d %d %d\n", x1, y1, x2, y2, dx, dy);

	//memcpy(&color, &priv->spring_color, sizeof(GdkColor));
	color=priv->spring_color;

	//printf("color %d\n", priv->spring_color.red);
	//SET_GRAY(color, 0xCA00);
	SET_COLOR(color,0.85);

	gdk_gc_set_rgb_fg_color(priv->spring_gc,&color);
	gdk_draw_line(widget->window,	priv->spring_gc,
			x1+dx, y1+dy,
			x2+dx, y2+dy);		
		
	//SET_GRAY(color, 0xE900);
	color=priv->spring_color;
	SET_COLOR(color,0.90);
	gdk_gc_set_rgb_fg_color(priv->spring_gc,&color);
	gdk_draw_line(widget->window,	priv->spring_gc,
			x1+2*dx+dy, y1+2*dy+dx,
			x2+2*dx+dy, y2+2*dy+dx);		

	//SET_GRAY(color, 0xEF00);
	color=priv->spring_color;
	SET_COLOR(color, 0.95);
	gdk_gc_set_rgb_fg_color(priv->spring_gc,&color);
	gdk_draw_line(widget->window,	priv->spring_gc,
			x1+3*dx+2*dy, y1+3*dy+2*dx,
			x2+3*dx+2*dy, y2+3*dy+2*dx);		

	//SET_GRAY(color, 0xFA00);
	color=priv->spring_color;
	SET_COLOR(color, 0.99);
	gdk_gc_set_rgb_fg_color(priv->spring_gc,&color);
	gdk_draw_line(widget->window,	priv->spring_gc,
			x1+4*dx+3*dy, y1+4*dy+3*dx,
			x2+4*dx+3*dy, y2+4*dy+3*dx);	
}
static gboolean
moko_finger_scroll_draw_spring_shadow(GtkWidget *widget, GdkEventExpose *event)
{
	MokoFingerScrollPrivate *priv = FINGER_SCROLL_PRIVATE (widget);
	gint  padding_top;
	gint  padding_bottom;
	gint  padding_left;
	gint  padding_right;
	GtkAllocation *allocation = &widget->allocation;
	GdkColor color;

	gtk_alignment_get_padding(priv->align, &padding_top, &padding_bottom, &padding_left, &padding_right);
	if ( moko_finger_scroll_is_normall_padding( MOKO_FINGER_SCROLL(widget) ) )
		return TRUE;
	
	if( priv ->spring_gc  == NULL)
		priv ->spring_gc = gdk_gc_new(GTK_WIDGET(widget)->window);

	
	if( padding_top > 0 )
	{	
		PRINT("draw top spring shadow\n");
		moko_finger_scroll_draw_shadow_line(widget,  	allocation->x + padding_left-1, 
														allocation->y+padding_top,														
														allocation->x + allocation->width - padding_right, 
														allocation->y+padding_top, 
														0,
														 -1);
	}
	
	if( padding_left > 0 )
	{	
		moko_finger_scroll_draw_shadow_line(widget,  	allocation->x + padding_left, 
														allocation->y+padding_top-1,														
														allocation->x + padding_left,
														allocation->y+allocation->height-padding_bottom, 
														-1,
														 0);
		PRINT("draw left spring\n");
	}		

	if( padding_bottom > moko_finger_scroll_get_h_indicatior_width( MOKO_FINGER_SCROLL(widget)))
	{
		PRINT("draw bottom spring\n");
		moko_finger_scroll_draw_shadow_line(widget,  	allocation->x + padding_left, 
														allocation->y+allocation->height-padding_bottom -1,														
														allocation->x + allocation->width - padding_right , 
														allocation->y+allocation->height-padding_bottom -1 , 
														0,
														 1);
	}

	if( padding_right > moko_finger_scroll_get_v_indicatior_width( MOKO_FINGER_SCROLL(widget)))
	{
		PRINT("draw right spring\n");
		moko_finger_scroll_draw_shadow_line(widget,  	allocation->x +allocation->width- padding_right -1 , 
														allocation->y+padding_top,														
														allocation->x +allocation->width- padding_right -1,
														allocation->y+allocation->height-padding_bottom, 															1,
														0);
	}
	
	return TRUE;
}
static void
moko_finger_scroll_cairo_set_color(cairo_t *cr, GdkColor *color, double alpha)
{
	double r, g, b;
	r=color->pixel>>16; 
	r/=255.0;
	
	g=(color->pixel>>8)&0xFF;
	g/=255.0;
	
	b=color->pixel&0xFF;
	b/=255.0;
	
	cairo_set_source_rgba(cr, r, g, b, alpha);
	
}
static gboolean
moko_finger_scroll_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
	MokoFingerScrollPrivate *priv = FINGER_SCROLL_PRIVATE (widget);
	cairo_t *cr;
	
	moko_finger_scroll_draw_spring_area(widget, event);
	moko_finger_scroll_draw_spring_shadow(widget, event);
	
	PRINT("priv->vindicator_mode %d\n", priv->vindicator_mode );
	
	if(GTK_BIN (priv->align)->child && 
		(priv->vindicator_mode ==MOKO_FINGER_SCROLL_INDICATOR_MODE_ALPHA ||
		 priv->hindicator_mode ==MOKO_FINGER_SCROLL_INDICATOR_MODE_ALPHA 
				)
	    )
	{
		GdkGCValues insensitive, select;
		GdkColormap *colormap;

		gdk_gc_get_values((widget->style->fg_gc[GTK_STATE_INSENSITIVE]), &insensitive);
		gdk_gc_get_values((widget->style->base_gc[GTK_STATE_SELECTED]), &select);
		
		
		gdk_window_set_composited(GTK_BIN (priv->align)->child->window, TRUE);
		cr=gdk_cairo_create(widget->window);
		gdk_cairo_set_source_pixmap(cr, GTK_BIN (priv->align)->child->window,
									          GTK_BIN (priv->align)->child->allocation.x,	
										  GTK_BIN (priv->align)->child->allocation.y);

		cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
		cairo_paint_with_alpha(cr, 0.99); //cairo 1.5.8 don't work with 1.0

		cairo_set_line_cap(cr,     CAIRO_LINE_CAP_ROUND);
		cairo_set_line_width(cr, priv->scroll_width);
		
		if(priv->vscroll)
		{
			gint y, height, x;
	
			y = widget->allocation.y +	
					((priv->vadjust->value/priv->vadjust->upper)*
					 (widget->allocation.height -
					  (priv->hscroll ? priv->scroll_width : 0)));
			height = (widget->allocation.y +
					(((priv->vadjust->value +
					   priv->vadjust->page_size)/
					  priv->vadjust->upper)*
					 (widget->allocation.height -
					  (priv->hscroll ? priv->scroll_width : 0)))) -
					  y;
			
			x = priv->vscroll_rect.x + priv->scroll_width/2;

			moko_finger_scroll_cairo_set_color(cr,&insensitive.foreground, priv->indicator_alpha_current);
	
			cairo_move_to(cr, 	x,  priv->vscroll_rect.y +priv->scroll_width/2);
			cairo_line_to(cr, 	x, priv->vscroll_rect.y+priv->vscroll_rect.height -priv->scroll_width/2);

			cairo_stroke(cr);

			moko_finger_scroll_cairo_set_color(cr,&select.foreground, priv->indicator_alpha_current);
			
			y+=priv->scroll_width/2;
			height -= priv->scroll_width;
			if(height<0)
				height=0;

			cairo_move_to(cr, x, y );
			cairo_line_to(cr, x, y+height);

			cairo_stroke(cr);

		}
		
		if (priv->hscroll) 
		{
			gint x, width,y;
				
			x = widget->allocation.x +
					((priv->hadjust->value/priv->hadjust->upper)*
					 (widget->allocation.width  -
					  (priv->vscroll ? priv->scroll_width : 0)));
			width = (widget->allocation.x +
				(((priv->hadjust->value +
				   priv->hadjust->page_size)/
				  priv->hadjust->upper)*
				 (widget->allocation.width -
				  (priv->vscroll ? priv->scroll_width : 0)))) -
				  x;

			y = 	priv->hscroll_rect.y + priv->scroll_width/2;			

			moko_finger_scroll_cairo_set_color(cr,&insensitive.foreground, priv->indicator_alpha_current);
	
			cairo_move_to(cr,  priv->hscroll_rect.x+priv->scroll_width/2, y);
			cairo_line_to(cr,  priv->hscroll_rect.x+priv->hscroll_rect.width-priv->scroll_width/2,y);

			cairo_stroke(cr);

			
			moko_finger_scroll_cairo_set_color(cr,&select.foreground, priv->indicator_alpha_current);

			x += priv->scroll_width/2;
			width -=  priv->scroll_width;
			if(width <0)
				width = 0;

			cairo_move_to(cr, x, y);
			cairo_line_to(cr, x+width, y);

			cairo_stroke(cr);

				
		}

		cairo_destroy(cr);
	}
	else
	{
		if (GTK_BIN (priv->align)->child) {
			if (priv->vscroll) {
				gint y, height;
				gdk_draw_rectangle (widget->window,
					widget->style->fg_gc[GTK_STATE_INSENSITIVE],
					TRUE,
					priv->vscroll_rect.x , priv->vscroll_rect.y,
					priv->vscroll_rect.width,
					priv->vscroll_rect.height);
			
				y = widget->allocation.y +	
					((priv->vadjust->value/priv->vadjust->upper)*
					 (widget->allocation.height -
					  (priv->hscroll ? priv->scroll_width : 0)));
				height = (widget->allocation.y +
					(((priv->vadjust->value +
					   priv->vadjust->page_size)/
					  priv->vadjust->upper)*
					 (widget->allocation.height -
					  (priv->hscroll ? priv->scroll_width : 0)))) -
					  y;
			
				gdk_draw_rectangle (widget->window,
					widget->style->base_gc[GTK_STATE_SELECTED],
					TRUE, priv->vscroll_rect.x, y,
					priv->vscroll_rect.width, height);
				}
		
			if (priv->hscroll) {
				gint x, width;
				gdk_draw_rectangle (widget->window,
					widget->style->fg_gc[GTK_STATE_INSENSITIVE],
					TRUE,
					priv->hscroll_rect.x, priv->hscroll_rect.y,
					priv->hscroll_rect.width,
					priv->hscroll_rect.height);

				x = widget->allocation.x +
					((priv->hadjust->value/priv->hadjust->upper)*
					 (widget->allocation.width  -
					  (priv->vscroll ? priv->scroll_width : 0)));
				width = (widget->allocation.x +
				(((priv->hadjust->value +
				   priv->hadjust->page_size)/
				  priv->hadjust->upper)*
				 (widget->allocation.width -
				  (priv->vscroll ? priv->scroll_width : 0)))) -
				  x;

				gdk_draw_rectangle (widget->window,
					widget->style->base_gc[GTK_STATE_SELECTED],
					TRUE, x, priv->hscroll_rect.y, width,
					priv->hscroll_rect.height);
			}
		}
	}
	return GTK_WIDGET_CLASS (
		moko_finger_scroll_parent_class)->expose_event (widget, event);
}

static void
moko_finger_scroll_destroy (GtkObject *object)
{
	MokoFingerScrollPrivate *priv = FINGER_SCROLL_PRIVATE (object); 

	PRINT("moko_finger_scroll_destroy %p\n", object);

	if (priv->hadjust) {
		g_object_unref (G_OBJECT (priv->hadjust));
		priv->hadjust = NULL;
	}

	if (priv->vadjust) {
		g_object_unref (G_OBJECT (priv->vadjust));
		priv->vadjust = NULL;
	}
	
	if (priv->spring_gc)
	{
		gdk_gc_unref(priv->spring_gc);	
		priv->spring_gc = NULL;
	}
	
	if (priv->indicator_timeout_id )
	{
		PRINT("remove indicator timeout id %d\n", priv->indicator_timeout_id);

		g_source_remove(priv->indicator_timeout_id);
		priv->indicator_timeout_id = 0;
	}
	
	if (priv->idle_id )
	{
		PRINT("remove idle id %d\n", priv->idle_id );
		g_source_remove(priv->idle_id);
		priv->idle_id = 0;
	}
	GTK_OBJECT_CLASS (moko_finger_scroll_parent_class)->destroy (object);
}

static void
moko_finger_scroll_remove_cb (GtkContainer *container,
			      GtkWidget    *child,
			      MokoFingerScroll *scroll)
{
	g_signal_handlers_disconnect_by_func (child,
		moko_finger_scroll_refresh, scroll);
	g_signal_handlers_disconnect_by_func (child,
		gtk_widget_queue_resize, scroll);
}

static void
moko_finger_scroll_add (GtkContainer *container,
			GtkWidget    *child)
{
	MokoFingerScrollPrivate *priv = FINGER_SCROLL_PRIVATE (container);
	
	gtk_container_add (GTK_CONTAINER (priv->align), child);
	g_signal_connect_swapped (G_OBJECT (child), "size-allocate",
		G_CALLBACK (moko_finger_scroll_refresh), container);
	g_signal_connect_swapped (G_OBJECT (child), "size-request",
		G_CALLBACK (gtk_widget_queue_resize), container);

	if (!gtk_widget_set_scroll_adjustments (child, priv->hadjust, priv->vadjust))
		g_warning("%s: cannot add non scrollable widget, wrap it in a viewport", __FUNCTION__);
}

static void
moko_finger_scroll_get_property (GObject * object, guint property_id,
				 GValue * value, GParamSpec * pspec)
{
	MokoFingerScrollPrivate *priv = FINGER_SCROLL_PRIVATE (object);

	switch (property_id) {
	    case PROP_ENABLED :
		g_value_set_boolean (value, priv->enabled);
		break;
	    case PROP_MODE :
		g_value_set_int (value, priv->mode);
		break;
	    case PROP_VELOCITY_MIN :
		g_value_set_double (value, priv->vmin);
		break;
	    case PROP_VELOCITY_MAX :
		g_value_set_double (value, priv->vmax);
		break;
	    case PROP_DECELERATION :
		g_value_set_double (value, priv->decel);
		break;
	    case PROP_SPS :
		g_value_set_uint (value, priv->sps);
		break;
	    case PROP_VINDICATOR:
		g_value_set_int (value, priv->vindicator_mode);
		break;
	    case PROP_HINDICATOR:
		g_value_set_int (value, priv->hindicator_mode);
		break;
	    case PROP_SPRING_SPEED:
		g_value_set_double(value, priv->spring_decel);
		break;
	    case PROP_IS_DETECT_DOUBLE_CLICK:
		g_value_set_boolean (value, priv->is_detect_double_click);
	 	break;
	   case PROP_SPRING_COLOR:
		g_value_set_uint (value, priv->spring_color.pixel);
		break;
	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
moko_finger_scroll_set_property (GObject * object, guint property_id,
				 const GValue * value, GParamSpec * pspec)
{
	MokoFingerScrollPrivate *priv = FINGER_SCROLL_PRIVATE (object);
	switch (property_id) {
	    case PROP_ENABLED :
		priv->enabled = g_value_get_boolean (value);
		gtk_event_box_set_above_child (
			GTK_EVENT_BOX (object), priv->enabled);
		break;
	    case PROP_MODE :
		priv->mode = g_value_get_int (value);
		break;
	    case PROP_VELOCITY_MIN :
		priv->vmin = g_value_get_double (value);
		break;
	    case PROP_VELOCITY_MAX :
		priv->vmax = g_value_get_double (value);
		break;
	    case PROP_DECELERATION :
		priv->decel = g_value_get_double (value);
		break;
	    case PROP_SPS :
		priv->sps = g_value_get_uint (value);
		break;

	    case PROP_VINDICATOR:
		priv->vindicator_mode=g_value_get_int(value );
		moko_finger_scroll_refresh(object);
		break;
	    case PROP_HINDICATOR:
		priv->hindicator_mode=g_value_get_int(value );
		moko_finger_scroll_refresh(object);
		break;
	    
            case PROP_SPRING_SPEED:
		priv->spring_decel = g_value_get_double(value);
		break;
	
	    case PROP_IS_DETECT_DOUBLE_CLICK:
		priv->is_detect_double_click = g_value_get_boolean(value );
		break;

	    case  PROP_SPRING_COLOR:
		priv->spring_color.pixel=g_value_get_uint(value );
		break;

	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
moko_finger_scroll_dispose (GObject * object)
{
	if (G_OBJECT_CLASS (moko_finger_scroll_parent_class)->dispose)
		G_OBJECT_CLASS (moko_finger_scroll_parent_class)->
			dispose (object);
}

static void
moko_finger_scroll_finalize (GObject * object)
{
	G_OBJECT_CLASS (moko_finger_scroll_parent_class)->finalize (object);
}

static void
moko_finger_scroll_size_request (GtkWidget      *widget,
				 GtkRequisition *requisition)
{
	/* Request tiny size, seeing as we have no decoration of our own.
	 */
	requisition->width = 32;
	requisition->height = 32;
}

static void
moko_finger_scroll_style_set (GtkWidget *widget, GtkStyle *previous_style)
{
	MokoFingerScrollPrivate *priv = FINGER_SCROLL_PRIVATE (widget);

	GTK_WIDGET_CLASS (moko_finger_scroll_parent_class)->
		style_set (widget, previous_style);
	
	gtk_widget_style_get (widget, "indicator-width", &priv->scroll_width,
		NULL);
}

static void
moko_finger_scroll_class_init (MokoFingerScrollClass * klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

	g_type_class_add_private (klass, sizeof (MokoFingerScrollPrivate));

	object_class->get_property = moko_finger_scroll_get_property;
	object_class->set_property = moko_finger_scroll_set_property;
	object_class->dispose = moko_finger_scroll_dispose;
	object_class->finalize = moko_finger_scroll_finalize;

	gtkobject_class->destroy = moko_finger_scroll_destroy;
	
	widget_class->size_request = moko_finger_scroll_size_request;
	widget_class->expose_event = moko_finger_scroll_expose_event;
	widget_class->style_set = moko_finger_scroll_style_set;
	
	container_class->add = moko_finger_scroll_add;

	g_object_class_install_property (
		object_class,
		PROP_ENABLED,
		g_param_spec_boolean (
			"enabled",
			"Enabled",
			"Enable or disable finger-scroll.",
			TRUE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property (
		object_class,
		PROP_VINDICATOR,
		g_param_spec_int (
			"vindicator_mode",
			"vindicator mode",
			"vindicator mode",
			MOKO_FINGER_SCROLL_INDICATOR_MODE_AUTO,
			MOKO_FINGER_SCROLL_INDICATOR_MODE_ALPHA,
			MOKO_FINGER_SCROLL_INDICATOR_MODE_ALPHA,
			G_PARAM_READWRITE | G_PARAM_CONSTRUCT));


	g_object_class_install_property (
		object_class,
		PROP_HINDICATOR,
		g_param_spec_int (
			"hindicator_mode",
			"hindicator_mode",
			"hindicator_mode",
			MOKO_FINGER_SCROLL_INDICATOR_MODE_AUTO,
			MOKO_FINGER_SCROLL_INDICATOR_MODE_ALPHA,
			MOKO_FINGER_SCROLL_INDICATOR_MODE_ALPHA,
			G_PARAM_READWRITE | G_PARAM_CONSTRUCT));


	
	g_object_class_install_property (
		object_class,
		PROP_MODE,
		g_param_spec_int (
			"mode",
			"Scroll mode",
			"Change the finger-scrolling mode.",
			MOKO_FINGER_SCROLL_MODE_PUSH,
			MOKO_FINGER_SCROLL_MODE_AUTO,
			MOKO_FINGER_SCROLL_MODE_AUTO,
			G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property (
		object_class,
		PROP_VELOCITY_MIN,
		g_param_spec_double (
			"velocity_min",
			"Minimum scroll velocity",
			"Minimum distance the child widget should scroll "
				"per 'frame', in pixels.",
			0, G_MAXDOUBLE, 0,
			G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property (
		object_class,
		PROP_VELOCITY_MAX,
		g_param_spec_double (
			"velocity_max",
			"Maximum scroll velocity",
			"Maximum distance the child widget should scroll "
				"per 'frame', in pixels.",
			0, G_MAXDOUBLE, 48,
			G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property (
		object_class,
		PROP_DECELERATION,
		g_param_spec_double (
			"deceleration",
			"Deceleration multiplier",
			"The multiplier used when decelerating when in "
				"acceleration scrolling mode.",
			0, 1.0, 0.95,
			G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
	

	g_object_class_install_property (
		object_class,
		PROP_SPRING_SPEED,
		g_param_spec_double (
			"spring_speed",
			"Deceleration spring multiplier",
			"The multiplier used when decelerating when in spring",
			0, 1.0, 0.85,
			G_PARAM_READWRITE | G_PARAM_CONSTRUCT));


	g_object_class_install_property (
		object_class,
		PROP_SPS,
		g_param_spec_uint (
			"sps",
			"Scrolls per second",
			"Amount of scroll events to generate per second.",
			0, G_MAXUINT, 15,
			G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
	
	gtk_widget_class_install_style_property (
		widget_class,
		g_param_spec_uint (
			"indicator-width",
			"Width of the scroll indicators",
			"Pixel width used to draw the scroll indicators.",
			0, G_MAXUINT, 6,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_IS_DETECT_DOUBLE_CLICK,
		g_param_spec_boolean (
			"is_detect_double_click",
			"is_detect_double_click",
			"enable and disable double click detected",
			TRUE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property (
		object_class,
		PROP_SPRING_COLOR,
		g_param_spec_uint (
			"spring_color",
			"spring area color",
			"spring area color",
			0, G_MAXUINT, 0xFFFFFFFF,
			G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	 
}

static void
moko_finger_scroll_init (MokoFingerScroll * self)
{
	MokoFingerScrollPrivate *priv = FINGER_SCROLL_PRIVATE (self);
	int h, v;
	
	priv->moved = FALSE;
	priv->clicked = FALSE;
	priv->last_time = 0;
	priv->vscroll = TRUE;
	priv->hscroll = TRUE;
	priv->scroll_width = 6;
	priv->h_spring  = 0;
	priv->v_spring = 0;
	priv->spring_decel = 0.8;

	priv->spring_color.pixel = 0x00FFFFFF;
	
	priv->indicator_alpha = 0.7;
	priv->indicator_alpha_current=0;
	priv->indicator_alpha_decress_rate = 0.95;
	
	priv->indicator_off_timeout = 40;

	gtk_event_box_set_above_child (GTK_EVENT_BOX (self), TRUE);
	gtk_event_box_set_visible_window (GTK_EVENT_BOX (self), FALSE);
	
	priv->align = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
	GTK_CONTAINER_CLASS (moko_finger_scroll_parent_class)->add (
		GTK_CONTAINER (self), priv->align);

	priv->idle_id = 0;
	priv->indicator_timeout_id = 0;

	//h =   priv->scroll_width;
	//v =   priv->scroll_width;

	//if( priv->hindicator_mode == MOKO_FINGER_SCROLL_INDICATOR_MODE_ALPHA)
	//	h =0;
//	if( priv->vindicator_mode == MOKO_FINGER_SCROLL_INDICATOR_MODE_ALPHA)
	//	v = 0;

	//gtk_alignment_set_padding (GTK_ALIGNMENT (priv->align),
	//	0, h, 0, v);

	gtk_widget_show (priv->align);
	
	gtk_widget_add_events (GTK_WIDGET (self), GDK_POINTER_MOTION_HINT_MASK);

	priv->hadjust = GTK_ADJUSTMENT (
		gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
	priv->vadjust = GTK_ADJUSTMENT (
		gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));

	g_object_ref_sink (G_OBJECT (priv->hadjust));
	g_object_ref_sink (G_OBJECT (priv->vadjust));
	
	g_signal_connect (G_OBJECT (self), "button-press-event",
		G_CALLBACK (moko_finger_scroll_button_press_cb), NULL);
	g_signal_connect (G_OBJECT (self), "button-release-event",
		G_CALLBACK (moko_finger_scroll_button_release_cb), NULL);
	g_signal_connect (G_OBJECT (self), "motion-notify-event",
		G_CALLBACK (moko_finger_scroll_motion_notify_cb), NULL);
	g_signal_connect (G_OBJECT (priv->align), "remove",
		G_CALLBACK (moko_finger_scroll_remove_cb), self);

	g_signal_connect_swapped (G_OBJECT (priv->hadjust), "changed",
		G_CALLBACK (moko_finger_scroll_refresh), self);
	g_signal_connect_swapped (G_OBJECT (priv->vadjust), "changed",
		G_CALLBACK (moko_finger_scroll_refresh), self);
	g_signal_connect_swapped (G_OBJECT (priv->hadjust), "value-changed",
		G_CALLBACK (moko_finger_scroll_redraw), self);
	g_signal_connect_swapped (G_OBJECT (priv->vadjust), "value-changed",
		G_CALLBACK (moko_finger_scroll_redraw), self);
}

/**
 * moko_finger_scroll_new:
 * 
 * Create a new finger scroll widget
 *
 * Returns: the newly created #MokoFingerScroll
 */

GtkWidget *
moko_finger_scroll_new (void)
{
	return g_object_new (MOKO_TYPE_FINGER_SCROLL, NULL);
}

/**
 * moko_finger_scroll_new_full:
 * @mode: #MokoFingerScrollMode
 * @enabled: Value for the enabled property
 * @vel_min: Value for the velocity-min property
 * @vel_max: Value for the velocity-max property
 * @decel: Value for the deceleration property
 * @sps: Value for the sps property
 *
 * Create a new #MokoFingerScroll widget and set various properties
 *
 * returns: the newly create #MokoFingerScrull
 */

GtkWidget *
moko_finger_scroll_new_full (gint mode, gboolean enabled,
			     gdouble vel_min, gdouble vel_max,
			     gdouble decel, guint sps)
{
	return g_object_new (MOKO_TYPE_FINGER_SCROLL,
			     "mode", mode,
			     "enabled", enabled,
			     "velocity_min", vel_min,
			     "velocity_max", vel_max,
			     "deceleration", decel,
			     "sps", sps,
			     NULL);
}

/**
 * moko_finger_scroll_add_with_viewport:
 * @scroll: A #MokoFingerScroll
 * @child: Child widget to add to the viewport
 *
 * Convenience function used to add a child to a #GtkViewport, and add the
 * viewport to the scrolled window.
 */

void
moko_finger_scroll_add_with_viewport (MokoFingerScroll *scroll, GtkWidget *child)
{
	GtkWidget *viewport = gtk_viewport_new (NULL, NULL);
	gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport), GTK_SHADOW_NONE);
	gtk_container_add (GTK_CONTAINER (viewport), child);
	gtk_widget_show (viewport);
	gtk_container_add (GTK_CONTAINER (scroll), viewport);
}



GType moko_finger_scroll_mode_get_type(void)
{
	static GType etype = 0;
	if (etype  == 0) {
		static const GEnumValue values[] = 
		{
			{MOKO_FINGER_SCROLL_MODE_PUSH, "MOKO_FINGER_SCROLL_MODE_PUSH", ""},
			{MOKO_FINGER_SCROLL_MODE_ACCEL, "MOKO_FINGER_SCROLL_MODE_ACCEL", ""},
			{MOKO_FINGER_SCROLL_MODE_AUTO, "MOKO_FINGER_SCROLL_MODE_AUTO", ""},
			{0, NULL, NULL}
		};
	
		etype = g_flags_register_static (g_intern_static_string("MokoFingerScrollMode"),values);
	}
	return etype;
}


GType moko_finger_scroll_indicator_mode_get_type(void)
{
	static GType etype = 0;
	if (etype  == 0) {
		static const GEnumValue values[] = 
		{
			{MOKO_FINGER_SCROLL_INDICATOR_MODE_AUTO, "MOKO_FINGER_SCROLL_INDICATOR_MODE_AUTO", ""},
			{MOKO_FINGER_SCROLL_INDICATOR_MODE_SHOW, "MOKO_FINGER_SCROLL_INDICATOR_MODE_SHOW", ""},
			{MOKO_FINGER_SCROLL_INDICATOR_MODE_HIDE,  "MOKO_FINGER_SCROLL_INDICATOR_MODE_HIDE", ""},
			{MOKO_FINGER_SCROLL_INDICATOR_MODE_ALPHA, "MOKO_FINGER_SCROLL_INDICATOR_MODE_ALPHA",""},
			{0, NULL, NULL}
		};
	
		etype = g_flags_register_static (g_intern_static_string("MokoFingerScrollIndicatorMode"),values);
	}
	return etype;
}
