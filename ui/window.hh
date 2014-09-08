// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_WINDOW_HH__
#define __RAPICORN_WINDOW_HH__

#include <ui/viewport.hh>
#include <ui/screenwindow.hh>

namespace Rapicorn {

/* --- Window --- */
class WindowImpl : public virtual ViewportImpl, public virtual WindowIface {
  EventLoop            &loop_;
  ScreenWindow*         screen_window_;
  EventContext          last_event_context_;
  Signal_commands::Emission *commands_emission_;
  String                     last_command_;
  vector<WidgetImpl*>     last_entered_children_;
  ScreenWindow::Config  config_;
  uint                  notify_displayed_id_;
  uint                  auto_focus_ : 1;
  uint                  entered_ : 1;
  uint                  pending_win_size_ : 1;
  uint                  pending_expose_ : 1;
  void          uncross_focus           (WidgetImpl        &fwidget);
protected:
  void          set_focus               (WidgetImpl         *widget);
  virtual void  set_parent              (ContainerImpl    *parent);
  virtual void  dispose                 ();
public:
  static const int      PRIORITY_RESIZE         = EventLoop::PRIORITY_UPDATE - 1; ///< Execute resizes right before GUI updates.
  explicit              WindowImpl              ();
  virtual WindowImpl*   as_window_impl          ()              { return this; }
  WidgetImpl*             get_focus               () const;
  cairo_surface_t*      create_snapshot         (const Rect  &subarea);
  static  void          forcefully_close_all    ();
  // properties
  virtual String        title                   () const;
  virtual void          title                   (const String &window_title);
  virtual bool          auto_focus              () const;
  virtual void          auto_focus              (bool afocus);
  // grab handling
  virtual void          add_grab                                (WidgetImpl &child, bool unconfined = false);
  void                  add_grab                                (WidgetImpl *child, bool unconfined = false);
  virtual void          remove_grab                             (WidgetImpl &child);
  void                  remove_grab                             (WidgetImpl *child);
  virtual WidgetImpl*   get_grab                                (bool                   *unconfined = NULL);
  // main loop
  virtual EventLoop*    get_loop                                ();
  // signals
  typedef Aida::Signal<void ()> NotifySignal;
  /* WindowIface */
  virtual bool          screen_viewable                         ();
  virtual void          show                                    ();
  virtual bool          closed                                  ();
  virtual void          close                                   ();
  virtual bool          snapshot                                (const String &pngname);
  virtual bool          synthesize_enter                        (double xalign = 0.5,
                                                                 double yalign = 0.5);
  virtual bool          synthesize_leave                        ();
  virtual bool          synthesize_click                        (WidgetIface &widget,
                                                                 int        button,
                                                                 double     xalign = 0.5,
                                                                 double     yalign = 0.5);
  virtual bool          synthesize_delete                       ();
  void                  draw_child                              (WidgetImpl &child);
private:
  void                  notify_displayed                        (void);
  virtual void          remove_grab_widget                      (WidgetImpl               &child);
  void                  grab_stack_changed                      ();
  virtual              ~WindowImpl                              ();
  virtual void          dispose_widget                          (WidgetImpl               &widget);
  /* misc */
  vector<WidgetImpl*>   widget_difference                       (const vector<WidgetImpl*>    &clist, /* preserves order of clist */
                                                                 const vector<WidgetImpl*>    &cminus);
  /* sizing */
  void                  resize_window                           (const Allocation *new_area = NULL);
  virtual void          do_invalidate                           ();
  virtual void          beep                                    ();
  /* rendering */
  virtual void          draw_now                                ();
  virtual void          render                                  (RenderContext &rcontext, const Rect &rect);
  /* screen_window ops */
  virtual void          create_screen_window                    ();
  virtual bool          has_screen_window                       ();
  virtual void          destroy_screen_window                   ();
  void                  idle_show                               ();
  /* main loop */
  virtual bool          event_dispatcher                        (const EventLoop::State &state);
  virtual bool          resizing_dispatcher                     (const EventLoop::State &state);
  virtual bool          drawing_dispatcher                      (const EventLoop::State &state);
  virtual bool          command_dispatcher                      (const EventLoop::State &state);
  virtual bool          custom_command                          (const String       &command_name,
                                                                 const StringSeq    &command_args);
  /* event handling */
  virtual void          cancel_widget_events                      (WidgetImpl               *widget);
  void                  cancel_widget_events                      (WidgetImpl &widget) { cancel_widget_events (&widget); }
  bool                  dispatch_mouse_movement                 (const Event            &event);
  bool                  dispatch_event_to_pierced_or_grab       (const Event            &event);
  bool                  dispatch_button_press                   (const EventButton      &bevent);
  bool                  dispatch_button_release                 (const EventButton      &bevent);
  bool                  dispatch_cancel_event                   (const Event            &event);
  bool                  dispatch_enter_event                    (const EventMouse       &mevent);
  bool                  dispatch_move_event                     (const EventMouse       &mevent);
  bool                  dispatch_leave_event                    (const EventMouse       &mevent);
  bool                  dispatch_button_event                   (const Event            &event);
  bool                  dispatch_focus_event                    (const EventFocus       &fevent);
  bool                  move_focus_dir                          (FocusDirType            focus_dir);
  bool                  dispatch_key_event                      (const Event            &event);
  bool                  dispatch_data_event                     (const Event            &event);
  bool                  dispatch_scroll_event                   (const EventScroll      &sevent);
  bool                  dispatch_win_size_event                 (const Event            &event);
  bool                  dispatch_win_delete_event               (const Event            &event);
  bool                  dispatch_win_destroy                    ();
  virtual bool          dispatch_event                          (const Event            &event);
  bool                  has_queued_win_size                     ();
  /* --- GrabEntry --- */
  struct GrabEntry {
    WidgetImpl *widget;
    bool  unconfined;
    explicit            GrabEntry (WidgetImpl *i, bool uc) : widget (i), unconfined (uc) {}
  };
  vector<GrabEntry>     grab_stack_;
  /* --- ButtonState --- */
  struct ButtonState {
    WidgetImpl           *widget;
    uint                button;
    explicit            ButtonState     (WidgetImpl *i, uint b) : widget (i), button (b) {}
    explicit            ButtonState     () : widget (NULL), button (0) {}
    bool                operator< (const ButtonState &bs2) const
    {
      const ButtonState &bs1 = *this;
      return bs1.widget < bs2.widget || (bs1.widget == bs2.widget &&
                                     bs1.button < bs2.button);
    }
  };
  map<ButtonState,uint> button_state_map_;
public: // tailored member access for WidgetImpl
  /// @cond INTERNAL
  class Internal {
    friend               class WidgetImpl; // only friends can access private class members
    static ScreenWindow* screen_window (WindowImpl &window)                     { return window.screen_window_; }
    static void          set_focus     (WindowImpl &window, WidgetImpl *widget) { window.set_focus (widget); }
  };
  /// @endcond
};

} // Rapicorn

#endif  /* __RAPICORN_WINDOW_HH__ */
