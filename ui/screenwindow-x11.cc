// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "screenwindow.hh"
#include "uithread.hh"
#include <cairo/cairo-xlib.h>
#include <algorithm>
#include <unistd.h>

#define EDEBUG(...)     RAPICORN_KEY_DEBUG ("XEvents", __VA_ARGS__)
#define VDEBUG(...)     RAPICORN_KEY_DEBUG ("XEvent2", __VA_ARGS__) // verbose event debugging
static auto dbe_x11sync = RAPICORN_DEBUG_OPTION ("x11sync", "Synchronize X11 operations for correct error attribution.");

typedef ::Pixmap XPixmap; // avoid conflicts with Rapicorn::Pixmap

#include "screenwindow-xaux.cc" // helpers, need dbe_x11sync

template<class T> cairo_status_t cairo_status_from_any (T t);
template<> cairo_status_t cairo_status_from_any (cairo_status_t c)          { return c; }
template<> cairo_status_t cairo_status_from_any (cairo_rectangle_list_t *c) { return c ? c->status : CAIRO_STATUS_NULL_POINTER; }
template<> cairo_status_t cairo_status_from_any (cairo_font_options_t *c)   { return c ? cairo_font_options_status (c) : CAIRO_STATUS_NULL_POINTER; }
template<> cairo_status_t cairo_status_from_any (cairo_font_face_t *c)      { return c ? cairo_font_face_status (c) : CAIRO_STATUS_NULL_POINTER; }
template<> cairo_status_t cairo_status_from_any (cairo_scaled_font_t *c)    { return c ? cairo_scaled_font_status (c) : CAIRO_STATUS_NULL_POINTER; }
template<> cairo_status_t cairo_status_from_any (cairo_path_t *c)           { return c ? c->status : CAIRO_STATUS_NULL_POINTER; }
template<> cairo_status_t cairo_status_from_any (cairo_t *c)                { return c ? cairo_status (c) : CAIRO_STATUS_NULL_POINTER; }
template<> cairo_status_t cairo_status_from_any (cairo_device_t *c)         { return c ? cairo_device_status (c) : CAIRO_STATUS_NULL_POINTER; }
template<> cairo_status_t cairo_status_from_any (cairo_surface_t *c)        { return c ? cairo_surface_status (c) : CAIRO_STATUS_NULL_POINTER; }
template<> cairo_status_t cairo_status_from_any (cairo_pattern_t *c)        { return c ? cairo_pattern_status (c) : CAIRO_STATUS_NULL_POINTER; }
template<> cairo_status_t cairo_status_from_any (const cairo_region_t *c)   { return c ? cairo_region_status (c) : CAIRO_STATUS_NULL_POINTER; }

#define RAPICORN_CHECK_CAIRO_STATUS(cairox)             do {    \
    cairo_status_t ___s = cairo_status_from_any (cairox);       \
    if (___s != CAIRO_STATUS_SUCCESS)                           \
      RAPICORN_CRITICAL ("cairo status (%s): %s", #cairox,      \
                         cairo_status_to_string (___s));        \
  } while (0)
#define CHECK_CAIRO_STATUS(cairox)      RAPICORN_CHECK_CAIRO_STATUS (cairox)

