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
#include "viewport.hh"

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#define DEBUG_EVENTS    0

namespace { // Anon
using namespace Rapicorn;

/* --- prototypes --- */
class ViewportGtk;
static gboolean     rapicorn_viewport_ancestor_event (GtkWidget  *ancestor,
                                                      GdkEvent   *event,
                                                      GtkWidget  *widget);
static bool         gdk_window_has_ancestor          (GdkWindow  *window,
                                                      GdkWindow  *ancestor);
static bool         gtk_widget_has_local_display     (GtkWidget  *widget);
typedef enum {
  BACKING_STORE_NOT_USEFUL,
  BACKING_STORE_WHEN_MAPPED,
  BACKING_STORE_ALWAYS
} BackingStore;
static BackingStore gdk_window_enable_backing        (GdkWindow  *window,
                                                      BackingStore bs_type);

/* --- GDK_THREADS_* / AutoLocker glue --- */
struct RapicronGdkSyncLock {
  void
  lock()
  {
    GDK_THREADS_ENTER();
    /* protect execution of any Gtk/Gdk functions */
  }
  void
  unlock()
  {
    /* X commands enqueued by any Gtk/Gdk functions so far
     * may still be queued and need to be flushed. also, any
     * X events that may have arrived already need to be
     * handled by the gtk_main() loop. waking up the default
     * main context will do exactly this, wake up gtk_main(),
     * and call XPending() which flushes any queued events.
     */
    g_main_context_wakeup (NULL);
    /* unprotect Gtk/Gdk */
    GDK_THREADS_LEAVE();
  }
};
static RapicronGdkSyncLock GTK_GDK_THREAD_SYNC;

/* --- RapicornViewport --- */
#define RAPICORN_TYPE_VIEWPORT              (rapicorn_viewport_get_type ())
#define RAPICORN_VIEWPORT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), RAPICORN_TYPE_VIEWPORT, RapicornViewport))
#define RAPICORN_VIEWPORT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), RAPICORN_TYPE_VIEWPORT, RapicornViewportClass))
#define RAPICORN_IS_VIEWPORT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), RAPICORN_TYPE_VIEWPORT))
#define RAPICORN_IS_VIEWPORT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), RAPICORN_TYPE_VIEWPORT))
#define RAPICORN_VIEWPORT_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), RAPICORN_TYPE_VIEWPORT, RapicornViewportClass))
struct RapicornViewportClass : public GtkContainerClass {};
struct RapicornViewport : public GtkContainer {
  ViewportGtk       *viewport;
  GdkVisibilityState visibility_state;
  bool               fast_local_blitting;
  guint32            last_time;
  guint32            last_motion_time;
  BackingStore       backing_store;
  GdkModifierType    last_modifier;
  double             last_x, last_y;
};
G_DEFINE_TYPE (RapicornViewport, rapicorn_viewport, GTK_TYPE_CONTAINER);
static ViewportGtk *rapicorn_viewport__cxxinit_viewport;        // protected by global GDK lock
static uint         rapicorn_viewport__alive_counter;           // protected by global GDK lock
static bool         rapicorn_viewport__auto_quit_gtk = false;   // protected by global GDK lock

/* --- ViewportGtk class --- */
struct ViewportGtk : public virtual Viewport {
  union {
    RapicornViewport   *m_viewport;
    GtkWidget          *m_widget;
  };
  WindowType            m_window_type;
  EventReceiver        &m_receiver;
  uint                  m_draw_counter;
  bool                  m_splash_screen;
  float                 m_root_x, m_root_y;
  float                 m_request_width, m_request_height;
  WindowState           m_window_state;
  Color                 m_average_background;
  explicit              ViewportGtk             (const String   &backend_name,
                                                 WindowType      viewport_type,
                                                 EventReceiver  &receiver);
  virtual void          present_viewport        ();
  virtual void          trigger_hint_action     (WindowHint     hint);
  virtual void          start_user_move         (uint           button,
                                                 float          root_x,
                                                 float          root_y);
  virtual void          start_user_resize       (uint           button,
                                                 float          root_x,
                                                 float          root_y,
                                                 AnchorType     edge);
  virtual void          show                    (void);
  virtual void          hide                    (void);
  virtual void          blit_plane              (Plane          *plane,
                                                 uint            draw_stamp);
  virtual void          copy_area               (double          src_x,
                                                 double          src_y,
                                                 double          width,
                                                 double          height,
                                                 double          dest_x,
                                                 double          dest_y);
  virtual void          invalidate_plane        (const std::vector<Rect> &rects,
                                                 uint                     draw_stamp);
  virtual void          enqueue_win_draws       (void);
  virtual void          enqueue_mouse_moves     (void);
  virtual uint          last_draw_stamp         ();
  virtual State         get_state               ();
  virtual void          set_config              (const Config   &config);
  void                  enqueue_locked          (Event         *event);
  bool                  is_splash_screen        () { return m_window_type & WINDOW_TYPE_SPLASH; }
};
static Viewport::Factory<ViewportGtk> viewport_gtk_factory ("GtkWindow"); // FIXME: should register only after gtk_init() has been called

/* --- ViewportGtk methods --- */
ViewportGtk::ViewportGtk (const String  &backend_name,
                          WindowType     viewport_type,
                          EventReceiver &receiver) :
  m_viewport (NULL), m_window_type (viewport_type),
  m_receiver (receiver), m_draw_counter (0),
  m_root_x (NAN), m_root_y (NAN),
  m_request_width (33), m_request_height (33),
  m_window_state (WindowState (0)), m_average_background (0xff808080)
{
  AutoLocker locker (GTK_GDK_THREAD_SYNC);
  rapicorn_viewport__alive_counter++;
  bool is_override_redirect = (m_window_type == WINDOW_TYPE_DESKTOP ||
                               m_window_type == WINDOW_TYPE_DROPDOWN_MENU ||
                               m_window_type == WINDOW_TYPE_POPUP_MENU ||
                               m_window_type == WINDOW_TYPE_TOOLTIP ||
                               m_window_type == WINDOW_TYPE_NOTIFICATION ||
                               m_window_type == WINDOW_TYPE_COMBO ||
                               m_window_type == WINDOW_TYPE_DND);
  GtkWindow *window = NULL;
  if (backend_name == "GtkWindow")
    window = (GtkWindow*) gtk_window_new (is_override_redirect ? GTK_WINDOW_POPUP : GTK_WINDOW_TOPLEVEL);
  /* set GdkWindowTypeHint */
  switch (m_window_type)
    {
    case WINDOW_TYPE_NORMAL:            gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_NORMAL);		break;
    case WINDOW_TYPE_DESKTOP:           gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_DESKTOP);	break;
    case WINDOW_TYPE_DOCK:              gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_DOCK);		break;
    case WINDOW_TYPE_TOOLBAR:           gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_TOOLBAR);	break;
    case WINDOW_TYPE_MENU:              gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_MENU);		break;
    case WINDOW_TYPE_UTILITY:           gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_UTILITY);	break;
    case WINDOW_TYPE_SPLASH:            gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_SPLASHSCREEN);	break;
    case WINDOW_TYPE_DIALOG:            gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_DIALOG);		break;
