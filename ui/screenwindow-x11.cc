// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "screenwindow.hh"
#include <ui/utilities.hh>
#include <cairo/cairo-xlib.h>
#include <fcntl.h> // for wakeup pipe
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define EDEBUG(...)     RAPICORN_KEY_DEBUG ("XEvents", __VA_ARGS__)
#define XDEBUG(...)     RAPICORN_KEY_DEBUG ("X11", __VA_ARGS__)

#define CHECK_CAIRO_STATUS(status)      do {    \
  cairo_status_t ___s = (status);               \
  if (___s != CAIRO_STATUS_SUCCESS)             \
    EDEBUG ("%s: %s", cairo_status_to_string (___s), #status);   \
  } while (0)

namespace { // Anon
using namespace Rapicorn;

// == declarations ==
class X11Thread;
class X11Context;
static X11Thread*       start_x11_thread (X11Context &x11context);
static void             stop_x11_thread  (X11Thread *x11_thread);
static Mutex x11_rmutex (RECURSIVE_LOCK);

// == X11Item ==
struct X11Item {
  explicit X11Item() {}
  virtual ~X11Item() {}
};

// == X11 Error Handling ==
static XErrorHandler xlib_error_handler = NULL;
static int
x11_error (Display *error_display, XErrorEvent *error_event)
{
  int result = -1;
  try
    {
      atexit (abort);
      result = xlib_error_handler (error_display, error_event);
    }
  catch (...)
    {
      // ignore C++ exceptions from atexit handlers
    }
  abort();
  return result;
}

// == X11Context ==
struct X11Context {
  X11Thread            *x11_thread;
  Display              *display;
  int                   screen;
  Visual               *visual;
  int                   depth;
  Window                root_window;
  map<size_t, X11Item*> x11ids;
  explicit              X11Context (const String &x11display);
  Atom                  atom       (const String &text, bool force_create = true);
  String                atom       (Atom atom) const;
  /*dtor*/             ~X11Context ();
};

X11Context::X11Context (const String &x11display) :
  x11_thread (NULL), display (NULL), screen (0), visual (NULL), depth (0)
{
  ScopedLock<Mutex> x11locker (x11_rmutex);
  do_once { xlib_error_handler = XSetErrorHandler (x11_error); }
  const char *s = x11display.c_str();
  display = XOpenDisplay (s[0] ? s : NULL);
  XDEBUG ("XOpenDisplay(%s): %s", CQUOTE (x11display.c_str()), display ? "success" : "failed to connect");
  if (!display)
    return;
  x11_thread = start_x11_thread (*this);
  if (1) // FIXME: toggle sync
    XSynchronize (display, true);
  screen = DefaultScreen (display);
  visual = DefaultVisual (display, screen);
  depth = DefaultDepth (display, screen);
  root_window = XRootWindow (display, screen);
}

X11Context::~X11Context ()
{
  if (x11_thread)
    {
      stop_x11_thread (x11_thread);
      x11_thread = NULL;
    }
  x11ids.clear();
  if (display)
    {
      XCloseDisplay (display);
      display = NULL;
    }
}

Atom
X11Context::atom (const String &text, bool force_create)
{
  Atom a = XInternAtom (display, text.c_str(), !force_create);
  // FIXME: optimize roundtrips
  return a;
}

String
X11Context::atom (Atom atom) const
{
  char *res = XGetAtomName (display, atom);
  // FIXME: optimize roundtrips
  String result (res ? res : "");
  if (res)
    XFree (res);
  return result;
}

// == ScreenDriverX11 ==
struct ScreenDriverX11 : ScreenDriver {
  X11Context *m_x11context;
  uint        m_open_count;
  virtual bool
  open ()
  {
    if (!m_x11context)
      {
        assert_return (m_open_count == 0, false);
        const char *s = getenv ("DISPLAY");
        String ds = s ? s : "";
        X11Context *x11context = new X11Context (ds);
        if (!x11context->display)
          {
            delete x11context;
            return false;
          }
        m_x11context = x11context;
      }
    m_open_count++;
    return true;
  }
  virtual void
  close ()
  {
    assert_return (m_open_count > 0);
    m_open_count--;
    if (!m_open_count)
      {
        X11Context *x11context = m_x11context;
        m_x11context = NULL;
        delete x11context;
      }
  }
  virtual ScreenWindow* create_screen_window (const ScreenWindow::Setup &setup, ScreenWindow::EventReceiver &receiver);
  ScreenDriverX11() : ScreenDriver ("X11Window", -1), m_x11context (NULL), m_open_count (0) {}
};
static ScreenDriverX11 screen_driver_x11;

// == ScreenWindowX11 ==
struct ScreenWindowX11 : public virtual ScreenWindow, public virtual X11Item {
  X11Context           &x11context;
  ScreenDriverX11      &m_x11driver;
  EventReceiver        &m_receiver;
  State                 m_state;
  Color                 m_average_background;
  Window                m_window;
  int                   m_last_motion_time;
  bool                  m_override_redirect, m_mapped, m_focussed;
  EventContext          m_event_context;
  std::vector<Rect>     m_expose_rectangles;
  explicit              ScreenWindowX11         (ScreenDriverX11 &x11driver, const ScreenWindow::Setup &setup, EventReceiver &receiver);
  virtual              ~ScreenWindowX11         ();
  virtual State         get_state               ();
  virtual void          configure               (const Config &config);
  void                  setup                   (const ScreenWindow::Setup &setup);
  virtual void          beep                    ();
  virtual void          show                    ();
  virtual void          present                 ();
  virtual bool          viewable                ();
  virtual void          blit_surface            (cairo_surface_t *surface, Rapicorn::Region region);
  virtual void          start_user_move         (uint button, double root_x, double root_y);
  virtual void          start_user_resize       (uint button, double root_x, double root_y, AnchorType edge);
  void                  enqueue_locked          (Event *event);
  bool                  process_event           (const XEvent &xevent);
};

ScreenWindow*
ScreenDriverX11::create_screen_window (const ScreenWindow::Setup &setup, ScreenWindow::EventReceiver &receiver)
{
  assert_return (m_open_count > 0, NULL);
  return new ScreenWindowX11 (*this, setup, receiver);
}

ScreenWindowX11::ScreenWindowX11 (ScreenDriverX11 &x11driver, const ScreenWindow::Setup &setup, EventReceiver &receiver) :
  x11context (*x11driver.m_x11context), m_x11driver (x11driver), m_receiver (receiver), m_average_background (0xff808080),
  m_window (0), m_last_motion_time (0), m_override_redirect (false), m_mapped (false), m_focussed (false)
{
  ScopedLock<Mutex> x11locker (x11_rmutex);
  m_x11driver.open();
  m_state.setup.window_type = setup.window_type;
  m_override_redirect = (setup.window_type == WINDOW_TYPE_DESKTOP ||
                         setup.window_type == WINDOW_TYPE_DROPDOWN_MENU ||
                         setup.window_type == WINDOW_TYPE_POPUP_MENU ||
                         setup.window_type == WINDOW_TYPE_TOOLTIP ||
                         setup.window_type == WINDOW_TYPE_NOTIFICATION ||
                         setup.window_type == WINDOW_TYPE_COMBO ||
                         setup.window_type == WINDOW_TYPE_DND);
  XSetWindowAttributes attributes;
  attributes.event_mask        = ExposureMask | StructureNotifyMask | EnterWindowMask | LeaveWindowMask |
                                 KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask |
                                 PointerMotionMask | PointerMotionHintMask | ButtonMotionMask |
                                 Button1MotionMask | Button2MotionMask | Button3MotionMask | Button4MotionMask | Button5MotionMask;
  attributes.background_pixel  = XWhitePixel (x11context.display, x11context.screen);
  attributes.border_pixel      = XBlackPixel (x11context.display, x11context.screen);
  attributes.override_redirect = m_override_redirect;
  attributes.win_gravity       = StaticGravity;
  attributes.bit_gravity       = StaticGravity;
  unsigned long attribute_mask = CWWinGravity | CWBitGravity |
                                 /*CWBackPixel |*/ CWBorderPixel | CWOverrideRedirect | CWEventMask;
  const int border = 3, request_width = 33, request_height = 33;
  m_window = XCreateWindow (x11context.display, x11context.root_window, 0, 0, request_width, request_height, border,
                            x11context.depth, InputOutput, x11context.visual, attribute_mask, &attributes);
  if (m_window)
    x11context.x11ids[m_window] = this;
  printerr ("window: %lu\n", m_window);
  // window setup
  this->setup (setup);
  // FIXME:  set Window hints & focus
  // configure state for this window
  {
    XConfigureEvent xev = { ConfigureNotify, /*serial*/ 0, true, x11context.display, m_window, m_window,
                            0, 0, request_width, request_height, border, /*above*/ 0, m_override_redirect };
    XEvent xevent;
    xevent.xconfigure = xev;
    process_event (xevent);
  }
}

ScreenWindowX11::~ScreenWindowX11()
{
  if (m_window)
    x11context.x11ids.erase (m_window);
  m_x11driver.close();
}

void
ScreenWindowX11::show (void)
{
  ScopedLock<Mutex> x11locker (x11_rmutex);
  XMapWindow (x11context.display, m_window);
}

static const char*
notify_mode (int notify_type)
{
  switch (notify_type)
    {
    case NotifyNormal:          return "Normal";
    case NotifyGrab:            return "Grab";
    case NotifyUngrab:          return "Ungrab";
    case NotifyWhileGrabbed:    return "WhileGrabbed";
    default:                    return "Unknown";
    }
}

static const char*
notify_detail (int notify_type)
{
  switch (notify_type)
    {
    case NotifyAncestor:         return "Ancestor";
    case NotifyVirtual:          return "Virtual";
    case NotifyInferior:         return "Inferior";
    case NotifyNonlinear:        return "Nonlinear";
    case NotifyNonlinearVirtual: return "NonlinearVirtual";
    default:                     return "Unknown";
    }
}

bool
ScreenWindowX11::process_event (const XEvent &xevent)
{
  m_event_context.synthesized = xevent.xany.send_event;
  const char ss = m_event_context.synthesized ? 'S' : 's';
  bool consumed = false;
  switch (xevent.type)
    {
    case CreateNotify: {
      const XCreateWindowEvent &xev = xevent.xcreatewindow;
      EDEBUG ("Creat: %c=%lu w=%lu a=%+d%+d%+dx%d b=%d", ss, xev.serial, xev.window, xev.x, xev.y, xev.width, xev.height, xev.border_width);
      consumed = true;
      break; }
    case ConfigureNotify: {
      const XConfigureEvent &xev = xevent.xconfigure;
      if (xev.event != xev.window)
        break;
      EDEBUG ("Confg: %c=%lu w=%lu a=%+d%+d%+dx%d b=%d", ss, xev.serial, xev.window, xev.x, xev.y, xev.width, xev.height, xev.border_width);
      if (m_state.width != xev.width || m_state.height != xev.height)
        {
          m_expose_rectangles.push_back (Rect (Point (0, 0), xev.width, xev.height));
          m_state.width = xev.width;
          m_state.height = xev.height;
          enqueue_locked (create_event_win_size (m_event_context, 0, m_state.width, m_state.height));
        }
      consumed = true;
      break; }
    case MapNotify: {
      const XMapEvent &xev = xevent.xmap;
      if (xev.event != xev.window)
        break;
      EDEBUG ("Map  : %c=%lu w=%lu", ss, xev.serial, xev.window);
      m_mapped = true;
      consumed = true;
      break; }
    case UnmapNotify: {
      const XUnmapEvent &xev = xevent.xunmap;
      if (xev.event != xev.window)
        break;
      EDEBUG ("Unmap: %c=%lu w=%lu", ss, xev.serial, xev.window);
      m_mapped = false;
      consumed = true;
      break; }
    case Expose: {
      const XExposeEvent &xev = xevent.xexpose;
      EDEBUG ("Expos: %c=%lu w=%lu a=%+d%+d%+dx%d c=%d", ss, xev.serial, xev.window, xev.x, xev.y, xev.width, xev.height, xev.count);
      std::vector<Rect> rectangles;
      m_expose_rectangles.push_back (Rect (Point (xev.x, xev.y), xev.width, xev.height));
      if (xev.count == 0)
        {
          enqueue_locked (create_event_win_draw (m_event_context, 0, m_expose_rectangles));
          m_expose_rectangles.clear();
        }
      consumed = true;
      break; }
    case EnterNotify: case LeaveNotify: {
      const XCrossingEvent &xev = xevent.xcrossing;
      const EventType etype = xevent.type == EnterNotify ? MOUSE_ENTER : MOUSE_LEAVE;
      const char  *kind = xevent.type == EnterNotify ? "Enter" : "Leave";
      EDEBUG ("%s: %c=%lu w=%lu c=%lu p=%+d%+d Notify:%s+%s", kind, ss, xev.serial, xev.window, xev.subwindow, xev.x, xev.y,
              notify_mode (xev.mode), notify_detail (xev.detail));
      m_event_context.time = xev.time; m_event_context.x = xev.x; m_event_context.y = xev.y; m_event_context.modifiers = ModifierState (xev.state);
      m_focussed = xev.focus;
      enqueue_locked (create_event_mouse (xev.detail == NotifyInferior ? MOUSE_MOVE : etype, m_event_context));
      if (xev.detail != NotifyInferior)
        m_last_motion_time = xev.time;
      consumed = true;
      break; }
    case KeyPress: case KeyRelease: {
      const XKeyEvent &xev = xevent.xkey;
      const char  *kind = xevent.type == KeyPress ? "dn" : "up";
      const KeySym ksym = XKeycodeToKeysym (x11context.display, xev.keycode, 0);
      const char  *kstr = XKeysymToString (ksym);
      EDEBUG ("KEY%s: %c=%lu w=%lu c=%lu p=%+d%+d k=%s", kind, ss, xev.serial, xev.window, xev.subwindow, xev.x, xev.y, kstr);
      m_event_context.time = xev.time; m_event_context.x = xev.x; m_event_context.y = xev.y; m_event_context.modifiers = ModifierState (xev.state);
      enqueue_locked (create_event_key (xevent.type == KeyPress ? KEY_PRESS : KEY_RELEASE, m_event_context, KeyValue (ksym), kstr));
      consumed = true;
      break; }
    case ButtonPress: case ButtonRelease: {
      const XButtonEvent &xev = xevent.xbutton;
      const char  *kind = xevent.type == ButtonPress ? "dn" : "up";
      if (xev.window != m_window)
        break;
      EDEBUG ("BUT%s: %c=%lu w=%lu c=%lu p=%+d%+d b=%d", kind, ss, xev.serial, xev.window, xev.subwindow, xev.x, xev.y, xev.button);
      m_event_context.time = xev.time; m_event_context.x = xev.x; m_event_context.y = xev.y; m_event_context.modifiers = ModifierState (xev.state);
      enqueue_locked (create_event_button (xevent.type == ButtonPress ? BUTTON_PRESS : BUTTON_RELEASE, m_event_context, xev.button));
      consumed = true;
      break; }
    case MotionNotify: {
      const XMotionEvent &xev = xevent.xmotion;
      if (xev.window != m_window)
        break;
      if (xev.is_hint)
        {
          int nevents = 0;
          XTimeCoord *xcoords = XGetMotionEvents (x11context.display, m_window, m_last_motion_time + 1, xev.time - 1, &nevents);
          if (xcoords)
            {
              for (int i = 0; i < nevents; ++i)
                if (xcoords[i].x == 0)
                  {
                    /* xserver-xorg-1:7.6+7ubuntu7 + libx11-6:amd64-2:1.4.4-2ubuntu1 may generate buggy motion
                     * history events with x=0 for X220t trackpoints.
                     */
                    EDEBUG ("  ...: S=%lu w=%lu c=%lu p=%+d%+d (discarding)", xev.serial, xev.window, xev.subwindow, xcoords[i].x, xcoords[i].y);
                  }
                else
                  {
                    m_event_context.time = xcoords[i].time; m_event_context.x = xcoords[i].x; m_event_context.y = xcoords[i].y;
                    enqueue_locked (create_event_mouse (MOUSE_MOVE, m_event_context));
                    EDEBUG ("  ...: S=%lu w=%lu c=%lu p=%+d%+d", xev.serial, xev.window, xev.subwindow, xcoords[i].x, xcoords[i].y);
                  }
              XFree (xcoords);
            }
        }
      EDEBUG ("Mtion: %c=%lu w=%lu c=%lu p=%+d%+d%s", ss, xev.serial, xev.window, xev.subwindow, xev.x, xev.y, xev.is_hint ? " (hint)" : "");
      m_event_context.time = xev.time; m_event_context.x = xev.x; m_event_context.y = xev.y; m_event_context.modifiers = ModifierState (xev.state);
      enqueue_locked (create_event_mouse (MOUSE_MOVE, m_event_context));
      m_last_motion_time = xev.time;
      consumed = true;
      break; }
    default: ;
    }
  return consumed;
}

void
ScreenWindowX11::blit_surface (cairo_surface_t *surface, Rapicorn::Region region)
{
  ScopedLock<Mutex> x11locker (x11_rmutex);
  if (!m_window)
    return;
  assert_return (CAIRO_STATUS_SUCCESS == cairo_surface_status (surface));
  // surface for drawing on the X11 window
  cairo_surface_t *xsurface = cairo_xlib_surface_create (x11context.display, m_window, x11context.visual, m_state.width, m_state.height);
  assert_return (xsurface && cairo_surface_status (xsurface) == CAIRO_STATUS_SUCCESS);
  cairo_xlib_surface_set_size (xsurface, m_state.width, m_state.height);
  assert_return (cairo_surface_status (xsurface) == CAIRO_STATUS_SUCCESS);
  // cairo context
  cairo_t *xcr = cairo_create (xsurface);
  assert_return (xcr);
  assert_return (CAIRO_STATUS_SUCCESS == cairo_status (xcr));
  // actual drawing
  cairo_save (xcr);
  vector<Rect> rects;
  region.list_rects (rects);
  for (size_t i = 0; i < rects.size(); i++)
    cairo_rectangle (xcr, rects[i].x, rects[i].y, rects[i].width, rects[i].height);
  cairo_clip (xcr);
  cairo_set_source_surface (xcr, surface, 0, 0);
  cairo_set_operator (xcr, CAIRO_OPERATOR_OVER);
  cairo_paint (xcr);
  cairo_restore (xcr);
  EDEBUG ("BlitS: nrects=%zu", rects.size());
  // cleanup
  assert (CAIRO_STATUS_SUCCESS == cairo_status (xcr));
  if (xcr)
    {
      cairo_destroy (xcr);
      xcr = NULL;
    }
  if (xsurface)
    {
      cairo_surface_destroy (xsurface);
      xsurface = NULL;
    }
  XFlush (x11context.display);
}

void
ScreenWindowX11::enqueue_locked (Event *event)
{
  // ScopedLock<Mutex> x11locker (x11_rmutex);
  m_receiver.enqueue_async (event);
}

ScreenWindow::State
ScreenWindowX11::get_state ()
{
  ScopedLock<Mutex> x11locker (x11_rmutex);
  return m_state;
}

enum XPEmpty { KEEP_EMPTY, DELETE_EMPTY };

static bool
set_text_property (X11Context &x11context, Window window, const String &property, XICCEncodingStyle ecstyle,
                   const String &value, XPEmpty when_empty = KEEP_EMPTY)
{
  bool success = true;
  if (when_empty == DELETE_EMPTY && value.empty())
    XDeleteProperty (x11context.display, window, x11context.atom (property));
  else if (ecstyle == XUTF8StringStyle)
    XChangeProperty (x11context.display, window, x11context.atom (property), x11context.atom ("UTF8_STRING"), 8,
                     PropModeReplace, (uint8*) value.c_str(), value.size());
  else
    {
      char *text = const_cast<char*> (value.c_str());
      XTextProperty xtp = { 0, };
      const int result = Xutf8TextListToTextProperty (x11context.display, &text, 1, ecstyle, &xtp);
      printerr ("XUTF8CONVERT: target=%s len=%zd result=%d: %s -> %s\n", x11context.atom(xtp.encoding).c_str(), value.size(), result, text, xtp.value);
      if (result >= 0 && xtp.nitems && xtp.value)
        XChangeProperty (x11context.display, window, x11context.atom (property), xtp.encoding, xtp.format,
                         PropModeReplace, xtp.value, xtp.nitems);
      else
        success = false;
      if (xtp.value)
        XFree (xtp.value);
    }
  return success;
}

static void
cairo_set_source_color (cairo_t *cr, Color c)
{
  cairo_set_source_rgba (cr, c.red() / 255., c.green() / 255., c.blue() / 255., c.alpha() / 255.);
}

static Pixmap
create_checkerboard_pixmap (Display *display, Visual *visual, Drawable drawable, uint depth,
                            int tile_size, Color c1, Color c2)
{
  const int bw = tile_size * 2, bh = tile_size * 2;
  Pixmap xpixmap = XCreatePixmap (display, drawable, bw, bh, depth);
  cairo_surface_t *xsurface = cairo_xlib_surface_create (display, xpixmap, visual, bw, bh);
  assert_return (xsurface && cairo_surface_status (xsurface) == CAIRO_STATUS_SUCCESS, 0);
  cairo_t *cr = cairo_create (xsurface);
  cairo_set_source_color (cr, c1);
  cairo_rectangle (cr,         0,         0, tile_size, tile_size);
  cairo_rectangle (cr, tile_size, tile_size, tile_size, tile_size);
  cairo_fill (cr);
  cairo_set_source_color (cr, c2);
  cairo_rectangle (cr,         0, tile_size, tile_size, tile_size);
  cairo_rectangle (cr, tile_size,         0, tile_size, tile_size);
  cairo_fill (cr);
  cairo_destroy (cr);
  cairo_surface_destroy (xsurface);
  return xpixmap;
}

void
ScreenWindowX11::setup (const ScreenWindow::Setup &setup)
{
  // window is not yet mapped
  m_state.setup.window_type = setup.window_type;
  m_state.setup.window_flags = setup.window_flags;
  if (setup.modal)
    FIXME ("modal window's unimplemented");
  m_state.setup.modal = setup.modal;
  m_state.setup.bg_average = setup.bg_average;
  set_text_property (x11context, m_window, "WM_WINDOW_ROLE", XStringStyle, setup.session_role, DELETE_EMPTY);   // ICCCM
  m_state.setup.session_role = setup.session_role;
  m_state.setup.bg_average = setup.bg_average;
  Color c1 = setup.bg_average, c2 = setup.bg_average;
  c1.tint (0, 0.96, 0.96);
  c2.tint (0, 1.03, 1.03);
  Pixmap xpixmap = create_checkerboard_pixmap (x11context.display, x11context.visual, m_window, x11context.depth, 8, c1, c2);
  XSetWindowBackgroundPixmap (x11context.display, m_window, xpixmap); // m_state.setup.bg_average ?
  XFreePixmap (x11context.display, xpixmap);
}

void
ScreenWindowX11::configure (const Config &config)
{
  ScopedLock<Mutex> x11locker (x11_rmutex);
  if (config.title != m_state.config.title)
    {
      set_text_property (x11context, m_window, "WM_NAME", XStdICCTextStyle, config.title);                      // ICCCM
      set_text_property (x11context, m_window, "_NET_WM_NAME", XUTF8StringStyle, config.title);                 // EWMH
      m_state.config.title = config.title;
    }
  enqueue_locked (create_event_win_size (m_event_context, 0, m_state.width, m_state.height));
}

bool
ScreenWindowX11::viewable (void) { /*FIXME*/ return false; }

void
ScreenWindowX11::beep() { /*FIXME*/ }

void
ScreenWindowX11::present () { /*FIXME*/ }

void
ScreenWindowX11::start_user_move (uint button, double root_x, double root_y) { /*FIXME*/ }

void
ScreenWindowX11::start_user_resize (uint button, double root_x, double root_y, AnchorType edge) { /*FIXME*/ }

// == X11 thread ==
class X11Thread {
  X11Context &x11context;
  bool
  filter_event (const XEvent &xevent)
  {
    switch (xevent.type)
      {
      case MappingNotify:
        XRefreshKeyboardMapping (const_cast<XMappingEvent*> (&xevent.xmapping));
        return true;
      default:
        return false;
      }
  }
  void
  process_x11()
  {
    while (XPending (x11context.display))
      {
        XEvent xevent = { 0, };
        XNextEvent (x11context.display, &xevent);
        bool consumed = filter_event (xevent);
        X11Item *xitem = x11context.x11ids[xevent.xany.window];
        if (xitem)
          {
            ScreenWindowX11 *scw = dynamic_cast<ScreenWindowX11*> (xitem);
            if (scw)
              consumed = scw->process_event (xevent);
          }
        if (!consumed)
          {
            const char ss = xevent.xany.send_event ? 'S' : 's';
            EDEBUG ("Extra: %c=%lu w=%lu event_type=%d", ss, xevent.xany.serial, xevent.xany.window, xevent.type);
          }
      }
  }
  void
  run()
  {
    ScopedLock<Mutex> x11locker (x11_rmutex);
    const bool running = true;
    while (running)
      {
        process_x11();
        fd_set rfds;
        FD_ZERO (&rfds);
        FD_SET (ConnectionNumber (x11context.display), &rfds);
        int maxfd = ConnectionNumber (x11context.display);
        struct timeval tv;
        tv.tv_sec = 9999, tv.tv_usec = 0;
        x11locker.unlock();
        // XDEBUG ("Xpoll: nfds=%d", maxfd + 1);
        int retval = select (maxfd + 1, &rfds, NULL, NULL, &tv);
        x11locker.lock();
        if (retval < 0)
          XDEBUG ("select(%d): %s", ConnectionNumber (x11context.display), strerror (errno));
      }
  }
  std::thread m_thread_handle;
public:
  X11Thread (X11Context &ix11context) : x11context (ix11context) {}
  void
  start()
  {
    ScopedLock<Mutex> x11locker (x11_rmutex);
    assert_return (std::thread::id() == m_thread_handle.get_id());
    m_thread_handle = std::thread (&X11Thread::run, this);
  }
};

static X11Thread*
start_x11_thread (X11Context &x11context)
{
  ScopedLock<Mutex> x11locker (x11_rmutex);
  X11Thread *x11_thread = new X11Thread (x11context);
  x11_thread->start();
  return x11_thread;
}

static void
stop_x11_thread (X11Thread *x11_thread)
{
  assert_return (x11_thread != NULL);
  ScopedLock<Mutex> x11locker (x11_rmutex);
  FIXME ("stop and join X11Thread");
  assert_unreached();
}

} // Rapicorn