namespace Rapicorn {

// == X11Widget ==
struct X11Widget {
  explicit X11Widget() {}
  virtual ~X11Widget() {}
};

// == X11Context ==
class X11Context {
  MainLoop             &loop_;
  vector<size_t>        queued_updates_; // XIDs
  map<size_t, X11Widget*> x11ids_;
  AsyncNotifyingQueue<ScreenCommand*> &command_queue_;
  AsyncBlockingQueue<ScreenCommand*>  &reply_queue_;
  bool                  x11_dispatcher  (const EventLoop::State &state);
  bool                  x11_io_handler  (PollFD &pfd);
  void                  process_x11     ();
  bool                  filter_event    (const XEvent&);
  void                  process_updates ();
  bool                  cmd_dispatcher  (const EventLoop::State &state);
public:
  ScreenDriver         &screen_driver;
  Display              *display;
  int                   screen;
  Visual               *visual;
  int                   depth;
  Window                root_window;
  XIM                   input_method;
  XIMStyle              input_style;
  int8                  shared_mem_;
  X11Widget*            x11id_get   (size_t xid);
  void                  x11id_set   (size_t xid, X11Widget *x11widget);
  Atom                  atom         (const String &text, bool force_create = true);
  String                atom         (Atom atom) const;
  bool                  local_x11    ();
  void                  queue_update (size_t xid);
  void                  run        ();
  bool                  connect    ();
  String                target_atom_to_mime             (const Atom target_atom);
  Atom                  mime_to_target_atom             (const String &mime, Atom last_failed = None);
  void                  text_plain_to_target_atoms      (vector<Atom> &targets);
  explicit              X11Context (ScreenDriver &driver, AsyncNotifyingQueue<ScreenCommand*> &command_queue, AsyncBlockingQueue<ScreenCommand*> &reply_queue);
  virtual              ~X11Context ();
};

static inline int
time_cmp (Time t1, Time t2)
{
  if (t1 > t2)
    return t1 - t2 < 2147483647 ? +1 : -1;      // +1: t1 is later than t2
  if (t1 < t2)
    return t2 - t1 < 2147483647 ? -1 : +1;      // -1: t1 is earlier than t2
  return 0;
}

static ScreenDriverFactory<X11Context> screen_driver_x11 ("X11Window", -1);

// == ContentOffer ==
struct ContentOffer {           // primary_
  StringVector  content_types;  // selection mime types
  Time          time;           // selection time
  ContentOffer() : time (0) {}
};

// == ContentRequest ==
struct ContentRequest {
  uint64                 nonce;
  XSelectionRequestEvent xsr;
  String                 data_type;
  String                 data;
  bool                   data_provided;
  ContentRequest (bool _data_provided = false) : nonce (0), xsr ({ 0, }), data_provided (_data_provided) {}
};

// == SelectionInput ==
enum SelectionInputState {
  WAIT_INVALID,
  WAIT_FOR_NOTIFY,
  WAIT_FOR_PROPERTY,
  WAIT_FOR_RESULT,
};
struct SelectionInput {                 // isel_
  SelectionInputState   state;
  Atom                  slot;           // receiving property (usually RAPICORN_SELECTION)
  vector<uint8>         data;
  uint64                nonce;
  Atom                  source;         // requested selection
  String                content_type;   // requested type
  Atom                  target;         // X11 Atom for requested type
  Window                owner;
  Time                  time;
  int                   size_est;       // lower bound provided by INCR response
  RawData               raw;
  SelectionInput() : state (WAIT_INVALID), slot (0), nonce (0), source (0), target (0), owner (0), time (0), size_est (0) {}
};

// == ScreenWindowX11 ==
struct ScreenWindowX11 : public virtual ScreenWindow, public virtual X11Widget {
  X11Context           &x11context;
  Config                config_;
  State                 state_;
  Window                window_;
  XIC                   input_context_;
  XPixmap               wm_icon_;
  EventContext          event_context_;
  Rapicorn::Region      expose_region_;
  cairo_surface_t      *expose_surface_;
  int                   last_motion_time_, pending_configures_, pending_exposes_;
  bool                  override_redirect_, crossing_focus_;
  vector<uint32>        queued_updates_;       // "atoms" not yet updated
  SelectionInput       *isel_;
  ContentOffer         *primary_;
  vector<ContentRequest> content_requests_;
  explicit              ScreenWindowX11         (X11Context &_x11context);
  virtual              ~ScreenWindowX11         ();
  void                  destroy_x11_resources   ();
  void                  handle_command          (ScreenCommand *command);
  void                  setup_window            (const ScreenWindow::Setup &setup);
  void                  create_window           (const ScreenWindow::Setup &setup, const ScreenWindow::Config &config);
  void                  configure_window        (const Config &config, bool sizeevent);
  void                  blit                    (cairo_surface_t *surface, const Rapicorn::Region &region);
  void                  filtered_event          (const XEvent &xevent);
  bool                  process_event           (const XEvent &xevent);
  bool                  send_selection_notify   (Window req_window, Atom selection, Atom target, Atom req_property, Time req_time);
  void                  request_selection       (Atom source, uint64 nonce, String data_type, Atom last_failed = None);
  void                  receive_selection       (const XEvent &xev);
  void                  handle_content_request  (size_t nth);
  void                  client_message          (const XClientMessageEvent &xevent);
  void                  blit_expose_region      ();
  void                  force_update            (Window window);
  virtual ScreenDriver& screen_driver_async     () const { return x11context.screen_driver; } // executed from arbitrary threads
};

ScreenWindowX11::ScreenWindowX11 (X11Context &_x11context) :
  x11context (_x11context),
  window_ (None), input_context_ (NULL), wm_icon_ (None), expose_surface_ (NULL),
  last_motion_time_ (0), pending_configures_ (0), pending_exposes_ (0),
  override_redirect_ (false), crossing_focus_ (false), isel_ (NULL), primary_ (NULL)
{}

ScreenWindowX11::~ScreenWindowX11()
{
  if (expose_surface_ || wm_icon_ || input_context_ || window_)
    {
      critical ("%s: stale X11 resource during deletion: ex=%p ic=0x%x im=%p w=0x%x", STRFUNC, expose_surface_, wm_icon_, input_context_, window_);
      destroy_x11_resources(); // shouldn't happen, this potentially issues X11 calls from outside the X11 thread
    }
  if (isel_)
    delete isel_;
  if (primary_)
    delete primary_;
}

void
ScreenWindowX11::destroy_x11_resources()
{
  if (expose_surface_)
    {
      cairo_surface_destroy (expose_surface_);
      expose_surface_ = NULL;
    }
  if (wm_icon_)
    {
      XFreePixmap (x11context.display, wm_icon_);
      wm_icon_ = None;
    }
  if (input_context_)
    {
      XDestroyIC (input_context_);
      input_context_ = NULL;
    }
  if (window_)
    {
      XDestroyWindow (x11context.display, window_);
      x11context.x11id_set (window_, NULL);
      window_ = 0;
    }
}

void
ScreenWindowX11::create_window (const ScreenWindow::Setup &setup, const ScreenWindow::Config &config)
{
  assert_return (!window_ && !expose_surface_);
  state_.window_type = setup.window_type;
  update_state (state_);
  override_redirect_ = (setup.window_type == WINDOW_TYPE_DESKTOP ||
                         setup.window_type == WINDOW_TYPE_DROPDOWN_MENU ||
                         setup.window_type == WINDOW_TYPE_POPUP_MENU ||
                         setup.window_type == WINDOW_TYPE_TOOLTIP ||
                         setup.window_type == WINDOW_TYPE_NOTIFICATION ||
                         setup.window_type == WINDOW_TYPE_COMBO ||
                         setup.window_type == WINDOW_TYPE_DND);
  XSetWindowAttributes attributes;
  attributes.event_mask        = ExposureMask | StructureNotifyMask | SubstructureNotifyMask | VisibilityChangeMask | PropertyChangeMask |
                                 FocusChangeMask | EnterWindowMask | LeaveWindowMask | PointerMotionMask | PointerMotionHintMask |
                                 KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | ButtonMotionMask |
                                 0 * OwnerGrabButtonMask; // don't use owner_events for automatic grabs
  attributes.background_pixel  = XWhitePixel (x11context.display, x11context.screen);
  attributes.border_pixel      = XBlackPixel (x11context.display, x11context.screen);
  attributes.override_redirect = override_redirect_;
  // CWWinGravity: attributes.win_gravity - many WMs are buggy if this is not the default NorthWestGravity
  attributes.bit_gravity       = StaticGravity;
  attributes.save_under        = override_redirect_;
  attributes.backing_store     = x11context.local_x11() ? NotUseful : WhenMapped;
  unsigned long attribute_mask = CWBitGravity | CWBackingStore | CWSaveUnder |
                                 CWBackPixel | CWBorderPixel | CWOverrideRedirect | CWEventMask;
  const int border = 0, request_width = MAX (1, config.request_width), request_height = MAX (1, config.request_height);
  const int request_x = 0, request_y = 0;
  // create and register window
  const ulong create_serial = NextRequest (x11context.display);
  window_ = XCreateWindow (x11context.display, x11context.root_window, request_x, request_y, request_width, request_height, border,
                            x11context.depth, InputOutput, x11context.visual, attribute_mask, &attributes);
  assert (window_ != 0);
  x11context.x11id_set (window_, this);
  // adjust X hints & settings
  vector<Atom> atoms;
  atoms.push_back (x11context.atom ("WM_DELETE_WINDOW")); // request client messages instead of XKillClient
  atoms.push_back (x11context.atom ("WM_TAKE_FOCUS"));
  atoms.push_back (x11context.atom ("_NET_WM_PING"));
  XSetWMProtocols (x11context.display, window_, atoms.data(), atoms.size());
  // initialize state
  state_.root_x = state_.deco_x = request_x;
  state_.root_y = state_.deco_y = request_y;
  state_.width = request_width;
  state_.height = request_height;
  update_state (state_);
  // window setup
  setup_window (setup);
  configure_window (config, true);
  // create input context if possible
  String imerr = x11_input_context (x11context.display, window_, &attributes.event_mask,
                                    x11context.input_method, x11context.input_style, &input_context_);
  if (!imerr.empty())
    XDEBUG ("XIM: window=%u: %s", window_, imerr.c_str());
  // configure initial state for this window
  XConfigureEvent xev = { ConfigureNotify, create_serial, false, x11context.display, window_, window_,
                          0, 0, request_width, request_height, border, /*above*/ 0, override_redirect_, };
  XEvent xevent;
  xevent.xconfigure = xev;
  process_event (xevent);
}

struct PendingSensor {
  Drawable window;
  int pending_configures, pending_exposes;
};

static Bool
pending_event_sensor (Display *display, XEvent *event, XPointer arg)
{
  PendingSensor &ps = *(PendingSensor*) arg;
  if (event->xany.window == ps.window)
    switch (event->xany.type)
      {
      case Expose:              ps.pending_exposes++;           break;
      case ConfigureNotify:     ps.pending_configures++;        break;
      }
  return False;
}

static void
check_pending (Display *display, Drawable window, int *pending_configures, int *pending_exposes)
{
  XEvent dummy;
  PendingSensor ps = { window, 0, 0 };
  XCheckIfEvent (display, &dummy, pending_event_sensor, XPointer (&ps));
  *pending_configures = ps.pending_configures;
  *pending_exposes = ps.pending_exposes;
}

void
ScreenWindowX11::filtered_event (const XEvent &xevent)
{
  const bool sent = xevent.xany.send_event != 0;
  const char ff = event_context_.synthesized ? 'F' : 'f';
  switch (xevent.type)
    {
    case KeyPress: {    // XIM often filteres our key presses, but we still need to learn about time & modifier updates
      const char *kind = xevent.type == KeyPress ? "DN" : "UP";
      const XKeyEvent &xev = xevent.xkey;
      if (!sent && xev.keycode != 0 && xev.serial != 0)
        {
          event_context_.time = xev.time;
          event_context_.x = xev.x;
          event_context_.y = xev.y;
          event_context_.modifiers = ModifierState (xev.state);
        }
      EDEBUG ("Key%s: %c=%u w=%u c=%u p=%+d%+d mod=%x cod=%d", kind, ff, xev.serial, xev.window, xev.subwindow, xev.x, xev.y, xev.state, xev.keycode);
      break; }
    }
}

bool
ScreenWindowX11::process_event (const XEvent &xevent)
{
  event_context_.synthesized = xevent.xany.send_event;
  const char ss = event_context_.synthesized ? 'S' : 's';
  bool consumed = false;
  switch (xevent.type)
    {
    case CreateNotify: {
      const XCreateWindowEvent &xev = xevent.xcreatewindow;
      EDEBUG ("Creat: %c=%u w=%u a=%+d%+d%+dx%d b=%d", ss, xev.serial, xev.window, xev.x, xev.y, xev.width, xev.height, xev.border_width);
      consumed = true;
      break; }
    case ConfigureNotify: {
      const XConfigureEvent &xev = xevent.xconfigure;
      if (xev.window != window_)
        break;
      if (xev.send_event) // WM notification, x/y are given in root coordinates
        {
          // update our idea of the window position, assuming constant deco extents
          state_.deco_x += - state_.root_x + xev.x;
          state_.deco_y += - state_.root_y + xev.y;
          state_.root_x = xev.x;
          state_.root_y = xev.y;
          update_state (state_);
        }
      else if (state_.width != xev.width || state_.height != xev.height)
        {
          state_.width = xev.width;
          state_.height = xev.height;
          if (expose_surface_)
            {
              cairo_surface_destroy (expose_surface_);
              expose_surface_ = NULL;
            }
          expose_region_.clear();
          update_state (state_);
        }
      if (pending_configures_)
        pending_configures_--;
      if (!pending_configures_)
        check_pending (x11context.display, window_, &pending_configures_, &pending_exposes_);
      enqueue_event (create_event_win_size (event_context_, state_.width, state_.height, pending_configures_ > 0));
      if (!pending_configures_)
        {
          queued_updates_.push_back (x11context.atom ("_NET_FRAME_EXTENTS")); // determine real origin
          x11context.queue_update (window_);
        }
      EDEBUG ("Confg: %c=%u w=%u a=%+d%+d%+dx%d b=%d c=%d", ss, xev.serial, xev.window, state_.root_x, state_.root_y,
              state_.width, state_.height, xev.border_width, pending_configures_);
      consumed = true;
      break; }
    case GravityNotify: {
      const XGravityEvent &xev = xevent.xgravity;
      VDEBUG ("Gravt: %c=%u w=%u p=%+d%+d", ss, xev.serial, xev.window, xev.x, xev.y);
      consumed = true;
      break; }
    case MapNotify: {
      const XMapEvent &xev = xevent.xmap;
      if (xev.window != window_)
        break;
      EDEBUG ("Map  : %c=%u w=%u e=%u", ss, xev.serial, xev.window, xev.event);
      state_.visible = true;
      queued_updates_.push_back (x11context.atom ("_NET_FRAME_EXTENTS")); // determine real origin
      force_update (xev.window); // immediately figure deco and root position
      consumed = true;
      break; }
    case UnmapNotify: {
      const XUnmapEvent &xev = xevent.xunmap;
      if (xev.window != window_)
        break;
      EDEBUG ("Unmap: %c=%u w=%u e=%u", ss, xev.serial, xev.window, xev.event);
      state_.visible = false;
      update_state (state_);
      enqueue_event (create_event_cancellation (event_context_));
      consumed = true;
      break; }
    case ReparentNotify:  {
      const XReparentEvent &xev = xevent.xreparent;
      EDEBUG ("Rprnt: %c=%u w=%u p=%u @=%+d%+d ovr=%d", ss, xev.serial, xev.window, xev.parent, xev.x, xev.y, xev.override_redirect);
      consumed = true;
      break; }
    case VisibilityNotify: {
      const XVisibilityEvent &xev = xevent.xvisibility;
      EDEBUG ("Visbl: %c=%u w=%u notify=%s", ss, xev.serial, xev.window, visibility_state (xev.state));
      consumed = true;
      break; }
    case PropertyNotify: {
      const XPropertyEvent &xev = xevent.xproperty;
      const bool deleted = xev.state == PropertyDelete;
      VDEBUG ("Prop%c: %c=%u w=%u prop=%s", deleted ? 'D' : 'C', ss, xev.serial, xev.window, x11context.atom (xev.atom).c_str());
      event_context_.time = xev.time;
      if (isel_ && isel_->slot == xev.atom)
        receive_selection (xevent);
      else
        {
          queued_updates_.push_back (xev.atom);
          x11context.queue_update (window_);
        }
      consumed = true;
      break; }
    case SelectionNotify: {
      const XSelectionEvent &xev = xevent.xselection;
      EDEBUG ("SelNy: %c=%u [%lx] %s(%s) -> %ld(%s)", ss, xev.serial,
              xev.time, x11context.atom (xev.selection), x11context.atom (xev.target),
              xev.requestor, x11context.atom (xev.property));
      if (isel_)
        receive_selection (xevent);
      consumed = true;
      break; }
    case SelectionRequest: {
      const XSelectionRequestEvent &xev = xevent.xselectionrequest;
      EDEBUG ("SelRq: %c=%u [%lx] own=%u %s(%s) -> %ld(%s)", ss, xev.serial,
              xev.time, xev.owner, x11context.atom (xev.selection), x11context.atom (xev.target),
              xev.requestor, x11context.atom (xev.property));
      if (XA_PRIMARY == xev.selection && xev.target && primary_ && time_cmp (xev.time, primary_->time) >= 0)
        {
          const String mime_type = x11context.target_atom_to_mime (xev.target);
          ContentRequest cr (mime_type.empty());
          cr.nonce = (uint64 (rand()) << 32) | rand(); // FIXME: need rand_int64
          cr.xsr = xev;
          content_requests_.push_back (cr);
          if (!cr.data_provided)        // have mime_type
            enqueue_event (create_event_data (CONTENT_REQUEST, event_context_, mime_type, "", cr.nonce));
          else                          // have no mime_type
            handle_content_request (content_requests_.size() - 1);
        }
      else
        send_selection_notify (xev.requestor, xev.selection, xev.target, None, xev.time); // reject
      consumed = true;
      break; }
    case Expose: {
      const XExposeEvent &xev = xevent.xexpose;
      std::vector<Rect> rectangles;
      expose_region_.add (Rect (Point (xev.x, xev.y), xev.width, xev.height));
      if (pending_exposes_)
        pending_exposes_--;
      if (!pending_exposes_ && xev.count == 0)
        check_pending (x11context.display, window_, &pending_configures_, &pending_exposes_);
      String hint = pending_exposes_ ? " (E+)" : expose_surface_ ? "" : " (nodata)";
      VDEBUG ("Expos: %c=%u w=%u a=%+d%+d%+dx%d c=%d%s", ss, xev.serial, xev.window, xev.x, xev.y, xev.width, xev.height, xev.count, hint.c_str());
      if (!pending_exposes_ && xev.count == 0)
        blit_expose_region();
      consumed = true;
      break; }
    case KeyPress: case KeyRelease: {
      const XKeyEvent &xev = xevent.xkey;
      const char  *kind = xevent.type == KeyPress ? "DN" : "UP";
      KeySym keysym = 0;
      int n = 0;
      String utf8data;
      if (input_context_ && xevent.type == KeyPress) // Xutf8LookupString is undefined for KeyRelease
        {
          Status ximstatus = XBufferOverflow;
          char buffer[512];
          n = Xutf8LookupString (input_context_, const_cast<XKeyPressedEvent*> (&xev), buffer, sizeof (buffer), &keysym, &ximstatus);
          if (ximstatus == XBufferOverflow)
            {
              Xutf8ResetIC (input_context_);
              n = 0;
              keysym = 0;
            }
          if (n > 0 && (ximstatus == XLookupChars || ximstatus == XLookupBoth))
            utf8data.assign (buffer, n);
          if (not (ximstatus == XLookupBoth || ximstatus == XLookupKeySym))
            keysym = 0;
        }
      else // !input_context_
        {
          char buffer[512];
          n = XLookupString (const_cast<XKeyEvent*> (&xev), buffer, sizeof (buffer), &keysym, NULL);
          if (n > 0 && xevent.type == KeyPress)
            {
              /* XLookupString(3) is documented to return Latin-1 characters, but modern implementations
               * seem to work locale specific. So we may or may not need to convert to UTF-8. Yay!
               */
              if (!utf8_is_locale_charset() || !utf8_validate (String (buffer, n)))
                for (int i = 0; i < n; i++)
                  {
                    const uint8 l1char = buffer[i];                   // Latin-1 character
                    char utf[8];
                    const int l = utf8_from_unichar (l1char, utf);    // convert to UTF-8
                    utf8data.append (utf, l);
                  }
              else
                utf8data.append (buffer, n);
            }
          // utf8data is empty for KeyRelease, but we try to fill at least keysym
        }
      EDEBUG ("Key%s: %c=%u w=%u c=%u p=%+d%+d mod=%x cod=%d sym=%04x uc=%04x str=%s", kind, ss, xev.serial, xev.window, xev.subwindow, xev.x, xev.y, xev.state, xev.keycode, uint (keysym), key_value_to_unichar (keysym), utf8data);
      if (xev.send_event || xev.keycode == 0 || xev.serial == 0)
        ; // avid corrupting event_context_ from synthesized event
      else
        {
          event_context_.time = xev.time; event_context_.x = xev.x; event_context_.y = xev.y; event_context_.modifiers = ModifierState (xev.state);
        }
      if (keysym || !utf8data.empty())
        enqueue_event (create_event_key (xevent.type == KeyPress ? KEY_PRESS : KEY_RELEASE, event_context_, KeyValue (keysym), utf8data));
      // enqueue_event (create_event_data (CONTENT_DATA, event_context_, "text/plain;charset=utf-8", utf8data));
      consumed = true;
      break; }
    case ButtonPress: case ButtonRelease: {
      const XButtonEvent &xev = xevent.xbutton;
      const char  *kind = xevent.type == ButtonPress ? "DN" : "UP";
      if (xev.window != window_)
        break;
      EDEBUG ("But%s: %c=%u w=%u c=%u p=%+d%+d b=%d", kind, ss, xev.serial, xev.window, xev.subwindow, xev.x, xev.y, xev.button);
      event_context_.time = xev.time; event_context_.x = xev.x; event_context_.y = xev.y; event_context_.modifiers = ModifierState (xev.state);
      if (xevent.type == ButtonPress)
        switch (xev.button)
          {
          case 4:  enqueue_event (create_event_scroll (SCROLL_UP, event_context_));                break;
          case 5:  enqueue_event (create_event_scroll (SCROLL_DOWN, event_context_));              break;
          case 6:  enqueue_event (create_event_scroll (SCROLL_LEFT, event_context_));              break;
          case 7:  enqueue_event (create_event_scroll (SCROLL_RIGHT, event_context_));             break;
          default: enqueue_event (create_event_button (BUTTON_PRESS, event_context_, xev.button)); break;
          }
      else // ButtonRelease
        switch (xev.button)
          {
          case 4: case 5: case 6: case 7: break; // scrolling
          default: enqueue_event (create_event_button (BUTTON_RELEASE, event_context_, xev.button)); break;
          }
      consumed = true;
      break; }
    case MotionNotify: {
      const XMotionEvent &xev = xevent.xmotion;
      if (xev.window != window_)
        break;
      if (xev.is_hint)
        {
          int nevents = 0;
          XTimeCoord *xcoords = XGetMotionEvents (x11context.display, window_, last_motion_time_ + 1, xev.time - 1, &nevents);
          if (xcoords)
            {
              for (int i = 0; i < nevents; ++i)
                if (xcoords[i].x == 0)
                  {
                    /* xserver-xorg-1:7.6+7ubuntu7 + libx11-6:amd64-2:1.4.4-2ubuntu1 may generate buggy motion
                     * history events with x=0 for X220t trackpoints.
                     */
                    VDEBUG ("  ...: S=%u w=%u c=%u p=%+d%+d (discarding)", xev.serial, xev.window, xev.subwindow, xcoords[i].x, xcoords[i].y);
                  }
                else
                  {
                    event_context_.time = xcoords[i].time; event_context_.x = xcoords[i].x; event_context_.y = xcoords[i].y;
                    enqueue_event (create_event_mouse (MOUSE_MOVE, event_context_));
                    VDEBUG ("  ...: S=%u w=%u c=%u p=%+d%+d", xev.serial, xev.window, xev.subwindow, xcoords[i].x, xcoords[i].y);
                  }
              XFree (xcoords);
            }
        }
      VDEBUG ("Mtion: %c=%u w=%u c=%u p=%+d%+d%s", ss, xev.serial, xev.window, xev.subwindow, xev.x, xev.y, xev.is_hint ? " (hint)" : "");
      event_context_.time = xev.time; event_context_.x = xev.x; event_context_.y = xev.y; event_context_.modifiers = ModifierState (xev.state);
      enqueue_event (create_event_mouse (MOUSE_MOVE, event_context_));
      last_motion_time_ = xev.time;
      consumed = true;
      break; }
    case EnterNotify: case LeaveNotify: {
      const XCrossingEvent &xev = xevent.xcrossing;
      const EventType etype = xevent.type == EnterNotify ? MOUSE_ENTER : MOUSE_LEAVE;
      const char *kind = xevent.type == EnterNotify ? "Enter" : "Leave";
      EDEBUG ("%s: %c=%u w=%u c=%u p=%+d%+d notify=%s+%s", kind, ss, xev.serial, xev.window, xev.subwindow, xev.x, xev.y,
              notify_mode (xev.mode), notify_detail (xev.detail));
      event_context_.time = xev.time; event_context_.x = xev.x; event_context_.y = xev.y; event_context_.modifiers = ModifierState (xev.state);
      enqueue_event (create_event_mouse (xev.detail == NotifyInferior ? MOUSE_MOVE : etype, event_context_));
      if (xev.detail != NotifyInferior)
        {
          crossing_focus_ = xev.focus;
          last_motion_time_ = xev.time;
        }
      consumed = true;
      break; }
    case FocusIn: case FocusOut: {
      const XFocusChangeEvent &xev = xevent.xfocus;
      const char *kind = xevent.type == FocusIn ? "FocIn" : "FcOut";
      EDEBUG ("%s: %c=%u w=%u notify=%s+%s", kind, ss, xev.serial, xev.window, notify_mode (xev.mode), notify_detail (xev.detail));
      if (!(xev.detail == NotifyInferior ||     // subwindow focus changed
            xev.detail == NotifyPointer ||      // pointer focus changed
            xev.detail == NotifyPointerRoot ||  // root focus changed
            xev.detail == NotifyDetailNone))    // root focus is discarded
        {
          const bool now_focus = xevent.type == FocusIn;
          if (now_focus != state_.active)
            {
              state_.active = now_focus;
              update_state (state_);
              enqueue_event (create_event_focus (state_.active ? FOCUS_IN : FOCUS_OUT, event_context_));
              if (input_context_)
                {
                  if (state_.active)
                    XSetICFocus (input_context_);
                  else
                    XUnsetICFocus (input_context_);
                }
            }
        }
      consumed = true;
      break; }
    case ClientMessage: {
      const XClientMessageEvent &xev = xevent.xclient;
      if (rapicorn_debug_check()) // avoid atom() round-trips
        {
          const Atom mtype = xev.message_type == x11context.atom ("WM_PROTOCOLS") ? xev.data.l[0] : xev.message_type;
          EDEBUG ("ClMsg: %c=%u w=%u t=%s f=%u", ss, xev.serial, xev.window, x11context.atom (mtype).c_str(), xev.format);
        }
      client_message (xev);
      consumed = true;
      break; }
    case DestroyNotify: {
      const XDestroyWindowEvent &xev = xevent.xdestroywindow;
      if (xev.window != window_)
        break;
      x11context.x11id_set (window_, NULL);
      window_ = 0;
      enqueue_event (create_event_win_destroy (event_context_));
      EDEBUG ("Destr: %c=%u w=%u", ss, xev.serial, xev.window);
      consumed = true;
      break; }
    default:
      EDEBUG ("What?: %c=%u w=%u type=%d", ss, xevent.xany.serial, xevent.xany.window, xevent.xany.type);
      break;
    }
  return consumed;
}

void
ScreenWindowX11::receive_selection (const XEvent &xevent)
{
  return_unless (isel_ != NULL);
  // SelectionNotify (after XConvertSelection)
  if (xevent.type == SelectionNotify && isel_->state == WAIT_FOR_NOTIFY)
    {
      const XSelectionEvent &xev = xevent.xselection;
      if (window_ == xev.requestor && isel_->source == xev.selection && isel_->time == xev.time)
        {
          bool retry = false;
          if (isel_->slot == xev.property)       // Conversion succeeded (except for Qt, see below)
            {
              const bool success = x11_get_property_data (x11context.display, xev.requestor, isel_->slot, isel_->raw, 0, True);
              if (isel_->raw.property_type == x11context.atom ("INCR"))
                {
                  isel_->size_est = isel_->raw.data32.size() > 0 ? isel_->raw.data32[0] : 0;
                  isel_->raw.data32.clear();
                  isel_->raw.property_type = None;
                  isel_->raw.format_returned = 0;
                  isel_->state = WAIT_FOR_PROPERTY;
                }
              else if (!success || isel_->raw.property_type == None)
                retry = true;                   // Conversion failed (Qt style), might retry other result types
              else if (success && isel_->raw.property_type == x11context.atom ("TEXT"))
                retry = true;                   // Work around a Qt bug, might retry other result types
              else
                isel_->state = WAIT_FOR_RESULT;  // Property fully received
            }
          else // xev.property == None
            retry = true;                       // Conversion failed, might retry other result types
          if (retry)
            {
              // Conversion failed, try re-requesting using fallbacks
              SelectionInput *tsel = isel_;
              isel_ = NULL;
              request_selection (tsel->source, tsel->nonce, tsel->content_type, tsel->target);
              delete tsel;
              return;
            }
        }
    }
  // Property NewValue (after SelectionNotify + INCR)
  if (xevent.type == PropertyNotify && isel_->state == WAIT_FOR_PROPERTY)
    {
      const XPropertyEvent &xev = xevent.xproperty;
      if (xev.state == PropertyNewValue && isel_->slot == xev.atom)
        {
          RawData raw;
          const bool success = x11_get_property_data (x11context.display, window_, isel_->slot, raw, 0, True);
          isel_->raw.property_type = raw.property_type;
          isel_->raw.format_returned = raw.format_returned;
          isel_->raw.data32.insert (isel_->raw.data32.end(), raw.data32.begin(), raw.data32.end());
          isel_->raw.data16.insert (isel_->raw.data16.end(), raw.data16.begin(), raw.data16.end());
          isel_->raw.data8.insert (isel_->raw.data8.end(), raw.data8.begin(), raw.data8.end());
          if (!success || (raw.data32.size() == 0 && raw.data16.size() == 0 && raw.data8.size() == 0))
            isel_->state = WAIT_FOR_RESULT; // last property received
        }
    }
  // IPC completed
  if (isel_->state == WAIT_FOR_RESULT)
    {
      isel_->state = WAIT_INVALID;
      String content_type, content_data;
      if (isel_->content_type == "text/plain" &&
          x11_convert_string_property (x11context.display, isel_->raw.property_type, isel_->raw.data8, &content_data))
        content_type = isel_->content_type;
      enqueue_event (create_event_data (CONTENT_DATA, event_context_, content_type, content_data, isel_->nonce));
      delete isel_;
      isel_ = NULL;
    }
}

void
ScreenWindowX11::client_message (const XClientMessageEvent &xev)
{
  const Atom mtype = xev.message_type == x11context.atom ("WM_PROTOCOLS") ? xev.data.l[0] : xev.message_type;
  if      (mtype == x11context.atom ("WM_DELETE_WINDOW"))
    {
      const uint32 saved_time = event_context_.time;
      event_context_.time = xev.data.l[1];
      enqueue_event (create_event_win_delete (event_context_));
      event_context_.time = saved_time; // avoid time warps from client messages
    }
  else if (mtype == x11context.atom ("WM_TAKE_FOCUS"))
    {
      XErrorEvent dummy = { 0, };
      x11_trap_errors (&dummy); // guard against being unmapped
      XSetInputFocus (x11context.display, window_, RevertToPointerRoot, xev.data.l[1]);
      XSync (x11context.display, False);
      x11_untrap_errors();
    }
  else if (mtype == x11context.atom ("_NET_WM_PING"))
    {
      XEvent xevent = *(XEvent*) &xev;
      xevent.xclient.data.l[3] = xevent.xclient.data.l[4] = 0; // [0]=_PING, [1]=time, [2]=window_
      xevent.xclient.window = x11context.root_window;
      XSendEvent (x11context.display, xevent.xclient.window, False, SubstructureNotifyMask | SubstructureRedirectMask, &xevent);
    }
}

void
ScreenWindowX11::force_update (Window window)
{
  const State old_state = state_;
  std::stable_sort (queued_updates_.begin(), queued_updates_.end());
  auto qend = std::unique (queued_updates_.begin(), queued_updates_.end());
  queued_updates_.resize (qend - queued_updates_.begin());
  vector<String> updates;
  for (auto it : queued_updates_)
    updates.push_back (x11context.atom (it));
  uint ignored = 0;
  for (auto aname : updates)
    if (aname == "_NET_FRAME_EXTENTS")
      window_deco_origin (x11context.display, window_, &state_.root_x, &state_.root_y, &state_.deco_x, &state_.deco_y);
    else if (aname == "_NET_WM_VISIBLE_NAME")
      state_.visible_title = x11_get_string_property (x11context.display, window_, x11context.atom (aname));
    else if (aname == "_NET_WM_VISIBLE_ICON_NAME")
      state_.visible_alias = x11_get_string_property (x11context.display, window_, x11context.atom (aname));
    else if (aname == "WM_STATE")
      {
        vector<uint32> datav = x11_get_property_data32 (x11context.display, window_, x11context.atom (aname));
        if (datav.size())
          state_.window_flags = Flags ((state_.window_flags & ~ICONIFY) | (datav[0] == IconicState ? ICONIFY : 0));
      }
    else if (aname == "_NET_WM_STATE")
      {
        vector<uint32> datav = x11_get_property_data32 (x11context.display, window_, x11context.atom (aname));
        uint32 f = 0;
        for (size_t i = 0; i < datav.size(); i++)
          if      (datav[i] == x11context.atom ("_NET_WM_STATE_MODAL"))           f += MODAL;
          else if (datav[i] == x11context.atom ("_NET_WM_STATE_STICKY"))          f += STICKY;
          else if (datav[i] == x11context.atom ("_NET_WM_STATE_MAXIMIZED_VERT"))  f += VMAXIMIZED;
          else if (datav[i] == x11context.atom ("_NET_WM_STATE_MAXIMIZED_HORZ"))  f += HMAXIMIZED;
          else if (datav[i] == x11context.atom ("_NET_WM_STATE_SHADED"))          f += SHADED;
          else if (datav[i] == x11context.atom ("_NET_WM_STATE_SKIP_TASKBAR"))    f += SKIP_TASKBAR;
          else if (datav[i] == x11context.atom ("_NET_WM_STATE_SKIP_PAGER"))      f += SKIP_PAGER;
          else if (datav[i] == x11context.atom ("_NET_WM_STATE_HIDDEN"))          f += HIDDEN;
          else if (datav[i] == x11context.atom ("_NET_WM_STATE_FULLSCREEN"))      f += FULLSCREEN;
          else if (datav[i] == x11context.atom ("_NET_WM_STATE_ABOVE"))           f += ABOVE_ALL;
          else if (datav[i] == x11context.atom ("_NET_WM_STATE_BELOW"))           f += BELOW_ALL;
          else if (datav[i] == x11context.atom ("_NET_WM_STATE_DEMANDS_ATTENTION")) f += ATTENTION;
          else if (datav[i] == x11context.atom ("_NET_WM_STATE_FOCUSED"))         f += FOCUS_DECO;
        state_.window_flags = Flags ((state_.window_flags & ~_WM_STATE_MASK) | f);
      }
    else if (aname == "_MOTIF_WM_HINTS")
      {
        Mwm funcs = Mwm (FUNC_CLOSE | FUNC_MINIMIZE | FUNC_MAXIMIZE), deco = DECOR_ALL;
        get_mwm_hints (x11context.display, window_, &funcs, &deco);
        uint32 f = 0;
        if (funcs & FUNC_CLOSE)                                                   f += DELETABLE;
        if (funcs & FUNC_MINIMIZE || deco & DECOR_MINIMIZE)                       f += MINIMIZABLE;
        if (funcs & FUNC_MAXIMIZE || deco & DECOR_MAXIMIZE)                       f += MAXIMIZABLE;
        if (deco & (DECOR_ALL | DECOR_BORDER | DECOR_RESIZEH | DECOR_TITLE))      f += DECORATED;
        state_.window_flags = Flags ((state_.window_flags & ~_DECO_MASK) | f);
      }
    else
      ignored++;
  if (ignored < updates.size())
    {
      update_state (state_);
      if (rapicorn_debug_check())
        {
          const unsigned long update_serial = XNextRequest (x11context.display) - 1;
          if (old_state.visible_title != state_.visible_title)
            EDEBUG ("State: S=%u w=%u title=%s", update_serial, window_, CQUOTE (state_.visible_title));
          if (old_state.visible_alias != state_.visible_alias)
            EDEBUG ("State: S=%u w=%u alias=%s", update_serial, window_, CQUOTE (state_.visible_alias));
          EDEBUG ("State: S=%u w=%u r=%+d%+d o=%+d%+d flags=%s", update_serial, window_, state_.root_x, state_.root_y,
                  state_.deco_x, state_.deco_y, flags_name (state_.window_flags).c_str());
        }
    }
}

static Rect
cairo_image_surface_coverage (cairo_surface_t *surface)
{
  int w = cairo_image_surface_get_width (surface);
  int h = cairo_image_surface_get_height (surface);
  double x_offset = 0, y_offset = 0;
  cairo_surface_get_device_offset (surface, &x_offset, &y_offset);
  return Rect (Point (x_offset, y_offset), w, h);
}

void
ScreenWindowX11::blit (cairo_surface_t *surface, const Rapicorn::Region &region)
{
  CHECK_CAIRO_STATUS (surface);
  if (!window_)
    return;
  const Rect fullwindow = Rect (0, 0, state_.width, state_.height);
  if (region.count_rects() == 1 && fullwindow == region.extents() && fullwindow == cairo_image_surface_coverage (surface))
    {
      // special case, surface matches exactly the entire window
      if (expose_surface_)
        cairo_surface_destroy (expose_surface_);
      expose_surface_ = cairo_surface_reference (surface);
    }
  else
    {
      if (!expose_surface_)
        expose_surface_ = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, state_.width, state_.height);
      cairo_t *cr = cairo_create (expose_surface_);
      // clip to region
      vector<Rect> rects;
      region.list_rects (rects);
      for (size_t i = 0; i < rects.size(); i++)
        cairo_rectangle (cr, rects[i].x, rects[i].y, rects[i].width, rects[i].height);
      cairo_clip (cr);
      // render onto expose_surface_
      cairo_set_source_surface (cr, surface, 0, 0);
      cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
      cairo_paint (cr);
      // cleanup
      cairo_destroy (cr);
    }
  // redraw expose region
  expose_region_.add (region);
  blit_expose_region();
}

