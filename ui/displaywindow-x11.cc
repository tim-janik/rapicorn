// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "displaywindow.hh"
#include "uithread.hh"
#include <cairo/cairo-xlib.h>
#include <algorithm>
#include <unistd.h>

#define EDEBUG(...)     RAPICORN_KEY_DEBUG ("XEvents", __VA_ARGS__)
#define VDEBUG(...)     RAPICORN_KEY_DEBUG ("XEvent2", __VA_ARGS__) // verbose event debugging
static auto dbe_x11sync = RAPICORN_DEBUG_OPTION ("x11sync", "Synchronize X11 operations for correct error attribution.");

typedef ::Pixmap XPixmap; // avoid conflicts with Rapicorn::Pixmap

#include "displaywindow-xaux.cc" // helpers, need dbe_x11sync

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

template<class Container, class PtrPredicate> typename Container::value_type*
find_element (Container &container, PtrPredicate f)
{
  auto it = std::find_if (container.begin(), container.end(), f);
  return it == container.end() ? NULL : &*it;
}

// == X11Widget ==
struct X11Widget {
  explicit     X11Widget() {}
  virtual     ~X11Widget() {}
  virtual bool timer    (const LoopState &state, int64 *timeout_usecs_p) = 0;
};

// == EventStake ==
struct EventStake {
  Window        window;
  unsigned long mask;
  uint          ref_count;
  bool          destroyed;
  EventStake() : window (0), mask (0), ref_count (0), destroyed (false) {}
};

// == IncrTransfer ==
struct IncrTransfer {
  Window       window;
  Atom         property;
  Atom         type;
  size_t       byte_width;
  size_t       offset;
  vector<char> bytes;
  uint64       timeout;
  IncrTransfer() : window (0), property (0), type (0), byte_width (0), offset (0), timeout (0) {}
};
#define INCR_TRANSFER_TIMEOUT   (25 * 1000 * 1000)

