/*
 * This file is part of hildon-desktop

 * Copyright (C) 2006, 2007 Nokia Corporation.

 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 * Author:  Johan Bilien <johan.bilien@nokia.com>

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.

 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA

 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "hildon-home-area.h"

#include <libhildondesktop/hildon-desktop-home-item.h>

#ifdef HAVE_X_COMPOSITE
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xrender.h>

#include <libhildondesktop/hildon-desktop-picture.h>
#endif

#include <errno.h>
#include <string.h>
#include <stdlib.h> /* malloc */

/* .desktop keys */
#define HH_APPLET_GROUP                 "Desktop Entry"
#define HH_APPLET_KEY_NAME              "Name"
#define HH_APPLET_KEY_LIBRARY           "X-home-applet"
#define HH_APPLET_KEY_X                 "X"
#define HH_APPLET_KEY_Y                 "Y"
#define HH_APPLET_KEY_WIDTH             "X-home-applet-width"
#define HH_APPLET_KEY_HEIGHT            "X-home-applet-height"
#define HH_APPLET_KEY_STACK_INDEX       "X-home-applet-stack-index"
#define HH_APPLET_KEY_MINWIDTH          "X-home-applet-minwidth"
#define HH_APPLET_KEY_MINHEIGHT         "X-home-applet-minheight"
#define HH_APPLET_KEY_RESIZABLE         "X-home-applet-resizable"

enum
{
  PROP_SNAP_TO_GRID = 1,
  PROP_APPLET_PADDING,
  PROP_APPLET_DEFAULT_ALPHA,
  PROP_CHILD_X,
  PROP_CHILD_Y,
  PROP_CHILD_DATA

};

typedef struct
{
  GtkWidget    *widget;
  Window        window;
  gint          x;
  gint          y;
  gint          stack_index;
  gulong        realize_handler;
#ifdef HAVE_X_COMPOSITE
  gint          mask_width, mask_height;
  Damage        damage;
  Picture       picture;
  Picture       alpha_mask;
  Picture       alpha_mask_unscaled;
  gint          background_width, background_height;
  gint          state;
  guint         style_set_handler,
                size_allocate_handler,
                state_change_handler;
#endif
} ChildData;

typedef struct
{
  GdkRectangle  rect;
  gint          stack_index;
} Layout;

typedef struct HildonHomeAreaPriv_
{
  gboolean      layout_changed;

  gboolean      snap_to_grid;

  gint          applet_padding;

  GHashTable   *layout;

  GList        *children_data;

  gdouble       default_alpha;

#ifdef HAVE_X_COMPOSITE
  Picture       picture;
#endif
  gint          x_offset;
  gint          y_offset;

} HildonHomeAreaPriv;

#define HILDON_HOME_AREA_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HILDON_TYPE_HOME_AREA, HildonHomeAreaPriv));


static GtkFixedClass *parent_class;

static void
hildon_home_area_class_init (HildonHomeAreaClass *klass);

static void
hildon_home_area_init (HildonHomeArea *area);

static void
hildon_home_area_layout_changed (HildonHomeArea *area);

static void
hildon_home_area_applet_added (HildonHomeArea *area, GtkWidget *applet);

static void
hildon_home_area_add (GtkContainer *area, GtkWidget *applet);

static void
hildon_home_area_remove (GtkContainer *area, GtkWidget *applet);

static void
hildon_home_area_calculate_real_size (gint *width, gint *height);

static void
hildon_home_area_forall (GtkContainer  *container,
                         gboolean       include_internals,
                         GtkCallback    callback,
                         gpointer       user_data);

static void
hildon_home_area_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec);

static void
hildon_home_area_get_property (GObject      *object,
                               guint         property_id,
                               GValue *value,
                               GParamSpec   *pspec);
static void
hildon_home_area_set_child_property (GtkContainer    *container,
                                     GtkWidget       *child,
                                     guint            property_id,
                                     const GValue    *value,
                                     GParamSpec      *pspec);

static void
hildon_home_area_get_child_property (GtkContainer    *container,
                                     GtkWidget       *child,
                                     guint            property_id,
                                     GValue          *value,
                                     GParamSpec      *pspec);

static void
hildon_home_area_size_request (GtkWidget       *widget,
                               GtkRequisition  *request);

static void
hildon_home_area_size_allocate (GtkWidget      *widget,
                                GtkAllocation  *allocation);

static void
hildon_home_area_child_realize (GtkWidget             *child,
                                HildonHomeArea        *area);


static void
hildon_home_area_realize (GtkWidget *widget);

#ifdef HAVE_X_COMPOSITE

static void
hildon_home_area_unrealize (GtkWidget *widget);

static gboolean
hildon_home_area_expose (GtkWidget      *widget,
                         GdkEventExpose *event);

static void
hildon_home_area_create_child_data (HildonHomeArea     *area,
                                    GtkWidget          *child);

static void
hildon_home_area_child_style_set (GtkWidget            *child,
                                  GtkStyle             *old_style,
                                  HildonHomeArea       *area);

static void
hildon_home_area_child_size_allocate (GtkWidget        *child,
                                      GtkAllocation    *allocation,
                                      HildonHomeArea   *area);

static void
hildon_home_area_child_build_alpha_mask_unscaled (HildonHomeArea       *area,
                                                  GtkWidget            *child);

static void
hildon_home_area_child_build_alpha_mask (HildonHomeArea       *area,
                                         GtkWidget            *child);

static void
hildon_home_area_child_state_change (ChildData         *child_data);

#endif

static void
hildon_home_area_sort_stack (HildonHomeArea *area);

static void
hildon_home_area_finalize (GObject *object);

static void
hildon_home_area_place (HildonHomeArea *area, GtkWidget *widget);



static void
child_data_free (ChildData *child_data)
{
#ifdef HAVE_X_COMPOSITE
  if (child_data->alpha_mask != None &&
      child_data->alpha_mask != child_data->alpha_mask_unscaled)
    XRenderFreePicture (GDK_DISPLAY (), child_data->alpha_mask);

  if (child_data->alpha_mask_unscaled != None)
    XRenderFreePicture (GDK_DISPLAY (), child_data->alpha_mask_unscaled);

  if (child_data->picture)
    XRenderFreePicture (GDK_DISPLAY (), child_data->picture);

  if (child_data->damage)
    XDamageDestroy (GDK_DISPLAY (), child_data->damage);
#endif

  g_free (child_data);

}

static gint
find_by_window (ChildData *data, Window *w)
{
  return !(data->window == *w);
}


GType
hildon_home_area_get_type (void)
{
  static GType area_type = 0;

  if ( !area_type )
    {
      static const GTypeInfo area_info =
        {
          sizeof (HildonHomeAreaClass),
          NULL, /* base_init */
          NULL, /* base_finalize */
          (GClassInitFunc) hildon_home_area_class_init,
          NULL, /* class_finalize */
          NULL, /* class_data */
          sizeof (HildonHomeArea),
          0,    /* n_preallocs */
          (GInstanceInitFunc) hildon_home_area_init,
        };

      area_type = g_type_register_static (GTK_TYPE_CONTAINER,
                                          "HildonHomeArea",
                                          &area_info,0);
  }

  return area_type;
}



