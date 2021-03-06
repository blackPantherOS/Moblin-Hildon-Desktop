/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2007 Nokia Corporation.
 *
 * Author:  Johan Bilien <johan.bilien@nokia.com>
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

#include "hildon-desktop-panel-window-composite.h"

#include <libhildonwm/hd-wm.h>

#include <libhildondesktop/hildon-desktop-picture.h>

#ifdef HAVE_X_COMPOSITE
#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xdamage.h>

static void
hildon_desktop_panel_window_composite_style_set (GtkWidget     *widget,
                                                 GtkStyle      *old_style);

static gboolean
hildon_desktop_panel_window_composite_expose (GtkWidget             *widget,
                                              GdkEventExpose        *event);

static void
hildon_desktop_panel_window_composite_realize (GtkWidget       *widget);

static void
hildon_desktop_panel_window_composite_unrealize (GtkWidget       *widget);

static gboolean
hildon_desktop_panel_window_composite_configure (GtkWidget             *widget,
                                                 GdkEventConfigure     *event);
static void
desktop_window_changed (DesktopWindowData *data);

struct _HildonDesktopPanelWindowCompositePrivate
{
  Picture       background_picture;
  guint         background_width, background_height;

  Picture       pattern_picture;
  Picture       pattern_mask;
  gint          pattern_width, pattern_height;

  gint          x, y, width, height;

  XTransform    transform;
  gboolean      scale;

};

struct _DesktopWindowData
{
  gboolean      composite;
  Window        home_window;
  Damage        home_damage;
  GdkWindow    *home_gwindow;
  Picture       home_picture;
  gint          ref_count;
  guint         desktop_window_changed_handler;
  GSList       *instances;
  int           xdamage_event_base;

};
#endif

static void
hildon_desktop_panel_window_composite_class_init (HildonDesktopPanelWindowCompositeClass *klass);

static void
hildon_desktop_panel_window_composite_base_init (HildonDesktopPanelWindowCompositeClass *klass);

static void
hildon_desktop_panel_window_composite_base_finalize (HildonDesktopPanelWindowCompositeClass *klass);

static void
hildon_desktop_panel_window_composite_init (HildonDesktopPanelWindowComposite *window);

static HildonDesktopPanelWindowClass   *parent_class = NULL;

GType hildon_desktop_panel_window_composite_get_type(void)
{
  static GType window_type = 0;

  if (!window_type) {
    static const GTypeInfo window_info = {
      sizeof(HildonDesktopPanelWindowCompositeClass),
      (GBaseInitFunc) hildon_desktop_panel_window_composite_base_init,
      (GBaseFinalizeFunc) hildon_desktop_panel_window_composite_base_finalize,
      (GClassInitFunc) hildon_desktop_panel_window_composite_class_init,
      NULL,       /* class_finalize */
      NULL,       /* class_data */
      sizeof(HildonDesktopPanelWindowComposite),
      0,  /* n_preallocs */
      (GInstanceInitFunc) hildon_desktop_panel_window_composite_init,
    };
    window_type = g_type_register_static(HILDON_DESKTOP_TYPE_PANEL_WINDOW,
                                         "HildonDesktopPanelWindowComposite",
                                         &window_info, 0);
  }
  return window_type;
}

static void
hildon_desktop_panel_window_composite_base_init (HildonDesktopPanelWindowCompositeClass *klass)
{
#ifdef HAVE_X_COMPOSITE
  static DesktopWindowData     *data = NULL;

  if (!data)
  {
    gint        damage_error, composite_error;
    gint        composite_event_base;

    data = g_new0 (DesktopWindowData, 1);

    if (XDamageQueryExtension (GDK_DISPLAY (),
                               &data->xdamage_event_base,
                               &damage_error) &&

        XCompositeQueryExtension (GDK_DISPLAY (),
                                  &composite_event_base,
                                  &composite_error))
    {
      HDWM *wm;

      data->composite = TRUE;

      gdk_x11_register_standard_event_type (gdk_display_get_default (),
                                            data->xdamage_event_base +
                                            XDamageNotify,
                                            1);
      wm = hd_wm_get_singleton ();

      data->desktop_window_changed_handler =
          g_signal_connect_swapped (wm, "notify::desktop-window",
                                    G_CALLBACK (desktop_window_changed),
                                    data);
    }
  }

  klass->desktop_window_data = data;
  data->ref_count ++;
#endif
}

