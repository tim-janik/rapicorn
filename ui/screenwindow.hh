// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_SCREEN_WINDOW_HH__
#define __RAPICORN_SCREEN_WINDOW_HH__

#include <ui/events.hh>
#include <ui/region.hh>

namespace Rapicorn {

/// Interface class for managing window contents on screens and display devices.
class ScreenWindow : public virtual Deletable, protected NonCopyable, public virtual std::enable_shared_from_this<ScreenWindow> {
  Mutex                 m_async_mutex;
  std::list<Event*>     m_async_event_queue;
  std::function<void()> m_async_wakeup;
protected:
  virtual              ~ScreenWindow     ();
  void                  enqueue_event    (Event *event);
public:
  Event*                pop_event        ();
  void                  push_event       (Event *event);
  bool                  has_event        ();
  bool                  peek_events      (const std::function<bool (Event*)> &pred);
  void                  set_event_wakeup (const std::function<void()> &wakeup);
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
  void          configure               (const Config &config); ///< Change window configuration.
  void          beep                    ();                     ///< Issue an audible bell.
  void          show                    ();                     ///< Show window on screen.
  void          present                 ();                     ///< Demand user attention for this window.
  bool          viewable                ();                     ///< Check if the window is viewable, i.e. not iconified/shaded/etc.
  void          blit_surface            (cairo_surface_t *surface, const Rapicorn::Region &region);     ///< Blit/paint window region.
  void          start_user_move         (uint button, double root_x, double root_y);                    ///< Trigger window movement.
  void          start_user_resize       (uint button, double root_x, double root_y, AnchorType edge);   ///< Trigger window resizing.
  void          destroy                 ();                     ///< Destroy onscreen window and reset event wakeup.
protected:
  enum CommandType {
    CMD_CREATE = 1, CMD_CONFIGURE, CMD_BEEP, CMD_SHOW, CMD_PRESENT, CMD_BLIT, CMD_MOVE, CMD_RESIZE, CMD_DESTROY,
  };
  struct Command        /// Structure for internal asynchronous operations.
  {
    CommandType       type;
    union {
      struct { Config          *config; Setup *setup; };
      struct { cairo_surface_t *surface; Rapicorn::Region *region; };
      struct { int              button, root_x, root_y; };
    };
    Command (CommandType type);
    Command (CommandType type, const Config &cfg);
    Command (CommandType type, const ScreenWindow::Setup &cs, const ScreenWindow::Config &cfg);
    Command (CommandType type, cairo_surface_t *surface, const Rapicorn::Region &region);
    Command (CommandType type, int button, int root_x, int root_y);
    ~Command();
  };
  explicit      ScreenWindow    ();
  virtual void  queue_command   (Command *command) = 0;
  bool          update_state    (const State &state);
private:
  Spinlock      m_spin;
  State         m_state;
  bool          m_accessed;
public: typedef struct Command Command;
};
typedef std::shared_ptr<ScreenWindow> ScreenWindowP;

/// Management class for ScreenWindow driver implementations.
class ScreenDriver : protected NonCopyable {
protected:
  ScreenDriver         *m_sibling;
  String                m_name;
  int                   m_priority;
  /// Construct with backend @a name, a lower @a priority will score better for "auto" selection.
  explicit              ScreenDriver            (const String &name, int priority = 0);
public:
  /// Open a driver or increase opening reference count.
  virtual bool          open                    () = 0;
  /// Create a new ScreenWindow from an opened driver.
  virtual ScreenWindowP create_screen_window    (const ScreenWindow::Setup &setup,
                                                 const ScreenWindow::Config &config) = 0;
  /// Decrease opening reference count, closes down driver resources if the last count drops.
  virtual void          close                   () = 0;
  /// Open a specific named driver, "auto" will try to find the best match.
  static ScreenDriver*  open_screen_driver      (const String &backend_name);
  /// Comparator for "auto" scoring.
  static bool           driver_priority_lesser  (ScreenDriver *d1, ScreenDriver *d2);
};

// == Implementations ==
ScreenWindow::Setup::Setup() :
  window_type (WindowType (0)), request_flags (ScreenWindow::Flags (0))
{}

ScreenWindow::Config::Config() :
  root_x (INT_MIN), root_y (INT_MIN), request_width (0), request_height (0), width_inc (0), height_inc (0)
{}

ScreenWindow::State::State() :
  window_flags (ScreenWindow::Flags (0)),
  width (0), height (0), root_x (INT_MIN), root_y (INT_MIN), deco_x (INT_MIN), deco_y (INT_MIN),
  visible (0), active (0)
{}

bool
ScreenWindow::State::operator== (const State &o) const
{
  return window_type == o.window_type && window_flags == o.window_flags && width == o.width && height == o.height &&
    root_x == o.root_x && root_y == o.root_y && deco_x == o.deco_x && deco_y == o.deco_y &&
    visible == o.visible && active == o.active && visible_title == o.visible_title && visible_alias == o.visible_alias;
}

} // Rapicorn

#endif  /* __RAPICORN_SCREEN_WINDOW_HH__ */