static void
hildon_home_area_class_init (HildonHomeAreaClass *klass)
{
  GObjectClass         *object_class;
  GtkWidgetClass       *widget_class;
  GtkContainerClass    *container_class;
  GParamSpec           *pspec;
#ifdef HAVE_X_COMPOSITE
  int                   damage_error, composite_error, composite_event_base;
#endif

  object_class = G_OBJECT_CLASS (klass);
  widget_class = GTK_WIDGET_CLASS (klass);
  container_class = GTK_CONTAINER_CLASS (klass);
  parent_class = g_type_class_peek_parent (klass);

#ifdef HAVE_X_COMPOSITE
  if (XDamageQueryExtension (GDK_DISPLAY (),
                             &klass->xdamage_event_base,
                             &damage_error) &&

      XCompositeQueryExtension (GDK_DISPLAY (),
                                &composite_event_base,
                                &composite_error))
    {
      klass->composite = TRUE;

      gdk_x11_register_standard_event_type (gdk_display_get_default (),
                                            klass->xdamage_event_base +
                                            XDamageNotify,
                                            1);
    }
  else
    klass->composite = FALSE;
#endif

  g_type_class_add_private (klass, sizeof (HildonHomeAreaPriv));

  klass->layout_changed    = hildon_home_area_layout_changed;
  klass->applet_added      = hildon_home_area_applet_added;

  widget_class->size_allocate = hildon_home_area_size_allocate;
  widget_class->size_request  = hildon_home_area_size_request;
  widget_class->realize       = hildon_home_area_realize;

#ifdef HAVE_X_COMPOSITE
  if (klass->composite)
    {
      widget_class->unrealize    = hildon_home_area_unrealize;
      widget_class->expose_event = hildon_home_area_expose;
    }
#endif

  container_class->add                = hildon_home_area_add;
  container_class->remove             = hildon_home_area_remove;
  container_class->set_child_property = hildon_home_area_set_child_property;
  container_class->get_child_property = hildon_home_area_get_child_property;
  container_class->forall             = hildon_home_area_forall;

  object_class->set_property = hildon_home_area_set_property;
  object_class->get_property = hildon_home_area_get_property;
  object_class->finalize = hildon_home_area_finalize;

  g_signal_new ("applet-added",
                G_OBJECT_CLASS_TYPE (object_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HildonHomeAreaClass, applet_added),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__POINTER,
                G_TYPE_NONE,
                1,
                GTK_TYPE_WIDGET);

  g_signal_new ("applet-selected",
                G_OBJECT_CLASS_TYPE (object_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HildonHomeAreaClass, applet_selected),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__POINTER,
                G_TYPE_NONE,
                1,
                GTK_TYPE_WIDGET);

  g_signal_new ("applet-change-start",
                G_OBJECT_CLASS_TYPE (object_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HildonHomeAreaClass, applet_change_start),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__POINTER,
                G_TYPE_NONE,
                1,
                GTK_TYPE_WIDGET);

  g_signal_new ("applet-change-end",
                G_OBJECT_CLASS_TYPE (object_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HildonHomeAreaClass, applet_change_end),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__POINTER,
                G_TYPE_NONE,
                1,
                GTK_TYPE_WIDGET);

  g_signal_new ("layout-changed",
                G_OBJECT_CLASS_TYPE (object_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HildonHomeAreaClass, layout_changed),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                0);

  pspec =  g_param_spec_boolean ("snap-to-grid",
                                 "Snap to grid",
                                 "Whether applets should snap to grid",
                                 TRUE,
                                 G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

  g_object_class_install_property (object_class,
                                   PROP_SNAP_TO_GRID,
                                   pspec);


  pspec =  g_param_spec_int ("applet-padding",
                             "Applet padding",
                             "Padding between newly added applets",
                             0,
                             G_MAXINT,
                             10,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

  g_object_class_install_property (object_class,
                                   PROP_APPLET_PADDING,
                                   pspec);

  pspec =  g_param_spec_int ("applet-default-alpha",
                             "Applet default alpha",
                             "Default value for the alpha channel "
                             "(in percentage)",
                             0,
                             100,
                             75,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

  g_object_class_install_property (object_class,
                                   PROP_APPLET_DEFAULT_ALPHA,
                                   pspec);

  pspec = g_param_spec_pointer ("child-data",
                                "Child data",
                                "Data used for compositing",
                                G_PARAM_READWRITE);

  gtk_container_class_install_child_property (container_class,
                                              PROP_CHILD_DATA,
                                              pspec);

  pspec = g_param_spec_int ("x",
                            "x",
                            "X coordinate",
                            0,
                            G_MAXINT,
                            0,
                            G_PARAM_READWRITE);

  gtk_container_class_install_child_property (container_class,
                                              PROP_CHILD_X,
                                              pspec);

  pspec = g_param_spec_int ("y",
                            "y",
                            "Y coordinate",
                            0,
                            G_MAXINT,
                            0,
                            G_PARAM_READWRITE);

  gtk_container_class_install_child_property (container_class,
                                              PROP_CHILD_Y,
                                              pspec);

}

static void
hildon_home_area_init (HildonHomeArea *area)
{
  GTK_WIDGET_SET_FLAGS (area, GTK_NO_WINDOW);
}

static void
hildon_home_area_finalize (GObject *object)
{
  HildonHomeAreaPriv *priv;
  priv = HILDON_HOME_AREA_GET_PRIVATE (object);

  if (priv->layout)
    {
      g_hash_table_destroy (priv->layout);
      priv->layout = NULL;
    }

  if (G_OBJECT_CLASS (parent_class)->finalize)
    G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
hildon_home_area_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  HildonHomeAreaPriv      *priv;
  priv = HILDON_HOME_AREA_GET_PRIVATE (HILDON_HOME_AREA (object));

  switch (property_id)
    {
      case PROP_SNAP_TO_GRID:
           priv->snap_to_grid = g_value_get_boolean (value);
           break;

      case PROP_APPLET_PADDING:
           priv->applet_padding = g_value_get_int (value);
           break;

      case PROP_APPLET_DEFAULT_ALPHA:
           priv->default_alpha = (gdouble)g_value_get_int (value) / 100;
           break;

      default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
          break;
    }
}

static void
hildon_home_area_get_property (GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  HildonHomeAreaPriv      *priv;
  priv = HILDON_HOME_AREA_GET_PRIVATE (HILDON_HOME_AREA (object));

  switch (property_id)
    {
      case PROP_SNAP_TO_GRID:
           g_value_set_boolean (value, priv->snap_to_grid);
           break;

      case PROP_APPLET_PADDING:
           g_value_set_int (value, priv->applet_padding);
           break;

      case PROP_APPLET_DEFAULT_ALPHA:
           g_value_set_int (value, (gint)(priv->default_alpha * 100));
           break;

      default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
          break;
    }
}

static gint
find_by_widget (ChildData *data, GtkWidget *widget)
{
  return !(data->widget == widget);
}

static ChildData *
hildon_home_area_get_child_data (HildonHomeArea *area,
                                 GtkWidget      *child)
{
  HildonHomeAreaPriv   *priv = HILDON_HOME_AREA_GET_PRIVATE (area);
  GList                *list_item;

  list_item = g_list_find_custom (priv->children_data,
                                  child,
                                  (GCompareFunc)find_by_widget);


  if (list_item)
    return (ChildData *)list_item->data;
  else
    return NULL;
}

static void
hildon_home_area_set_child_property (GtkContainer    *container,
                                     GtkWidget       *child,
                                     guint            property_id,
                                     const GValue    *value,
                                     GParamSpec      *pspec)
{
  HildonHomeArea       *area = HILDON_HOME_AREA (container);
  HildonHomeAreaPriv   *priv = HILDON_HOME_AREA_GET_PRIVATE (area);
  ChildData            *child_data;

  child_data = hildon_home_area_get_child_data (area, child);

  switch (property_id)
    {
      case PROP_CHILD_DATA:
          if (child_data)
            {
              priv->children_data =
                  g_list_remove (priv->children_data, child_data);
              g_free (child_data);
            }
          priv->children_data =
              g_list_append (priv->children_data, g_value_get_pointer (value));
          break;
      case PROP_CHILD_X:
          if (!child_data) return;

          hildon_home_area_move (HILDON_HOME_AREA (container),
                                 child,
                                 g_value_get_int (value),
                                 child_data->y);
          break;
      case PROP_CHILD_Y:
          if (!child_data) return;

          hildon_home_area_move (HILDON_HOME_AREA (container),
                                 child,
                                 child_data->x,
                                 g_value_get_int (value));
      default:
          GTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container,
                                                        property_id,
                                                        pspec);
          break;
    }

}

static void
hildon_home_area_get_child_property (GtkContainer    *container,
                                     GtkWidget       *child,
                                     guint            property_id,
                                     GValue          *value,
                                     GParamSpec      *pspec)
{
  HildonHomeArea       *area = HILDON_HOME_AREA (container);
  ChildData            *child_data;

  g_return_if_fail (child->parent == GTK_WIDGET (container));

  child_data = hildon_home_area_get_child_data (area, child);

  g_return_if_fail (child_data);

  switch (property_id)
    {
      case PROP_CHILD_DATA:
          g_value_set_pointer (value, child_data);
          break;
      case PROP_CHILD_X:
          g_value_set_int (value, child_data->x);
          break;
      case PROP_CHILD_Y:
          g_value_set_int (value, child_data->y);
          break;

      default:
          GTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container,
                                                        property_id,
                                                        pspec);
          break;
    }
}

/* Largely copied from GtkFixed */
static void
hildon_home_area_size_request (GtkWidget       *widget,
                               GtkRequisition  *requisition)
{
  HildonHomeAreaPriv   *priv = HILDON_HOME_AREA_GET_PRIVATE (widget);
  GList                *children;
  GtkRequisition        child_requisition = {0};
  ChildData            *child;

  requisition->width = 0;
  requisition->height = 0;

  children = priv->children_data;
  while (children)
    {
      child = children->data;
      children = children->next;

      if (GTK_WIDGET_VISIBLE (child->widget))
        {
          gtk_widget_size_request (child->widget, &child_requisition);

          requisition->height = MAX (requisition->height,
                                     child->y +
                                     child_requisition.height);
          requisition->width = MAX (requisition->width,
                                    child->x +
                                    child_requisition.width);
        }
    }

  requisition->height += GTK_CONTAINER (widget)->border_width * 2;
  requisition->width  += GTK_CONTAINER (widget)->border_width * 2;

}

static void
hildon_home_area_allocate_child (HildonHomeArea        *area,
                                 ChildData             *child)
{
  if (GTK_WIDGET_VISIBLE (child->widget))
    {
      GtkWidget        *widget = GTK_WIDGET (area);
      GtkAllocation     child_allocation;
      GtkRequisition    child_requisition;
      guint16           border_width;

      border_width = GTK_CONTAINER (area)->border_width;

      gtk_widget_get_child_requisition (child->widget, &child_requisition);

      child_allocation.x = widget->allocation.x + child->x + border_width;
      child_allocation.y = widget->allocation.y + child->y + border_width;

      child_allocation.width = child_requisition.width;
      child_allocation.height = child_requisition.height;

      gtk_widget_size_allocate (child->widget, &child_allocation);
    }
}


/* Largely copied from GtkFixed */
static void
hildon_home_area_size_allocate (GtkWidget      *widget,
                                GtkAllocation  *allocation)
{
  HildonHomeAreaPriv   *priv = HILDON_HOME_AREA_GET_PRIVATE (widget);
  GList                *children;

  widget->allocation = *allocation;

  children = priv->children_data;
  while (children)
    {
      ChildData        *child = children->data;
      children = children->next;

      hildon_home_area_allocate_child (HILDON_HOME_AREA (widget),
                                       child);

    }

}
static void
hildon_home_area_realize (GtkWidget *widget)
{
  GTK_WIDGET_CLASS (parent_class)->realize (widget);

#ifdef HAVE_X_COMPOSITE
  if (HILDON_HOME_AREA_GET_CLASS (widget)->composite)
    {

      XCompositeRedirectSubwindows (GDK_DISPLAY (),
                                    GDK_WINDOW_XID (widget->window),
                                    CompositeRedirectManual);

    }
#endif

  hildon_home_area_sort_stack (HILDON_HOME_AREA (widget));

}


#ifdef HAVE_X_COMPOSITE

static void
hildon_home_area_unrealize (GtkWidget *widget)
{
  HildonHomeAreaPriv           *priv;

  priv = HILDON_HOME_AREA_GET_PRIVATE (widget);

  XCompositeUnredirectSubwindows (GDK_DISPLAY (),
                                  GDK_WINDOW_XID (widget->window),
                                  CompositeRedirectManual);

  if (priv->picture != None)
    XRenderFreePicture (GDK_DISPLAY (), priv->picture);

  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}
#endif

static void
hildon_home_area_layout_changed (HildonHomeArea *area)
{
  HildonHomeAreaPriv      *priv;
  priv = HILDON_HOME_AREA_GET_PRIVATE (area);

  priv->layout_changed = TRUE;
}


static void
hildon_home_area_applet_added (HildonHomeArea *area, GtkWidget *applet)
{
  HildonHomeAreaPriv      *priv;
  g_return_if_fail (area);

  priv = HILDON_HOME_AREA_GET_PRIVATE (area);
}

static void
hildon_home_area_add (GtkContainer *area, GtkWidget *applet)
{
  HildonHomeAreaPriv   *priv;
  g_return_if_fail (area);

  priv = HILDON_HOME_AREA_GET_PRIVATE (area);

  if (HILDON_DESKTOP_IS_HOME_ITEM (applet))
    {
      Layout   *layout = NULL;

      if (priv->layout)
        {
          const gchar *name =
              hildon_desktop_item_get_id (HILDON_DESKTOP_ITEM (applet));

          layout = g_hash_table_lookup (priv->layout, name);
        }

      if (layout)
        {
          if (layout->rect.width > 0 && layout->rect.height > 0)
            gtk_widget_set_size_request (applet,
                                         layout->rect.width,
                                         layout->rect.height);
          hildon_home_area_put (HILDON_HOME_AREA (area),
                                applet,
                                layout->rect.x, layout->rect.y,
                                layout->stack_index);
        }
      else
        {
          hildon_home_area_place (HILDON_HOME_AREA (area), applet);
        }

    }
}

static void
hildon_home_area_forall (GtkContainer  *container,
                         gboolean       include_internals,
                         GtkCallback    callback,
                         gpointer       user_data)
{
  HildonHomeAreaPriv   *priv;
  GList                *i;

  priv = HILDON_HOME_AREA_GET_PRIVATE (container);

  i = priv->children_data;
  while (i)
  {
    ChildData        *child = i->data;

    i = i->next;
    callback (child->widget, user_data);
  }
}

#ifdef HAVE_X_COMPOSITE
static void
hildon_home_area_child_compose (ChildData *child_data,
                                HildonHomeArea *area)
{
  HildonHomeAreaPriv   *priv;
  GtkAllocation        *alloc;

  priv = HILDON_HOME_AREA_GET_PRIVATE (area);

  if (child_data->picture == None)
    return;

  alloc = &(child_data->widget->allocation);

  g_return_if_fail (priv->picture != None);

  XRenderComposite (GDK_DISPLAY (),
                    (child_data->state == 0)?PictOpOver:PictOpSrc,
                    child_data->picture,
                    (child_data->state == 0)?child_data->alpha_mask:None,
                    priv->picture,
                    0,
                    0,
                    0,
                    0,
                    alloc->x - priv->x_offset,
                    alloc->y - priv->y_offset,
                    alloc->width,
                    alloc->height);
}

static void
hildon_home_area_child_style_set (GtkWidget            *child,
                                  GtkStyle             *old_style,
                                  HildonHomeArea       *area)
{
  ChildData    *child_data;

  if (!HILDON_DESKTOP_IS_HOME_ITEM (child))
    return;

  if (!old_style)
    return;

  /* HACK: Pretend the child shaped so GDK does not add the
   * child to the parent's clip list (see #412882) */
  if (GTK_WIDGET_REALIZED (child))
    ((GdkWindowObject *)child->window)->shaped = TRUE;

  gtk_container_child_get (GTK_CONTAINER (area), child,
                           "child-data", &child_data,
                           NULL);

  hildon_home_area_child_build_alpha_mask_unscaled (area, child);
  hildon_home_area_child_build_alpha_mask (area, child);

}

static void
hildon_home_area_child_size_allocate (GtkWidget        *child,
                                      GtkAllocation    *allocation,
                                      HildonHomeArea   *area)
{
  ChildData    *data;
  gint          state = 0;

  g_object_get (child,
                "state", &state,
                NULL);

  gtk_container_child_get (GTK_CONTAINER (area), child,
                           "child-data", &data,
                           NULL);

  if (state == 0 && /* not moving or resizing */
      (data->mask_width  != allocation->width ||
       data->mask_height != allocation->height))
    hildon_home_area_child_build_alpha_mask (area, child);

}

static void
hildon_home_area_child_build_alpha_mask_unscaled (HildonHomeArea       *area,
                                                  GtkWidget            *child)
{
  HildonHomeAreaPriv           *priv;
  ChildData                    *child_data;
  const gchar                  *mask_file_name = NULL;

  priv = HILDON_HOME_AREA_GET_PRIVATE (area);

  gtk_container_child_get (GTK_CONTAINER (area), child,
                           "child-data", &child_data,
                           NULL);

  if (child_data->alpha_mask_unscaled != None)
    {
      XRenderFreePicture  (GDK_DISPLAY (), child_data->alpha_mask_unscaled);
      child_data->alpha_mask_unscaled = None;
    }

  if (!child->style->rc_style)
    return;

  mask_file_name = child->style->rc_style->bg_pixmap_name[GTK_STATE_PRELIGHT];

  if (!mask_file_name)
    return;

  hildon_desktop_picture_and_mask_from_file (mask_file_name,
                                             NULL,
                                             &child_data->alpha_mask_unscaled,
                                             &child_data->background_width,
                                             &child_data->background_height);

}

static void
hildon_home_area_child_build_alpha_mask (HildonHomeArea *area,
                                         GtkWidget *child)
{
  ChildData    *child_data;
  GtkBorder    *borders = NULL;


  if (!GTK_WIDGET_REALIZED (child))
    return;

  gtk_container_child_get (GTK_CONTAINER (area), child,
                           "child-data", &child_data,
                           NULL);

  if (child_data->alpha_mask_unscaled == None)
    hildon_home_area_child_build_alpha_mask_unscaled (area, child);

  if (child_data->alpha_mask_unscaled == None)
    return;

  if (child_data->alpha_mask != None)
    XRenderFreePicture (GDK_DISPLAY (),
                        child_data->alpha_mask);

  child_data->alpha_mask = None;

  gtk_widget_style_get (child,
                        "background-borders", &borders,
                        NULL);

  if (borders && child_data->alpha_mask_unscaled)
    {
      Pixmap                    p;
      gint                      w, h, x, y;
      gint                      center_w, center_h, scenter_w, scenter_h;
      XRenderPictureAttributes  pa;
      XRenderPictFormat        *format;

      w = child->allocation.width;
      h = child->allocation.height;

      if (w < 0 || h < 0)
        {
          g_free (borders);
          return;
        }

      center_w = w - borders->left - borders->right;
      center_h = h - borders->top  - borders->bottom;

      scenter_w = child_data->background_width - borders->left - borders->right;
      scenter_h = child_data->background_height - borders->top - borders->bottom;

      if (scenter_h < 0 || scenter_w < 0)
        {
          g_warning ("Invalid border values, alpha mask will not be applied");
          g_free (borders);
          return;
        }

      p = XCreatePixmap (GDK_DISPLAY (),
                         GDK_WINDOW_XID (child->window),
                         child->allocation.width,
                         child->allocation.height,
                         8);

      format = XRenderFindStandardFormat (GDK_DISPLAY (),
                                          PictStandardA8);

      pa.repeat = True;
      child_data->alpha_mask = XRenderCreatePicture (GDK_DISPLAY (),
                                                     p,
                                                     format,
                                                     CPRepeat,
                                                     &pa);

      XRenderComposite (GDK_DISPLAY (),
                        PictOpSrc,
                        child_data->alpha_mask_unscaled,
                        None,
                        child_data->alpha_mask,
                        0, 0,
                        0, 0,
                        0, 0,
                        borders->left, borders->top);

      x = borders->left;
      while (x < borders->left + center_w)
        {
          XRenderComposite (GDK_DISPLAY (),
                            PictOpSrc,
                            child_data->alpha_mask_unscaled,
                            None,
                            child_data->alpha_mask,
                            borders->left, 0,
                            0, 0,
                            x, 0,
                            scenter_w, borders->top);

          x += scenter_w;
        }

      XRenderComposite (GDK_DISPLAY (),
                        PictOpSrc,
                        child_data->alpha_mask_unscaled,
                        None,
                        child_data->alpha_mask,
                        borders->left + scenter_w, 0,
                        0, 0,
                        borders->left + center_w, 0,
                        borders->right, borders->top);

      y = borders->top;
      while (y < borders->top + center_h)
        {
          XRenderComposite (GDK_DISPLAY (),
                            PictOpSrc,
                            child_data->alpha_mask_unscaled,
                            None,
                            child_data->alpha_mask,
                            0, borders->top,
                            0, 0,
                            0, y,
                            borders->left, scenter_h);
          y += scenter_h;
        }

      y = borders->top;
      while (y < borders->top + center_h)
        {
          x = borders->left;
          while (x < borders->left + center_w)
            {
              XRenderComposite (GDK_DISPLAY (),
                                PictOpSrc,
                                child_data->alpha_mask_unscaled,
                                None,
                                child_data->alpha_mask,
                                borders->left, borders->top,
                                0, 0,
                                x, y,
                                scenter_w, scenter_h);
              x += scenter_w;
            }
          y += scenter_h;
        }

      y = borders->top;
      while (y < borders->top + center_h)
        {
          XRenderComposite (GDK_DISPLAY (),
                            PictOpSrc,
                            child_data->alpha_mask_unscaled,
                            None,
                            child_data->alpha_mask,
                            borders->left + scenter_w, borders->top,
                            0, 0,
                            borders->left + center_w, y,
                            borders->right, scenter_h);
          y += scenter_h;
        }

      XRenderComposite (GDK_DISPLAY (),
                        PictOpSrc,
                        child_data->alpha_mask_unscaled,
                        None,
                        child_data->alpha_mask,
                        0, borders->top + scenter_h,
                        0, 0,
                        0, borders->top + center_h,
                        borders->left, borders->bottom);

      x = borders->left;
      while (x < borders->left + center_w)
        {
          XRenderComposite (GDK_DISPLAY (),
                            PictOpSrc,
                            child_data->alpha_mask_unscaled,
                            None,
                            child_data->alpha_mask,
                            borders->left, borders->top + scenter_h,
                            0, 0,
                            x, borders->top + center_h,
                            scenter_w, borders->bottom);
          x += scenter_w;
        }

      XRenderComposite (GDK_DISPLAY (),
                        PictOpSrc,
                        child_data->alpha_mask_unscaled,
                        None,
                        child_data->alpha_mask,
                        borders->left + scenter_w, borders->top + scenter_h,
                        0, 0,
                        borders->left + center_w, borders->top + center_h,
                        borders->right, borders->bottom);

      child_data->mask_width = w;
      child_data->mask_height = h;

      XFreePixmap (GDK_DISPLAY (), p);
      g_free (borders);
    }
  else
    child_data->alpha_mask = child_data->alpha_mask_unscaled;

}

static void
hildon_home_area_child_state_change (ChildData *child_data)
{
  gint state;

  g_object_get (child_data->widget,
                "state", &state,
                NULL);

  child_data->state = state;

  if (state == 0 && /* not moving or resizing */
      (child_data->mask_width  != child_data->widget->allocation.width ||
       child_data->mask_height != child_data->widget->allocation.height))
    hildon_home_area_child_build_alpha_mask (HILDON_HOME_AREA (child_data->widget->parent),
                                             child_data->widget);

}


static gboolean
hildon_home_area_expose (GtkWidget *widget,
                         GdkEventExpose *event)
{
  HildonHomeAreaPriv *priv;

  priv = HILDON_HOME_AREA_GET_PRIVATE (widget);

  if (GTK_WIDGET_DRAWABLE (widget))
    {
      XRectangle                rectangle;
      XserverRegion             region;
      GdkDrawable              *drawable;
      Window                   *wchildren, root, parent;
      guint                     n_children, i;

      gdk_window_get_internal_paint_info (widget->window,
                                          &drawable,
                                          &priv->x_offset,
                                          &priv->y_offset);

      rectangle.x = event->area.x - priv->x_offset;
      rectangle.y = event->area.y - priv->y_offset;
      rectangle.width = event->area.width;
      rectangle.height = event->area.height;

      priv->picture = hildon_desktop_picture_from_drawable (drawable);

      region = XFixesCreateRegion (GDK_DISPLAY (),
                                   &rectangle,
                                   1);

#if 0
      XFixesSetPictureClipRegion (GDK_DISPLAY (),
                                  priv->picture,
                                  0,
                                  0,
                                  region);
#endif

      gdk_error_trap_push ();
      XQueryTree (GDK_DISPLAY (),
                  GDK_WINDOW_XID (widget->window),
                  &root,
                  &parent,
                  &wchildren,
                  &n_children);
      if (gdk_error_trap_pop ())
        {
          if (wchildren)
            XFree (wchildren);

          XRenderFreePicture (GDK_DISPLAY (),
                              priv->picture);
          XFixesDestroyRegion (GDK_DISPLAY (),
                               region);
          return FALSE;
        }

      for (i = 0; i < n_children; i++)
        {
          GList        *l = NULL;

          l = g_list_find_custom (priv->children_data,
                                  &wchildren[i],
                                  (GCompareFunc)find_by_window);

          if (l)
            hildon_home_area_child_compose (l->data,
                                            HILDON_HOME_AREA (widget));
        }

      XFree (wchildren);

      XFixesDestroyRegion (GDK_DISPLAY (), region);
      XRenderFreePicture (GDK_DISPLAY (), priv->picture);
    }

  return TRUE;
}

static GdkFilterReturn
hildon_home_area_child_window_filter (GdkXEvent        *xevent,
                                      GdkEvent         *event,
                                      GtkWidget        *child)
{
  XEvent               *aevent = xevent;
  HildonHomeArea       *area = HILDON_HOME_AREA (child->parent);
  HildonHomeAreaClass  *klass = HILDON_HOME_AREA_GET_CLASS (area);

  if (aevent->type == klass->xdamage_event_base + XDamageNotify)
    {
      XDamageNotifyEvent       *ev = xevent;
      XserverRegion             parts;
      XRectangle               *rects;
      guint                     i;
      int                       n_rect;

      parts = XFixesCreateRegion (GDK_DISPLAY (), 0, 0);
      XDamageSubtract (GDK_DISPLAY (), ev->damage, None, parts);

      rects = XFixesFetchRegion (GDK_DISPLAY (),
                                 parts,
                                 &n_rect);

      XFixesDestroyRegion (GDK_DISPLAY (),
                           parts);

      for (i = 0; i < n_rect; i++)
        {
          GdkRectangle  rect;

          rect.x = child->allocation.x + rects[i].x;
          rect.y = child->allocation.y + rects[i].y;
          rect.width  = rects[i].width;
          rect.height = rects[i].height;

          gdk_window_invalidate_rect (GTK_WIDGET (area)->window,
                                      &rect,
                                      FALSE);
        }

      XFree (rects);
    }

  return GDK_FILTER_CONTINUE;
}


static void
hildon_home_area_create_child_data (HildonHomeArea     *area,
                                    GtkWidget          *child)
{
  ChildData            *child_data;
  XWindowAttributes     attr;

  gtk_container_child_get (GTK_CONTAINER (area), child,
                           "child-data", &child_data,
                           NULL);

  g_return_if_fail (child_data);

  child_data->window = GDK_WINDOW_XID (child->window);

  XGetWindowAttributes (GDK_DISPLAY(), child_data->window, &attr);

  if (attr.class == InputOutput)
    {
      XRenderPictFormat            *format;
      XRenderPictureAttributes      pa;

      format = XRenderFindVisualFormat (GDK_DISPLAY(), attr.visual);

      pa.subwindow_mode = IncludeInferiors;

      if (child_data->picture)
        XRenderFreePicture (GDK_DISPLAY (),
                            child_data->picture);

      child_data->picture = XRenderCreatePicture (GDK_DISPLAY (),
                                                  child_data->window,
                                                  format,
                                                  CPSubwindowMode,
                                                  &pa);

      child_data->damage = XDamageCreate (GDK_DISPLAY (),
                                          child_data->window,
                                          XDamageReportNonEmpty);

      gdk_window_add_filter (child_data->widget->window,
                             (GdkFilterFunc)
                                 hildon_home_area_child_window_filter,
                             child_data->widget);
    }
}
#endif

static gint
sort_by_stack_index (ChildData *child1, ChildData *child2)
{
  return child1->stack_index - child2->stack_index;
}


static void
hildon_home_area_sort_stack (HildonHomeArea *area)
{
  HildonHomeAreaPriv   *priv;
  GList                *child;

  priv = HILDON_HOME_AREA_GET_PRIVATE (area);

  priv->children_data = g_list_sort (priv->children_data,
                                     (GCompareFunc)sort_by_stack_index);

  for (child = priv->children_data; child ; child = child->next)
    {
      ChildData        *data = child->data;
      if (HILDON_DESKTOP_IS_HOME_ITEM (data->widget))
        hildon_desktop_home_item_raise (HILDON_DESKTOP_HOME_ITEM (data->widget));
    }
}

static void
hildon_home_area_child_realize (GtkWidget             *child,
                                HildonHomeArea        *area)
{
  ChildData    *data;

  gtk_container_child_get (GTK_CONTAINER (area), child,
                           "child-data", &data,
                           NULL);

  data->window = GDK_WINDOW_XID (child->window);
#ifdef HAVE_X_COMPOSITE

  /* HACK: Pretend the child shaped so GDK does not add the
   * child to the parent's clip list (see #412882) */
  ((GdkWindowObject *)child->window)->shaped = TRUE;

  if (data->picture == None)
    hildon_home_area_create_child_data (HILDON_HOME_AREA (area), child);

  if (data->alpha_mask == None)
    hildon_home_area_child_build_alpha_mask (area, child);
#endif

  hildon_home_area_sort_stack (area);
}

static GdkRectangle *
create_rectangle (gint x, gint y, gint w, gint h)
{
  GdkRectangle *r;

  r = g_new (GdkRectangle, 1);
  r->x = x;
  r->y = y;
  r->width = w;
  r->height = h;

  return r;
}

static void
substract_rectangle (GdkRectangle     *original,
                     GdkRectangle     *rectangle,
                     GList           **result)
{
  *result = NULL;

  /* top */
  if (rectangle->y > original->y)
    *result = g_list_append (*result,
                             create_rectangle (original->x,
                                               original->y,
                                               original->width,
                                               rectangle->y - original->y));

  /* left */
  if (rectangle->x > original->x)
    *result = g_list_append (*result,
                             create_rectangle (original->x,
                                               original->y,
                                               rectangle->x - original->x,
                                               original->height));

  /* right */
  if (rectangle->x + rectangle->width < original->x + original->width)
    *result = g_list_append (*result,
                             create_rectangle (rectangle->x + rectangle->width,
                                               original->y,
                                               original->x + original->width -
                                               rectangle->x - rectangle->width,
                                               original->height));

  /* bottom */
  if (rectangle->y + rectangle->height < original->y + original->height)
    *result = g_list_append (*result,
                             create_rectangle (original->x,
                                               rectangle->y + rectangle->height,
                                               original->width,
                                               original->y + original->height -
                                               rectangle->y - rectangle->height));

}

static void
substract_rectangle_from_region (GList        *region,
                                 GdkRectangle  *rectangle)
{
  GList *i = region;

  while (i)
    {
      GdkRectangle     *r = (GdkRectangle *)i->data;
      GdkRectangle      tmp;

      if (gdk_rectangle_intersect (r, rectangle, &tmp))
        {
          GList *pieces = NULL;

          substract_rectangle (r, rectangle, &pieces);

          if (pieces)
            {
              GList *last_piece = g_list_last (pieces);

              /* Insert the new list of rectangles as a replacement of the
               * original */

              last_piece->next = i->next;
              (pieces)->prev = i->prev;

              if (i->prev)
                {
                  i->prev->next = pieces;
                  if (i->next)
                    i->next->prev = last_piece;

                  g_free (i->data);
                  g_list_free_1 (i);
                  i = last_piece->next;

                }
              else
                {
                  /* In this case, we keep the first element of region
                   * and get rid of the first element of pieces */

                  if (i->next)
                    {
                      i->next->prev = (pieces->next)?last_piece:region;
                    }

                  region->data = pieces->data;
                  if (pieces->next)
                    {
                      region->next = pieces->next;
                      pieces->next->prev = region;
                    }

                  i = last_piece->next;

                  g_list_free_1 (pieces);
                }

            }
          else
            i = i->next;
        }
      else
        i = i->next;
    }
}

static void
remove_widget (GtkWidget *widget, GList *region)
{
  GdkRectangle r = {0};
  gint x, y;
  gint padding;
  guint width, height;

  gtk_container_child_get (GTK_CONTAINER (widget->parent), widget,
                           "x", &x,
                           "y", &y,
                           NULL);

  g_object_get (widget->parent,
                "applet-padding", &padding,
                NULL);

  if (widget->allocation.width == 1 && widget->allocation.height == 1)
  {
    GtkRequisition req;

    gtk_widget_size_request (widget, &req);
    width = req.width;
    height = req.height;
  }
  else
  {
    width = widget->allocation.width;
    height = widget->allocation.height;
  }

  r.x = x?x-padding:0;
  r.y = y?y-padding:0;

  r.width = width;
  if (x + width != widget->parent->allocation.width)
    r.width += padding;
  if (x)
    r.width += padding;

  r.height = height;
  if (y + height != widget->parent->allocation.height)
    r.height += padding;
  if (y)
    r.height += padding;

  substract_rectangle_from_region (region, &r);
}

/* Sorts top - left first */
static gint
sort_rectangles (GdkRectangle *r1, GdkRectangle *r2)
{
  return (r1->y - r2->y)?
      r1->y - r2->y:
      r1->x - r2->x;

}

static void
hildon_home_area_place (HildonHomeArea *area, GtkWidget *w)
{
  HildonHomeAreaPriv      *priv;
  GdkRectangle         *area_rectangle;
  GList                *region = NULL;
  GtkRequisition    req = {0};
  GList            *i_rect;
  const gchar      *name;

  name = hildon_desktop_item_get_id (HILDON_DESKTOP_ITEM (w));

  priv = HILDON_HOME_AREA_GET_PRIVATE (area);

  area_rectangle = create_rectangle (0,
                                     0,
                                     GTK_WIDGET (area)->allocation.width,
                                     GTK_WIDGET (area)->allocation.height);

  region = g_list_append (region, area_rectangle);

  gtk_container_foreach (GTK_CONTAINER (area),
                         (GtkCallback)remove_widget,
                         region);

  region = g_list_sort (region, (GCompareFunc)sort_rectangles);

  gtk_widget_set_child_visible (w, FALSE);
  gtk_widget_set_parent (w, GTK_WIDGET (area));
  gtk_widget_size_request (w, &req);

  if (req.width > area_rectangle->width ||
      req.height > area_rectangle->height)
  {
    /* Applet is bigger than the area ... Put it in top-left corner */
    Layout           *layout = g_new (Layout, 1);

    layout->rect.x = 0;
    layout->rect.y = 0;
    layout->rect.width = req.width;
    layout->rect.height = req.height;
    layout->stack_index = G_MAXINT;

    g_hash_table_insert (priv->layout, g_strdup (name), layout);
    hildon_home_area_put (area, w, 0, 0, G_MAXINT);
    g_signal_emit_by_name (area, "layout-changed");
    gtk_widget_set_child_visible (w, TRUE);

    g_list_foreach (region, (GFunc)g_free, NULL);
    g_list_free (region);

    return;
  }

  while (TRUE)
  {
    i_rect = region;

    while (i_rect)
    {
      GdkRectangle *r = (GdkRectangle *)i_rect->data;

      if (r->width  >= req.width &&
          r->height >= req.height)
      {
        Layout           *layout = g_new (Layout, 1);

        layout->rect.x = r->x;
        layout->rect.y = r->y;
        layout->rect.width = req.width;
        layout->rect.height = req.height;
        layout->stack_index = G_MAXINT;

        g_hash_table_insert (priv->layout, g_strdup (name), layout);
        hildon_home_area_put (area, w, r->x, r->y, G_MAXINT);
        gtk_widget_set_child_visible (w, TRUE);
        g_signal_emit_by_name (area, "layout-changed");


        break;
      }

      i_rect = g_list_next (i_rect);

    }

    if (!i_rect)
    {
      /* Not enough place in this layer, we need to add one */
      g_list_foreach (region, (GFunc)g_free, NULL);
      g_list_free (region);
      region = NULL;
      region = g_list_append (region, area_rectangle);
    }
    else
      break;
  }

  g_list_foreach (region, (GFunc)g_free, NULL);
  g_list_free (region);
}

static void
hildon_home_area_remove (GtkContainer *area, GtkWidget *applet)
{
  HildonHomeAreaPriv   *priv;
  ChildData            *child_data;

  g_return_if_fail (area);

  priv = HILDON_HOME_AREA_GET_PRIVATE (area);
  gtk_container_child_get (area, applet,
                           "child-data", &child_data,
                           NULL);

  if (child_data)
    {
      g_signal_handler_disconnect (applet,
                                   child_data->realize_handler);

#ifdef HAVE_X_COMPOSITE
      if (HILDON_HOME_AREA_GET_CLASS (area)->composite)
        {
          g_signal_handler_disconnect (applet,
                                       child_data->style_set_handler);
          g_signal_handler_disconnect (applet,
                                       child_data->size_allocate_handler);
          g_signal_handler_disconnect (applet,
                                       child_data->state_change_handler);
        }
#endif

      priv->children_data = g_list_remove (priv->children_data, child_data);

      child_data_free (child_data);
    }

  if (HILDON_DESKTOP_IS_HOME_ITEM (applet))
  {
    const gchar *name =
        hildon_desktop_item_get_id (HILDON_DESKTOP_ITEM (applet));

    if (g_hash_table_remove (priv->layout, name))
      g_signal_emit_by_name (area, "layout-changed");

  }

  gtk_widget_unparent (applet);

}

/* Public functions */

GtkWidget *
hildon_home_area_new ()
{
  return g_object_new (HILDON_TYPE_HOME_AREA, NULL);
}

static void
hildon_home_area_child_save_position (ChildData *child, GKeyFile *keyfile)
{
  const gchar  *id;
  guint         width, height;

  if (!HILDON_DESKTOP_IS_HOME_ITEM (child->widget))
    return;

  id = hildon_desktop_item_get_id (HILDON_DESKTOP_ITEM (child->widget));
  g_return_if_fail (id);

  width  = child->widget->allocation.width;
  height = child->widget->allocation.height;

  if (width == 1 && height == 1)
  {
    GtkRequisition req = {0};
    /* If the child has not been allocated yet, use the size request ... */
    gtk_widget_size_request (child->widget, &req);
    width  = req.width;
    height = req.height;
  }

  g_key_file_set_integer (keyfile,
                          id,
                          HH_APPLET_KEY_X,
                          child->x);

  g_key_file_set_integer (keyfile,
                          id,
                          HH_APPLET_KEY_Y,
                          child->y);

  g_key_file_set_integer (keyfile,
                          id,
                          HH_APPLET_KEY_STACK_INDEX,
                          child->stack_index);

  g_key_file_set_integer (keyfile,
                          id,
                          HH_APPLET_KEY_WIDTH,
                          width);

  g_key_file_set_integer (keyfile,
                          id,
                          HH_APPLET_KEY_HEIGHT,
                          height);

}

gint
hildon_home_area_save_configuration (HildonHomeArea *area,
                                     const gchar *path,
                                     GError **error)
{
  HildonHomeAreaPriv   *priv;
  GKeyFile             *keyfile;
  FILE                 *file;
  gchar                *buffer = NULL;
  gsize                 buffer_size;
  GError               *local_error = NULL;
  gint                  ret;
  guint                 n_children, i;
  Window               *wchildren, root, parent;

  g_return_val_if_fail (HILDON_IS_HOME_AREA (area) && path, -1);

  priv = HILDON_HOME_AREA_GET_PRIVATE (area);

  gdk_error_trap_push ();
  XQueryTree (GDK_DISPLAY (),
              GDK_WINDOW_XID (GTK_WIDGET (area)->window),
              &root,
              &parent,
              &wchildren,
              &n_children);
  if (gdk_error_trap_pop ())
    {
      if (wchildren)
        XFree (wchildren);

      return -1;
    }

  keyfile = g_key_file_new ();

  for (i = 0; i < n_children; i++)
    {
      GList            *l = NULL;

      l = g_list_find_custom (priv->children_data,
                              &wchildren[i],
                              (GCompareFunc)find_by_window);

      if (l)
        {
          ChildData    *child = l->data;
          child->stack_index = i;
          hildon_home_area_child_save_position (child, keyfile);
        }
    }

  XFree (wchildren);

  file = fopen (path, "w");

  if (!file)
    {
      g_warning ("Could not open %s for saving the applet configuration", path);
      g_key_file_free (keyfile);
      g_set_error (error,
                   G_FILE_ERROR,
                   g_file_error_from_errno (errno),
                   strerror (errno));
      return -1;
    }

  buffer = g_key_file_to_data (keyfile, &buffer_size, &local_error);

  if (local_error)
    {
      g_warning ("Could not create buffer to save applet configuration: %s",
                 local_error->message);
      g_propagate_error (error, local_error);
      if (file)
        fclose (file);
      return 1;
    }

  g_key_file_free (keyfile);

  if (buffer_size == 0)
    {
      const char *empty_file = "# No applet";
      int len =  strlen (empty_file);
      ret = fwrite (empty_file, len, 1, file);
    }

  else
    ret = fwrite (buffer, buffer_size, 1, file);

  g_free (buffer);

  if (file)
    fclose (file);

  if (ret != 1)
    return -1;

  return 0;
}

static void
hildon_home_area_calculate_real_size (gint *width, gint *height)
{
  GdkScreen *screen;
  GKeyFile *key_file;
  GError *error = NULL;
  gint panel_height = 52; //default marquee panel height

  key_file = g_key_file_new();

  g_key_file_load_from_file (key_file,
                             "/etc/hildon-desktop/desktop.conf",
                             G_KEY_FILE_NONE,
                             &error);

  if (error)
  {
    g_warning ("Error loading desktop configuration file: %s",
               error->message);
    g_error_free (error);
  }
  else
  {
    panel_height = g_key_file_get_integer (key_file,
                                           "Marquee",
                                           "X-Size-Height",
                                           &error);

    if (error)
    {
      g_warning ("Error loading marquee panel height: %s",
                 error->message);
      g_error_free (error);
    }
  }

  screen = gdk_screen_get_default ();
  if (screen)
  {
    *width = gdk_screen_get_width (screen);
    *height = gdk_screen_get_height (screen) - panel_height;
  }
}

void
hildon_home_area_load_configuration (HildonHomeArea *area,
                                     const gchar *path,
                                     GError **error)
{
  HildonHomeAreaPriv      *priv;
  GKeyFile *keyfile;
  GError *local_error = NULL;
  gchar **groups = NULL;
  gsize n_groups;
  GList *applets = NULL;

  g_return_if_fail (area);
  priv = HILDON_HOME_AREA_GET_PRIVATE (area);

  applets = gtk_container_get_children (GTK_CONTAINER (area));

  keyfile = g_key_file_new ();

  g_key_file_load_from_file (keyfile, path, G_KEY_FILE_NONE, &local_error);

  if (local_error) goto cleanup;

  if (priv->layout)
    g_hash_table_destroy (priv->layout);

  priv->layout = g_hash_table_new_full (g_str_hash,
                                        g_str_equal,
                                        (GDestroyNotify)g_free,
                                        (GDestroyNotify)g_free);

  groups = g_key_file_get_groups (keyfile, &n_groups);

  while (n_groups > 0)
    {
      GtkWidget     *applet = NULL;
      gint           x,y,width = -1,height = -1, stack_index = G_MAXINT;
      GList         *list_element;

      x = g_key_file_get_integer (keyfile,
                                  groups[n_groups-1],
                                  HH_APPLET_KEY_X,
                                  &local_error);
      y = g_key_file_get_integer (keyfile,
                                  groups[n_groups-1],
                                  HH_APPLET_KEY_Y,
                                  &local_error);

      width = g_key_file_get_integer (keyfile,
                                      groups[n_groups-1],
                                      HH_APPLET_KEY_WIDTH,
                                      NULL);

      height = g_key_file_get_integer (keyfile,
                                       groups[n_groups-1],
                                       HH_APPLET_KEY_HEIGHT,
                                       NULL);

      hildon_home_area_calculate_real_size (&width, &height);

      stack_index = g_key_file_get_integer (keyfile,
                                            groups[n_groups-1],
                                            HH_APPLET_KEY_STACK_INDEX,
                                            NULL);

      if (local_error) goto cleanup;

      else
        {
          Layout *layout = g_new (Layout, 1);

          layout->rect.x = x;
          layout->rect.y = y;
          layout->rect.width = width;
          layout->rect.height = height;
          layout->stack_index = stack_index;

          g_hash_table_insert (priv->layout,
                               g_strdup (groups[n_groups-1]),
                               layout);
        }

      /* Do we already have this applet loaded? */

      list_element = g_list_find_custom (applets,
                                         groups[n_groups-1],
                                         (GCompareFunc)
                                         hildon_desktop_item_find_by_id);

      if (list_element)
        {
          applet = GTK_WIDGET (list_element->data);
          hildon_home_area_move (area,
                                 applet,
                                 x,
                                 y);

          applets = g_list_remove (applets, applet);
        }


      if (applet)
        {
          gtk_widget_set_size_request (applet, width, height);
        }
      n_groups --;
    }

  /* Remove all the applets left in the list, they are no longer  */
  g_list_foreach (applets, (GFunc)gtk_widget_destroy, NULL);

cleanup:
  g_list_free (applets);
  if (groups)
    g_strfreev (groups);

  if (local_error)
    {
      g_warning ("An error occurred when reading HildonHome applets' "
                 "configuration: %s",
               local_error->message);
      g_propagate_error (error, local_error);
    }

  g_key_file_free (keyfile);
}

gboolean
hildon_home_area_get_layout_changed (HildonHomeArea *area)
{
  HildonHomeAreaPriv      *priv;
  g_return_val_if_fail (area, FALSE);
  priv = HILDON_HOME_AREA_GET_PRIVATE (area);

  return priv->layout_changed;
}

void
hildon_home_area_put (HildonHomeArea *area,
                      GtkWidget *widget,
                      gint x, gint y,
                      gint stack_index)
{
  ChildData    *child_data;
  gint          state = 0;

  g_return_if_fail (HILDON_IS_HOME_AREA (area) && GTK_IS_WIDGET (widget));

  if (widget->parent != GTK_WIDGET (area))
    gtk_widget_set_parent (widget, GTK_WIDGET (area));

  if (HILDON_DESKTOP_IS_HOME_ITEM (widget))
    g_object_get (widget,
                  "state", &state,
                  NULL);

  child_data = g_new0 (ChildData, 1);
  child_data->widget = widget;
  child_data->x = x;
  child_data->y = y;
  child_data->stack_index = stack_index;
#ifdef HAVE_X_COMPOSITE
  child_data->state = state;
#endif
  gtk_container_child_set (GTK_CONTAINER (area), widget,
                           "child-data", child_data,
                           NULL);

  child_data->realize_handler =
      g_signal_connect_after (widget, "realize",
                              G_CALLBACK (hildon_home_area_child_realize),
                              area);

  if (GTK_WIDGET_REALIZED (widget))
    {
      hildon_home_area_sort_stack (area);
      child_data->window = GDK_WINDOW_XID (widget->window);
    }

#ifdef HAVE_X_COMPOSITE
  if (HILDON_HOME_AREA_GET_CLASS (area)->composite)
    {
      child_data->style_set_handler =
          g_signal_connect_after (widget, "style-set",
                                  G_CALLBACK (hildon_home_area_child_style_set),
                                  area);
      child_data->size_allocate_handler =
          g_signal_connect (widget, "size-allocate",
                            G_CALLBACK (hildon_home_area_child_size_allocate),
                            area);

      if (HILDON_DESKTOP_IS_HOME_ITEM (widget))
        child_data->state_change_handler =
            g_signal_connect_swapped (widget, "notify::state",
                                      G_CALLBACK (
                                         hildon_home_area_child_state_change),
                                      child_data);



      if (GTK_WIDGET_REALIZED (widget))
        {
          hildon_home_area_create_child_data (HILDON_HOME_AREA (area), widget);
          hildon_home_area_child_style_set (widget,
                                            widget->style,
                                            HILDON_HOME_AREA (area));

          /* HACK: pretend the child is shaped to avoid GDK removing it
           * from the parent's clip (see #412882) */
          ((GdkWindowObject *)widget->window)->shaped = TRUE;
        }
    }
#endif
}

void
hildon_home_area_move (HildonHomeArea *area, GtkWidget *widget, gint x, gint y)
{
  ChildData *data;

  g_return_if_fail (HILDON_IS_HOME_AREA (area) && GTK_IS_WIDGET (widget) &&
                    widget->parent == GTK_WIDGET (area));

  data = hildon_home_area_get_child_data (area, widget);

  g_return_if_fail (data);

  if (GTK_WIDGET_VISIBLE (area) && GTK_WIDGET_VISIBLE (widget))
    {
      GdkRectangle invalid = {data->x + GTK_WIDGET (area)->allocation.x,
                              data->y + GTK_WIDGET (area)->allocation.y,
                              widget->allocation.width,
                              widget->allocation.height};

      gdk_window_invalidate_rect (GTK_WIDGET (area)->window, &invalid, TRUE);
      invalid.x = x + GTK_WIDGET (area)->allocation.x;
      invalid.y = y + GTK_WIDGET (area)->allocation.y;
      gdk_window_invalidate_rect (GTK_WIDGET (area)->window, &invalid, TRUE);
    }

  data->x = x;
  data->y = y;

  hildon_home_area_allocate_child (area, data);

}
