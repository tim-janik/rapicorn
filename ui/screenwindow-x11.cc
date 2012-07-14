// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "screenwindow.hh"
#include <ui/utilities.hh>
#include <cairo/cairo-xlib.h>
#include <fcntl.h> // for wakeup pipe
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define EDEBUG(...)     RAPICORN_KEY_DEBUG ("Events", __VA_ARGS__)
#define XDEBUG(...)     RAPICORN_KEY_DEBUG ("X11", __VA_ARGS__)
#define CQUOTE(str)     String (str ? String ("\"" + String (str) + "\"") : String ("NULL")).c_str() // FIXME

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

static X11Context&
x11_context ()
{
  static X11Context *x11p = NULL;
  do_once {
    const char *s = getenv ("DISPLAY");
    String ds = s ? s : "";
    static X11Context x11context (ds);
    if (!x11context.display)
      fatal ("failed to open X11 display: %s", CQUOTE (ds.c_str()));
    x11p = &x11context;
  }
  return *x11p;
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
  virtual ScreenWindow* create_screen_window (WindowType screen_window_type, ScreenWindow::EventReceiver &receiver);
  ScreenDriverX11() : ScreenDriver ("X11Window", -1), m_x11context (NULL), m_open_count (0) {}
};
static ScreenDriverX11 screen_driver_x11;

// == ScreenWindowX11 ==
struct ScreenWindowX11 : public virtual ScreenWindow, public virtual X11Item {
  X11Context           &x11context;
  ScreenDriverX11      &m_x11driver;
  EventReceiver        &m_receiver;
  Info                  m_info;
  State                 m_state;
  Color                 m_average_background;
  Window                m_window;
  int                   m_last_motion_time;
  bool                  m_mapped, m_focussed;
  EventContext          m_event_context;
  std::vector<Rect>     m_expose_rectangles;
  explicit              ScreenWindowX11         (ScreenDriverX11 &x11driver, WindowType screen_window_type, EventReceiver &receiver);
  virtual              ~ScreenWindowX11         ();
  virtual void          trigger_hint_action     (WindowHint hint);
  virtual void          start_user_move         (uint button, double root_x, double root_y);
  virtual void          start_user_resize       (uint button, double root_x, double root_y, AnchorType edge);
  virtual void          present_screen_window   ();
  virtual void          show                    ();
  virtual bool          visible                 ();
  virtual bool          viewable                ();
  virtual void          hide                    ();
  virtual void          enqueue_win_draws       ();
  virtual uint          last_draw_stamp         ();
  virtual Info          get_info                ();
  virtual State         get_state               ();
  virtual void          beep                    ();
  virtual void          blit_surface            (cairo_surface_t *surface, Rapicorn::Region region);
  virtual void          set_config              (const Config &config, bool force_resize_draw);
  void                  enqueue_locked          (Event *event);
  bool                  process_event           (const XEvent &xevent);
};

ScreenWindow*
ScreenDriverX11::create_screen_window (WindowType screen_window_type, ScreenWindow::EventReceiver &receiver)
{
  assert_return (m_open_count > 0, NULL);
  return new ScreenWindowX11 (*this, screen_window_type, receiver);
}