#if GTK_CHECK_VERSION (2, 9, 0)
    case WINDOW_TYPE_DROPDOWN_MENU:     gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_DROPDOWN_MENU);  break;
    case WINDOW_TYPE_POPUP_MENU:        gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_POPUP_MENU);	break;
    case WINDOW_TYPE_TOOLTIP:           gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_TOOLTIP);	break;
    case WINDOW_TYPE_NOTIFICATION:      gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_NOTIFICATION);	break;
    case WINDOW_TYPE_COMBO:             gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_COMBO);		break;
    case WINDOW_TYPE_DND:               gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_DND);		break;
#endif
    default: break;
    }
  BIRNET_ASSERT (rapicorn_viewport__cxxinit_viewport == NULL);
  rapicorn_viewport__cxxinit_viewport = this;
  m_viewport = RAPICORN_VIEWPORT (g_object_ref (g_object_new (RAPICORN_TYPE_VIEWPORT, "parent", window, NULL)));
  rapicorn_viewport__cxxinit_viewport = NULL;
  BIRNET_ASSERT (m_viewport->viewport == this);
  g_object_set_data (G_OBJECT (m_viewport), "RapicornViewport-my-GtkWindow", window); // flag to indicate the window is owned by RapicornViewport
}
// FIXME: add rapicorn_viewport__alive_counter--; to ~ViewportGtk and gtk_main_quit() via idle if 0

static GtkWindow*
rapicorn_viewport_get_toplevel_window (RapicornViewport *rviewport)
{
  GtkWidget *widget = GTK_WIDGET (rviewport);
  if (widget)
    while (widget->parent)
      widget = widget->parent;
  return GTK_IS_WINDOW (widget) ? (GtkWindow*) widget : NULL;
}

static GtkWindow*
rapicorn_viewport_get_my_window (RapicornViewport *rviewport)
{
  GtkWindow *window = rapicorn_viewport_get_toplevel_window (rviewport);
  if (window && window == g_object_get_data (G_OBJECT (rviewport), "RapicornViewport-my-GtkWindow"))
    return window;
  return NULL;
}

void
ViewportGtk::present_viewport ()
{
  AutoLocker locker (GTK_GDK_THREAD_SYNC);
  GtkWindow *window = rapicorn_viewport_get_toplevel_window (m_viewport);
  if (window && GTK_WIDGET_DRAWABLE (window))
    gtk_window_present (window);
}

void
ViewportGtk::enqueue_locked (Event *event)
{
  // AutoLocker locker (GTK_GDK_THREAD_SYNC);
  m_receiver.enqueue_async (event);
}

void
ViewportGtk::start_user_move (uint           button,
                              float          root_x,
                              float          root_y)
{
  AutoLocker locker (GTK_GDK_THREAD_SYNC);
  GtkWindow *window = rapicorn_viewport_get_toplevel_window (m_viewport);
  if (window && GTK_WIDGET_DRAWABLE (window))
    gtk_window_begin_move_drag (window, button, root_x, root_y, GDK_CURRENT_TIME);
}

static GdkWindowEdge
get_gdk_window_edge (AnchorType anchor)
{
  switch (anchor)
    {
    case ANCHOR_NONE:
    case ANCHOR_CENTER:         return GDK_WINDOW_EDGE_SOUTH_EAST;
    case ANCHOR_EAST:           return GDK_WINDOW_EDGE_EAST;
    case ANCHOR_NORTH_EAST:     return GDK_WINDOW_EDGE_NORTH_EAST;
    case ANCHOR_NORTH:          return GDK_WINDOW_EDGE_NORTH;
    case ANCHOR_NORTH_WEST:     return GDK_WINDOW_EDGE_NORTH_WEST;
    case ANCHOR_WEST:           return GDK_WINDOW_EDGE_WEST;
    case ANCHOR_SOUTH_WEST:     return GDK_WINDOW_EDGE_SOUTH_WEST;
    case ANCHOR_SOUTH:          return GDK_WINDOW_EDGE_SOUTH;
    case ANCHOR_SOUTH_EAST:     return GDK_WINDOW_EDGE_SOUTH_EAST;
    }
  g_assert_not_reached();
}

void
ViewportGtk::start_user_resize (uint           button,
                                float          root_x,
                                float          root_y,
                                AnchorType     edge)
{
  AutoLocker locker (GTK_GDK_THREAD_SYNC);
  GtkWindow *window = rapicorn_viewport_get_toplevel_window (m_viewport);
  if (window && GTK_WIDGET_DRAWABLE (window))
    gtk_window_begin_resize_drag (window, get_gdk_window_edge (edge), button, root_x, root_y, GDK_CURRENT_TIME);
}

void
ViewportGtk::show (void)
{
  AutoLocker locker (GTK_GDK_THREAD_SYNC);
  if (m_viewport)
    {
      gtk_widget_show (m_widget);
      GtkWindow *window = rapicorn_viewport_get_my_window (m_viewport);
      if (window)
        gtk_widget_show (GTK_WIDGET (window));
    }
}

void
ViewportGtk::hide (void)
{
  AutoLocker locker (GTK_GDK_THREAD_SYNC);
  if (m_viewport)
    {
      GtkWindow *window = rapicorn_viewport_get_my_window (m_viewport);
      if (window)
        {
          gtk_widget_hide (GTK_WIDGET (window));
          gtk_widget_unrealize (GTK_WIDGET (window));
        }
      gtk_widget_hide (m_widget);
    }
}