// == X11Context ==
class X11Context {
  MainLoopP                            loop_;
  vector<size_t>                       queued_updates_; // XIDs
  map<size_t, X11Widget*>              x11ids_;
  AsyncNotifyingQueue<DisplayCommand*> &command_queue_;
  AsyncBlockingQueue<DisplayCommand*>  &reply_queue_;
  vector<EventStake>                   event_stakes_;
  vector<IncrTransfer>                 incr_transfers_;
  int8                                 shared_mem_;
  bool                  x11_timer               (const LoopState &state);
  bool                  x11_dispatcher          (const LoopState &state);
  bool                  x11_io_handler          (PollFD &pfd);
  void                  process_x11             ();
  bool                  filter_event            (const XEvent&);
  void                  process_updates         ();
  bool                  cmd_dispatcher          (const LoopState &state);
  EventStake*           find_event_stake        (Window window, bool create);
  void                  continue_incr           (Window window, Atom property);
public:
  DisplayDriver         &display_driver;
  Display              *display;
  Visual               *visual;
  size_t                max_request_bytes;
  size_t                max_property_bytes;
  int                   screen;
  int                   depth;
  Window                root_window;
  XIM                   input_method;
  XIMStyle              input_style;
  X11Widget*            x11id_get   (size_t xid);
  void                  x11id_set   (size_t xid, X11Widget *x11widget);
  Atom                  atom         (const String &text, bool force_create = true);
  String                atom         (Atom atom) const;
  bool                  local_x11    ();
  void                  queue_update (size_t xid);
  void                  run        ();
  bool                  connect    ();
  void                  ref_events                      (Window window, unsigned long event_mask, const XSetWindowAttributes *attrs = NULL);
  void                  unref_events                    (Window window);
  void                  window_destroyed                (Window window);
  bool                  transfer_incr_property          (Window window, Atom property, Atom type, int element_bits, const void *elements, size_t nelements);
  String                target_atom_to_mime             (const Atom target_atom);
  Atom                  mime_to_target_atom             (const String &mime, Atom last_failed = None);
  void                  mime_list_target_atoms          (const String &mime, vector<Atom> &atoms);
  explicit              X11Context (DisplayDriver &driver, AsyncNotifyingQueue<DisplayCommand*> &command_queue, AsyncBlockingQueue<DisplayCommand*> &reply_queue);
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

static DisplayDriverFactory<X11Context> display_driver_x11 ("X11Window", -1);

// == ContentOffer ==
struct ContentOffer {           // offers_
  StringVector  content_types;  // selection mime types
  Time          time;           // selection time
  Atom          selection;      // e.g. XA_PRIMARY
  uint64        nonce;
  ContentOffer() : time (0), selection (0), nonce (0) {}
};

// == ContentRequest ==
struct ContentRequest {
  uint64                 request_id;
  XSelectionRequestEvent xsr;
  String                 data_type;
  String                 data;
  bool                   data_provided;
  ContentRequest () : request_id (0), xsr ({ 0, }), data_provided (false) {}
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
  ContentSourceType     content_source;
  Atom                  source_atom;    // requested selection
  uint64                nonce;
  String                content_type;   // requested type
  Atom                  target;         // X11 Atom for requested type
  Window                owner;
  Time                  time;
  int                   size_est;       // lower bound provided by INCR response
  RawData               raw;
  uint64                timeout;
  SelectionInput() : state (WAIT_INVALID), slot (0), content_source (ContentSourceType (0)), source_atom (0),
                     nonce (0), target (0), owner (0), time (0), size_est (0), timeout (0) {}
};
#define SELECTION_INPUT_TIMEOUT   (15 * 1000 * 1000)

// == DisplayWindowX11 ==
struct DisplayWindowX11 : public virtual DisplayWindow, public virtual X11Widget {
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
  vector<ContentOffer>  offers_;
  vector<ContentRequest> content_requests_;
  explicit              DisplayWindowX11        (X11Context &_x11context);
  virtual              ~DisplayWindowX11        ();
  virtual bool          timer                   (const LoopState &state, int64 *timeout_usecs_p);
  void                  destroy_x11_resources   ();
  void                  handle_command          (DisplayCommand *command);
  void                  setup_window            (const DisplayWindow::Setup &setup);
  void                  create_window           (const DisplayWindow::Setup &setup, const DisplayWindow::Config &config);
  void                  configure_window        (const Config &config, bool sizeevent);
  void                  blit                    (cairo_surface_t *surface, const Rapicorn::Region &region);
  void                  filtered_event          (const XEvent &xevent);
  bool                  process_event           (const XEvent &xevent);
  bool                  send_selection_notify   (Window req_window, Atom selection, Atom target, Atom req_property, Time req_time);
  void                  request_selection       (ContentSourceType content_source, Atom source, uint64 nonce, String data_type, Atom last_failed = None);
  void                  receive_selection       (const XEvent &xev);
  void                  handle_content_request  (size_t nth, ContentOffer *offer);
  void                  client_message          (const XClientMessageEvent &xevent);
  void                  blit_expose_region      ();
  void                  force_update            (Window window);
  virtual DisplayDriver& display_driver_async     () const { return x11context.display_driver; } // executed from arbitrary threads
};

DisplayWindowX11::DisplayWindowX11 (X11Context &_x11context) :
  x11context (_x11context),
  window_ (None), input_context_ (NULL), wm_icon_ (None), expose_surface_ (NULL),
  last_motion_time_ (0), pending_configures_ (0), pending_exposes_ (0),
  override_redirect_ (false), crossing_focus_ (false), isel_ (NULL)
{}

DisplayWindowX11::~DisplayWindowX11()
{
  if (expose_surface_ || wm_icon_ || input_context_ || window_)
    {
      critical ("%s: stale X11 resource during deletion: ex=%p ic=0x%x im=%p w=0x%x", __func__, expose_surface_, wm_icon_, input_context_, window_);
      destroy_x11_resources(); // shouldn't happen, this potentially issues X11 calls from outside the X11 thread
    }
  if (isel_)
    delete isel_;
}

void
DisplayWindowX11::destroy_x11_resources()
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
      x11context.window_destroyed (window_);
      x11context.unref_events (window_);
      window_ = 0;
    }
}

bool
DisplayWindowX11::timer (const LoopState &state, int64 *timeout_usecs_p)
{
  const uint64 now = state.current_time_usecs;
  if (state.phase == state.PREPARE || state.phase == state.CHECK)
    {
      if (isel_ && isel_->timeout <= now)
        return true;
      return false;
    }
  else if (state.phase == state.DISPATCH)
    {
      if (isel_ && isel_->timeout <= now)
        {
          // request aborted
          if (isel_->state != WAIT_INVALID)
            enqueue_event (create_event_data (CONTENT_DATA, event_context_, isel_->content_source, isel_->nonce, "", ""));
          delete isel_;
          isel_ = NULL;
        }
      return true;
    }
  else if (state.phase == state.DESTROY)
    ;
  return false;
}