void
ScreenWindowX11::blit_expose_region()
{
  if (!expose_surface_ || !state_.visible)
    {
      expose_region_.clear();
      return;
    }
  CHECK_CAIRO_STATUS (expose_surface_);
  const unsigned long blit_serial = XNextRequest (x11context.display) - 1;
  // surface for drawing on the X11 window
  cairo_surface_t *xsurface = cairo_xlib_surface_create (x11context.display, window_, x11context.visual, state_.width, state_.height);
  CHECK_CAIRO_STATUS (xsurface);
  // cairo context
  cairo_t *xcr = cairo_create (xsurface);
  CHECK_CAIRO_STATUS (xcr);
  // clip to expose_region_
  vector<Rect> rects;
  expose_region_.list_rects (rects);
  uint coverage = 0;
  for (size_t i = 0; i < rects.size(); i++)
    {
      cairo_rectangle (xcr, rects[i].x, rects[i].y, rects[i].width, rects[i].height);
      coverage += rects[i].width * rects[i].height;
    }
  cairo_clip (xcr);
  // paint expose_region_
  cairo_set_source_surface (xcr, expose_surface_, 0, 0);
  cairo_set_operator (xcr, CAIRO_OPERATOR_OVER);
  cairo_paint (xcr);
  CHECK_CAIRO_STATUS (xcr);
  XFlush (x11context.display);
  // debugging info
  if (rapicorn_debug_check())
    {
      const Rect extents = expose_region_.extents();
      VDEBUG ("BlitS: S=%u w=%u e=%+d%+d%+dx%d nrects=%u coverage=%.1f%%", blit_serial, window_,
              int (extents.x), int (extents.y), int (extents.width), int (extents.height),
              rects.size(), coverage * 100.0 / (state_.width * state_.height));
    }
  // cleanup
  expose_region_.clear();
  cairo_destroy (xcr);
  cairo_surface_destroy (xsurface);
}