#if 0
struct IdleBlitter {
  RapicornViewport *self;
  Plane            *plane;
  GdkPixbuf        *pixbuf;
  uint              draw_stamp;
  int               px, py, window_height, j, k;
  IdleBlitter (RapicornViewport *_viewport,
               Plane            *_plane,
               uint              _draw_stamp) :
    self (_viewport), plane (_plane), pixbuf (NULL),
    draw_stamp (_draw_stamp),
    px (0), py (0), window_height (0), j (0), k(0)
  {
    g_object_ref (G_OBJECT (self));
  }
  ~IdleBlitter()
  {
    g_object_unref (G_OBJECT (self));
    if (plane)
      delete plane;
    if (pixbuf)
      gdk_pixbuf_unref (pixbuf);
  }
  bool
  blit_plane ()
  {
    ViewportGtk *viewport = self->viewport;
    GtkWidget *widget = GTK_WIDGET (self);
    /* ignore outdated planes, especially on slow displays.
     * this is especially important on remote displays where due to the
     * incremental updates, multiple concurring blit-queues can pile
     * up unless outdated planes are discarded.
     */
    if (!GTK_WIDGET_DRAWABLE (widget) ||
        (!self->fast_local_blitting &&                          /* when on remote display */
         draw_stamp != viewport->m_draw_counter))               /* outdated plane */
      {
        /* this window possibly has backing store enabled on the server,
         * which reduces the amount of expose events sent. here, we rely
         * on future expose events, so we better make sure to get them.
         */
        if (self->backing_store != BACKING_STORE_NOT_USEFUL)
          {
            int ww, wh;
            gdk_window_get_size (widget->window, &ww, &wh);
            gdk_window_clear_area_e (widget->window, 0, 0, ww, wh);
          }
        return false;
      }
    /* when blitting to screen, we prefer incremental screen updates to reduce
     * latency (at least for slow X connections) and increase responsiveness.
     * we split up larger areas into small tiles which are blitted individually.
     * the GdkRGB tile size is usually around 128x128 pixels so anything beyond
     * that is furtherly split up anyways.
     */
    if (!pixbuf)
      {
        /* initial setup */
        pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE /* alpha */, 8 /* bits */, plane->width(), plane->height());
        bool success = plane->rgb_convert (gdk_pixbuf_get_width (pixbuf), gdk_pixbuf_get_height (pixbuf),
                                           gdk_pixbuf_get_rowstride (pixbuf), gdk_pixbuf_get_pixels (pixbuf));
        BIRNET_ASSERT (success);
        gdk_window_get_size (widget->window, NULL, &window_height);
        px = plane->xstart();
        py = window_height - (plane->ystart() + gdk_pixbuf_get_height (pixbuf));
        delete plane;
        plane = NULL;
      }
    /* loop body */
    int xstep = 256, ystep = 64;
    if (self->fast_local_blitting)
      {
        /* no real need to tile on local displays */
        xstep = 8192;
        ystep = 8192;
      }
    int pw = gdk_pixbuf_get_width (pixbuf), ph = gdk_pixbuf_get_height (pixbuf);
    int dest_x = px + j, dest_y = py + k, w = MIN (xstep, pw - j), h = MIN (ystep, ph - k);
    gdk_draw_pixbuf (widget->window, widget->style->black_gc,
                     pixbuf, j, k, /* src (x,y) */
                     dest_x, dest_y, w, h,
                     GDK_RGB_DITHER_MAX, dest_x, dest_y);
    /* loop counter increments and checks */
    j += xstep;
    if (j < pw)
      return true; /* continue */
    j = 0;
    k += ystep;
    if (k < ph)
      return true; /* continue */
    return false; /* all done */
  }
  static gboolean
  idle_blit_plane_locked (IdleBlitter *iblitter)
  {
    bool repeat = iblitter->blit_plane();
    if (!repeat)
      delete iblitter;
    return repeat;
  }
  static gboolean
  idle_blit_plane (gpointer data)
  {
    AutoLocker locker (GTK_GDK_THREAD_SYNC);
    IdleBlitter *iblitter = static_cast<IdleBlitter*> (data);
    bool repeat = idle_blit_plane_locked (iblitter);
    return repeat;
  }
};
#endif

void
ViewportGtk::blit_plane (Plane *plane,
                         uint   draw_stamp)
{
  AutoLocker locker (GTK_GDK_THREAD_SYNC);
  if (m_viewport)
    {
      int priority;
      if (m_viewport->fast_local_blitting)
        priority = -G_MAXINT / 2;       /* run with PRIORITY_NOW to blit immediately on local displays */
      else
        priority = GTK_PRIORITY_REDRAW; /* allow event processing to interrupt blitting on remote displays */
#if 0
      if (GTK_WIDGET_DRAWABLE (m_widget))
        {
          /* add idle handler to g_main_context_default() to be executed from gtk_main() */
          g_idle_add_full (priority, IdleBlitter::idle_blit_plane, new IdleBlitter (m_viewport, plane, draw_stamp), NULL);
          /* call idle handler directly */
          IdleBlitter *ib = new IdleBlitter (m_viewport, plane, draw_stamp);
          while (TRUE)
            {
              bool done = !IdleBlitter::idle_blit_plane_locked (ib);
              if (done)
                break;
            }
        }
#endif
      if (GTK_WIDGET_DRAWABLE (m_widget))
        {
          /* convert to pixbuf */
          GdkPixbuf *pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE /* alpha */, 8 /* bits */, plane->width(), plane->height());
          bool success = plane->rgb_convert (gdk_pixbuf_get_width (pixbuf), gdk_pixbuf_get_height (pixbuf),
                                             gdk_pixbuf_get_rowstride (pixbuf), gdk_pixbuf_get_pixels (pixbuf));
          BIRNET_ASSERT (success);
          int window_height;
          gdk_window_get_size (m_widget->window, NULL, &window_height);
          int dest_x = plane->xstart();
          int dest_y = window_height - (plane->ystart() + gdk_pixbuf_get_height (pixbuf));
          delete plane;
          plane = NULL;
          /* blit pixbuf to screen */
          int pw = gdk_pixbuf_get_width (pixbuf), ph = gdk_pixbuf_get_height (pixbuf);
          gdk_draw_pixbuf (m_widget->window, m_widget->style->black_gc,
                           pixbuf, 0, 0, /* src (x,y) */
                           dest_x, dest_y, pw, ph,
                           GDK_RGB_DITHER_MAX, dest_x, dest_y);
          gdk_pixbuf_unref (pixbuf);
        }
    }
  else
    delete plane;
}

void
ViewportGtk::copy_area (double          src_x,
                        double          src_y,
                        double          width,
                        double          height,
                        double          dest_x,
                        double          dest_y)
{
  AutoLocker locker (GTK_GDK_THREAD_SYNC);
  if (GTK_WIDGET_DRAWABLE (m_widget))
    {
      /* copy the area */
      int window_height;
      gdk_window_get_size (m_widget->window, NULL, &window_height);
      gdk_gc_set_exposures (m_widget->style->black_gc, TRUE);
      gdk_draw_drawable (m_widget->window, m_widget->style->black_gc, // FIXME: use gdk_window_move_region() with newer Gtk+
                         m_widget->window, src_x, window_height - src_y - height,
                         dest_x, window_height - dest_y - height, width, height);
      gdk_gc_set_exposures (m_widget->style->black_gc, FALSE);
      /* ensure last GraphicsExpose events are processed before the next copy */
      GdkEvent *event = gdk_event_get_graphics_expose (m_widget->window);
      while (event)
        {
          gtk_widget_send_expose (m_widget, event);
          bool last = event->expose.count == 0;
          gdk_event_free (event);
          event = last ? NULL : gdk_event_get_graphics_expose (m_widget->window);
        }
    }
}