ScreenWindowX11::ScreenWindowX11 (ScreenDriverX11 &x11driver, WindowType screen_window_type, EventReceiver &receiver) :
  x11context (*x11driver.m_x11context), m_x11driver (x11driver), m_receiver (receiver), m_average_background (0xff808080),
  m_window (0), m_last_motion_time (0), m_mapped (false), m_focussed (false)
{
  ScopedLock<Mutex> x11locker (x11_rmutex);
  m_x11driver.open();
  m_info.window_type = screen_window_type;
  const Config sample_config;
  const bool is_override_redirect =
    (m_info.window_type == WINDOW_TYPE_DESKTOP ||
     m_info.window_type == WINDOW_TYPE_DROPDOWN_MENU ||
     m_info.window_type == WINDOW_TYPE_POPUP_MENU ||
     m_info.window_type == WINDOW_TYPE_TOOLTIP ||
     m_info.window_type == WINDOW_TYPE_NOTIFICATION ||
     m_info.window_type == WINDOW_TYPE_COMBO ||
     m_info.window_type == WINDOW_TYPE_DND);
  XSetWindowAttributes attributes;
  attributes.event_mask        = ExposureMask | StructureNotifyMask | EnterWindowMask | LeaveWindowMask |
                                 KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask |
                                 PointerMotionMask | PointerMotionHintMask | ButtonMotionMask |
                                 Button1MotionMask | Button2MotionMask | Button3MotionMask | Button4MotionMask | Button5MotionMask;
  attributes.background_pixel  = XWhitePixel (x11context.display, x11context.screen);
  attributes.border_pixel      = XBlackPixel (x11context.display, x11context.screen);
  attributes.override_redirect = is_override_redirect;
  attributes.win_gravity       = StaticGravity;
  attributes.bit_gravity       = StaticGravity;
  unsigned long attribute_mask = CWWinGravity | CWBitGravity |
                                 /*CWBackPixel |*/ CWBorderPixel | CWOverrideRedirect | CWEventMask;
  const int border = 3;
  m_window = XCreateWindow (x11context.display, x11context.root_window, 0, 0, sample_config.request_width, sample_config.request_height, border,
                            x11context.depth, InputOutput, x11context.visual, attribute_mask, &attributes);
  if (m_window)
    x11context.x11ids[m_window] = this;
  printerr ("window: %lu\n", m_window);
#if 0 // FIXME:  set GdkWindowTypeHint
  switch (m_info.window_type)
    {
    case WINDOW_TYPE_NORMAL:            gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_NORMAL);		break;
    case WINDOW_TYPE_DESKTOP:           gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_DESKTOP);	break;
    case WINDOW_TYPE_DOCK:              gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_DOCK);		break;
    case WINDOW_TYPE_TOOLBAR:           gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_TOOLBAR);	break;
    case WINDOW_TYPE_MENU:              gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_MENU);		break;
    case WINDOW_TYPE_UTILITY:           gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_UTILITY);	break;
    case WINDOW_TYPE_SPLASH:            gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_SPLASHSCREEN);	break;
    case WINDOW_TYPE_DIALOG:            gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_DIALOG);		break;
    case WINDOW_TYPE_DROPDOWN_MENU:     gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_DROPDOWN_MENU);  break;
    case WINDOW_TYPE_POPUP_MENU:        gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_POPUP_MENU);	break;
    case WINDOW_TYPE_TOOLTIP:           gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_TOOLTIP);	break;
    case WINDOW_TYPE_NOTIFICATION:      gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_NOTIFICATION);	break;
    case WINDOW_TYPE_COMBO:             gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_COMBO);		break;
    case WINDOW_TYPE_DND:               gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_DND);		break;
    default: break;
    }
  gtk_widget_grab_focus (GTK_WIDGET (m_screen_window));
#endif
  // configure state for this window
  {
    XConfigureEvent xev = { ConfigureNotify, /*serial*/ 0, true, x11context.display, m_window, m_window,
                            0, 0, int (sample_config.request_width), int (sample_config.request_height), border, // FIXME: uncast
                            /*above*/ 0, is_override_redirect };
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
}

void
ScreenWindowX11::enqueue_locked (Event *event)
{
  // ScopedLock<Mutex> x11locker (x11_rmutex);
  m_receiver.enqueue_async (event);
}

ScreenWindow::Info
ScreenWindowX11::get_info ()
{
  ScopedLock<Mutex> x11locker (x11_rmutex);
  return m_info;
}

ScreenWindow::State
ScreenWindowX11::get_state () // FIXME
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

void
ScreenWindowX11::set_config (const Config &config, bool force_resize_draw)
{
  ScopedLock<Mutex> x11locker (x11_rmutex);
  set_text_property (x11context, m_window, "WM_NAME", XStdICCTextStyle, config.title);                          // ICCCM
  set_text_property (x11context, m_window, "_NET_WM_NAME", XUTF8StringStyle, config.title);                     // EWMH
  set_text_property (x11context, m_window, "WM_WINDOW_ROLE", XStringStyle, config.session_role, DELETE_EMPTY);  // ICCCM
}

void
ScreenWindowX11::beep() { /*FIXME*/ }

void
ScreenWindowX11::hide (void) { /*FIXME*/ }

void
ScreenWindowX11::present_screen_window () { /*FIXME*/ }

void
ScreenWindowX11::start_user_move (uint button, double root_x, double root_y) { /*FIXME*/ }

void
ScreenWindowX11::start_user_resize (uint button, double root_x, double root_y, AnchorType edge) { /*FIXME*/ }

bool
ScreenWindowX11::visible (void) { /*FIXME*/ return false; }

bool
ScreenWindowX11::viewable (void) { /*FIXME*/ return false; }

void
ScreenWindowX11::enqueue_win_draws (void) { /*FIXME*/ }

uint
ScreenWindowX11::last_draw_stamp () { /*FIXME*/ return 0; }

void
ScreenWindowX11::trigger_hint_action (WindowHint window_hint) { /*FIXME*/ }

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