static void
cairo_set_source_color (cairo_t *cr, Color c)
{
  cairo_set_source_rgba (cr, c.red() / 255., c.green() / 255., c.blue() / 255., c.alpha() / 255.);
}

static XPixmap
create_checkerboard_pixmap (Display *display, Visual *visual, Drawable drawable, uint depth,
                            int tile_size, Color c1, Color c2)
{
  const int bw = tile_size * 2, bh = tile_size * 2;
  XPixmap xpixmap = XCreatePixmap (display, drawable, bw, bh, depth);
  cairo_surface_t *xsurface = cairo_xlib_surface_create (display, xpixmap, visual, bw, bh);
  CHECK_CAIRO_STATUS (xsurface);
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

static const char*
window_type_atom_name (WindowType window_type)
{
  switch (window_type)
    {
    case WINDOW_TYPE_DESKTOP:   	return "_NET_WM_WINDOW_TYPE_DESKTOP";
    case WINDOW_TYPE_DOCK:      	return "_NET_WM_WINDOW_TYPE_DOCK";
    case WINDOW_TYPE_TOOLBAR:   	return "_NET_WM_WINDOW_TYPE_TOOLBAR";
    case WINDOW_TYPE_MENU:      	return "_NET_WM_WINDOW_TYPE_MENU";
    case WINDOW_TYPE_UTILITY:   	return "_NET_WM_WINDOW_TYPE_UTILITY";
    case WINDOW_TYPE_SPLASH:    	return "_NET_WM_WINDOW_TYPE_SPLASH";
    case WINDOW_TYPE_DIALOG:    	return "_NET_WM_WINDOW_TYPE_DIALOG";
    case WINDOW_TYPE_DROPDOWN_MENU:	return "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU";
    case WINDOW_TYPE_POPUP_MENU:	return "_NET_WM_WINDOW_TYPE_POPUP_MENU";
    case WINDOW_TYPE_TOOLTIP:	        return "_NET_WM_WINDOW_TYPE_TOOLTIP";
    case WINDOW_TYPE_NOTIFICATION:	return "_NET_WM_WINDOW_TYPE_NOTIFICATION";
    case WINDOW_TYPE_COMBO:	        return "_NET_WM_WINDOW_TYPE_COMBO";
    case WINDOW_TYPE_DND:       	return "_NET_WM_WINDOW_TYPE_DND";
    default: ;
    case WINDOW_TYPE_NORMAL:	        return "_NET_WM_WINDOW_TYPE_NORMAL";
    }
}

static cairo_surface_t*
cairo_surface_from_pixmap (Rapicorn::Pixmap pixmap)
{
  const int stride = pixmap.width() * 4;
  uint32 *data = pixmap.row (0);
  cairo_surface_t *isurface =
    cairo_image_surface_create_for_data ((unsigned char*) data,
                                         CAIRO_FORMAT_ARGB32,
                                         pixmap.width(),
                                         pixmap.height(),
                                         stride);
  assert_return (CAIRO_STATUS_SUCCESS == cairo_surface_status (isurface), NULL);
  return isurface;
}

void
ScreenWindowX11::setup_window (const ScreenWindow::Setup &setup)
{
  assert_return (state_.visible == false);
  assert_return (wm_icon_ == 0);
  assert_return (state_.window_type == setup.window_type);
  vector<unsigned long> longs;
  // _NET_WM_WINDOW_TYPE
  longs.clear();
  longs.push_back (x11context.atom (window_type_atom_name (setup.window_type)));
  XChangeProperty (x11context.display, window_, x11context.atom ("_NET_WM_WINDOW_TYPE"),
                   XA_ATOM, 32, PropModeReplace, (uint8*) longs.data(), longs.size());
  // _NET_WM_STATE
  longs.clear();
  const uint64 f = setup.request_flags;
  if (f & MODAL)        longs.push_back (x11context.atom ("_NET_WM_STATE_MODAL"));
  if (f & STICKY)       longs.push_back (x11context.atom ("_NET_WM_STATE_STICKY"));
  if (f & VMAXIMIZED)   longs.push_back (x11context.atom ("_NET_WM_STATE_MAXIMIZED_VERT"));
  if (f & HMAXIMIZED)   longs.push_back (x11context.atom ("_NET_WM_STATE_MAXIMIZED_HORZ"));
  if (f & SHADED)       longs.push_back (x11context.atom ("_NET_WM_STATE_SHADED"));
  if (f & SKIP_TASKBAR) longs.push_back (x11context.atom ("_NET_WM_STATE_SKIP_TASKBAR"));
  if (f & SKIP_PAGER)   longs.push_back (x11context.atom ("_NET_WM_STATE_SKIP_PAGER"));
  if (f & FULLSCREEN)   longs.push_back (x11context.atom ("_NET_WM_STATE_FULLSCREEN"));
  if (f & ABOVE_ALL)    longs.push_back (x11context.atom ("_NET_WM_STATE_ABOVE"));
  if (f & BELOW_ALL)    longs.push_back (x11context.atom ("_NET_WM_STATE_BELOW"));
  if (f & ATTENTION)    longs.push_back (x11context.atom ("_NET_WM_STATE_DEMANDS_ATTENTION"));
  XChangeProperty (x11context.display, window_, x11context.atom ("_NET_WM_STATE"),
                   XA_ATOM, 32, PropModeReplace, (uint8*) longs.data(), longs.size());
  // Background
  Color c1 = setup.bg_average, c2 = setup.bg_average;
  if (debug_devel_check())
    {
      c1.tint (0, 0.96, 0.96);
      c2.tint (0, 1.03, 1.03);
    }
  XPixmap xpixmap = create_checkerboard_pixmap (x11context.display, x11context.visual, window_, x11context.depth, 8, c1, c2);
  XSetWindowBackgroundPixmap (x11context.display, window_, xpixmap);
  XFreePixmap (x11context.display, xpixmap);
  // WM Icon
  if (!wm_icon_)
    {
      Rapicorn::Pixmap iconpixmap ("res:icons/wm-gears.png");
      wm_icon_ = XCreatePixmap (x11context.display, window_, iconpixmap.width(), iconpixmap.height(), x11context.depth);
      cairo_surface_t *xsurface = cairo_xlib_surface_create (x11context.display, wm_icon_, x11context.visual, iconpixmap.width(), iconpixmap.height());
      CHECK_CAIRO_STATUS (xsurface);
      cairo_t *xcr = cairo_create (xsurface);
      CHECK_CAIRO_STATUS (xcr);
      cairo_surface_t *isurface = cairo_surface_from_pixmap (iconpixmap);
      CHECK_CAIRO_STATUS (isurface);
      cairo_set_source_surface (xcr, isurface, 0, 0);
      cairo_set_operator (xcr, CAIRO_OPERATOR_OVER);
      cairo_paint (xcr);
      cairo_destroy (xcr);
      cairo_surface_destroy (isurface);
      cairo_surface_destroy (xsurface);
    }
  // WM_COMMAND WM_CLIENT_MACHINE WM_LOCALE_NAME WM_HINTS WM_CLASS
  XWMHints wmhints = { InputHint | StateHint | IconPixmapHint, False, NormalState, wm_icon_, 0, 0, 0, 0, 0, };
  if (f & ATTENTION)
    wmhints.flags |= XUrgencyHint;
  if (f & (HIDDEN | ICONIFY))
    wmhints.initial_state = IconicState;
  const char *cmdv[2] = { program_file().c_str(), NULL };
  XClassHint class_hint = { const_cast<char*> (program_alias().c_str()), const_cast<char*> (program_ident().c_str()) };
  Xutf8SetWMProperties (x11context.display, window_, NULL, NULL, const_cast<char**> (cmdv), ARRAY_SIZE (cmdv) - 1,
                        NULL, &wmhints, &class_hint);
  // _MOTIF_WM_HINTS
  uint32 funcs = FUNC_RESIZE | FUNC_MOVE, deco = 0;
  if (f & DECORATED)    { deco |= DECOR_BORDER | DECOR_RESIZEH | DECOR_TITLE | DECOR_MENU; }
  if (f & MINIMIZABLE)  { funcs |= FUNC_MINIMIZE; deco |= DECOR_MINIMIZE; }
  if (f & MAXIMIZABLE)  { funcs |= FUNC_MAXIMIZE; deco |= DECOR_MAXIMIZE; }
  if (f & DELETABLE)    { funcs |= FUNC_CLOSE; deco |= DECOR_CLOSE; }
  adjust_mwm_hints (x11context.display, window_, Mwm (funcs), Mwm (deco));
  // WM_WINDOW_ROLE
  set_text_property (x11context.display, window_, x11context.atom ("WM_WINDOW_ROLE"),
                     XStringStyle, setup.session_role, DELETE_EMPTY);
  // _NET_WM_PID
  longs.clear();
  longs.push_back (getpid());
  if (false)
    XChangeProperty (x11context.display, window_, x11context.atom ("_NET_WM_PID"),
                     XA_CARDINAL, 32, PropModeReplace, (uint8*) longs.data(), longs.size());
  else // _NET_WM_PID is used for killing, we prefer an XKillClient() call however
    XDeleteProperty (x11context.display, window_, x11context.atom ("_NET_WM_PID"));
}

void
ScreenWindowX11::configure_window (const Config &config, bool sizeevent)
{
  // WM_NORMAL_HINTS, size & gravity
  XSizeHints szhint = { PWinGravity, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0 }, { 0, 0 }, 0, 0, StaticGravity };
  XSetWMNormalHints (x11context.display, window_, &szhint);
  // title
  if (config.title != config_.title)
    {
      set_text_property (x11context.display, window_, x11context.atom ("WM_NAME"), XStdICCTextStyle, config.title);      // ICCCM
      set_text_property (x11context.display, window_, x11context.atom ("_NET_WM_NAME"), XUTF8StringStyle, config.title); // EWMH
      config_.title = config.title;
      state_.visible_title = config_.title;   // compensate for WMs not supporting _NET_WM_VISIBLE_NAME
    }
  // iconified title
  if (config.alias != config_.alias)
    {
      set_text_property (x11context.display, window_, x11context.atom ("WM_ICON_NAME"), XStdICCTextStyle, config.alias, DELETE_EMPTY);
      set_text_property (x11context.display, window_, x11context.atom ("_NET_WM_ICON_NAME"), XUTF8StringStyle, config.alias, DELETE_EMPTY);
      config_.alias = config.alias;
      state_.visible_alias = config_.alias;   // compensate for WMs not supporting _NET_WM_VISIBLE_ICON_NAME
    }
  force_update (window_);
  if (sizeevent)
    enqueue_event (create_event_win_size (event_context_, state_.width, state_.height, pending_configures_ > 0));
}