void
ViewportGtk::invalidate_plane (const std::vector<Rect> &rects,
                               uint                     draw_stamp)
{
  AutoLocker locker (GTK_GDK_THREAD_SYNC);
  if (GTK_WIDGET_DRAWABLE (m_widget) &&
      draw_stamp == m_draw_counter) // ignore outdated invalidations
    {
      int window_height;
      gdk_window_get_size (m_widget->window, NULL, &window_height);
      /* coalesce rects and then queue expose events */
      for (uint i = 0; i < rects.size(); i++)
        gtk_widget_queue_draw_area (m_widget, rects[i].ll.x,
                                    window_height - (rects[i].ll.y + rects[i].height()),
                                    rects[i].width(), rects[i].height());
    }
}

void
ViewportGtk::enqueue_win_draws (void)
{
  AutoLocker locker (GTK_GDK_THREAD_SYNC);
  if (GTK_WIDGET_DRAWABLE (m_widget))
    gdk_window_process_updates (m_widget->window, TRUE);
}

void
ViewportGtk::enqueue_mouse_moves (void)
{
  AutoLocker locker (GTK_GDK_THREAD_SYNC);
  if (GTK_WIDGET_DRAWABLE (m_widget))
    {
      bool need_many_events = false;
      if (!need_many_events)
        {
          /* get only *one* new motion event */
          gdk_window_get_pointer (m_widget->window, NULL, NULL, NULL);
        }
      else
        {
          /* get pending motion events */
          GdkDisplay *display = gtk_widget_get_display (m_widget);
          gdk_device_get_history  (gdk_display_get_core_pointer (display),
                                   m_widget->window,
                                   m_viewport->last_motion_time,
                                   GDK_CURRENT_TIME,
                                   NULL, NULL);
        }
    }
}

uint
ViewportGtk::last_draw_stamp ()
{
  return m_draw_counter;
}

Viewport::State
ViewportGtk::get_state ()
{
  AutoLocker locker (GTK_GDK_THREAD_SYNC);
  GtkWindow *window = rapicorn_viewport_get_toplevel_window (m_viewport);
  State state;
  state.window_type = m_window_type;
  state.is_active = window && gtk_window_is_active (window);
  state.has_toplevel_focus = window && gtk_window_has_toplevel_focus (window);
  state.window_state = m_window_state;
  bool has_real_allocation = GTK_WIDGET_REALIZED (m_widget) && GTK_WIDGET_VISIBLE (m_widget);
  state.width = has_real_allocation ? m_widget->allocation.width : 0;
  state.height = has_real_allocation ? m_widget->allocation.height : 0;
  return state;
}

static void
configure_gtk_window (GtkWindow              *window,
                      const Viewport::Config &config)
{
  GtkWidget *widget = GTK_WIDGET (window);
  
  /* simple settings */
  gtk_window_set_modal (window, config.modal);
  gtk_window_set_title (window, config.title.c_str());
  gtk_window_set_role (window, config.session_role.c_str());
  if (config.initial_width > 0 && config.initial_height > 0)
    gtk_window_set_default_size (window, config.initial_width, config.initial_height);
  else
    gtk_window_set_default_size (window, -1, -1);
  
  /* geometry handling */
  if (GTK_WIDGET_REALIZED (window))
    gdk_window_set_geometry_hints (widget->window, NULL, GdkWindowHints (0));   // clear geometry on XServer
  GdkGeometry geometry = { 0, };
  uint geometry_mask = 0;
  if (config.min_width > 0 && config.min_height > 0)
    {
      geometry.min_width = iround (config.min_width);
      geometry.min_height = iround (config.min_height);
      geometry_mask |= GDK_HINT_MIN_SIZE;
    }
  if (config.max_width > 0 && config.max_height > 0)
    {
      geometry.max_width = iround (config.max_width);
      geometry.max_height = iround (config.max_height);
      geometry_mask |= GDK_HINT_MAX_SIZE;
    }
  if (config.base_width > 0 && config.base_height > 0)
    {
      geometry.base_width = iround (config.base_width);
      geometry.base_height = iround (config.base_height);
      geometry_mask |= GDK_HINT_BASE_SIZE;
    }
  if (config.width_inc > 0 && config.height_inc > 0)
    {
      geometry.width_inc = iround (config.width_inc);
      geometry.height_inc = iround (config.height_inc);
      geometry_mask |= GDK_HINT_RESIZE_INC;
    }
  if (config.min_aspect > 0 && config.max_aspect > 0)
    {
      geometry.min_aspect = config.min_aspect;
      geometry.max_aspect = config.max_aspect;
      geometry_mask |= GDK_HINT_ASPECT;
    }
  gtk_window_set_geometry_hints (window, NULL, &geometry, GdkWindowHints (geometry_mask));
  
  /* window_hint handling */
  gtk_window_set_decorated (window, bool (config.window_hint & Viewport::HINT_DECORATED));
#if GTK_CHECK_VERSION (2, 8, 0)
  gtk_window_set_urgency_hint (window, bool (config.window_hint & Viewport::HINT_URGENT));
#endif
#if GTK_CHECK_VERSION (99999, 9999, 0) // FIXME: gtk_window_set_shaded()
  gtk_window_set_shaded (window, bool (config.window_hint & Viewport::HINT_SHADED));
#endif
  gtk_window_set_keep_above (window, bool (config.window_hint & Viewport::HINT_ABOVE_ALL));
  gtk_window_set_keep_below (window, bool (config.window_hint & Viewport::HINT_BELOW_ALL));
  gtk_window_set_skip_taskbar_hint (window, bool (config.window_hint & Viewport::HINT_SKIP_TASKBAR));
  gtk_window_set_skip_pager_hint (window, bool (config.window_hint & Viewport::HINT_SKIP_PAGER));
  gtk_window_set_accept_focus (window, bool (config.window_hint & Viewport::HINT_ACCEPT_FOCUS));
  gtk_window_set_focus_on_map (window, bool (config.window_hint & Viewport::HINT_UNFOCUSED));
#if GTK_CHECK_VERSION (2, 8, 10)
  gtk_window_set_deletable (window, bool (config.window_hint & Viewport::HINT_DELETABLE));
#endif
}

