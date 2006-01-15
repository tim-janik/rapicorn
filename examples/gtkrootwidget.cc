/* Rapicorn
 * Copyright (C) 2005 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "gtkrootwidget.hh"

namespace Rapicorn {
namespace Gtk {

/* --- prototypes --- */
static gboolean root_widget_ancestor_event      (GtkWidget *ancestor, GdkEvent *event, GtkWidget *widget);
static bool     gdk_window_has_ancestor         (GdkWindow *window, GdkWindow *ancestor);
static void     call_me                         (GdkWindow *window);

/* --- type definition --- */
typedef RootWidget      BirnetCanvasGtkRootWidget;
typedef RootWidgetClass BirnetCanvasGtkRootWidgetClass;
G_DEFINE_TYPE (BirnetCanvasGtkRootWidget, root_widget, GTK_TYPE_CONTAINER);

/* --- functions --- */
static void
root_widget_init (RootWidget *self)
{
  GtkWidget *widget = GTK_WIDGET (self);
  self->root = NULL;
  self->last_x = -1;
  self->last_y = -1;
  self->last_modifier = GdkModifierType (0);
  gtk_widget_set_redraw_on_allocate (widget, TRUE);
  gtk_widget_set_double_buffered (widget, FALSE);
  gtk_widget_show (widget);
}

static void
root_widget_size_request (GtkWidget      *widget,
                          GtkRequisition *requisition)
{
  RootWidget *self = BIRNET_CANVAS_GTK_ROOT_WIDGET (widget);
  Root *root = self->root;
  if (root)
    {
      Requisition rq = root->size_request();
      requisition->width = iceil (rq.width);
      requisition->height = iceil (rq.height);
    }
}

static void
root_widget_size_allocate (GtkWidget      *widget,
                           GtkAllocation  *allocation)
{
  widget->allocation = *allocation;
  RootWidget *self = BIRNET_CANVAS_GTK_ROOT_WIDGET (widget);
  Root *root = self->root;
  if (root)
    {
      Allocation area;
      area.width = allocation->width;
      area.height = allocation->height;
      root->set_allocation (area);
    }
  if (GTK_WIDGET_REALIZED (widget))
    {
      gdk_window_move_resize (widget->window, allocation->x, allocation->y, allocation->width, allocation->height);
      gdk_flush(); /* resize now, so gravity settings take effect immediately */
    }
}

static void
root_widget_realize (GtkWidget *widget)
{
  RootWidget *self = BIRNET_CANVAS_GTK_ROOT_WIDGET (widget);
  Root *root = self->root;
  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
  GdkWindowAttr attributes = { 0, };
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);
  attributes.event_mask = GDK_EXPOSURE_MASK | GDK_ALL_EVENTS_MASK;
  gint attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  widget->window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
  gdk_window_set_user_data (widget->window, widget);
  widget->style = gtk_style_attach (widget->style, widget->window);
  Color argb_color = root ? dynamic_cast<Item*> (root)->style()->standard_color (STATE_NORMAL, COLOR_BACKGROUND) : Color (0xff808080);
  GdkColor gdkcolor = { 0, };
  gdkcolor.red = argb_color.red() * 0x0101;
  gdkcolor.green = argb_color.green() * 0x0101;
  gdkcolor.blue = argb_color.blue() * 0x0101;
  printf ("backgreound: %x %x %x\n", gdkcolor.red, gdkcolor.green, gdkcolor.blue);
  if (gdk_colormap_alloc_color (attributes.colormap, &gdkcolor, FALSE, TRUE))
    {
      gdk_window_set_background (widget->window, &gdkcolor);
      gdk_colormap_free_colors (attributes.colormap, &gdkcolor, 1);
    }
  /* for flicker-free static gravity, we must set static-gravities on the complete ancestry */
  for (GdkWindow *twindow = widget->window; twindow; twindow = gdk_window_get_parent (twindow))
    gdk_window_set_static_gravities (twindow, TRUE);
  /* catch toplevel unmap events */
  GtkWidget *toplevel = gtk_widget_get_toplevel (widget);
  gtk_widget_add_events (toplevel, GDK_STRUCTURE_MASK);
  g_signal_connect (toplevel, "event", G_CALLBACK (root_widget_ancestor_event), self);
  if (widget->parent == toplevel && toplevel->window &&  // FIXME: hacking up parent GtkWindow
      gdk_colormap_alloc_color (attributes.colormap, &gdkcolor, FALSE, TRUE))
    {
      gtk_widget_set_app_paintable (toplevel, TRUE);
      gdk_window_set_background (toplevel->window, &gdkcolor);
      gdk_colormap_free_colors (attributes.colormap, &gdkcolor, 1);
    }
}