static const char *const mime_target_atoms[] = {           // determine X11 selection property types from mime
  "text/plain", "UTF8_STRING",
  "text/plain", "COMPOUND_TEXT",
  "text/plain", "TEXT",
  "text/plain", "STRING",
};

String
X11Context::target_atom_to_mime (const Atom target_atom)
{
  // X11 atom lookup
  for (size_t i = 0; i + 1 < ARRAY_SIZE (mime_target_atoms); i += 2)
    if (target_atom == atom (mime_target_atoms[i+1]))
      return mime_target_atoms[i];
  // direct mime type plausible?
  const String target = atom (target_atom);
  if (target.size() && target[0] >= 'a' && target[0] <= 'z' && target.find ('/') != target.npos && target.find ('/') == target.rfind ('/'))
    return target;      // starts with lower case alpha and has one slash, possibly mime type
  return "";
}

Atom
X11Context::mime_to_target_atom (const String &mime, Atom last_failed)
{
  for (size_t i = 0; i + 1 < ARRAY_SIZE (mime_target_atoms); i += 2)
    if (last_failed)                                    // first find last failing request type
      {
        if (last_failed == atom (mime_target_atoms[i+1]))
          last_failed = None;                           // skipped all types until last request
        continue;
      }
    else if (mime == mime_target_atoms[i])
      return atom (mime_target_atoms[i+1]);
  return None;
}

