// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_DISPLAY_WINDOW_HH__
#define __RAPICORN_DISPLAY_WINDOW_HH__

#include <ui/events.hh>
#include <ui/region.hh>

namespace Rapicorn {
class DisplayDriver;
class DisplayCommand;

/// Interface class for managing window contents on screens and display devices.
class DisplayWindow : public virtual std::enable_shared_from_this<DisplayWindow> {
  RAPICORN_CLASS_NON_COPYABLE (DisplayWindow);
public:
  /// Flags used to request and reflect certain window operations and states.
  enum Flags {
    MODAL          = 1 <<  0,   ///< Hint to the window manager that window receives input exclusively.
    STICKY         = 1 <<  1,   ///< Window is fixed and kept on screen when virtual desktops change.
    VMAXIMIZED     = 1 <<  2,   ///< Window is vertically maximized.
    HMAXIMIZED     = 1 <<  3,   ///< Window is horizontally maximized.
    SHADED         = 1 <<  4,   ///< Only the decoration bar for this window is shown.
    SKIP_TASKBAR   = 1 <<  5,   ///< The window is exempt from taskbar listings.
    SKIP_PAGER     = 1 <<  6,   ///< The window is exempt from virtual desktop pager display.
    HIDDEN         = 1 <<  7,   ///< Window manager indication for non-visible window state.
    FULLSCREEN     = 1 <<  8,   ///< Window covers the entire screen, no decoration, for presentation mode.
    ABOVE_ALL      = 1 <<  9,   ///< The window is shown on top of most other windows.
    BELOW_ALL      = 1 << 10,   ///< The window is shown below most other windows.
    ATTENTION      = 1 << 11,   ///< The window indicates need for user attention.
    FOCUS_DECO     = 1 << 12,   ///< Window decoration indicates active focus state.
    _WM_STATE_MASK = 0x00003fff,
    DECORATED      = 1 << 24,   ///< The window is decorated by window managers.
    MINIMIZABLE    = 1 << 25,   ///< The window manager offers the maximization action for this window.
    MAXIMIZABLE    = 1 << 26,   ///< The window manager offers the maximization action for this window.
    DELETABLE      = 1 << 27,   ///< The window manager offers the deletion action for this window.
    _DECO_MASK     = 0x0f000000,
    ACCEPT_FOCUS   = 1 << 28,   ///< The window enters keyboard focus mode when selected by the user.
    UNFOCUSED      = 1 << 29,   ///< The window does not get automatic keyboard focus when initially shown.
    ICONIFY        = 1 << 30,   ///< The window is in iconified state, (minimized, but icon shown).
  };
  static String flags_name (uint64 flags, String combo = ","); ///< Convert flags to string.
  /// Structure requesting the initial window setup.
  struct Setup {
    WindowType  window_type;    ///< Requested window type.
    Flags       request_flags;  ///< Requested window hints.
    String      session_role;   ///< String to uniquely identify this window, despite similar titles.
    Color       bg_average;
    inline      Setup();
  };
  /// Structure requesting window configuration changes.
  struct Config {
    String      title;                          ///< Window decoration title for this window.
    String      alias;                          ///< Brief title alias as e.g. icon subtitle.
    int         root_x, root_y;                 ///< Requested window position.
    int         request_width, request_height;  ///< Requested window size.
    int         width_inc, height_inc;          ///< Requests grid aligned resizing.
    inline      Config();
  };
  /// Structure describing the current window state.
  struct State {
    WindowType  window_type;            ///< Window type at creation.
    Flags       window_flags;           ///< Actual state of the backend window.
    String      visible_title;          ///< User visible title, as displayed by window manager.
    String      visible_alias;          ///< User visible alias, as displayed by window manager.
    int         width, height;          ///< Size of the window.
    int         root_x, root_y;         ///< Root position of the window.
    int         deco_x, deco_y;         ///< Position of window decorations.
    bool        visible;                ///< Visibility state, usuall true after show().
    bool        active;                 ///< Indicator for keyboard input focus.
    inline      State();
    inline bool operator== (const State &o) const;
    inline bool operator!= (const State &o) const { return !operator== (o); }
  };
  // == public API ==
  State         get_state               ();                     ///< Retrieve the current window state.
  void          beep                    ();                     ///< Issue an audible bell.
  void          show                    ();                     ///< Show window on screen.
  void          present                 (bool user_activation); ///< Make window the active window, user activation might enforce this.
  bool          viewable                ();                     ///< Check if the window is viewable, i.e. not iconified/shaded/etc.
  void          destroy                 ();                     ///< Destroy onscreen windows and reset event wakeup.
  void          configure               (const Config &config, bool sizeevent); ///< Change window configuration, requesting size event.
  void          blit_surface            (cairo_surface_t *surface, const Rapicorn::Region &region);   ///< Blit/paint window region.
  void          start_user_move         (uint button, double root_x, double root_y);                  ///< Trigger window movement.
  void          start_user_resize       (uint button, double root_x, double root_y, Anchor edge);     ///< Trigger window resizing.
  void          set_content_owner       (ContentSourceType source, uint64 nonce, const StringVector &data_types); ///< Yields CONTENT_REQUEST & CONTENT_CLEAR.
  void          request_content         (ContentSourceType source, uint64 nonce, const String &data_type); ///< Yields CONTENT_DATA.
  void          provide_content         (const String &data_type, const String &data, uint64 request_id); ///< Reply for CONTENT_REQUEST.
  Event*        pop_event               ();                     ///< Fetch the next event for this Window.
  void          push_event              (Event *event);         ///< Push back an event, so it's the next event returned by pop().
  bool          has_event               ();                     ///< Indicates if pop_event() will return non-NULL.
  void          set_event_wakeup        (const std::function<void()> &wakeup);  ///< Callback used to notify new event arrival.
  bool          peek_events             (const std::function<bool (Event*)> &pred);     ///< Peek/find events via callback.
protected:
  explicit               DisplayWindow          ();
  virtual               ~DisplayWindow          ();
  virtual DisplayDriver& display_driver_async   () const = 0;                   ///< Acces DisplayDriver, called from any thread.
  void                   enqueue_event          (Event *event);                 ///< Add an event to the back of the event queue.
  bool                   update_state           (const State &state);           ///< Updates the state returned from get_state().
  void                   queue_command          (DisplayCommand *command);      ///< Helper to queue commands on DisplayDriver.
private:
  State                 async_state_;
  bool                  async_state_accessed_;
  Spinlock              async_spin_;
  std::list<Event*>     async_event_queue_;
  std::function<void()> async_wakeup_;
};

struct DisplayCommand   /// Structure for internal asynchronous communication between DisplayWindow and DisplayDriver.
{
  enum Type { ERROR, OK, CREATE, CONFIGURE, BEEP, SHOW, PRESENT, BLIT, UMOVE, URESIZE, CONTENT, OWNER, PROVIDE, DESTROY, SHUTDOWN, };
  const Type            type;
  DisplayWindow        *const display_window;
  String                string;
  StringVector          string_list;
  DisplayWindow::Config *config;
  DisplayWindow::Setup  *setup;
  cairo_surface_t      *surface;
  Rapicorn::Region     *region;
  union { uint64        nonce, u64; };
  int                   root_x, root_y, button;
  ContentSourceType     source;
  bool                  need_resize;
  /*ctor*/             ~DisplayCommand ();
  explicit              DisplayCommand (Type type, DisplayWindow *window);
  static bool           reply_type     (Type type);
};

/// Management class for DisplayWindow driver implementations.
class DisplayDriver {
  AsyncNotifyingQueue<DisplayCommand*> command_queue_;
  AsyncBlockingQueue<DisplayCommand*>  reply_queue_;
  std::thread                          thread_handle_;
  RAPICORN_CLASS_NON_COPYABLE (DisplayDriver);
protected:
  DisplayDriver        *sibling_;
  String                name_;
  int                   priority_;
  virtual void          run (AsyncNotifyingQueue<DisplayCommand*> &command_queue,
                             AsyncBlockingQueue<DisplayCommand*>  &reply_queue) = 0;
  /// Construct with backend @a name, a lower @a priority will score better for "auto" selection.
  explicit              DisplayDriver           (const String &name, int priority = 0);
  virtual              ~DisplayDriver           ();
  void                  queue_command           (DisplayCommand *display_command);
  bool                  open_L                  ();
  void                  close_L                 ();
public:
  /// Create a new DisplayWindow from an opened driver.
  DisplayWindow*        create_display_window   (const DisplayWindow::Setup &setup, const DisplayWindow::Config &config);
  /// Open a specific named driver, "auto" will try to find the best match.
  static DisplayDriver* retrieve_display_driver (const String &backend_name);
  /// Comparator for "auto" scoring.
  static bool           driver_priority_lesser  (const DisplayDriver *d1, const DisplayDriver *d2);
  static void           forcefully_close_all    ();
  ///@cond
  class Friends {
    friend class DisplayWindow;
    static void queue_command (DisplayDriver &d, DisplayCommand *c) { d.queue_command (c); }
  };
  ///@endcond
};

/// Template for factory registration of DisplayDriver implementations.
template<class DriverImpl>
struct DisplayDriverFactory : public DisplayDriver {
  std::atomic<int> running;
  DisplayDriverFactory (const String &name, int priority = 0) :
    DisplayDriver (name, priority), running (false)
  {}
  virtual void
  run (AsyncNotifyingQueue<DisplayCommand*> &command_queue, AsyncBlockingQueue<DisplayCommand*> &reply_queue)
  {
    running = true;
    DriverImpl driver (*this, command_queue, reply_queue);
    if (driver.connect())
      {
        DisplayCommand *cmd = new DisplayCommand (DisplayCommand::OK, NULL);
        reply_queue.push (cmd);
        driver.run();
      }
    else
      {
        DisplayCommand *cmd = new DisplayCommand (DisplayCommand::ERROR, NULL);
        cmd->string = "Driver connection failed";
        reply_queue.push (cmd);
      }
    running = false;
  }
  virtual
  ~DisplayDriverFactory()
  {
    assert (running == false);
  }
};

// == Implementations ==
DisplayWindow::Setup::Setup() :
  window_type (WindowType (0)), request_flags (DisplayWindow::Flags (0))
{}

DisplayWindow::Config::Config() :
  root_x (INT_MIN), root_y (INT_MIN), request_width (0), request_height (0), width_inc (0), height_inc (0)
{}

DisplayWindow::State::State() :
  window_flags (DisplayWindow::Flags (0)),
  width (0), height (0), root_x (INT_MIN), root_y (INT_MIN), deco_x (INT_MIN), deco_y (INT_MIN),
  visible (0), active (0)
{}

bool
DisplayWindow::State::operator== (const State &o) const
{
  return window_type == o.window_type && window_flags == o.window_flags && width == o.width && height == o.height &&
    root_x == o.root_x && root_y == o.root_y && deco_x == o.deco_x && deco_y == o.deco_y &&
    visible == o.visible && active == o.active && visible_title == o.visible_title && visible_alias == o.visible_alias;
}

} // Rapicorn

#endif  /* __RAPICORN_DISPLAY_WINDOW_HH__ */