static void
root_widget_unrealize (GtkWidget *widget)
{
  RootWidget *self = BIRNET_CANVAS_GTK_ROOT_WIDGET (widget);
  Root *root = self->root;
  GtkWidget *toplevel = gtk_widget_get_toplevel (widget);
  g_signal_handlers_disconnect_by_func (toplevel, (void*) (root_widget_ancestor_event), self);
  if (root)
    root->dispatch_cancel_events ();
  GTK_WIDGET_CLASS (root_widget_parent_class)->unrealize (widget);
}

static gboolean
root_widget_event (GtkWidget *widget,
                   GdkEvent  *event)
{
  RootWidget *self = BIRNET_CANVAS_GTK_ROOT_WIDGET (widget);
  Root *root = self->root;
  if (!root)
    return false;
  bool handled = FALSE;
  /* extract common event information */
  GdkWindow *window = widget->window;
  EventContext econtext;
  gint ww, wh;
  if (true)
    {
      gdk_window_get_size (window, &ww, &wh);
      double doublex, doubley;
      GdkModifierType modifier_type;
      if (event->any.window != window ||
          !gdk_event_get_coords (event, &doublex, &doubley) ||
          !gdk_event_get_state (event, &modifier_type))
        {
          doublex = self->last_x;
          doubley = self->last_y;
          modifier_type = self->last_modifier;
        }
      econtext.time = gdk_event_get_time (event);
      econtext.synthesized = event->any.send_event;
      econtext.x = iround (doublex);
      /* vertical canvas/plane axis extends upwards */
      econtext.y = iround (wh - doubley);
      /* ensure modifier compatibility */
      econtext.modifiers = ModifierState (modifier_type);
      static_assert (GDK_SHIFT_MASK   == (int) MOD_SHIFT);
      static_assert (GDK_LOCK_MASK    == (int) MOD_CAPS_LOCK);
      static_assert (GDK_CONTROL_MASK == (int) MOD_CONTROL);
      static_assert (GDK_MOD1_MASK    == (int) MOD_MOD1);
      static_assert (GDK_MOD2_MASK    == (int) MOD_MOD2);
      static_assert (GDK_MOD3_MASK    == (int) MOD_MOD3);
      static_assert (GDK_MOD4_MASK    == (int) MOD_MOD4);
      static_assert (GDK_MOD5_MASK    == (int) MOD_MOD5);
      static_assert (GDK_BUTTON1_MASK == (int) MOD_BUTTON1);
      static_assert (GDK_BUTTON2_MASK == (int) MOD_BUTTON2);
      static_assert (GDK_BUTTON3_MASK == (int) MOD_BUTTON3);
    }
  /* forward events */
  switch (event->type)
    {
    case GDK_ENTER_NOTIFY:
      handled = root->dispatch_move_event (econtext);
      break;
    case GDK_MOTION_NOTIFY:
      handled = root->dispatch_move_event (econtext);
      /* trigger new motion events (since we use motion-hint) */
      if (event->any.window == window)
        gdk_window_get_pointer (window, NULL, NULL, NULL);
      break;
    case GDK_LEAVE_NOTIFY:
      handled = root->dispatch_leave_event (econtext);
      break;
    case GDK_KEY_PRESS:
    case GDK_KEY_RELEASE:
      char *key_name;
      key_name = g_strndup (event->key.string, event->key.length);
      /* KeyValue is modelled to match GDK keyval */
      handled = root->dispatch_key_event (econtext, event->type == GDK_KEY_PRESS, KeyValue (event->key.keyval), key_name);
      g_free (key_name);
      break;
    case GDK_BUTTON_PRESS:
    case GDK_2BUTTON_PRESS:
    case GDK_3BUTTON_PRESS:
      handled = root->dispatch_button_event (econtext, true, event->button.button);
      break;
    case GDK_BUTTON_RELEASE:
      handled = root->dispatch_button_event (econtext, false, event->button.button);
      break;
    case GDK_SCROLL:
      if (event->scroll.direction == GDK_SCROLL_UP)
        handled = root->dispatch_scroll_event (econtext, SCROLL_UP);
      else if (event->scroll.direction == GDK_SCROLL_LEFT)
        handled = root->dispatch_scroll_event (econtext, SCROLL_LEFT);
      else if (event->scroll.direction == GDK_SCROLL_RIGHT)
        handled = root->dispatch_scroll_event (econtext, SCROLL_RIGHT);
      else if (event->scroll.direction == GDK_SCROLL_DOWN)
        handled = root->dispatch_scroll_event (econtext, SCROLL_DOWN);
      break;
    case GDK_FOCUS_CHANGE:
      handled = root->dispatch_focus_event (econtext, event->focus_change.in);
      break;
    case GDK_DELETE:
    case GDK_DESTROY:
    case GDK_UNMAP:
      root->dispatch_cancel_events ();
      break;
    case GDK_VISIBILITY_NOTIFY:
      if (event->visibility.state == GDK_VISIBILITY_FULLY_OBSCURED)
        root->dispatch_cancel_events ();
      call_me (window);
      break;
    case GDK_EXPOSE:
      GdkRectangle *areas;
      gint n_areas;
      if (event->expose.region)
        gdk_region_get_rectangles (event->expose.region, &areas, &n_areas);
      else
        n_areas = 1, areas = (GdkRectangle*) g_memdup (&event->expose.area, sizeof (event->expose.area));
      for (gint j = 0; j < n_areas; j++)
        {
          const GdkRectangle *area = &areas[j];
          gint nl = 4096, ml = 4096;
          if (0)
            {
              /* prefer incremental screen updates. we split up larger areas
               * into small tiles which are blitted immediately. the GdkRGB
               * tile size is 128x128 so anything beyond that is split up anyways.
               */
              nl = 64;
              ml = 128;
            }
          else if (0)
            {
              nl = 800;
              ml = 600;
            }
          for (gint m = 0; m < area->height; m += ml)
            for (gint n = 0; n < area->width; n += nl)
              {
                gint x = area->x + n, y = area->y + m, w = MIN (nl, area->width - n), h = MIN (ml, area->height - m);
                /* vertical canvas/plane axis extends upwards */
                gint realy = wh - (y + h);
                /* render canvas contents */
                Plane plane (x, realy, w, h);
                root->render (plane);
                /* blit to screen */
                GdkPixbuf *pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE /* alpha */, 8 /* bits */, plane.width(), plane.height());
                if (plane.rgb_convert (gdk_pixbuf_get_width (pixbuf), gdk_pixbuf_get_height (pixbuf),
                                       gdk_pixbuf_get_rowstride (pixbuf), gdk_pixbuf_get_pixels (pixbuf)))
                  gdk_draw_pixbuf (window, widget->style->black_gc, pixbuf, 0, 0, x, y,
                                   gdk_pixbuf_get_width (pixbuf), gdk_pixbuf_get_height (pixbuf), GDK_RGB_DITHER_MAX, x, y);
                gdk_pixbuf_unref (pixbuf);
              }
        }
      g_free (areas);
      break;
    default:
    case GDK_NO_EXPOSE:
    case GDK_PROPERTY_NOTIFY:   case GDK_CLIENT_EVENT:
    case GDK_CONFIGURE:         case GDK_MAP:
    case GDK_SELECTION_CLEAR:   case GDK_SELECTION_REQUEST:     case GDK_SELECTION_NOTIFY:
    case GDK_PROXIMITY_IN:      case GDK_PROXIMITY_OUT:
    case GDK_WINDOW_STATE:
      /* nothing to forward */
      break;
    }
  return handled;
}