void
X11Context::text_plain_to_target_atoms (vector<Atom> &targets)
{
  const String text_plain = "text/plain";
  for (size_t i = 0; i + 1 < ARRAY_SIZE (mime_target_atoms); i += 2)
    if (text_plain == mime_target_atoms[i])
      targets.push_back (atom (mime_target_atoms[i+1]));
}

bool
ScreenWindowX11::send_selection_notify (Window req_window, Atom selection, Atom target, Atom req_property, Time req_time)
{
  XEvent xevent = { 0, };
  XSelectionEvent &notify = xevent.xselection;
  notify.type = SelectionNotify;
  notify.serial = 0;
  notify.send_event = True;
  notify.display = NULL;
  notify.requestor = req_window;
  notify.selection = selection;
  notify.target = target;
  notify.property = req_property;
  notify.time = req_time;
  // send notify, guard against foreign window deletion
  XErrorEvent dummy = { 0, };
  x11_trap_errors (&dummy);
  Status xstatus = XSendEvent (x11context.display, notify.requestor, False, NoEventMask, &xevent);
  XSync (x11context.display, False);
  x11_untrap_errors();
  return xstatus != 0; // success
}

void
ScreenWindowX11::handle_content_request (const size_t nth)
{
  assert_return (nth < content_requests_.size());
  ContentRequest &cr = content_requests_[nth];
  assert_return (cr.data_provided == true);
  const XSelectionRequestEvent &xev = cr.xsr;
  const Atom requestor_property = xev.property ? xev.property : xev.target; // ICCCM convention
  if (xev.target == x11context.atom ("TIMESTAMP"))
    {
      vector<unsigned long> ints;
      ints.push_back (primary_->time);
      if (save_set_property (x11context.display, xev.requestor, requestor_property, x11context.atom ("INTEGER"),
                             32, ints.data(), ints.size()))
        send_selection_notify (xev.requestor, xev.selection, xev.target, requestor_property, xev.time);
    }
  else if (xev.target == x11context.atom ("MULTIPLE")) // FIXME
    {
      send_selection_notify (xev.requestor, xev.selection, xev.target, None, xev.time); // reject
    }
  else if (xev.target == x11context.atom ("TARGETS")) // FIXME
    {
      send_selection_notify (xev.requestor, xev.selection, xev.target, None, xev.time); // reject
    }
  else if (strncmp (&cr.data_type[0], "text/", 5) == 0)
    {
      XICCEncodingStyle ecstyle = XUTF8StringStyle;
      if      (xev.target == x11context.atom ("UTF8_STRING"))
        ecstyle = XUTF8StringStyle;
      else if (xev.target == x11context.atom ("TEXT"))
        ecstyle = XCompoundTextStyle;
      else if (xev.target == x11context.atom ("COMPOUND_TEXT"))
        ecstyle = XCompoundTextStyle;
      else if (xev.target == x11context.atom ("STRING"))
        ecstyle = XStringStyle;
      Atom ptype;
      int pformat;
      vector<uint8> chars;
      bool transferred = x11_convert_string_for_text_property (x11context.display, ecstyle, cr.data, &chars, &ptype, &pformat);
      transferred = transferred && pformat == 8 &&
                    save_set_property (x11context.display, xev.requestor, requestor_property, ptype,
                                       8, chars.data(), chars.size());
      send_selection_notify (xev.requestor, xev.selection, xev.target, transferred ? requestor_property : None, xev.time);
    }
  else
    {
      const Atom target = x11context.mime_to_target_atom (cr.data_type);
      const bool transferred = !target ? false :
                               save_set_property (x11context.display, xev.requestor, requestor_property,
                                                  target, 8, cr.data.data(), cr.data.size());
      send_selection_notify (xev.requestor, xev.selection, xev.target, transferred ? requestor_property : None, xev.time);
    }
  content_requests_.erase (content_requests_.begin() + nth);
}

