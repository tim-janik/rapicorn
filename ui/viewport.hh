// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_VIEWPORT_HH__
#define __RAPICORN_VIEWPORT_HH__

#include <ui/container.hh>
#include <ui/displaywindow.hh>

namespace Rapicorn {

class ViewportImpl;
typedef std::shared_ptr<ViewportImpl> ViewportImplP;
typedef std::weak_ptr<ViewportImpl>   ViewportImplW;

// == ViewportImpl ==
class ViewportImpl : public virtual ResizeContainerImpl, public virtual ViewportIface {
  struct                                GrabEntry;
  struct                                ButtonState;
  typedef std::map<ButtonState,uint>    ButtonStateMap;
  Region                                expose_region_;
  DisplayWindow                        *display_window_;
  vector<WidgetImplP>                   last_entered_children_;
  EventContext                          last_event_context_;
  ButtonStateMap                        button_state_map_;
  vector<GrabEntry>                     grab_stack_;
  DisplayWindow::Config                 dw_config_;
  size_t                                immediate_event_hash_;
  uint                                  event_dispatcher_id_, drawing_dispatcher_id_;
  uint                                  tunable_requisition_counter_ : 8;
  uint                                  auto_focus_ : 1;
  uint                                  entered_ : 1;
  uint                                  pending_win_size_ : 1;
  uint                                  pending_expose_ : 1;
  uint                                  need_resize_ : 1;
  WidgetFlag                   pop_need_resize            ();
  WidgetFlag                   negotiate_sizes            (const Allocation *new_window_area);
  void                         resize_redraw              (const Allocation *new_window_area, bool resize_only = false);
  bool                         can_resize_redraw          ();
  void                         maybe_resize_viewport      ();
  void                         negotiate_initial_size     ();
  void                         collapse_expose_region     ();
  const Region&                peek_expose_region         () const       { return expose_region_; }
  void                         discard_expose_region      ()             { expose_region_.clear(); }
  bool                         exposes_pending            () const       { return !expose_region_.empty(); }
  static WidgetFlag            check_widget_requisition   (WidgetImpl &widget, bool discard_tuned);
  WidgetFlag                   check_widget_allocation    (WidgetImpl &widget);
  void                         uncross_focus              (WidgetImpl &fwidget);
  void                         draw_now                   ();
  void                         show_display_window        ();
  bool                         drawing_dispatcher         (const LoopState &state);
protected:
  virtual const AncestryCache* fetch_ancestry_cache       () override;
  virtual void                 hierarchy_changed          (WidgetImpl *old_toplevel) override;
  virtual void                 render                     (RenderContext &rcontext);
  virtual void                 dispose_widget             (WidgetImpl &widget);
  void                         push_immediate_event       (Event *event);
  void                         ensure_resized             ();
  virtual void                 create_display_window      ();
  virtual void                 destroy_display_window     ();
public:
  explicit                     ViewportImpl               ();
  virtual                     ~ViewportImpl               () override;
  virtual ViewportImpl*        as_viewport_impl           () override     { return this; }
  // properties
  virtual String               title                      () const override;
  virtual void                 title                      (const String &viewport_title) override;
  virtual bool                 auto_focus                 () const override;
  virtual void                 auto_focus                 (bool afocus) override;
  // ViewportIface
  virtual bool                 screen_viewable            () override;
  virtual void                 show                       () override;
  virtual bool                 closed                     () override;
  virtual void                 close                      () override;
  virtual void                 destroy                    () override;
  virtual bool                 snapshot                   (const String &pngname) override;
  virtual void                 query_idle                 () override;
  bool                         has_display_window         () const        { return display_window_ != NULL; }
  // grab & focus handling
  virtual void                 add_grab                   (WidgetImpl &child, bool constrained = true);
  virtual bool                 remove_grab                (WidgetImpl &child);
  bool                         remove_grab                (WidgetImpl *child);
  virtual WidgetImpl*          get_grab                   (bool *constrained = NULL);
  bool                         is_grabbing                (WidgetImpl &descendant);
  virtual void                 unset_focus                () override     { set_focus (NULL); }
  virtual WidgetIfaceP         get_focus                  () override;
  WidgetImpl*                  get_focus_widget           ()              { return shared_ptr_cast<WidgetImpl> (get_focus()).get(); }
  virtual WidgetIfaceP         get_entered                () override;
  WidgetImpl*                  get_entered_widget         ()              { return shared_ptr_cast<WidgetImpl> (get_entered()).get(); }
  // resize and draw
  void                         expose_region              (const Region &region);
  cairo_surface_t*             create_snapshot            (const IRect  &subarea);
  bool                         requisitions_tunable       () const        { return tunable_requisition_counter_ > 0; }
  void                         draw_child                 (WidgetImpl &child);
  void                         queue_resize_redraw        ();
  // internal API
  DisplayWindow*               display_window             (Internal = Internal()) const;
  void                         set_focus                  (WidgetImpl *widget, Internal = Internal());
private:
  // event handling
  bool                         has_queued_win_size        ();
  bool                         dispatch_mouse_movement    (const Event &event);
  bool                         dispatch_event_to_entered  (const Event &event);
  bool                         dispatch_button_press      (const EventButton &bevent);
  bool                         dispatch_button_release    (const EventButton &bevent);
  bool                         dispatch_move_event        (const EventMouse &mevent);
  bool                         move_container_focus       (ContainerImpl &container, FocusDir focus_dir);
  bool                         dispatch_key_event         (const Event &event);
  bool                         dispatch_win_size_event    (const Event &event);
  bool                         dispatch_event             (const Event &event);
  bool                         event_dispatcher           (const LoopState &state);
  bool                         immediate_event_dispatcher (const LoopState &state);
  void                         clear_immediate_event      ();
  void                         cancel_widget_events       (WidgetImpl *widget);
  void                         cancel_widget_events       (WidgetImpl &widget)    { cancel_widget_events (&widget); }
  void                         remove_grab_widget         (WidgetImpl &child);
  void                         grab_stack_changed         ();
  ButtonStateMap::iterator     button_state_map_earliest  (const uint button, const bool captured);
  // ButtonState
  struct ButtonState {
    WidgetImpl                &widget;
    const uint64               serializer;
    const uint                 button;
    const bool                 captured;
    explicit                   ButtonState (uint64 s, WidgetImpl &w, uint b, bool c = false) : widget (w), serializer (s), button (b), captured (c) {}
    explicit                   ButtonState () = delete;
    bool                       operator<   (const ButtonState &bs2) const;
  };
  // GrabEntry
  struct GrabEntry {
    WidgetImpl                *widget;
    bool                       constrained;
    explicit                   GrabEntry   (WidgetImpl &wid, bool con) : widget (&wid), constrained (con) {}
  };
};

} // Rapicorn

#endif  /* __RAPICORN_VIEWPORT_HH__ */