static void
hildon_desktop_panel_window_composite_base_finalize (HildonDesktopPanelWindowCompositeClass *klass)
{
#ifdef HAVE_X_COMPOSITE
  if (klass->desktop_window_data)
  {
    klass->desktop_window_data->ref_count --;
    if (klass->desktop_window_data->ref_count == 0)
    {
      if (klass->desktop_window_data->composite)
      {
        HDWM     *wm = hd_wm_get_singleton ();
        g_signal_handler_disconnect (wm,
                                     klass->desktop_window_data->desktop_window_changed_handler);
      }
      g_free (klass->desktop_window_data);
    }

    klass->desktop_window_data = NULL;
  }
#endif

}

static void
hildon_desktop_panel_window_composite_init (HildonDesktopPanelWindowComposite *window)
{
#ifdef HAVE_X_COMPOSITE
  window->priv = G_TYPE_INSTANCE_GET_PRIVATE (window, HILDON_DESKTOP_TYPE_PANEL_WINDOW_COMPOSITE, HildonDesktopPanelWindowCompositePrivate);

  gtk_widget_set_app_paintable (GTK_WIDGET (window), TRUE);
#endif


}

static void
hildon_desktop_panel_window_composite_class_init (HildonDesktopPanelWindowCompositeClass *klass)
{
#ifdef HAVE_X_COMPOSITE
  {
    if (klass->desktop_window_data->composite)
    {
      GtkWidgetClass   *widget_class;

      widget_class = GTK_WIDGET_CLASS (klass);

      widget_class->style_set =
          hildon_desktop_panel_window_composite_style_set;
      widget_class->realize = hildon_desktop_panel_window_composite_realize;
      widget_class->unrealize = hildon_desktop_panel_window_composite_unrealize;
      widget_class->expose_event =
          hildon_desktop_panel_window_composite_expose;
      widget_class->configure_event       =
          hildon_desktop_panel_window_composite_configure;

    }

    g_type_class_add_private (klass,
                              sizeof (HildonDesktopPanelWindowCompositePrivate));

  }
#endif

  parent_class = g_type_class_peek_parent (klass);

}

#ifdef HAVE_X_COMPOSITE

static void
hildon_desktop_panel_window_composite_update_background (HildonDesktopPanelWindowComposite *window,
                                                         GdkRectangle *area)
{
  HildonDesktopPanelWindowCompositePrivate     *priv = window->priv;
  HildonDesktopPanelWindowCompositeClass       *klass =
      HILDON_DESKTOP_PANEL_WINDOW_COMPOSITE_GET_CLASS (window);
  DesktopWindowData                            *data =
        klass->desktop_window_data;

  if (!priv->width || !priv->height) return;

  if (priv->background_picture == None ||
      priv->width != priv->background_width ||
      priv->height != priv->background_height)
  {
    const GdkColor      white = { 0xFFFFFFFF, 0xFFFF, 0xFFFF, 0xFFFF };

    if (priv->background_picture != None)
      XRenderFreePicture (GDK_DISPLAY (), priv->background_picture);

    priv->background_picture = hildon_desktop_picture_from_color (&white,
                                                                  priv->width,
                                                                  priv->height);

    g_return_if_fail (priv->background_picture != None);

    priv->background_width  = priv->width;
    priv->background_height = priv->height;

    area->x = 0;
    area->y = 0;
    area->width  = priv->width;
    area->height = priv->height;
  }

  if (data->home_picture != None)

  {
    XRenderComposite (GDK_DISPLAY (),
                      PictOpSrc,
                      data->home_picture,
                      None,
                      priv->background_picture,
                      priv->x + area->x, priv->y + area->y,
                      priv->x + area->x, priv->y + area->y,
                      area->x,
                      area->y,
                      area->width,
                      area->height);
  }

  if (priv->pattern_picture != None)
  {
    if (priv->scale)
    {
      XRenderSetPictureTransform (GDK_DISPLAY (),
                                  priv->pattern_picture,
                                  &priv->transform);

      if (priv->pattern_mask != None)
        XRenderSetPictureTransform (GDK_DISPLAY (),
                                    priv->pattern_mask,
                                    &priv->transform);
    }

    XRenderComposite (GDK_DISPLAY (),
                      PictOpOver,
                      priv->pattern_picture,
                      priv->pattern_mask,
                      priv->background_picture,
                      area->x, area->y,
                      area->x, area->y,
                      area->x, area->y,
                      area->width, area->height);

  }

  gdk_window_invalidate_rect (GTK_WIDGET (window)->window,
                              area,
                              TRUE);

}