void
DisplayWindowX11::create_window (const DisplayWindow::Setup &setup, const DisplayWindow::Config &config)
{
  assert_return (!window_ && !expose_surface_);
  state_.window_type = setup.window_type;
  update_state (state_);
  override_redirect_ = (setup.window_type == WindowType::DESKTOP ||
                         setup.window_type == WindowType::DROPDOWN_MENU ||
                         setup.window_type == WindowType::POPUP_MENU ||
                         setup.window_type == WindowType::TOOLTIP ||
                         setup.window_type == WindowType::NOTIFICATION ||
                         setup.window_type == WindowType::COMBO ||
                         setup.window_type == WindowType::DND);
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
  // ensure to keep this window's event mask around
  x11context.ref_events (window_, attributes.event_mask, &attributes);
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
DisplayWindowX11::filtered_event (const XEvent &xevent)
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
DisplayWindowX11::process_event (const XEvent &xevent)
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
      ContentOffer *offer = find_element (offers_, [&xev] (const ContentOffer &o) { return o.selection == xev.selection; });
      if (offer && xev.target && (xev.time == CurrentTime || time_cmp (xev.time, offer->time) >= 0))
        {
          ContentRequest cr;
          cr.request_id = random_nonce();
          cr.xsr = xev;
          content_requests_.push_back (cr);
          if (xev.target == x11context.atom ("TIMESTAMP") || xev.target == x11context.atom ("TARGETS"))
            {
              // we take a shortcut here, b/c no actual content is required
              const size_t nth = content_requests_.size() - 1;
              content_requests_[nth].data_provided = true;
              handle_content_request (nth, offer);
            }
          else
            {
              ContentSourceType source = offer->selection == XA_PRIMARY ? CONTENT_SOURCE_SELECTION :
                                         offer->selection == x11context.atom ("CLIPBOARD") ? CONTENT_SOURCE_CLIPBOARD :
                                         ContentSourceType (0);
              const String mime_type = x11context.target_atom_to_mime (xev.target);
              enqueue_event (create_event_data (CONTENT_REQUEST, event_context_, source, offer->nonce, mime_type, "", cr.request_id));
            }
        }
      else
        send_selection_notify (xev.requestor, xev.selection, xev.target, None, xev.time); // reject
      consumed = true;
      break; }
    case SelectionClear: {
      const XSelectionClearEvent &xev = xevent.xselectionclear;
      EDEBUG ("SelCl: %c=%u [%lx] own=%u %s", ss, xev.serial, xev.time, xev.window, x11context.atom (xev.selection));
      ContentOffer *offer = find_element (offers_, [&xev] (const ContentOffer &o) { return o.selection == xev.selection; });
      if (offer && (xev.time == CurrentTime || time_cmp (xev.time, offer->time) >= 0))
        {
          ContentSourceType source = offer->selection == XA_PRIMARY ? CONTENT_SOURCE_SELECTION :
                                     offer->selection == x11context.atom ("CLIPBOARD") ? CONTENT_SOURCE_CLIPBOARD :
                                     ContentSourceType (0);
          enqueue_event (create_event_data (CONTENT_CLEAR, event_context_, source, offer->nonce, "", ""));
          offers_.erase (offers_.begin() + (offer - &offers_[0]));
        }
      consumed = true;
      break; }
    case Expose: {
      const XExposeEvent &xev = xevent.xexpose;
      std::vector<DRect> rectangles;
      expose_region_.add (DRect (Point (xev.x, xev.y), xev.width, xev.height));
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
DisplayWindowX11::receive_selection (const XEvent &xevent)
{
  return_unless (isel_ != NULL);
  // SelectionNotify (after XConvertSelection)
  if (xevent.type == SelectionNotify && isel_->state == WAIT_FOR_NOTIFY)
    {
      const XSelectionEvent &xev = xevent.xselection;
      if (window_ == xev.requestor && isel_->source_atom == xev.selection && isel_->time == xev.time)
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
              request_selection (tsel->content_source, tsel->source_atom, tsel->nonce, tsel->content_type, tsel->target);
              delete tsel;
              return;
            }
          isel_->timeout = timestamp_realtime() + SELECTION_INPUT_TIMEOUT; // refresh timeout to keep safe from cleanup
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
          isel_->timeout = timestamp_realtime() + SELECTION_INPUT_TIMEOUT; // refresh timeout to keep safe from cleanup
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
      enqueue_event (create_event_data (CONTENT_DATA, event_context_, isel_->content_source, isel_->nonce, content_type, content_data));
      delete isel_;
      isel_ = NULL;
    }
}

static bool prevent_startup_focus = RAPICORN_FLIPPER ("prevent-startup-focus",
                                                      "Avoid grabbing the X11 input focus immediately after application"
                                                      "startup.");

void
DisplayWindowX11::client_message (const XClientMessageEvent &xev)
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
      if (prevent_startup_focus && timestamp_realtime() - timestamp_startup() < 1000000)
        ; // focus stealing prevention within 1 second after startup
      else
        XSetInputFocus (x11context.display, window_, RevertToPointerRoot, xev.data.l[1]);
      XSync (x11context.display, False);
      x11_untrap_errors();
    }
  else if (mtype == x11context.atom ("_NET_WM_PING"))
    {
      XEvent xevent = *(const XEvent*) &xev;
      xevent.xclient.data.l[3] = xevent.xclient.data.l[4] = 0; // [0]=_PING, [1]=time, [2]=window_
      xevent.xclient.window = x11context.root_window;
      XSendEvent (x11context.display, xevent.xclient.window, False, SubstructureNotifyMask | SubstructureRedirectMask, &xevent);
    }
}

void
DisplayWindowX11::force_update (Window window)
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

