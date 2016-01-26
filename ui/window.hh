// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_WINDOW_HH__
#define __RAPICORN_WINDOW_HH__

#include <ui/viewport.hh>
#include <ui/displaywindow.hh>

namespace Rapicorn {

class WindowImpl;
typedef std::shared_ptr<WindowImpl> WindowImplP;
typedef std::weak_ptr<WindowImpl>   WindowImplW;

/* --- Window --- */
class WindowImpl : public virtual ViewportImpl, public virtual WindowIface {
  const EventLoopP      loop_;
  ThemeInfoP            theme_info_;
  DisplayWindow*        display_window_;
  EventContext          last_event_context_;
  Signal_commands::Emission *commands_emission_;
  String                     last_command_;
  vector<WidgetImplP>   last_entered_children_;
  DisplayWindow::Config config_;
  size_t                immediate_event_hash_;
  uint                  auto_focus_ : 1;
  uint                  entered_ : 1;
  uint                  pending_win_size_ : 1;
  uint                  pending_expose_ : 1;
  void                  uncross_focus           (WidgetImpl &fwidget);
protected:
  void                  set_focus               (WidgetImpl *widget);
  virtual void          set_parent              (ContainerImpl *parent);
  virtual void          construct               () override;
  virtual void          dispose                 () override;
public:
  static const int      PRIORITY_RESIZE         = EventLoop::PRIORITY_UPDATE - 1; ///< Execute resizes right before GUI updates.
  explicit              WindowImpl              ();
  virtual              ~WindowImpl              () override;
  virtual WindowImpl*   as_window_impl          ()              { return this; }
  WidgetImpl*           get_focus               () const;
  cairo_surface_t*      create_snapshot         (const Rect  &subarea);
  static  void          forcefully_close_all    ();
  // properties
  virtual String        title                   () const override;
  virtual void          title                   (const String &window_title) override;
  virtual bool          auto_focus              () const override;
  virtual void          auto_focus              (bool afocus) override;
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
  virtual void          remove_grab_widget                      (WidgetImpl               &child);
  void                  grab_stack_changed                      ();
  virtual void          dispose_widget                          (WidgetImpl               &widget);
  /* misc */
  vector<WidgetImplP>   widget_difference                       (const vector<WidgetImplP>    &clist, /* preserves order of clist */
                                                                 const vector<WidgetImplP>    &cminus);
  /* sizing */
  void                  resize_window                           (const Allocation *new_area = NULL);
  virtual void          do_invalidate                           ();
  virtual void          beep                                    ();
  /* rendering */
  virtual void          draw_now                                ();
  virtual void          render                                  (RenderContext &rcontext, const Rect &rect);
  /* display_window ops */
  virtual void          create_display_window                    ();
  virtual bool          has_display_window                       ();
  virtual void          destroy_display_window                   ();
  void                  async_show                               ();
  /* main loop */
  void                  push_immediate_event                    (Event *event);
  void                  clear_immediate_event                   ();
  bool                  immediate_event_dispatcher              (const LoopState &state);
  virtual bool          event_dispatcher                        (const LoopState &state);
  virtual bool          resizing_dispatcher                     (const LoopState &state);
  virtual bool          drawing_dispatcher                      (const LoopState &state);
  virtual bool          command_dispatcher                      (const LoopState &state);
  virtual bool          custom_command                          (const String    &command_name,
                                                                 const StringSeq &command_args);
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
  bool                  move_focus_dir                          (FocusDir            focus_dir);
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
    friend                class WidgetImpl; // only friends can access private class members
    static DisplayWindow* display_window (WindowImpl &window)                     { return window.display_window_; }
    static void           set_focus     (WindowImpl &window, WidgetImpl *widget) { window.set_focus (widget); }
  };
  /// @endcond
};

} // Rapicorn

#endif  /* __RAPICORN_WINDOW_HH__ */