static GdkFilterReturn
home_window_filter (GdkXEvent          *xevent,
                    GdkEvent           *event,
                    DesktopWindowData  *data)
{
  XEvent                                       *e = xevent;

  if (e->type == data->xdamage_event_base + XDamageNotify)
  {
    XserverRegion             parts;
    XDamageNotifyEvent       *ev = xevent;
    XRectangle               *rects;
    gint                     i, n_rect;

    parts = XFixesCreateRegion (GDK_DISPLAY (), 0, 0);

    XDamageSubtract (GDK_DISPLAY (), ev->damage, None, parts);

    rects = XFixesFetchRegion (GDK_DISPLAY (),
                               parts,
                               &n_rect);

    XFixesDestroyRegion (GDK_DISPLAY (),
                         parts);

    for (i = 0; i < n_rect; i++)
    {
      GSList   *w;
      for (w = data->instances; w ; w = w->next)
      {
        HildonDesktopPanelWindowComposite              *window = w->data;
        HildonDesktopPanelWindowCompositePrivate       *priv = window->priv;

        if (priv->x + priv->width > rects[i].x      &&
            priv->x < rects[i].x + rects[i].width   &&
            priv->y + priv->height > rects[i].y     &&
            priv->y < rects[i].y + rects[i].height)

        {
          GdkRectangle rect;

          /* Take the intersection, and offset it back
           * to relate to the Panel window geometry */
          rect.x = MAX (rects[i].x, priv->x) - priv->x;
          rect.y = MAX (rects[i].y, priv->y) - priv->y;
          rect.width =
              MIN (rects[i].x + rects[i].width,  priv->x + priv->width) -
              MAX (rects[i].x, priv->x);
          rect.height =
              MIN (rects[i].y + rects[i].height,  priv->y + priv->height) -
              MAX (rects[i].y, priv->y);

          hildon_desktop_panel_window_composite_update_background (window,
                                                                   &rect);
        }
      }
    }

    XFree (rects);
  }

  return GDK_FILTER_CONTINUE;
}

static void
desktop_window_changed (DesktopWindowData      *data)
{
  HDWM         *wm;
  Window        desktop_window;

  wm = hd_wm_get_singleton ();

  g_object_get (wm,
                "desktop-window", &desktop_window,
                NULL);

  if (desktop_window == data->home_window)
    return;

  data->home_window = desktop_window;

  if (data->home_picture != None)
  {
    XRenderFreePicture (GDK_DISPLAY (), data->home_picture);
    data->home_picture = None;
  }

  if (data->home_damage != None)
  {
    XDamageDestroy (GDK_DISPLAY (),
                    data->home_damage);
    data->home_damage = None;
  }

  if (GDK_IS_WINDOW (data->home_gwindow))
  {
    g_object_unref (data->home_gwindow);
    data->home_gwindow = NULL;
  }

  if (desktop_window != None)
  {
    gdk_error_trap_push ();

    XCompositeRedirectWindow (GDK_DISPLAY (),
                              desktop_window,
                              CompositeRedirectAutomatic);

    data->home_damage = XDamageCreate (GDK_DISPLAY (),
                                       desktop_window,
                                       XDamageReportNonEmpty);

    if (gdk_error_trap_pop ())
    {
      g_warning ("Could not redirect the desktop "
                 "window");
      return;
    }

    data->home_gwindow = gdk_window_foreign_new (desktop_window);

    if (GDK_IS_WINDOW (data->home_gwindow))
    {
      data->home_picture =
          hildon_desktop_picture_from_drawable (data->home_gwindow);

      gdk_window_add_filter (data->home_gwindow,
                             (GdkFilterFunc)
                             home_window_filter,
                             data);
    }
  }


}

static gboolean
hildon_desktop_panel_window_composite_configure (GtkWidget             *widget,
                                                 GdkEventConfigure     *event)
{
  HildonDesktopPanelWindowCompositePrivate *priv =
      HILDON_DESKTOP_PANEL_WINDOW_COMPOSITE (widget)->priv;

  priv->x = event->x;
  priv->y = event->y;
  priv->width = event->width;
  priv->height = event->height;

  if (priv->pattern_width != priv->width ||
      priv->pattern_height != priv->height)
  {
    XTransform scale = {{{ XDoubleToFixed ((gdouble)priv->background_width /priv->width), 0, 0},
                   {0, XDoubleToFixed ((gdouble)priv->background_height / priv->height), 0},
                   {0, 0, XDoubleToFixed (1.0)}}};

    priv->transform = scale;
    priv->scale = TRUE;
  }
  else
    priv->scale = FALSE;

  return GTK_WIDGET_CLASS (parent_class)->configure_event (widget, event);

}

