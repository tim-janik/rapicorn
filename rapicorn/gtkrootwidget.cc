/* Rapicorn
 * Copyright (C) 2005-2006 Tim Janik
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
#if     RAPICORN_WITH_GTK

namespace Rapicorn {
namespace Gtk {

/* --- prototypes --- */
static gboolean     root_widget_ancestor_event  (GtkWidget *ancestor, GdkEvent *event, GtkWidget *widget);
static EventContext root_widget_event_context   (GtkWidget *widget,
                                                 GdkEvent  *event = NULL,
                                                 gint      *window_height = NULL);
static bool         gdk_window_has_ancestor     (GdkWindow *window, GdkWindow *ancestor);
static void         call_me                     (GdkWindow *window);

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
  self->last_time = 0;
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
      Requisition rq = root->size_request();    // FIXME: direct root call
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
  if (GTK_WIDGET_REALIZED (widget))
    {
      gdk_window_move_resize (widget->window, allocation->x, allocation->y, allocation->width, allocation->height);
      gdk_flush(); /* resize now, so gravity settings take effect immediately */
    }
  if (root)
    {
      /* root_widget_event_context() relies on a proper GdkWindow size */
      root->enqueue_async (create_event_win_size (root_widget_event_context (widget), allocation->width, allocation->height));
      while (root->check (MainLoop::get_current_time_usecs()))
        root->dispatch();
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
    root->enqueue_async (create_event_cancellation (root_widget_event_context (widget)));
  GTK_WIDGET_CLASS (root_widget_parent_class)->unrealize (widget);
  while (root->check (MainLoop::get_current_time_usecs()))
    root->dispatch();
}