static DRect
cairo_image_surface_coverage (cairo_surface_t *surface)
{
  int w = cairo_image_surface_get_width (surface);
  int h = cairo_image_surface_get_height (surface);
  double x_offset = 0, y_offset = 0;
  cairo_surface_get_device_offset (surface, &x_offset, &y_offset);
  return DRect (Point (x_offset, y_offset), w, h);
}

void
DisplayWindowX11::blit (cairo_surface_t *surface, const Rapicorn::Region &region)
{
  CHECK_CAIRO_STATUS (surface);
  if (!window_)
    return;
  const DRect fullwindow = DRect (0, 0, state_.width, state_.height);
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
      vector<DRect> rects;
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
DisplayWindowX11::blit_expose_region()
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
  vector<DRect> rects;
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
      const DRect extents = expose_region_.extents();
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
    case WindowType::DESKTOP:   	return "_NET_WM_WINDOW_TYPE_DESKTOP";
    case WindowType::DOCK:      	return "_NET_WM_WINDOW_TYPE_DOCK";
    case WindowType::TOOLBAR:   	return "_NET_WM_WINDOW_TYPE_TOOLBAR";
    case WindowType::MENU:      	return "_NET_WM_WINDOW_TYPE_MENU";
    case WindowType::UTILITY:   	return "_NET_WM_WINDOW_TYPE_UTILITY";
    case WindowType::SPLASH:    	return "_NET_WM_WINDOW_TYPE_SPLASH";
    case WindowType::DIALOG:    	return "_NET_WM_WINDOW_TYPE_DIALOG";
    case WindowType::DROPDOWN_MENU:	return "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU";
    case WindowType::POPUP_MENU:	return "_NET_WM_WINDOW_TYPE_POPUP_MENU";
    case WindowType::TOOLTIP:	        return "_NET_WM_WINDOW_TYPE_TOOLTIP";
    case WindowType::NOTIFICATION:	return "_NET_WM_WINDOW_TYPE_NOTIFICATION";
    case WindowType::COMBO:	        return "_NET_WM_WINDOW_TYPE_COMBO";
    case WindowType::DND:       	return "_NET_WM_WINDOW_TYPE_DND";
    default: ;
    case WindowType::NORMAL:	        return "_NET_WM_WINDOW_TYPE_NORMAL";
    }
}

void
DisplayWindowX11::setup_window (const DisplayWindow::Setup &setup)
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
      Rapicorn::Pixmap iconpixmap ("@res Rapicorn/icons/wm-gears.png");
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
  const char *cmdv[2] = { program_argv0().c_str(), NULL };
  const String resname = program_alias(), resclass = program_name(); // keep strings alive until after Xutf8SetWMProperties
  XClassHint class_hint = { const_cast<char*> (resname.c_str()), const_cast<char*> (resclass.c_str()) };
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
DisplayWindowX11::configure_window (const Config &config, bool sizeevent)
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

bool
DisplayWindowX11::send_selection_notify (Window req_window, Atom selection, Atom target, Atom req_property, Time req_time)
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
  if (x11_untrap_errors())
    xstatus = 0;
  return xstatus != 0; // success
}

void
DisplayWindowX11::handle_content_request (const size_t nth, ContentOffer *const offer)
{
  assert_return (nth < content_requests_.size());
  ContentRequest &cr = content_requests_[nth];
  assert_return (cr.data_provided == true);
  const XSelectionRequestEvent &xev = cr.xsr;
  const Atom requestor_property = xev.property ? xev.property : xev.target; // ICCCM convention
  if (xev.target == x11context.atom ("TIMESTAMP") && offer)
    {
      vector<unsigned long> ints;
      ints.push_back (offer->time);
      const bool transferred = safe_set_property (x11context.display, xev.requestor, requestor_property, x11context.atom ("INTEGER"),
                                                  32, ints.data(), ints.size());
      send_selection_notify (xev.requestor, xev.selection, xev.target, transferred ? requestor_property : None, xev.time);
    }
  else if (xev.target == x11context.atom ("TARGETS") && offer)
    {
      vector<Atom> ints;
      for (const String &type : offer->content_types)
        x11context.mime_list_target_atoms (type, ints);
      ints.push_back (x11context.atom ("TIMESTAMP"));
      ints.push_back (x11context.atom ("TARGETS"));
      // unhandled: ints.push_back (x11context.atom ("MULTIPLE"));
      const bool transferred = safe_set_property (x11context.display, xev.requestor, requestor_property, x11context.atom ("ATOM"),
                                                  32, ints.data(), ints.size());
      send_selection_notify (xev.requestor, xev.selection, xev.target, transferred ? requestor_property : None, xev.time);
    }
  else if (xev.target == x11context.atom ("MULTIPLE"))
    {
      // reject; we can implement this if we have suitable test case (X11 client) that requests MULTIPLE
      send_selection_notify (xev.requestor, xev.selection, xev.target, None, xev.time);
    }
  else if (strncmp (&cr.data_type[0], "text/", 5) == 0)
    {
      XICCEncodingStyle ecstyle = XUTF8StringStyle;
      if      (xev.target == x11context.atom ("UTF8_STRING") ||
               xev.target == x11context.atom ("text/plain;charset=utf-8") ||
               xev.target == x11context.atom ("text/plain"))
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
                    x11context.transfer_incr_property (xev.requestor, requestor_property, ptype, 8, chars.data(), chars.size());
      send_selection_notify (xev.requestor, xev.selection, xev.target, transferred ? requestor_property : None, xev.time);
    }
  else
    {
      const Atom target = x11context.mime_to_target_atom (cr.data_type);
      const bool transferred = !target ? false :
                               x11context.transfer_incr_property (xev.requestor, requestor_property,
                                                                  target, 8, cr.data.data(), cr.data.size());
      send_selection_notify (xev.requestor, xev.selection, xev.target, transferred ? requestor_property : None, xev.time);
    }
  content_requests_.erase (content_requests_.begin() + nth);
}