static void
adjust_gtk_window (GtkWindow            *window,
                   Viewport::WindowHint  window_hint)
{
  /* actively alter window state */
  if (window_hint & Viewport::HINT_STICKY)
    gtk_window_stick (window);
  else
    gtk_window_unstick (window);
  if (window_hint & Viewport::HINT_ICONIFY)
    gtk_window_iconify (window);
  else
    gtk_window_deiconify (window);
  if (window_hint & Viewport::HINT_FULLSCREEN)
    gtk_window_fullscreen (window);
  else
    gtk_window_unfullscreen (window);
  uint fully_maximized = Viewport::HINT_HMAXIMIZED | Viewport::HINT_VMAXIMIZED;
  if ((window_hint & fully_maximized) == fully_maximized)
    gtk_window_maximize (window);
  else if ((window_hint & fully_maximized) == 0)
    gtk_window_unmaximize (window);
  else
    {
#if GTK_CHECK_VERSION (99999, 9999, 0) // FIXME: gtk_window_hmaximize() gtk_window_vmaximize()
      if (window_hint & Viewport::HINT_HMAXIMIZED)
        gtk_window_hmaximize (window);
      if (window_hint & Viewport::HINT_VMAXIMIZED)
        gtk_window_vmaximize (window);
#endif
    }
}

void
ViewportGtk::set_config (const Config &config)
{
  AutoLocker locker (GTK_GDK_THREAD_SYNC);
  GtkWindow *window = rapicorn_viewport_get_my_window (m_viewport);
  m_root_x = config.root_x;
  m_root_y = config.root_y;
  m_request_width = config.request_width;
  m_request_height = config.request_height;
  m_average_background = config.average_background;
  if (window)
    {
      configure_gtk_window (window, config);
      if (!GTK_WIDGET_DRAWABLE (window)) /* when unmapped or unrealized */
        {
          adjust_gtk_window (window, config.window_hint);
          if (isfinite (config.root_x) && isfinite (config.root_y))
            gtk_window_move (window, iround (config.root_x), iround (config.root_y));
        }
    }
}

void
ViewportGtk::trigger_hint_action (WindowHint window_hint)
{
  AutoLocker locker (GTK_GDK_THREAD_SYNC);
  GtkWindow *window = rapicorn_viewport_get_my_window (m_viewport);
  if (window)
    adjust_gtk_window (window, window_hint);
}

