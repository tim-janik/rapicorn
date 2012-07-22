// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_SCREEN_WINDOW_HH__
#define __RAPICORN_SCREEN_WINDOW_HH__

#include <ui/events.hh>
#include <ui/region.hh>

namespace Rapicorn {

class ScreenWindow : public virtual Deletable, protected NonCopyable {
protected:
  explicit              ScreenWindow () {}
public:
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
    _DECO_MASK = 0x0f000000,
    ACCEPT_FOCUS   = 1 << 28,   ///< The window enters keyboard focus mode when selected by the user.
    UNFOCUSED      = 1 << 29,   ///< The window does not get automatic keyboard focus when initially shown.
    ICONIFY        = 1 << 30,   ///< The window is in iconified state, (minimized, but icon shown).
  };
  static String flags_name (uint64 flags, String combo = ",");
  /// Structure requesting the initial window setup.
  struct Setup {
    WindowType  window_type;    ///< Requested window type.
    Flags       request_flags;  ///< Requested window hints.
    String      session_role;   ///< String to uniquely identify this window.
    Color       bg_average;
    inline      Setup();
  };
  /// Structure requesting window configuration changes.
  struct Config {
    String      title;                          ///< User visible title of this window.
    String      alias;                          ///< Brief title alias as e.g. icon subtitle.
    int         root_x, root_y;                 ///< Requested window position.
    int         request_width, request_height;  ///< Requested window size.
    int         width_inc, height_inc;          ///< Requests grid aligned resizing.
    inline      Config();
  };
  /// Structure describing the current window state.
  struct State {
    Flags       window_flags;           ///< Actual state of the backend window.
    bool        visible, active;
    int         width, height;
    int         root_x, root_y;
    int         deco_x, deco_y;
    Setup       setup;
    Config      config;
    inline      State();
  };
  /// Widget interface for receiving events.
  struct EventReceiver {
    virtual            ~EventReceiver           () {}
    virtual void        enqueue_async           (Event              *event) = 0;
  };
  // == public API ==
  virtual State         get_state               () = 0;                         ///< Retrieve the current window state.
  void          configure               (const Config &config);     ///< Change window configuration.
  void          beep                    ();                             ///< Issue an audible bell.
  void          show                    ();                         ///< Show window on screen.
  void          present                 ();                         ///< Demand user attention for this window.
  virtual bool          viewable                () = 0; ///< Check if the window is viewable, i.e. not minimized/iconified/etc.
  void          blit_surface            (cairo_surface_t *surface, const Rapicorn::Region &region);
  void          start_user_move         (uint button, double root_x, double root_y);
  void          start_user_resize       (uint button, double root_x, double root_y, AnchorType edge);
protected:
  enum CommandType {
    CMD_CREATE = 1, CMD_CONFIGURE, CMD_BEEP, CMD_SHOW, CMD_PRESENT, CMD_BLIT, CMD_MOVE, CMD_RESIZE,
  };
  struct Command {
    CommandType       type;
    union {
      struct { Config          *config; Setup *setup; EventReceiver *receiver; };
      struct { cairo_surface_t *surface; Rapicorn::Region *region; };
      struct { int              button, root_x, root_y; };
    };
    Command (CommandType type);
    Command (CommandType type, const Config &cfg);
    Command (CommandType type, const ScreenWindow::Setup &cs, const ScreenWindow::Config &cfg, ScreenWindow::EventReceiver &rc);
    Command (CommandType type, cairo_surface_t *surface, const Rapicorn::Region &region);
    Command (CommandType type, int button, int root_x, int root_y);
    ~Command();
  };
  virtual void  queue_command   (Command *command) = 0;
};

// == ScreenDriver ==
class ScreenDriver : protected NonCopyable {
protected:
  ScreenDriver         *m_sibling;
  String                m_name;
  int                   m_priority;
public:
  explicit              ScreenDriver            (const String &name, int priority = 0);
  virtual bool          open                    () = 0;
  virtual ScreenWindow* create_screen_window    (const ScreenWindow::Setup &setup,
                                                 const ScreenWindow::Config &config,
                                                 ScreenWindow::EventReceiver &receiver) = 0;
  virtual void          close                   () = 0;
  static ScreenDriver*  open_screen_driver      (const String &backend_name);
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
  window_flags (ScreenWindow::Flags (0)), visible (0), active (0),
  width (0), height (0), root_x (INT_MIN), root_y (INT_MIN), deco_x (INT_MIN), deco_y (INT_MIN)
{}

} // Rapicorn

#endif  /* __RAPICORN_SCREEN_WINDOW_HH__ */