void
DisplayWindowX11::request_selection (ContentSourceType content_source, Atom source, uint64 nonce, String data_type, Atom last_failed)
{
  if (!isel_)
    {
      SelectionInput *tsel = new SelectionInput();
      tsel->content_source = content_source;
      tsel->source_atom = source;
      tsel->content_type = data_type;
      tsel->target = x11context.mime_to_target_atom (tsel->content_type, last_failed);
      tsel->owner = XGetSelectionOwner (x11context.display, tsel->source_atom);
      if (tsel->owner != None && tsel->target != None)
        {
          tsel->nonce = nonce;
          tsel->time = event_context_.time;
          tsel->slot = x11context.atom ("RAPICORN_SELECTION");
          XDeleteProperty (x11context.display, window_, tsel->slot);
          XConvertSelection (x11context.display, tsel->source_atom, tsel->target, tsel->slot, window_, tsel->time);
          XDEBUG ("XConvertSelection: [%lx] %s(%s) -> %ld(%s); owner=%ld", tsel->time,
                  x11context.atom (tsel->source_atom), x11context.atom (tsel->target),
                  window_, x11context.atom (tsel->slot),
                  tsel->owner);
          tsel->state = WAIT_FOR_NOTIFY;
          isel_ = tsel;
          isel_->timeout = timestamp_realtime() + SELECTION_INPUT_TIMEOUT; // keep safe from cleanup
          return; // successfully initiated transfer
        }
      delete tsel;
    }
  // if PRIMARY is unowned, xterm yields CUT_BUFFER0 (from root window of screen 0, see ICCCM)
  if (source == XA_PRIMARY && data_type == "text/plain")
    {
      source = x11context.atom ("CUT_BUFFER0");
      String content_data;
      RawData raw;
      if (x11_get_property_data (x11context.display, RootWindow (x11context.display, 0), source, raw, 0) &&
          x11_convert_string_property (x11context.display, raw.property_type, raw.data8, &content_data))
        {
          XDEBUG ("XConvertSelection: failed, falling back to %s(%s)", x11context.atom (source), x11context.atom (raw.property_type));
          enqueue_event (create_event_data (CONTENT_DATA, event_context_, content_source, nonce, data_type, content_data));
          return; // successfully initiated transfer
        }
    }
  // request rejected
  enqueue_event (create_event_data (CONTENT_DATA, event_context_, content_source, nonce, "", ""));
}