static gboolean
root_widget_ancestor_event (GtkWidget *ancestor,
                            GdkEvent  *event,
                            GtkWidget *widget)
{
  RootWidget *self = BIRNET_CANVAS_GTK_ROOT_WIDGET (widget);
  Root *root = self->root;
  if (!root)
    return false;
  switch (event->type)
    {
    case GDK_DELETE:
    case GDK_DESTROY:
    case GDK_UNMAP:
      if (widget->window && gdk_window_has_ancestor (widget->window, ancestor->window))
        root->dispatch_cancel_events ();
      break;
    case GDK_WINDOW_STATE:
      if ((event->window_state.new_window_state & (GDK_WINDOW_STATE_WITHDRAWN | GDK_WINDOW_STATE_ICONIFIED)) &&
          widget->window && gdk_window_has_ancestor (widget->window, ancestor->window))
        root->dispatch_cancel_events ();
      break;
    case GDK_VISIBILITY_NOTIFY:
      if (event->visibility.state == GDK_VISIBILITY_FULLY_OBSCURED &&
          widget->window && gdk_window_has_ancestor (widget->window, ancestor->window))
        root->dispatch_cancel_events ();
      break;
    default: break;
    }
  return false;
}

static void
root_widget_dispose (GObject *object)
{
  RootWidget *self = BIRNET_CANVAS_GTK_ROOT_WIDGET (object);
  Root *root = self->root;
  if (root)
    {
      root->ref_diag ("Root");
      root->unref();
      self->root = NULL;
    }
  G_OBJECT_CLASS (root_widget_parent_class)->dispose (object);
}