static EventContext
root_widget_event_context (GtkWidget *widget,
                           GdkEvent  *event,
                           gint      *window_height)
{
  RootWidget *self = BIRNET_CANVAS_GTK_ROOT_WIDGET (widget);
  /* extract common event information */
  EventContext econtext;
  double doublex, doubley;
  GdkModifierType modifier_type;
  gint ww = 0, wh = 0;
  if (widget->window)
    {
      GdkWindow *window = widget->window;
      gdk_window_get_size (window, &ww, &wh);
      if (event)
        {
          if (event->any.window == window &&
              gdk_event_get_coords (event, &doublex, &doubley) &&
              gdk_event_get_state (event, &modifier_type))
            {
              self->last_time = gdk_event_get_time (event);
              self->last_x = doublex;
              self->last_y = doubley;
              self->last_modifier = modifier_type;
            }
          econtext.time = self->last_time;
          econtext.synthesized = event->any.send_event;
          doublex = self->last_x;
          doubley = self->last_y;
          modifier_type = self->last_modifier;
        }
      else
        {
          econtext.time = self->last_time;
          econtext.synthesized = true;
          doublex = -1;
          doubley = -1;
          modifier_type = GdkModifierType (0);
        }
    }
  econtext.x = iround (doublex);
  /* vertical canvas/plane axis extends upwards */
  econtext.y = iround (wh - doubley);
  /* ensure modifier compatibility */
  econtext.modifiers = ModifierState (modifier_type);
  BIRNET_STATIC_ASSERT (GDK_SHIFT_MASK   == (int) MOD_SHIFT);
  BIRNET_STATIC_ASSERT (GDK_LOCK_MASK    == (int) MOD_CAPS_LOCK);
  BIRNET_STATIC_ASSERT (GDK_CONTROL_MASK == (int) MOD_CONTROL);
  BIRNET_STATIC_ASSERT (GDK_MOD1_MASK    == (int) MOD_MOD1);
  BIRNET_STATIC_ASSERT (GDK_MOD2_MASK    == (int) MOD_MOD2);
  BIRNET_STATIC_ASSERT (GDK_MOD3_MASK    == (int) MOD_MOD3);
  BIRNET_STATIC_ASSERT (GDK_MOD4_MASK    == (int) MOD_MOD4);
  BIRNET_STATIC_ASSERT (GDK_MOD5_MASK    == (int) MOD_MOD5);
  BIRNET_STATIC_ASSERT (GDK_BUTTON1_MASK == (int) MOD_BUTTON1);
  BIRNET_STATIC_ASSERT (GDK_BUTTON2_MASK == (int) MOD_BUTTON2);
  BIRNET_STATIC_ASSERT (GDK_BUTTON3_MASK == (int) MOD_BUTTON3);
  if (window_height)
    *window_height = wh;
  return econtext;
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
  GdkWindow *window = widget->window;
  /* extract common event information */
  gint window_height = 0;
  EventContext econtext = root_widget_event_context (widget, event, &window_height);
  /* debug events */
  if (0)
    {
      GEnumClass *eclass = (GEnumClass*) g_type_class_ref (GDK_TYPE_EVENT_TYPE);
      const gchar *name = g_enum_get_value (eclass, event->type)->value_name;
      if (strncmp (name, "GDK_", 4) == 0)
        name += 4;
      g_printerr ("Rapicorn-EVENT: %-20s) time=0x%08x synth=%d x=%+7.2f y=%+7.2f state=0x%04x win=%p",
                  name, econtext.time, econtext.synthesized,
                  econtext.x, econtext.y, econtext.modifiers,
                  event->any.window);
      switch (event->type)
        {
          const gchar *mode, *detail;
          GEnumClass *ec1;
        case GDK_ENTER_NOTIFY:
        case GDK_LEAVE_NOTIFY:
          switch (event->crossing.mode) {
          case GDK_CROSSING_NORMAL:             mode = "normal";  break;
          case GDK_CROSSING_GRAB:               mode = "grab";    break;
          case GDK_CROSSING_UNGRAB:             mode = "ungrab";  break;
          default:                              mode = "unknown"; break;
          }
          switch (event->crossing.detail) {
          case GDK_NOTIFY_INFERIOR:             detail = "inferior";  break;
          case GDK_NOTIFY_ANCESTOR:             detail = "ancestor";  break;
          case GDK_NOTIFY_VIRTUAL:              detail = "virtual";   break;
          case GDK_NOTIFY_NONLINEAR:            detail = "nonlinear"; break;
          case GDK_NOTIFY_NONLINEAR_VIRTUAL:    detail = "nolinvirt"; break;
          default:                              detail = "unknown";   break;
          }
          g_printerr (" %s/%s sub=%p", mode, detail, event->crossing.subwindow);
          break;
        case GDK_SCROLL:
          ec1 = (GEnumClass*) g_type_class_ref (GDK_TYPE_SCROLL_DIRECTION);
          detail = g_enum_get_value (ec1, event->scroll.direction)->value_name;
          if (strncmp (detail, "GDK_", 4) == 0)
            detail += 4;
          g_printerr (" %s", detail);
          g_type_class_unref (ec1);
          break;
        default: ;
        }
      g_printerr ("\n");
      g_type_class_unref (eclass);
    }
  /* forward events */
  switch (event->type)
    {
    case GDK_ENTER_NOTIFY:
      root->enqueue_async (create_event_mouse (event->crossing.detail == GDK_NOTIFY_INFERIOR ? MOUSE_MOVE : MOUSE_ENTER, econtext));
      break;
    case GDK_MOTION_NOTIFY:
      root->enqueue_async (create_event_mouse (MOUSE_MOVE, econtext));
      /* trigger new motion events (since we use motion-hint) */
      if (event->any.window == window) // FIXME: should be executed after the move event was processed
        gdk_window_get_pointer (window, NULL, NULL, NULL);
      break;
    case GDK_LEAVE_NOTIFY:
      root->enqueue_async (create_event_mouse (event->crossing.detail == GDK_NOTIFY_INFERIOR ? MOUSE_MOVE : MOUSE_LEAVE, econtext));
      break;
    case GDK_KEY_PRESS:
    case GDK_KEY_RELEASE:
      char *key_name;
      key_name = g_strndup (event->key.string, event->key.length);
      /* KeyValue is modelled to match GDK keyval */
      // FIXME: handled = root->dispatch_key_event (econtext, event->type == GDK_KEY_PRESS, KeyValue (event->key.keyval), key_name);
      root->enqueue_async (create_event_key (event->type == GDK_KEY_PRESS ? KEY_PRESS : KEY_RELEASE, econtext, KeyValue (event->key.keyval), key_name));
      handled = TRUE;
      g_free (key_name);
      break;
    case GDK_FOCUS_CHANGE:
      root->enqueue_async (create_event_focus (event->focus_change.in ? FOCUS_IN : FOCUS_OUT, econtext));
      break;
    case GDK_BUTTON_PRESS:
      root->enqueue_async (create_event_button (BUTTON_PRESS, econtext, event->button.button));
      handled = TRUE;
      // FIXME: handled = root->dispatch_button_event (econtext, true, event->button.button);
      break;
    case GDK_2BUTTON_PRESS:
      root->enqueue_async (create_event_button (BUTTON_PRESS, econtext, event->button.button));
      handled = TRUE;
      break;
    case GDK_3BUTTON_PRESS:
      root->enqueue_async (create_event_button (BUTTON_PRESS, econtext, event->button.button));
      handled = TRUE;
      break;
    case GDK_BUTTON_RELEASE:
      root->enqueue_async (create_event_button (BUTTON_RELEASE, econtext, event->button.button));
      handled = TRUE;
      break;
    case GDK_SCROLL:
      if (event->scroll.direction == GDK_SCROLL_UP)
        root->enqueue_async (create_event_scroll (SCROLL_UP, econtext));
      else if (event->scroll.direction == GDK_SCROLL_LEFT)
        root->enqueue_async (create_event_scroll (SCROLL_LEFT, econtext));
      else if (event->scroll.direction == GDK_SCROLL_RIGHT)
        root->enqueue_async (create_event_scroll (SCROLL_RIGHT, econtext));
      else if (event->scroll.direction == GDK_SCROLL_DOWN)
        root->enqueue_async (create_event_scroll (SCROLL_DOWN, econtext));
      handled = TRUE;
      break;
    case GDK_DELETE:
    case GDK_DESTROY:
    case GDK_UNMAP:
      root->enqueue_async (create_event_cancellation (econtext));
      break;
    case GDK_VISIBILITY_NOTIFY:
      if (event->visibility.state == GDK_VISIBILITY_FULLY_OBSCURED)
        root->enqueue_async (create_event_cancellation (econtext));
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
                gint realy = window_height - (y + h);
                /* render canvas contents */
                Plane plane (x, realy, w, h);
                root->render (plane); // FIXME: direct root call
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
  while (root->check (MainLoop::get_current_time_usecs()))
    root->dispatch();
  return handled;
}

static gboolean
root_widget_ancestor_event (GtkWidget *ancestor,
                            GdkEvent  *event,
                            GtkWidget *widget)
{
  RootWidget *self = BIRNET_CANVAS_GTK_ROOT_WIDGET (widget);
  Root *root = self->root;
  EventContext econtext = root_widget_event_context (widget, event);
  if (!root)
    return false;
  switch (event->type)
    {
    case GDK_DELETE:
    case GDK_DESTROY:
    case GDK_UNMAP:
      if (widget->window && gdk_window_has_ancestor (widget->window, ancestor->window))
        root->enqueue_async (create_event_cancellation (econtext));
      break;
    case GDK_WINDOW_STATE:
      if ((event->window_state.new_window_state & (GDK_WINDOW_STATE_WITHDRAWN | GDK_WINDOW_STATE_ICONIFIED)) &&
          widget->window && gdk_window_has_ancestor (widget->window, ancestor->window))
        root->enqueue_async (create_event_cancellation (econtext));
      break;
    case GDK_VISIBILITY_NOTIFY:
      if (event->visibility.state == GDK_VISIBILITY_FULLY_OBSCURED &&
          widget->window && gdk_window_has_ancestor (widget->window, ancestor->window))
        root->enqueue_async (create_event_cancellation (econtext));
      break;
    default: break;
    }
  while (root->check (MainLoop::get_current_time_usecs()))
    root->dispatch();
  return false;
}

static void
root_widget_dispose (GObject *object)
{
  RootWidget *self = BIRNET_CANVAS_GTK_ROOT_WIDGET (object);
  Root *root = self->root;
  if (root)
    {
      while (root->check (MainLoop::get_current_time_usecs()))
        root->dispatch();
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
    GDK_THREADS_ENTER ();
    gtk_widget_queue_resize (widget);
    GDK_THREADS_LEAVE ();
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
        GDK_THREADS_ENTER ();
        gint ww, wh;
        gdk_window_get_size (widget->window, &ww, &wh);
        /* vertical canvas/plane axis extends upwards */
        gtk_widget_queue_draw_area (widget, area.x, wh - area.y - area.height, area.width, area.height);
        GDK_THREADS_LEAVE ();
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

#endif  /* RAPICORN_WITH_GTK */