void
DisplayWindowX11::handle_command (DisplayCommand *command)
{
  switch (command->type)
    {
      bool found;
    case DisplayCommand::CREATE: case DisplayCommand::OK: case DisplayCommand::ERROR: case DisplayCommand::SHUTDOWN:
      assert_unreached();
    case DisplayCommand::CONFIGURE:
      configure_window (*command->config, command->need_resize);
      break;
    case DisplayCommand::BEEP:
      XBell (x11context.display, 0);
      break;
    case DisplayCommand::SHOW:
      XMapRaised (x11context.display, window_);
      break;
    case DisplayCommand::PRESENT:
      {
        const bool user_activation = command->u64;
        XEvent xevent = { ClientMessage, }; // rest is zeroed
        xevent.xclient.window = window_;
        xevent.xclient.message_type = x11context.atom ("_NET_ACTIVE_WINDOW");
        xevent.xclient.format = 32;
        xevent.xclient.data.l[0] = user_activation ? 2 : 1; // source indication: 0=unkown, 1=application, 2=user-action
        xevent.xclient.data.l[1] = event_context_.time;
        xevent.xclient.data.l[2] = 0; // our currently active window; FIXME: add support for transient dialogs
        xevent.xclient.data.l[3] = xevent.xclient.data.l[4] = 0;
        XSendEvent (x11context.display, x11context.root_window, False, SubstructureNotifyMask | SubstructureRedirectMask, &xevent);
      }
      break;
    case DisplayCommand::BLIT:
      blit (command->surface, *command->region);
      break;
    case DisplayCommand::OWNER: {
      const StringVector &data_types = command->string_list;
      const Atom selection = command->source == CONTENT_SOURCE_SELECTION ? XA_PRIMARY :
                             command->source == CONTENT_SOURCE_CLIPBOARD ? x11context.atom ("CLIPBOARD") :
                             None;
      ContentOffer *offer = find_element (offers_, [selection] (const ContentOffer &o) { return o.selection == selection; });
      if (data_types.size() > 0)
        XSetSelectionOwner (x11context.display, selection, window_, event_context_.time);
      else if (offer)
        XSetSelectionOwner (x11context.display, selection, None, event_context_.time);
      if (window_ == XGetSelectionOwner (x11context.display, selection))
        {
          if (!offer)
            {
              offers_.resize (offers_.size()+1);
              offer = &offers_.back();
              offer->selection = selection;
              offer->nonce = command->nonce;
            }
          if (offer->nonce != command->nonce)
            {
              enqueue_event (create_event_data (CONTENT_CLEAR, event_context_, command->source, offer->nonce, "", ""));
              offer->nonce = command->nonce;
            }
          offer->content_types = data_types;
          offer->time = event_context_.time;
        }
      else
        {
          if (data_types.size() > 0) // tried to become owner but failed
            enqueue_event (create_event_data (CONTENT_CLEAR, event_context_, command->source, offer->nonce, "", ""));
          if (offer)
            offers_.erase (offers_.begin() + (offer - &offers_[0]));
          offer = NULL;
        }
      break; }
    case DisplayCommand::CONTENT: {
      const StringVector &data_types = command->string_list;
      assert_return (data_types.size() == 1);
      const Atom selection = command->source == CONTENT_SOURCE_SELECTION ? XA_PRIMARY :
                             command->source == CONTENT_SOURCE_CLIPBOARD ? x11context.atom ("CLIPBOARD") :
                             None;
      request_selection (command->source, selection, command->nonce, data_types[0]);
      break; }
    case DisplayCommand::PROVIDE: {
      const StringVector &data_types = command->string_list;
      assert_return (data_types.size() == 2);
      found = false;
      for (auto &cr : content_requests_)
        if (cr.request_id == command->nonce && !cr.data_provided)
          {
            cr.data_provided = true;
            cr.data_type = data_types[0];
            cr.data = data_types[1];
            handle_content_request (&cr - &content_requests_[0], NULL);
            found = true;
            break;
          }
      if (!found)
        RAPICORN_CRITICAL ("content provided for unknown request_id: %u (data_type=%s data_length=%u)",
                           command->nonce, data_types[0], data_types[1].size());
      break; }
    case DisplayCommand::UMOVE:     break;  // FIXME
    case DisplayCommand::URESIZE:   break;  // FIXME
    case DisplayCommand::DESTROY:
      destroy_x11_resources();
      delete this;
      break;
    }
  delete command;
}