static void
root_widget_class_init (RootWidgetClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gobject_class->dispose = root_widget_dispose;

  widget_class->size_request = root_widget_size_request;
  widget_class->size_allocate = root_widget_size_allocate;
  widget_class->realize = root_widget_realize;
  widget_class->unrealize = root_widget_unrealize;
  widget_class->event = root_widget_event;
}

class RootWidgetTrampoline {
  RootWidget *self;
public:
  void
  invalidated ()
  {
    GtkWidget *widget = GTK_WIDGET (self);
    gtk_widget_queue_resize (widget);
  }
  void
  delete_this ()
  {
    delete this;
  }
  void
  exposed (const Allocation &area)
  {
    GtkWidget *widget = GTK_WIDGET (self);
    if (widget->window)
      {
        gint ww, wh;
        gdk_window_get_size (widget->window, &ww, &wh);
        /* vertical canvas/plane axis extends upwards */
        gtk_widget_queue_draw_area (widget, area.x, wh - area.y - area.height, area.width, area.height);
      }
  }
  RootWidgetTrampoline (RootWidget *cself) :
    self (cself)
  {
    assert (self->root != NULL);
    Root *root = self->root;
    root->sig_expose += slot (*this, &RootWidgetTrampoline::exposed);
    root->sig_invalidate += slot (*this, &RootWidgetTrampoline::invalidated);
    root->sig_finalize += slot (*this, &RootWidgetTrampoline::delete_this);
  }
};

GtkWidget*
root_widget_from_root (Root &root)
{
  GtkWidget *widget = gtk_widget_new (BIRNET_CANVAS_GTK_TYPE_ROOT_WIDGET, NULL);
  RootWidget *self = BIRNET_CANVAS_GTK_ROOT_WIDGET (widget);
  self->root = &root;
  new RootWidgetTrampoline (self);
  return widget;
}

void
gtk_window_set_min_size (GtkWindow *window, gint min_width, gint min_height)
{
  GdkGeometry geometry = { 0, };
  geometry.min_width = min_width;
  geometry.min_height = min_height;
  gtk_window_set_geometry_hints (window, NULL, &geometry, GDK_HINT_MIN_SIZE);
}

static bool
gdk_window_has_ancestor (GdkWindow *window,
                         GdkWindow *ancestor)
{
  while (window)
    if (window == ancestor)
      return true;
    else
      window = gdk_window_get_parent (window);
  return false;
}

#include <X11/Xlib.h>
#include <gdk/gdkx.h>

static void
call_me (GdkWindow *window)
{
  const char *val1 = XGetDefault (GDK_WINDOW_XDISPLAY (window), "Xft", "antialias");
  const char *val2 = XGetDefault (GDK_WINDOW_XDISPLAY (window), "Xft", "dpi");
  if (0)
    g_printerr ("XGetDefault(): dpi=%s antialias=%s\n", val2, val1);
}

static void
gdk_window_set_any_gravity (GdkWindow *window, GdkGravity g, bool win, bool bit)
{
  XSetWindowAttributes xattributes = { 0, };
  guint xattributes_mask = 0;
  g_return_if_fail (window != NULL);
  xattributes.bit_gravity = ForgetGravity;
  xattributes.win_gravity = ForgetGravity;
  xattributes_mask |= CWBitGravity | CWWinGravity;
  if (g >= GDK_GRAVITY_NORTH_WEST)
    {
      xattributes.bit_gravity = g;
      xattributes.win_gravity = g;
    }
  XChangeWindowAttributes (GDK_WINDOW_XDISPLAY (window), GDK_WINDOW_XID (window), CWBitGravity,  &xattributes);
}

void
gdk_window_set_gravity (GdkWindow *window, GdkGravity g)
{
  gdk_window_set_any_gravity (window, g, TRUE, TRUE);
}

void
gdk_window_set_bit_gravity (GdkWindow *window, GdkGravity g)
{
  gdk_window_set_any_gravity (window, g, FALSE, TRUE);
}

void
gdk_window_set_win_gravity (GdkWindow *window, GdkGravity g)
{
  gdk_window_set_any_gravity (window, g, TRUE, FALSE);
}

} // Gtk
} // Rapicorn