void
ScreenWindowX11::request_selection (Atom source, uint64 nonce, String data_type, Atom last_failed)
{
  if (!isel_)
    {
      SelectionInput *tsel = new SelectionInput();
      tsel->source = source;
      tsel->content_type = data_type;
      tsel->target = x11context.mime_to_target_atom (tsel->content_type, last_failed);
      tsel->owner = XGetSelectionOwner (x11context.display, tsel->source);
      if (tsel->owner != None && tsel->target != None)
        {
          tsel->nonce = nonce;
          tsel->time = event_context_.time;
          tsel->slot = x11context.atom ("RAPICORN_SELECTION");
          XDeleteProperty (x11context.display, window_, tsel->slot);
          XConvertSelection (x11context.display, tsel->source, tsel->target, tsel->slot, window_, tsel->time);
          XDEBUG ("XConvertSelection: [%lx] %s(%s) -> %ld(%s); owner=%ld", tsel->time,
                  x11context.atom (tsel->source), x11context.atom (tsel->target),
                  window_, x11context.atom (tsel->slot),
                  tsel->owner);
          tsel->state = WAIT_FOR_NOTIFY;
          isel_ = tsel;
          return; // successfully initiated transfer
        }
      delete tsel;
    }
  // request rejected
  enqueue_event (create_event_data (CONTENT_DATA, event_context_, "", "", nonce));
}

void
ScreenWindowX11::handle_command (ScreenCommand *command)
{
  switch (command->type)
    {
      bool found;
    case ScreenCommand::CREATE: case ScreenCommand::OK: case ScreenCommand::ERROR: case ScreenCommand::SHUTDOWN:
      assert_unreached();
    case ScreenCommand::CONFIGURE:
      configure_window (*command->dconfig, command->dresize);
      break;
    case ScreenCommand::BEEP:
      XBell (x11context.display, 0);
      break;
    case ScreenCommand::SHOW:
      XMapWindow (x11context.display, window_);
      break;
    case ScreenCommand::BLIT:
      blit (command->surface, *command->region);
      break;
    case ScreenCommand::CONTENT:
      switch (command->source)
        {
        case 2:
          XSetSelectionOwner (x11context.display, XA_PRIMARY, command->data_types->size() > 0 ? window_ : None, event_context_.time);
          if (window_ == XGetSelectionOwner (x11context.display, XA_PRIMARY))
            {
              if (!primary_)
                primary_ = new ContentOffer();
              primary_->content_types = *command->data_types;
              primary_->time = event_context_.time;
            }
          else
            {
              if (primary_)
                delete primary_;
              primary_ = NULL;
            }
          break;
        case 3:
          assert_return (command->data_types->size() == 1);
          request_selection (XA_PRIMARY, command->nonce, (*command->data_types)[0]);
          break;
        case 4:
          XSetSelectionOwner (x11context.display, x11context.atom ("CLIPBOARD"), command->data_types->size() > 0 ? window_ : None, event_context_.time);
          // FIXME: implement clipboard copies
          break;
        case 5:
          assert_return (command->data_types->size() == 1);
          request_selection (x11context.atom ("CLIPBOARD"), command->nonce, (*command->data_types)[0]);
          break;
        }
      break;
    case ScreenCommand::PROVIDE: {
      assert_return (command->source == 2);
      assert_return (command->data_types->size() == 2);
      found = false;
      for (auto &cr : content_requests_)
        if (cr.nonce == command->nonce && !cr.data_provided)
          {
            cr.data_provided = true;
            cr.data_type = (*command->data_types)[0];
            cr.data = (*command->data_types)[1];
            handle_content_request (&cr - &content_requests_[0]);
            found = true;
            break;
          }
      if (!found)
        RAPICORN_CRITICAL ("content provided for unknown nonce: %u (data_type=%s data_length=%u)",
                           command->nonce, (*command->data_types)[0], (*command->data_types)[1].size());
      break; }
    case ScreenCommand::PRESENT:   break;  // FIXME
    case ScreenCommand::UMOVE:     break;  // FIXME
    case ScreenCommand::URESIZE:   break;  // FIXME
    case ScreenCommand::DESTROY:
      destroy_x11_resources();
      delete this;
      break;
    }
  delete command;
}