// == X11Context ==
X11Context::X11Context (DisplayDriver &driver, AsyncNotifyingQueue<DisplayCommand*> &command_queue,
                        AsyncBlockingQueue<DisplayCommand*> &reply_queue) :
  loop_ (NULL), command_queue_ (command_queue), reply_queue_ (reply_queue),
  shared_mem_ (-1), display_driver (driver), display (NULL), visual (NULL),
  max_request_bytes (0), max_property_bytes (0), screen (0), depth (0),
  root_window (0), input_method (NULL)
{
  XDEBUG ("%s: X11Context started", __func__);
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
  XDEBUG ("%s: X11Context stopped", __func__);
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
  max_request_bytes = XExtendedMaxRequestSize (display) * 4;
  if (!max_request_bytes)
    max_request_bytes = XMaxRequestSize (display) * 4;
  max_property_bytes = CLAMP (max_request_bytes - 256, 8192, 1048576);
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
X11Context::cmd_dispatcher (const LoopState &state)
{
  if (state.phase == state.PREPARE || state.phase == state.CHECK)
    return command_queue_.pending();
  else if (state.phase == state.DISPATCH)
    {
      for (DisplayCommand *cmd = command_queue_.pop(); cmd; cmd = command_queue_.pop())
        switch (cmd->type)
          {
            DisplayWindowX11 *display_window;
          case DisplayCommand::CREATE:
            display_window = new DisplayWindowX11 (*this);
            display_window->create_window (*cmd->setup, *cmd->config);
            delete cmd;
            reply_queue_.push (new DisplayCommand (DisplayCommand::OK, display_window));
            break;
          case DisplayCommand::SHUTDOWN:
            loop_->quit();
            delete cmd;
            reply_queue_.push (new DisplayCommand (DisplayCommand::OK, NULL));
            assert_return (x11ids_.empty(), true);
            break;
          default:
            display_window = dynamic_cast<DisplayWindowX11*> (cmd->display_window);
            if (cmd->display_window && display_window)
              display_window->handle_command (cmd);
            else
              {
                critical ("DisplayCommand without DisplayWindowX11: %p (type=%d)", cmd, cmd->type);
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
  loop_ = MainLoop::create();
  // ensure X11 file descriptor changes are handled
  loop_->exec_io_handler (Aida::slot (*this, &X11Context::x11_io_handler), ConnectionNumber (display), "r", EventLoop::PRIORITY_NORMAL);
  // ensure queued X11 events are processed (i.e. ones already read from fd)
  loop_->exec_dispatcher (Aida::slot (*this, &X11Context::x11_dispatcher), EventLoop::PRIORITY_NORMAL);
  // process cleanup actions after expiration times
  loop_->exec_dispatcher (Aida::slot (*this, &X11Context::x11_timer), EventLoop::PRIORITY_NORMAL);
  // ensure enqueued user commands are processed
  loop_->exec_dispatcher (Aida::slot (*this, &X11Context::cmd_dispatcher), EventLoop::PRIORITY_NOW);
  // ensure new command_queue events are noticed
  command_queue_.notifier ([&]() { loop_->wakeup(); });
  // process X11 events
  loop_->run();
  // prevent wakeups on stale objects
  command_queue_.notifier (NULL);
  // close down
  if (!x11ids_.empty()) // DisplayWindowX11 unconditionally references X11Context
    fatal ("%s: stopped handling X11Context with %d active windows", __func__, x11ids_.size());
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
  loop_->destroy_loop();
  loop_ = NULL;
}

bool
X11Context::x11_timer (const LoopState &state)
{
  const uint64 now = state.current_time_usecs;
  if (state.phase == state.PREPARE || state.phase == state.CHECK)
    {
      /* lazy timer: we're not forcing event loop wakeups for pending timeouts,
       * because we're currently just handling non-critical cleanups. the
       * cleanups are performed after the loop wakes up and timesouts have
       * expired, however long the loop was sleeping.
       */
      for (auto it : incr_transfers_)
        if (it.timeout <= now)
          return true;
      for (auto it : x11ids_)
        {
          X11Widget *xw = it.second;
          int64 timeout_usecs = -1;
          if (xw->timer (state, &timeout_usecs))
            return true;
        }
      return false;
    }
  else if (state.phase == state.DISPATCH)
    {
      for (size_t i = 0; i < incr_transfers_.size(); i++)
        if (incr_transfers_[i].timeout <= now)
          {
            // waiting for reply expired, killing request
            const IncrTransfer &it = incr_transfers_[i];
            safe_set_property (display, it.window, it.property, it.type, it.byte_width * 8, NULL, 0);
            incr_transfers_.erase (incr_transfers_.begin() + i); // invalidates iterators
            return true;
          }
      for (auto it : x11ids_)
        {
          X11Widget *xw = it.second;
          int64 timeout_usecs = -1;
          LoopState wstate = state;
          wstate.phase = state.CHECK;
          if (xw->timer (wstate, &timeout_usecs))
            {
              wstate.phase = state.DISPATCH;
              // dispatching X11Widget timer can change everything, e.g. x11ids_
              xw->timer (wstate, &timeout_usecs);
              return true;
            }
        }
      return true;
    }
  else if (state.phase == state.DESTROY)
    ;
  return false;
}

bool
X11Context::x11_dispatcher (const LoopState &state)
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
      // deliver to owning DisplayWindow
      X11Widget *xwidget = x11id_get (xevent.xany.window);
      DisplayWindowX11 *scw = !xwidget ? NULL : dynamic_cast<DisplayWindowX11*> (xwidget);
      if (scw && consumed)
        scw->filtered_event (xevent);           // preserve bookkeeping of DisplayWindow classes
      else if (scw)
        consumed = scw->process_event (xevent); // DisplayWindow event handling
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
      DisplayWindowX11 *scw = dynamic_cast<DisplayWindowX11*> (xwidget);
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
    case PropertyNotify: {
      const XPropertyEvent &xev = xevent.xproperty;
      if (!incr_transfers_.empty() && xev.state == PropertyDelete)
        continue_incr (xev.window, xev.atom);
      break; }
    case DestroyNotify: {
      const XDestroyWindowEvent &xev = xevent.xdestroywindow;
      if (xev.window)
        window_destroyed (xev.window);
      break; }
    }
  return filtered;
}

bool
X11Context::transfer_incr_property (Window window, Atom property, Atom type, int element_bits, const void *elements, size_t nelements)
{
  assert_return (element_bits == 8 || element_bits == 16 || element_bits == 32, false);
  const long nbytes = nelements * (element_bits / 8);
  // set property in one chunk
  if (nbytes < long (max_property_bytes))
    {
      const bool transferred = safe_set_property (display, window, property, type, element_bits, elements, nelements);
      if (transferred)
        XDEBUG ("Transfer: bits=%d bytes=%d %s -> %ld(%s)", element_bits, nelements * (element_bits / 8),
                atom (type), window, atom (property));
      return transferred;
    }
  // need to start incremental transfer
  ref_events (window, PropertyChangeMask); // avoid-race: request PropertyChangeMask before safe_set_property("INCR")
  if (safe_set_property (display, window, property, atom ("INCR"), 32, &nbytes, 1)) // ICCCM sec. 2.5
    {
      IncrTransfer it;
      it.window = window;
      it.property = property;
      it.type = type;
      it.byte_width = element_bits / 8;
      it.offset = 0;
      it.bytes.insert (it.bytes.end(), (const char*) elements, (const char*) elements + nelements * it.byte_width);
      it.timeout = timestamp_realtime() + INCR_TRANSFER_TIMEOUT;
      incr_transfers_.push_back (it);
      return true;
    }
  else
    {
      unref_events (window);
      return false;
    }
}

void
X11Context::continue_incr (Window window, Atom property)
{
  for (IncrTransfer &it : incr_transfers_)
    if (it.window == window && it.property == property)
      {
        const size_t num = MIN (it.bytes.size() - it.offset, max_property_bytes) / it.byte_width;
        const bool abort = false == safe_set_property (display, it.window, it.property, it.type, it.byte_width * 8,
                                                       it.bytes.data() + it.offset, num);
        it.offset += num * it.byte_width;
        if (num == 0 || abort) // 0-size termination sent
          {
            XDEBUG ("IncrTransfer: bits=%d bytes=%d %s -> %ld(%s)", it.byte_width * 8, it.bytes.size(),
                    atom (it.type), it.window, atom (it.property));
            unref_events (it.window); // no PropertyChangeMask events are needed beyond this
            incr_transfers_.erase (incr_transfers_.begin() + (&it - &incr_transfers_[0]));
          }
        else // refresh timeout to keep IncrTransfer safe from cleanup timer
          it.timeout = timestamp_realtime() + INCR_TRANSFER_TIMEOUT;
        return;
      }
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
    fatal ("%s: x11id=%d already in use for %p while setting to %p", __func__, xid, it->second, x11widget);
  x11ids_[xid] = x11widget;
}

X11Widget*
X11Context::x11id_get (size_t xid)
{
  // do lookup without modifying x11ids
  auto it = x11ids_.find (xid);
  return x11ids_.end() != it ? it->second : NULL;
}

EventStake*
X11Context::find_event_stake (Window window, bool create)
{
  for (auto &es : event_stakes_)
    if (es.window == window)
      return &es;
  if (!create)
    return NULL;
  EventStake es;
  es.window = window;
  event_stakes_.push_back (es);
  return &event_stakes_[event_stakes_.size()-1];
}

void
X11Context::window_destroyed (Window window)
{
  EventStake *es  = find_event_stake (window, false);
  if (es)
    es->destroyed = true;
}

void
X11Context::ref_events (Window window, unsigned long event_mask, const XSetWindowAttributes *attrs)
{
  EventStake &es = *find_event_stake (window, true);
  es.ref_count += 1;
  if (attrs)
    es.mask |= attrs->event_mask;
  if (event_mask & ~es.mask)
    {
      es.mask |= event_mask;
      if (!es.destroyed)
        {
          XErrorEvent dummy = { 0, };
          x11_trap_errors (&dummy);
          XSelectInput (display, es.window, es.mask);
          XSync (display, False);
          if (x11_untrap_errors())
            es.destroyed = true;
        }
    }
}

void
X11Context::unref_events (Window window)
{
  EventStake *window_event_stake = find_event_stake (window, false);
  assert_return (window_event_stake != NULL);
  assert_return (window_event_stake->ref_count > 0);
  window_event_stake->ref_count -= 1;
  if (window_event_stake->ref_count == 0)
    {
      if (!window_event_stake->destroyed)
        {
          XErrorEvent dummy = { 0, };
          x11_trap_errors (&dummy);
          XSelectInput (display, window_event_stake->window, 0);
          XSync (display, False);
          if (x11_untrap_errors())
            window_event_stake->destroyed = true;
        }
      event_stakes_.erase (event_stakes_.begin() + (window_event_stake - &event_stakes_[0]));
    }
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

static const char *const mime_target_atoms[] = {           // determine X11 selection property types from mime
  "text/plain", "UTF8_STRING",                  // works best for all clients, xterm et al
  "text/plain", "text/plain;charset=utf-8",     // listed in Qt targets, but not transmitted by it
  "text/plain", "text/plain",                   // listed by Gtk+ targets, but not understood by it
  "text/plain", "COMPOUND_TEXT",                // understood by all legacy X11 clients
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
X11Context::mime_list_target_atoms (const String &mime, vector<Atom> &atoms)
{
  for (size_t i = 0; i + 1 < ARRAY_SIZE (mime_target_atoms); i += 2)
    if (mime == mime_target_atoms[i])
      atoms.push_back (atom (mime_target_atoms[i+1]));
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
    loop_->exec_timer (Aida::slot (*this, &X11Context::process_updates), 50);
}

} // Rapicorn
