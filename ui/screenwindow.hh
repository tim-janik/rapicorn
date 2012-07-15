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
    DELETABLE      = 1 <<  0,   ///< Requests a [X] delete button or menu item on this window.
    UNFOCUSED      = 1 <<  1,   ///< Requests the window to not get automatic keyboard focus when shown.
    ACCEPT_FOCUS   = 1 <<  2,   ///< Requests to enter keyboard focus mode when selected by the user.
    FULLSCREEN     = 1 <<  3,   ///< Requests covering the entire screen, no decoration, for presentation mode.
    HMAXIMIZED     = 1 <<  4,   ///< Requests horizontal maximization for the window.
    VMAXIMIZED     = 1 <<  5,   ///< Requests vertical maximization for the window.
    ICONIFY        = 1 <<  6,   ///< Requests the window to be temporarily hidden (minimized).
    STICKY         = 1 <<  7,   ///< Requests to keep window fixed and on screen when virtual desktops change.
    SHADED         = 1 <<  8,   ///< Requests to only show the window decoration bar.
    DECORATED      = 1 <<  9,   ///< Requests window decorations.
    ABOVE_ALL      = 1 << 10,   ///< Requests the window to be shown on top of most other windows.
    BELOW_ALL      = 1 << 11,   ///< Requests the window to be shown below most other windows.
    SKIP_TASKBAR   = 1 << 12,   ///< Requests the window to be exempt from taskbar listings.
    SKIP_PAGER     = 1 << 13,   ///< Requests the window to be exempt from virtual desktop pager display.
  };
  /// Structure requesting the initial window setup.
  struct Setup {
    WindowType  window_type;    ///< Requested window type.
    Flags       window_flags;   ///< Requested window hints.
    bool        modal;          ///< Requests the window to receive input exclusively.
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
    Flags       window_flags;
    bool        visible;
    bool        active;         ///< Indicates that the window currently receives user input.
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
  window_type (WindowType (0)), window_flags (ScreenWindow::Flags (0)), modal (0)
{}

ScreenWindow::Config::Config() :
  root_x (INT_MIN), root_y (INT_MIN), request_width (0), request_height (0), width_inc (0), height_inc (0)
{}

ScreenWindow::State::State() :
  window_flags (ScreenWindow::Flags (0)), visible (0), active (0), width (0), height (0),
  root_x (INT_MIN), root_y (INT_MIN), deco_x (INT_MIN), deco_y (INT_MIN)
{}

// internal
void rapicorn_gtk_threads_enter         ();
void rapicorn_gtk_threads_leave         ();
void rapicorn_gtk_threads_shutdown      ();

} // Rapicorn

#endif  /* __RAPICORN_SCREEN_WINDOW_HH__ */