// == X11Context ==
X11Context::X11Context (ScreenDriver &driver, AsyncNotifyingQueue<ScreenCommand*> &command_queue,
                        AsyncBlockingQueue<ScreenCommand*> &reply_queue) :
  loop_ (*ref_sink (MainLoop::_new())), command_queue_ (command_queue), reply_queue_ (reply_queue),
  screen_driver (driver), display (NULL), screen (0), visual (NULL), depth (0),
  root_window (0), input_method (NULL), shared_mem_ (-1)
{
  XDEBUG ("%s: X11Context started", STRFUNC);
  do_once {
    const bool x11_initialized_for_threads = XInitThreads ();
    assert (x11_initialized_for_threads == true);
    xlib_error_handler = XSetErrorHandler (x11_error);
  }
}

X11Context::~X11Context ()
{
  assert_return (!display);
  assert_return (x11ids_.empty());
  assert_return (command_queue_.pending() == false);
  loop_.kill_sources();
  unref (&loop_);
  XDEBUG ("%s: X11Context stopped", STRFUNC);
}

bool
X11Context::connect()
{
  assert_return (!display && !screen && !visual && !depth && !root_window && !input_method && shared_mem_ < 0, false);
  const char *edsp = getenv ("DISPLAY");
  display = XOpenDisplay (edsp && edsp[0] ? edsp : NULL);
  XDEBUG ("XOpenDisplay(%s): %s", CQUOTE (edsp ? edsp : ""), display ? "success" : "failed to connect");
  if (!display)
    return false;
  XSynchronize (display, dbe_x11sync);
  screen = DefaultScreen (display);
  visual = DefaultVisual (display, screen);
  depth = DefaultDepth (display, screen);
  root_window = XRootWindow (display, screen);
  load_atom_cache (display);
  String imerr = x11_input_method (display, &input_method, &input_style);
  if (!imerr.empty() && x11_input_method (display, &input_method, &input_style, "@im=none") == "")
    imerr = ""; // @im=none gets us at least a working compose key
  XDEBUG ("XIM: %s", !imerr.empty() ? imerr.c_str() : string_format ("selected input method style 0x%04x", input_style).c_str());
  return true;
}

bool
X11Context::cmd_dispatcher (const EventLoop::State &state)
{
  if (state.phase == state.PREPARE || state.phase == state.CHECK)
    return command_queue_.pending();
  else if (state.phase == state.DISPATCH)
    {
      for (ScreenCommand *cmd = command_queue_.pop(); cmd; cmd = command_queue_.pop())
        switch (cmd->type)
          {
            ScreenWindowX11 *screen_window;
          case ScreenCommand::CREATE:
            screen_window = new ScreenWindowX11 (*this);
            screen_window->create_window (*cmd->setup, *cmd->config);
            delete cmd;
            reply_queue_.push (new ScreenCommand (ScreenCommand::OK, screen_window));
            break;
          case ScreenCommand::SHUTDOWN:
            loop_.quit();
            delete cmd;
            reply_queue_.push (new ScreenCommand (ScreenCommand::OK, NULL));
            assert_return (x11ids_.empty(), true);
            break;
          default:
            screen_window = dynamic_cast<ScreenWindowX11*> (cmd->screen_window);
            if (cmd->screen_window && screen_window)
              screen_window->handle_command (cmd);
            else
              {
                critical ("ScreenCommand without ScreenWindowX11: %p (type=%d)", cmd, cmd->type);
                delete cmd;
              }
            break;
          }
      return true; // keep alive
    }
  else if (state.phase == state.DESTROY)
    ;
  return false;
}

void
X11Context::run()
{
  // perform unlock/lock around poll() calls
  // loop_.set_lock_hooks ([] () { return true; }, [&x11locker] () { x11locker.lock(); }, [&x11locker] () { x11locker.unlock(); });
  // ensure X11 file descriptor changes are handled
  loop_.exec_io_handler (Aida::slot (*this, &X11Context::x11_io_handler), ConnectionNumber (display), "r", EventLoop::PRIORITY_NORMAL);
  // ensure queued X11 events are processed (i.e. ones already read from fd)
  loop_.exec_dispatcher (Aida::slot (*this, &X11Context::x11_dispatcher), EventLoop::PRIORITY_NORMAL);
  // ensure enqueued user commands are processed
  loop_.exec_dispatcher (Aida::slot (*this, &X11Context::cmd_dispatcher), EventLoop::PRIORITY_NOW);
  // ensure command_queue events are processed
  command_queue_.notifier ([&]() { loop_.wakeup(); });
  // process X11 events
  loop_.run();
  // prevent wakeups on stale objects
  command_queue_.notifier (NULL);
  // close down
  if (!x11ids_.empty()) // ScreenWindowX11s unconditionally reference X11Context
    fatal ("%s: stopped handling X11Context with %d active windows", STRFUNC, x11ids_.size());
  if (input_method)
    {
      XCloseIM (input_method);
      input_method = NULL;
      input_style = 0;
    }
  if (display)
    {
      XCloseDisplay (display); // XLib hangs if connection fd is closed prematurely
      display = NULL;
    }
  // remove sources and close Pfd file descriptor
  loop_.kill_sources();
}

bool
X11Context::x11_dispatcher (const EventLoop::State &state)
{
  if (state.phase == state.PREPARE || state.phase == state.CHECK)
    return XPending (display);
  else if (state.phase == state.DISPATCH)
    {
      process_x11();
      return true;
    }
  else if (state.phase == state.DESTROY)
    ;
  return false;
}

bool
X11Context::x11_io_handler (PollFD &pfd)
{
  process_x11();
  return true;
}

void
X11Context::process_x11()
{
  if (XPending (display))
    {
      XEvent xevent = { 0, };
      XNextEvent (display, &xevent);    // blocks if !XPending
      XEvent evcopy = xevent;
      // allow event handling hooks, e.g. needed by XIM
      bool consumed = XFilterEvent (&evcopy, None);
      consumed = consumed || filter_event (evcopy);
      // deliver to owning ScreenWindow
      X11Widget *xwidget = x11id_get (xevent.xany.window);
      ScreenWindowX11 *scw = !xwidget ? NULL : dynamic_cast<ScreenWindowX11*> (xwidget);
      if (scw && consumed)
        scw->filtered_event (xevent);           // preserve bookkeeping of ScreenWindows
      else if (scw)
        consumed = scw->process_event (xevent); // ScreenWindow event handling
      if (!consumed)
        {
          const char ss = xevent.xany.send_event ? 'S' : 's';
          XDEBUG ("Ev: %c=%u w=%u event_type=%d", ss, xevent.xany.serial, xevent.xany.window, xevent.type);
        }
    }
}

void
X11Context::process_updates ()
{
  vector<size_t> xids;
  xids.swap (queued_updates_);
  std::stable_sort (xids.begin(), xids.end());
  auto it = std::unique (xids.begin(), xids.end());
  xids.resize (it - xids.begin());
  for (auto id : xids)
    {
      X11Widget *xwidget = x11id_get (id);
      ScreenWindowX11 *scw = dynamic_cast<ScreenWindowX11*> (xwidget);
      if (scw)
        scw->force_update (id);
    }
}

bool
X11Context::filter_event (const XEvent &xevent)
{
  const char ss = xevent.xany.send_event ? 'S' : 's';
  bool filtered = false;
  switch (xevent.type)
    {
    case MappingNotify: { // only sent to X11 clients that previously queried key maps
      const XMappingEvent &xev = xevent.xmapping;
      XRefreshKeyboardMapping (const_cast<XMappingEvent*> (&xevent.xmapping));
      EDEBUG ("KyMap: %c=%u win=%u req=Mapping%s first_keycode=%d count=%d", ss, xev.serial, xev.window,
              xev.request == MappingPointer ? "Pointer" :
              (xev.request == MappingKeyboard ? "Keyboard" :
               (xev.request == MappingModifier ? "Modifier" : "Unknown")),
              xev.first_keycode, xev.count);
      filtered = true;
      break; }
    }
  return filtered;
}

void
X11Context::x11id_set (size_t xid, X11Widget *x11widget)
{
  auto it = x11ids_.find (xid);
  if (!x11widget)
    {
      if (x11ids_.end() != it)
        x11ids_.erase (it);
      return;
    }
  if (x11ids_.end() != it)
    fatal ("%s: x11id=%d already in use for %p while setting to %p", STRFUNC, xid, it->second, x11widget);
  x11ids_[xid] = x11widget;
}

X11Widget*
X11Context::x11id_get (size_t xid)
{
  // do lookup without modifying x11ids
  auto it = x11ids_.find (xid);
  return x11ids_.end() != it ? it->second : NULL;
}

Atom
X11Context::atom (const String &text, bool force_create)
{
  // XLib caches atoms well
  return XInternAtom (display, text.c_str(), !force_create);
}

String
X11Context::atom (Atom atom) const
{
  if (atom == None)
    return "<!--None-->";
  // XLib caches atoms well
  char *res = XGetAtomName (display, atom);
  String result (res ? res : "");
  if (res)
    XFree (res);
  return result;
}

bool
X11Context::local_x11()
{
  if (shared_mem_ < 0)
    shared_mem_ = x11_check_shared_image (display, visual, depth);
  XDEBUG ("XShmCreateImage: %s", shared_mem_ ? "ok" : "failed to attach");
  return shared_mem_;
}

void
X11Context::queue_update (size_t xid)
{
  const bool need_handler = queued_updates_.empty();
  queued_updates_.push_back (xid);
  if (need_handler)
    loop_.exec_timer (50, Aida::slot (*this, &X11Context::process_updates));
}

} // Rapicorn