static gboolean
hildon_desktop_panel_window_composite_expose (GtkWidget *widget,
                                              GdkEventExpose *event)
{

#if 0
  g_debug ("Got expose event on %s, area %i,%i %ix%i",
           gtk_widget_get_name (widget),
           event->area.x,
           event->area.y,
           event->area.width,
           event->area.height);
#endif
  if (GTK_WIDGET_DRAWABLE (widget))
  {
    HildonDesktopPanelWindowCompositePrivate   *priv =
        HILDON_DESKTOP_PANEL_WINDOW_COMPOSITE (widget)->priv;
    GdkDrawable *drawable;
    gint x_offset, y_offset;
    Picture picture;
    gboolean result;

    gdk_window_get_internal_paint_info (widget->window,
                                        &drawable,
                                        &x_offset,
                                        &y_offset);

    picture = hildon_desktop_picture_from_drawable (drawable);
    g_return_val_if_fail (picture, FALSE);

    g_object_set_data (G_OBJECT (drawable),
                       "picture", GINT_TO_POINTER (picture));

    if (priv->background_picture != None)
    {
      XRenderComposite (GDK_DISPLAY (),
                        PictOpSrc,
                        priv->background_picture,
                        None,
                        picture,
                        event->area.x,
                        event->area.y,
                        event->area.x,
                        event->area.y,
                        event->area.x - x_offset,
                        event->area.y - y_offset,
                        event->area.width,
                        event->area.height);

    }

    result = GTK_WIDGET_CLASS (parent_class)->expose_event (widget, event);

    XRenderFreePicture (GDK_DISPLAY (),
                        picture);

    g_object_set_data (G_OBJECT (drawable),
                       "picture", GINT_TO_POINTER (None));

    return result;

  }

  return FALSE;
}

static void
hildon_desktop_panel_window_composite_realize (GtkWidget     *widget)
{
  HildonDesktopPanelWindowCompositeClass       *klass;
  GTK_WIDGET_CLASS (parent_class)->realize (widget);

  klass = HILDON_DESKTOP_PANEL_WINDOW_COMPOSITE_GET_CLASS (widget);

  klass->desktop_window_data->instances =
      g_slist_append (klass->desktop_window_data->instances, widget);

  hildon_desktop_panel_window_composite_style_set (widget, widget->style);
}

static void
hildon_desktop_panel_window_composite_unrealize (GtkWidget *widget)
{
  HildonDesktopPanelWindowCompositeClass       *klass;

  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);

  klass = HILDON_DESKTOP_PANEL_WINDOW_COMPOSITE_GET_CLASS (widget);

  klass->desktop_window_data->instances =
      g_slist_remove (klass->desktop_window_data->instances, widget);

  hildon_desktop_panel_window_composite_style_set (widget, widget->style);
}

static void
hildon_desktop_panel_window_composite_style_set (GtkWidget   *widget,
                                                 GtkStyle    *old_style)
{
  HildonDesktopPanelWindowComposite            *window =
      HILDON_DESKTOP_PANEL_WINDOW_COMPOSITE (widget);
  HildonDesktopPanelWindowCompositePrivate     *priv = window->priv;
  const gchar                  *filename;
  GdkRectangle                  rect = {0};

  if (!GTK_WIDGET_REALIZED (widget) ||
      !widget->style || !widget->style->rc_style)
    return;

  if (priv->pattern_picture != None)
    {
      XRenderFreePicture (GDK_DISPLAY (),
                          priv->pattern_picture);
      priv->pattern_picture = None;
    }

  if (priv->pattern_mask != None)
    {
      XRenderFreePicture (GDK_DISPLAY (),
                          priv->pattern_mask);
      priv->pattern_mask = None;
    }

  filename = widget->style->rc_style->bg_pixmap_name[GTK_STATE_NORMAL];

  if (!filename)
    return;

  hildon_desktop_picture_and_mask_from_file (filename,
                                             &priv->pattern_picture,
                                             &priv->pattern_mask,
                                             &priv->pattern_width,
                                             &priv->pattern_height);

  if (priv->pattern_width != priv->width ||
      priv->pattern_height != priv->height)
  {
    XTransform scale = {{{ XDoubleToFixed ((gdouble)priv->pattern_width /priv->width), 0, 0},
                   {0, XDoubleToFixed ((gdouble)priv->pattern_height / priv->height), 0},
                   {0, 0, XDoubleToFixed (1.0)}}};

    priv->transform = scale;
    priv->scale = TRUE;
  }
  else
    priv->scale = FALSE;

  rect.width = widget->allocation.width;
  rect.height = widget->allocation.height;
  hildon_desktop_panel_window_composite_update_background (window,
                                                           &rect);

  GTK_WIDGET_CLASS (parent_class)->style_set (widget, old_style);

}

#endif