/* --- Gdk/Gtk+/Rapicorn utilities --- */
static Viewport::WindowState
get_rapicorn_window_state (GdkWindowState window_state)
{
  uint wstate = 0;
  if (window_state & GDK_WINDOW_STATE_WITHDRAWN)
    wstate |= Viewport::STATE_WITHDRAWN;
  if (window_state & GDK_WINDOW_STATE_ICONIFIED)
    wstate |= Viewport::STATE_ICONIFIED;
  if (window_state & GDK_WINDOW_STATE_MAXIMIZED)
    wstate |= Viewport::STATE_HMAXIMIZED | Viewport::STATE_VMAXIMIZED;
  if (window_state & GDK_WINDOW_STATE_STICKY)
    wstate |= Viewport::STATE_STICKY;
  if (window_state & GDK_WINDOW_STATE_FULLSCREEN)
    wstate |= Viewport::STATE_FULLSCREEN;
  if (window_state & GDK_WINDOW_STATE_ABOVE)
    wstate |= Viewport::STATE_ABOVE;
  if (window_state & GDK_WINDOW_STATE_BELOW)
    wstate |= Viewport::STATE_BELOW;
  return Viewport::WindowState (wstate);
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

static BackingStore
gdk_window_enable_backing (GdkWindow   *window,
                           BackingStore bs_type)
{
  g_return_val_if_fail (GDK_IS_WINDOW (window), BACKING_STORE_NOT_USEFUL);
  if (GDK_WINDOW_DESTROYED (window))
    return BACKING_STORE_NOT_USEFUL;
  
  XSetWindowAttributes attr = { 0, };
  attr.backing_store = NotUseful;
  if (bs_type == BACKING_STORE_WHEN_MAPPED)
    attr.backing_store = WhenMapped;
  else if (bs_type == BACKING_STORE_ALWAYS)
    attr.backing_store = Always;
  XChangeWindowAttributes (GDK_WINDOW_XDISPLAY (window),
                           GDK_WINDOW_XID (window),
                           CWBackingStore,
                           &attr);
  GdkScreen *screen = gdk_colormap_get_screen (gdk_drawable_get_colormap (window));
  int bs_screen = XDoesBackingStore (GDK_SCREEN_XSCREEN (screen));
  /* return backing store state for this window */
  if (bs_screen == Always &&
      bs_type   == BACKING_STORE_ALWAYS)
    return BACKING_STORE_ALWAYS;
  else if ((bs_screen == Always               || bs_screen == WhenMapped) &&
           (bs_type   == BACKING_STORE_ALWAYS || bs_type   == BACKING_STORE_WHEN_MAPPED))
    return BACKING_STORE_WHEN_MAPPED;
  else /* bs_screen == NotUseful || bs_type == BACKING_STORE_NOT_USEFUL */
    return BACKING_STORE_NOT_USEFUL;
}

static bool
gtk_widget_has_local_display (GtkWidget *widget)
{
  GtkWidget *toplevel = gtk_widget_get_toplevel (widget);
  if (GTK_WIDGET_TOPLEVEL (toplevel) && toplevel->window)
    {
      GdkDisplay *display = gtk_widget_get_display (toplevel);
      gpointer visual_flag = g_object_get_data (G_OBJECT (display), "GdkDisplay-local-display-flag");
      if (visual_flag == (void*) 0)
        {
          GdkImage *image = gdk_image_new (GDK_IMAGE_SHARED, gtk_widget_get_visual (toplevel), 1, 1);
          visual_flag = image ? (void*) 0x1 : (void*) 0x2;
          if (image)
            gdk_image_destroy (image);
          g_object_set_data (G_OBJECT (display), "GdkDisplay-local-display-flag", visual_flag);
        }
      return visual_flag == (void*) 0x1;
    }
  return false;
}

/* --- RapicornViewport methods --- */
static void
rapicorn_viewport_init (RapicornViewport *self)
{
  GtkWidget *widget = GTK_WIDGET (self);
  self->viewport = rapicorn_viewport__cxxinit_viewport;
  BIRNET_ASSERT (self->viewport != NULL);
  self->visibility_state = GDK_VISIBILITY_FULLY_OBSCURED;
  self->backing_store = BACKING_STORE_NOT_USEFUL;
  self->last_time = 0;
  self->last_motion_time = 0;
  self->last_x = -1;
  self->last_y = -1;
  self->last_modifier = GdkModifierType (0);
  gtk_widget_set_redraw_on_allocate (widget, TRUE);
  gtk_widget_set_double_buffered (widget, FALSE);
  gtk_widget_show (widget);
}

static EventContext
rapicorn_viewport_event_context (RapicornViewport *self,
                                 GdkEvent         *event = NULL,
                                 int              *window_height = NULL)
{
  /* extract common event information */
  EventContext econtext;
  double doublex, doubley;
  GdkModifierType modifier_type;
  GdkWindow *gdkwindow = GTK_WIDGET (self)->window;
  gint wh = 0;
  if (gdkwindow)
    gdk_window_get_size (gdkwindow, NULL, &wh);
  if (event && event->any.window == gdkwindow &&
      gdk_event_get_coords (event, &doublex, &doubley) &&
      gdk_event_get_state (event, &modifier_type))
    {
      self->last_time = gdk_event_get_time (event);
      self->last_x = doublex;
      /* vertical Rapicorn axis extends upwards */
      self->last_y = wh - doubley;
      self->last_modifier = modifier_type;
      econtext.synthesized = event->any.send_event;
    }
  else
    econtext.synthesized = true;
  econtext.time = self->last_time;
  if (gdkwindow)
    {
      doublex = self->last_x;
      doubley = self->last_y;
      modifier_type = self->last_modifier;
    }
  else
    {
      // unrealized
      doublex = doubley = -1;
      modifier_type = GdkModifierType (0);
    }
  econtext.x = iround (doublex);
  econtext.y = iround (doubley);
  econtext.modifiers = ModifierState (modifier_type);
  /* ensure modifier compatibility */
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

static void
rapicorn_viewport_change_visibility (RapicornViewport  *self,
                                     GdkVisibilityState visibility)
{
  ViewportGtk *viewport = self->viewport;
  self->visibility_state = visibility;
  if (self->visibility_state == GDK_VISIBILITY_FULLY_OBSCURED)
    {
      EventContext econtext = rapicorn_viewport_event_context (self);
      viewport->m_draw_counter++;   // ignore pending draws
      viewport->enqueue_locked (create_event_cancellation (econtext));
    }
}

static void
rapicorn_viewport_size_request (GtkWidget      *widget,
                                GtkRequisition *requisition)
{
  RapicornViewport *self = RAPICORN_VIEWPORT (widget);
  ViewportGtk *viewport = self->viewport;
  if (viewport)
    {
      requisition->width = iceil (viewport->m_request_width);
      requisition->height = iceil (viewport->m_request_height);
    }
}

static void
rapicorn_viewport_size_allocate (GtkWidget     *widget,
                                 GtkAllocation *allocation)
{
  widget->allocation = *allocation;
  RapicornViewport *self = RAPICORN_VIEWPORT (widget);
  ViewportGtk *viewport = self->viewport;
  if (GTK_WIDGET_REALIZED (widget))
    {
      gdk_window_move_resize (widget->window, allocation->x, allocation->y, allocation->width, allocation->height);
      gdk_flush(); /* resize now, so gravity settings take effect immediately */
    }
  if (viewport)
    {
      EventContext econtext = rapicorn_viewport_event_context (self); /* relies on proper GdkWindow size */
      viewport->enqueue_locked (create_event_win_size (econtext, ++viewport->m_draw_counter, allocation->width, allocation->height));
    }
}

static void
rapicorn_viewport_realize (GtkWidget *widget)
{
  RapicornViewport *self = RAPICORN_VIEWPORT (widget);
  ViewportGtk *viewport = self->viewport;
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
  int attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  widget->window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
  gdk_window_set_user_data (widget->window, widget);
  widget->style = gtk_style_attach (widget->style, widget->window);
  Color argb_color = viewport ? viewport->m_average_background : Color (0xff808080);
  GdkColor gdkcolor = { 0, };
  gdkcolor.red = argb_color.red() * 0x0101;
  gdkcolor.green = argb_color.green() * 0x0101;
  gdkcolor.blue = argb_color.blue() * 0x0101;
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
  gtk_widget_add_events (toplevel, GDK_STRUCTURE_MASK | GDK_VISIBILITY_NOTIFY_MASK);
  g_signal_connect (toplevel, "event", G_CALLBACK (rapicorn_viewport_ancestor_event), self);
  /* optimize GtkWindow for flicker-free child window moves if it is our immediate parent */
  if (widget->parent == toplevel && gdk_colormap_alloc_color (attributes.colormap, &gdkcolor, FALSE, TRUE))
    {
      gtk_widget_set_app_paintable (toplevel, TRUE);
      gdk_window_set_background (toplevel->window, &gdkcolor);
      gdk_colormap_free_colors (attributes.colormap, &gdkcolor, 1);
    }
  rapicorn_viewport_change_visibility (self, GDK_VISIBILITY_FULLY_OBSCURED);
  self->fast_local_blitting = gtk_widget_has_local_display (widget);
  if (!self->fast_local_blitting)
    self->backing_store = gdk_window_enable_backing (widget->window, BACKING_STORE_ALWAYS);
}

static void
rapicorn_viewport_map (GtkWidget *widget)
{
  RapicornViewport *self = RAPICORN_VIEWPORT (widget);
  ViewportGtk *viewport = self->viewport;
  bool block_auto_startup_notification = viewport && viewport->m_splash_screen;
  if (block_auto_startup_notification)
    gtk_window_set_auto_startup_notification (false);
  GTK_WIDGET_CLASS (rapicorn_viewport_parent_class)->map (widget); // chain
  if (block_auto_startup_notification)
    gtk_window_set_auto_startup_notification (true);
  rapicorn_viewport_change_visibility (self, GDK_VISIBILITY_FULLY_OBSCURED);
}

static void
rapicorn_viewport_unmap (GtkWidget *widget)
{
  RapicornViewport *self = RAPICORN_VIEWPORT (widget);
  rapicorn_viewport_change_visibility (self, GDK_VISIBILITY_FULLY_OBSCURED);
  GTK_WIDGET_CLASS (rapicorn_viewport_parent_class)->unmap (widget); // chain
}

static void
rapicorn_viewport_unrealize (GtkWidget *widget)
{
  RapicornViewport *self = RAPICORN_VIEWPORT (widget);
  ViewportGtk *viewport = self->viewport;
  GtkWidget *toplevel = gtk_widget_get_toplevel (widget);
  g_signal_handlers_disconnect_by_func (toplevel, (void*) rapicorn_viewport_ancestor_event, self);
  if (viewport)
    viewport->enqueue_locked (create_event_cancellation (rapicorn_viewport_event_context (self)));
  GTK_WIDGET_CLASS (rapicorn_viewport_parent_class)->unrealize (widget); // chain
  rapicorn_viewport_change_visibility (self, GDK_VISIBILITY_FULLY_OBSCURED);
  self->fast_local_blitting = false;
}

static void
debug_dump_event (GtkWidget          *widget,
                  const gchar        *prefix,
                  GdkEvent           *event,
                  const EventContext &econtext)
{
  GEnumClass *eclass = (GEnumClass*) g_type_class_ref (GDK_TYPE_EVENT_TYPE);
  const gchar *name = g_enum_get_value (eclass, event->type)->value_name;
  if (strncmp (name, "GDK_", 4) == 0)
    name += 4;
  gint wx, wy, ww, wh;
  gdk_window_get_position (widget->window, &wx, &wy);
  gdk_window_get_size (widget->window, &ww, &wh);
  g_printerr ("Rapicorn-EVENT:%s %-20s) time=0x%08x synth=%d x=%+7.2f y=%+7.2f state=0x%04x ewin=%p", prefix,
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
    case GDK_MOTION_NOTIFY:
      g_printerr (" is_hint=%d", event->motion.is_hint);
      break;
    case GDK_EXPOSE:
      {
        GdkRectangle &area = event->expose.area;
        g_printerr (" (bbox: %d,%d %dx%d)", area.x, area.y, area.width, area.height);
      }
      break;
    case GDK_SCROLL:
      ec1 = (GEnumClass*) g_type_class_ref (GDK_TYPE_SCROLL_DIRECTION);
      detail = g_enum_get_value (ec1, event->scroll.direction)->value_name;
      if (strncmp (detail, "GDK_", 4) == 0)
        detail += 4;
      g_printerr (" %s", detail);
      g_type_class_unref (ec1);
      break;
    case GDK_VISIBILITY_NOTIFY:
      switch (event->visibility.state)
        {
        case GDK_VISIBILITY_UNOBSCURED:         g_printerr (" unobscured"); break;
        case GDK_VISIBILITY_FULLY_OBSCURED:     g_printerr (" fully-obscured"); break;
        case GDK_VISIBILITY_PARTIAL:            g_printerr (" partial"); break;
        }
      break;
    default: ;
    }
  g_printerr (" (wwin=%p %d,%d %dx%d)\n", widget->window, wx, wy, ww, wh);
  g_type_class_unref (eclass);
}

static gboolean
rapicorn_viewport_event (GtkWidget *widget,
                         GdkEvent  *event)
{
  RapicornViewport *self = RAPICORN_VIEWPORT (widget);
  ViewportGtk *viewport = self->viewport;
  bool handled = false;
  int window_height = 0;
  EventContext econtext = rapicorn_viewport_event_context (self, event, &window_height);
  if (DEBUG_EVENTS) /* debug events */
    debug_dump_event (widget, "", event, econtext);
  if (!viewport)
    return false;
  /* translate events */
  switch (event->type)
    {
    case GDK_ENTER_NOTIFY:
      viewport->enqueue_locked (create_event_mouse (event->crossing.detail == GDK_NOTIFY_INFERIOR ? MOUSE_MOVE : MOUSE_ENTER, econtext));
      break;
    case GDK_MOTION_NOTIFY:
      viewport->enqueue_locked (create_event_mouse (MOUSE_MOVE, econtext));
      self->last_motion_time = gdk_event_get_time (event);
      /* trigger new motion events (since we use motion-hint) */
#if 0
      if (event->any.window == widget->window) /* will be requested by enqueue_mouse_moves() instead */
        gdk_window_get_pointer (window, NULL, NULL, NULL);
#endif
      break;
    case GDK_LEAVE_NOTIFY:
      viewport->enqueue_locked (create_event_mouse (event->crossing.detail == GDK_NOTIFY_INFERIOR ? MOUSE_MOVE : MOUSE_LEAVE, econtext));
      break;
    case GDK_FOCUS_CHANGE:
      viewport->enqueue_locked (create_event_focus (event->focus_change.in ? FOCUS_IN : FOCUS_OUT, econtext));
      break;
    case GDK_KEY_PRESS:
    case GDK_KEY_RELEASE:
      char *key_name;
      key_name = g_strndup (event->key.string, event->key.length);
      /* KeyValue is modelled to match GDK keyval */
      // FIXME: handled = root->dispatch_key_event (econtext, event->type == GDK_KEY_PRESS, KeyValue (event->key.keyval), key_name);
      viewport->enqueue_locked (create_event_key (event->type == GDK_KEY_PRESS ? KEY_PRESS : KEY_RELEASE, econtext, KeyValue (event->key.keyval), key_name));
      handled = TRUE;
      g_free (key_name);
      break;
    case GDK_BUTTON_PRESS:
      viewport->enqueue_locked (create_event_button (BUTTON_PRESS, econtext, event->button.button));
      handled = TRUE;
      // FIXME: handled = root->dispatch_button_event (econtext, true, event->button.button);
      break;
    case GDK_2BUTTON_PRESS:
      viewport->enqueue_locked (create_event_button (BUTTON_PRESS, econtext, event->button.button));
      handled = TRUE;
      break;
    case GDK_3BUTTON_PRESS:
      viewport->enqueue_locked (create_event_button (BUTTON_PRESS, econtext, event->button.button));
      handled = TRUE;
      break;
    case GDK_BUTTON_RELEASE:
      viewport->enqueue_locked (create_event_button (BUTTON_RELEASE, econtext, event->button.button));
      handled = TRUE;
      break;
    case GDK_SCROLL:
      if (event->scroll.direction == GDK_SCROLL_UP)
        viewport->enqueue_locked (create_event_scroll (SCROLL_UP, econtext));
      else if (event->scroll.direction == GDK_SCROLL_LEFT)
        viewport->enqueue_locked (create_event_scroll (SCROLL_LEFT, econtext));
      else if (event->scroll.direction == GDK_SCROLL_RIGHT)
        viewport->enqueue_locked (create_event_scroll (SCROLL_RIGHT, econtext));
      else if (event->scroll.direction == GDK_SCROLL_DOWN)
        viewport->enqueue_locked (create_event_scroll (SCROLL_DOWN, econtext));
      handled = TRUE;
      break;
#if GTK_CHECK_VERSION (2, 8, 0)
    case GDK_GRAB_BROKEN:
#endif
    case GDK_DELETE:
      viewport->enqueue_locked (create_event_win_delete (econtext));
      handled = TRUE;
      break;
    case GDK_DESTROY:
    case GDK_UNMAP:
      rapicorn_viewport_change_visibility (self, GDK_VISIBILITY_FULLY_OBSCURED);
      break;
    case GDK_VISIBILITY_NOTIFY:
      rapicorn_viewport_change_visibility (self, event->visibility.state);
      break;
    case GDK_WINDOW_STATE:
      viewport->m_window_state = get_rapicorn_window_state (event->window_state.new_window_state);
      if ((event->window_state.new_window_state & (GDK_WINDOW_STATE_WITHDRAWN | GDK_WINDOW_STATE_ICONIFIED)) &&
          widget->window && gdk_window_has_ancestor (widget->window, event->window_state.window))
        viewport->enqueue_locked (create_event_cancellation (econtext));
      break;
    case GDK_EXPOSE:
      {
        std::vector<Rect> rectangles;
        if (event->expose.region)
          {
            GdkRectangle *areas;
            gint n_areas;
            gdk_region_get_rectangles (event->expose.region, &areas, &n_areas);
            for (int i = 0; i < n_areas; i++)
              {
                gint realy = window_height - (areas[i].y + areas[i].height);
                rectangles.push_back (Rect (Point (areas[i].x, realy), areas[i].width, areas[i].height));
              }
            g_free (areas);
          }
        else
          {
            GdkRectangle &area = event->expose.area;
            gint realy = window_height - (area.y + area.height);
            rectangles.push_back (Rect (Point (area.x, realy), area.width, area.height));
          }
        viewport->enqueue_locked (create_event_win_draw (econtext, viewport->m_draw_counter, rectangles));
      }
      break;
    default:
    case GDK_NOTHING:
    case GDK_NO_EXPOSE:
    case GDK_PROPERTY_NOTIFY:   case GDK_CLIENT_EVENT:
    case GDK_CONFIGURE:         case GDK_MAP:
    case GDK_SELECTION_CLEAR:   case GDK_SELECTION_REQUEST:     case GDK_SELECTION_NOTIFY:
    case GDK_OWNER_CHANGE:
    case GDK_PROXIMITY_IN:      case GDK_PROXIMITY_OUT:
      /* nothing to forward */
      break;
    }
  return handled;
}

static gboolean
rapicorn_viewport_ancestor_event (GtkWidget *ancestor,
                                  GdkEvent  *event,
                                  GtkWidget *widget)
{
  RapicornViewport *self = RAPICORN_VIEWPORT (widget);
  ViewportGtk *viewport = self->viewport;
  EventContext econtext = rapicorn_viewport_event_context (self, event);
  if (!viewport)
    return false;
  if (DEBUG_EVENTS) /* debug events */
    debug_dump_event (widget, "GtkWindow:", event, econtext);
  bool handled = false;
  switch (event->type)
    {
    case GDK_VISIBILITY_NOTIFY:
      if (event->visibility.state == GDK_VISIBILITY_FULLY_OBSCURED)
        rapicorn_viewport_change_visibility (self, GDK_VISIBILITY_FULLY_OBSCURED);
      break;
    case GDK_WINDOW_STATE:
      viewport->m_window_state = get_rapicorn_window_state (event->window_state.new_window_state);
      if ((event->window_state.new_window_state & (GDK_WINDOW_STATE_WITHDRAWN | GDK_WINDOW_STATE_ICONIFIED)) &&
          widget->window && gdk_window_has_ancestor (widget->window, event->window_state.window))
        viewport->enqueue_locked (create_event_cancellation (econtext));
      break;
#if GTK_CHECK_VERSION (2, 8, 0)
    case GDK_GRAB_BROKEN:
#endif
    case GDK_DELETE:
      if (ancestor == (GtkWidget*) rapicorn_viewport_get_my_window (self))
        {
          viewport->enqueue_locked (create_event_win_delete (econtext));
          handled = true;
        }
      break;
    case GDK_DESTROY:
    case GDK_UNMAP:
      rapicorn_viewport_change_visibility (self, GDK_VISIBILITY_FULLY_OBSCURED);
      break;
    default: break;
    }
  return handled;
}

static void
rapicorn_viewport_dispose (GObject *object)
{
  RapicornViewport *self = RAPICORN_VIEWPORT (object);
  self->viewport = NULL;
  G_OBJECT_CLASS (rapicorn_viewport_parent_class)->dispose (object); // chain
}

static void
rapicorn_viewport_class_init (RapicornViewportClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gobject_class->dispose = rapicorn_viewport_dispose;

  widget_class->size_request = rapicorn_viewport_size_request;
  widget_class->size_allocate = rapicorn_viewport_size_allocate;
  widget_class->realize = rapicorn_viewport_realize;
  widget_class->map = rapicorn_viewport_map;
  widget_class->unmap = rapicorn_viewport_unmap;
  widget_class->unrealize = rapicorn_viewport_unrealize;
  widget_class->event = rapicorn_viewport_event;
}

/* --- RapicornGtkThread --- */
class RapicornGtkThread : public virtual Thread {
public:
  RapicornGtkThread() :
    Thread ("RapicornGtkThread")
  {}
  virtual void
  run ()
  {
    AutoLocker locker (GTK_GDK_THREAD_SYNC);
    gtk_main(); /* does GDK_THREADS_LEAVE(); ... GDK_THREADS_ENTER(); */
  }
};

} // Anon

namespace Rapicorn {

static Thread *gtkthread = NULL;

void
rapicorn_init_with_gtk_thread (int        *argcp,
                               char     ***argvp,
                               const char *app_name)
{
  /* non-GTK initialization */
  rapicorn_init (argcp, argvp, app_name);
  g_type_init();
  /* setup GDK_THREADS_ENTER/GDK_THREADS_LEAVE */
  gdk_threads_init();
  AutoLocker locker (GTK_GDK_THREAD_SYNC);
  /* GTK intilization */
  gtk_init (argcp, argvp);
  gtkthread = new RapicornGtkThread ();
  ref_sink (gtkthread);
  gtkthread->start();
}

bool
rapicorn_init_with_foreign_gtk (int        *argcp,
                                char     ***argvp,
                                const char *app_name,
                                bool        auto_quit_gtk)
{
  /* non-GTK initialization */
  rapicorn_init (argcp, argvp, app_name);
  g_type_init();
  /* setup GDK_THREADS_ENTER/GDK_THREADS_LEAVE */
  gdk_threads_init();
  AutoLocker locker (GTK_GDK_THREAD_SYNC);
  rapicorn_viewport__auto_quit_gtk = auto_quit_gtk;
  /* GTK intilization */
  return gtk_init_check (argcp, argvp); // allow $DISPLAY initialisation to fail
}

void
rapicorn_gtk_threads_enter ()
{
  GTK_GDK_THREAD_SYNC.lock();
}

void
rapicorn_gtk_threads_leave ()
{
  GTK_GDK_THREAD_SYNC.unlock();
}

} // Rapicorn
