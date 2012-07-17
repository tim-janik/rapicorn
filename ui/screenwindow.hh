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
    _NET_WM_STATE_MASK = 0x3fff,
    DECORATED      = 1 << 26,   ///< The window is decorated by window managers.
    UNFOCUSED      = 1 << 27,   ///< The window does not get automatic keyboard focus when initially shown.
    ACCEPT_FOCUS   = 1 << 28,   ///< The window enters keyboard focus mode when selected by the user.
    ICONIFY        = 1 << 29,   ///< The window is in iconified state, (minimized, but icon shown).
    DELETABLE      = 1 << 30,   ///< The window manager offers the deletion action for this window.
  };
  static String flags_name (uint64 flags, String combo = ",");
  /// Structure requesting the initial window setup.
  struct Setup {
    WindowType  window_type;    ///< Requested window type.
    Flags       window_flags;   ///< Requested window hints.
    String      session_role;   ///< String to uniquely identify this window.
    Color       bg_average;
    inline      Setup();
  };
  /// Structure requesting window configuration changes.
  struct Config {
    String      title;                          ///< User visible title of this window.
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
  virtual void          configure               (const Config &config) = 0;     ///< Change window configuration.
  virtual void          beep                    () = 0;                         ///< Issue an audible bell.
  virtual void          show                    () = 0;                         ///< Show window on screen.
  virtual void          present                 () = 0;                         ///< Demand user attention for this window.
  virtual bool          viewable                () = 0; ///< Check if the window is viewable, i.e. not minimized/iconified/etc.
  virtual void          blit_surface            (cairo_surface_t *surface, Rapicorn::Region region) = 0;
  virtual void          start_user_move         (uint button, double root_x, double root_y) = 0;
  virtual void          start_user_resize       (uint button, double root_x, double root_y, AnchorType edge) = 0;
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
  virtual ScreenWindow* create_screen_window    (const ScreenWindow::Setup &setup, ScreenWindow::EventReceiver &receiver) = 0;
  virtual void          close                   () = 0;
  static ScreenDriver*  open_screen_driver      (const String &backend_name);
  static bool           driver_priority_lesser  (ScreenDriver *d1, ScreenDriver *d2);
};

// == Implementations ==
ScreenWindow::Setup::Setup() :
  window_type (WindowType (0)), window_flags (ScreenWindow::Flags (0))
{}

ScreenWindow::Config::Config() :
  root_x (INT_MIN), root_y (INT_MIN), request_width (0), request_height (0), width_inc (0), height_inc (0)
{}

ScreenWindow::State::State() :
  window_flags (ScreenWindow::Flags (0)), visible (0), active (0),
  width (0), height (0), root_x (INT_MIN), root_y (INT_MIN), deco_x (INT_MIN), deco_y (INT_MIN)
{}

// internal
void rapicorn_gtk_threads_enter         ();
void rapicorn_gtk_threads_leave         ();
void rapicorn_gtk_threads_shutdown      ();

} // Rapicorn

#endif  /* __RAPICORN_SCREEN_WINDOW_HH__ */
