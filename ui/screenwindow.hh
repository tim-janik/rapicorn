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
  /* screen_window info (constant during runtime) */
  struct Info {
    WindowType  window_type;
    constexpr   Info();
  };
  /* screen_window state */
  typedef enum {
    STATE_STICKY        = 0x0004,
    STATE_WITHDRAWN     = 0x0008,
    STATE_HMAXIMIZED    = 0x0010,
    STATE_VMAXIMIZED    = 0x0020,
    STATE_FULLSCREEN    = 0x0040,
    STATE_ICONIFIED     = 0x0080,
    STATE_ABOVE         = 0x0100,
    STATE_BELOW         = 0x0200,
  } WindowState;
  struct State {
    bool        local_blitting;         /* blitting via shared memory */
    bool        is_active;
    bool        has_toplevel_focus;     /* for embedded windows, this may be false allthough is_active==true */
    WindowState window_state;
    double      width, height;
    constexpr   State();
  };
  /* screen_window configuration */
  typedef enum {
    HINT_DECORATED      = 0x0001,
    HINT_URGENT         = 0x0002,
    HINT_STICKY         = 0x0004,
    HINT_SHADED         = 0x0008,
    HINT_HMAXIMIZED     = 0x0010,
    HINT_VMAXIMIZED     = 0x0020,
    HINT_FULLSCREEN     = 0x0040,       /* no decoration, for presentation */
    HINT_ICONIFY        = 0x0080,       /* minimize */
    HINT_ABOVE_ALL      = 0x0100,
    HINT_BELOW_ALL      = 0x0200,
    HINT_SKIP_TASKBAR   = 0x0400,
    HINT_SKIP_PAGER     = 0x0800,
    HINT_ACCEPT_FOCUS   = 0x1000,
    HINT_UNFOCUSED      = 0x2000,       /* no focus at start up */
    HINT_DELETABLE      = 0x4000,       /* has [X] delete button */
  } WindowHint;
  struct Config {
    bool        modal;
    WindowHint  window_hint;
    String      title;
    String      session_role;
    double      root_x,         root_y;
    double      request_width,  request_height;
    double      min_width,      min_height;
    double      initial_width,  initial_height;
    double      max_width,      max_height;
    double      base_width,     base_height;
    double      width_inc,      height_inc;
    double      min_aspect,     max_aspect;     /* horizontal / vertical */
    Color       average_background;
    inline      Config();
  };
  /* --- frontend API --- */
  struct EventReceiver {
    virtual            ~EventReceiver           () {}
    virtual void        enqueue_async           (Event              *event) = 0;
  };
  /* --- public API --- */
  virtual Info          get_info                () = 0;
  virtual State         get_state               () = 0;
  virtual void          set_config              (const Config   &config,
                                                 bool            force_resize_draw = false) = 0;
  virtual void          beep                    (void) = 0;
  /* creation */
  static ScreenWindow*  create_screen_window    (const String   &backend_name,
                                                 WindowType      screen_window_type,
                                                 EventReceiver  &receiver);
  /* actions */
  virtual void          present_screen_window        () = 0;
  virtual void          trigger_hint_action     (WindowHint     hint) = 0;
  virtual void          start_user_move         (uint           button,
                                                 double         root_x,
                                                 double         root_y) = 0;
  virtual void          start_user_resize       (uint           button,
                                                 double         root_x,
                                                 double         root_y,
                                                 AnchorType     edge) = 0;
  virtual void          show                    (void) = 0;
  virtual bool          visible                 (void) = 0;
  virtual bool          viewable                (void) = 0;
  virtual void          hide                    (void) = 0;
  virtual uint          last_draw_stamp         () = 0;
  virtual void          enqueue_win_draws       (void) = 0;
  virtual void          blit_surface            (cairo_surface_t *surface, Rapicorn::Region region) = 0;
  /* --- backend API --- */
  class FactoryBase : protected NonCopyable {
    friend class ScreenWindow;
  protected:
    virtual              ~FactoryBase          ();
    const String          m_name;
    explicit              FactoryBase          (const String  &name) : m_name (name) {}
    static void           register_backend     (FactoryBase   &factory);
    virtual ScreenWindow* create_screen_window (WindowType     screen_window_type,
                                                EventReceiver &receiver) = 0;
  };
  template<class Backend>
  class Factory : public virtual FactoryBase {
  public:
    explicit              Factory              (const String  &name) : FactoryBase (name) { register_backend (*this); }
    virtual ScreenWindow* create_screen_window (WindowType     screen_window_type,
                                                EventReceiver &receiver) { return new Backend (m_name, screen_window_type, receiver); }
  };
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
  virtual ScreenWindow* create_screen_window    (WindowType screen_window_type, ScreenWindow::EventReceiver &receiver) = 0;
  virtual void          close                   () = 0;
  static ScreenDriver*  open_screen_driver      (const String &backend_name);
  static bool           driver_priority_lesser  (ScreenDriver *d1, ScreenDriver *d2);
};

// == Implementations ==
constexpr
ScreenWindow::State::State() :
  local_blitting (false), is_active (false), has_toplevel_focus (false), window_state (WindowState (0)), width (NAN), height (NAN)
{}

constexpr
ScreenWindow::Info::Info() :
  window_type (WindowType (0))
{}

ScreenWindow::Config::Config() :
  modal (false), window_hint (WindowHint (HINT_DECORATED | HINT_ACCEPT_FOCUS | HINT_DELETABLE)),
  root_x (NAN), root_y (NAN), request_width (33), request_height (33), min_width (0), min_height (0),
  initial_width (0), initial_height (0), max_width (0), max_height (0), base_width (0), base_height (0),
  width_inc (0), height_inc (0), min_aspect (0), max_aspect (0), average_background (0xff808080)
{}


// internal
void rapicorn_gtk_threads_enter         ();
void rapicorn_gtk_threads_leave         ();
void rapicorn_gtk_threads_shutdown      ();

} // Rapicorn

#endif  /* __RAPICORN_SCREEN_WINDOW_HH__ */
