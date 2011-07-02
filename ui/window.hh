// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_WINDOW_HH__
#define __RAPICORN_WINDOW_HH__

#include <ui/container.hh>
#include <ui/loop.hh>
#include <ui/viewp0rt.hh>

namespace Rapicorn {

/* --- Window --- */
class WindowImpl : public virtual SingleContainerImpl, public virtual Wind0wIface,
                   public virtual EventLoop::Source, public virtual Viewp0rt::EventReceiver {
  friend class  ItemImpl;
  EventLoop            &m_loop;
  EventLoop::Source    *m_source;
  Mutex                 m_async_mutex;
  std::list<Event*>     m_async_event_queue;
  Region                m_expose_region;
  Viewp0rt             *m_viewp0rt;
  uint                  m_tunable_requisition_counter : 24;
  uint                  m_entered : 1;
  uint                  m_auto_close : 1;
  EventContext          m_last_event_context;
  vector<ItemImpl*>     m_last_entered_children;
  Viewp0rt::Config      m_config;
  void          uncross_focus           (ItemImpl        &fitem);
protected:
  void          set_focus               (ItemImpl         *item);
  virtual void  set_parent              (ContainerImpl    *parent);
  virtual void  dispose                 ();
public:
  explicit              WindowImpl              ();
  ItemImpl*             get_focus               () const;
  cairo_surface_t*      create_snapshot         (const Rect  &subarea);
  ItemImpl*             find_item               (const String &name);
  virtual Wind0wIface&  wind0w                                  ();     // MT-safe
  // grab handling
  virtual void          add_grab                                (ItemImpl &child, bool unconfined = false);
  void                  add_grab                                (ItemImpl *child, bool unconfined = false);
  virtual void          remove_grab                             (ItemImpl &child);
  void                  remove_grab                             (ItemImpl *child);
  virtual ItemImpl*     get_grab                                (bool                   *unconfined = NULL);
  // main loop
  virtual EventLoop*    get_loop                                ();
  virtual void          enable_auto_close                       ();
  // signals
  typedef Signal<WindowImpl, bool (const String&, const StringVector&), CollectorWhile0<bool> >   CommandSignal;
  typedef Signal<WindowImpl, void ()> NotifySignal;
  NotifySignal          sig_displayed;
private:
  virtual void          remove_grab_item                        (ItemImpl               &child);
  void                  grab_stack_changed                      ();
  /*Des*/               ~WindowImpl                             ();
  virtual void          dispose_item                            (ItemImpl               &item);
  virtual bool          self_visible                            () const;
  /* misc */
  vector<ItemImpl*>     item_difference                         (const vector<ItemImpl*>    &clist, /* preserves order of clist */
                                                                 const vector<ItemImpl*>    &cminus);
  /* sizing */
  virtual void          size_request                            (Requisition            &requisition);
  using                 ItemImpl::size_request;
  virtual void          size_allocate                           (Allocation              area);
  virtual bool          tunable_requisitions                    ();
  void                  resize_all                              (Allocation             *new_area);
  virtual void          do_invalidate                           ();
  virtual void          beep                                    ();
  /* rendering */
  virtual void          render                                  (Display                &display);
  using                 ItemImpl::render;
  void                  collapse_expose_region                  ();
  virtual void          expose_window_region                    (const Region           &region);   // window item coords
  virtual void          copy_area                               (const Rect             &src,
                                                                 const Point            &dest);
  void                  expose_now                              ();
  virtual void          draw_now                                ();
  /* viewp0rt ops */
  virtual void          create_viewp0rt                         ();
  virtual bool          has_viewp0rt                            ();
  virtual void          destroy_viewp0rt                        ();
  virtual bool          viewable                                ();
  void                  idle_show                               ();
  /* main loop */
  virtual bool          prepare                                 (uint64                  current_time_usecs,
                                                                 int64                  *timeout_usecs_p);
  virtual bool          check                                   (uint64                  current_time_usecs);
  virtual bool          dispatch                                ();
  virtual bool          custom_command                          (const String       &command_name,
                                                                 const StringList   &command_args);
  /* Wind0wIface */
  virtual void          show                                    ();
  virtual bool          closed                                  ();
  virtual void          close                                   ();
  virtual bool          synthesize_enter                        (double xalign = 0.5,
                                                                 double yalign = 0.5);
  virtual bool          synthesize_leave                        ();
  virtual bool          synthesize_click                        (ItemIface &item,
                                                                 int        button,
                                                                 double     xalign = 0.5,
                                                                 double     yalign = 0.5);
  virtual bool          synthesize_delete                       ();
  /* event handling */
  virtual void          enqueue_async                           (Event                  *event);
  virtual void          cancel_item_events                      (ItemImpl               *item);
  void                  cancel_item_events                      (ItemImpl &item) { cancel_item_events (&item); }
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
  bool                  dispatch_scroll_event                   (const EventScroll      &sevent);
  bool                  dispatch_win_size_event                 (const Event            &event);
  bool                  dispatch_win_draw_event                 (const Event            &event);
  bool                  dispatch_win_delete_event               (const Event            &event);
  virtual bool          dispatch_event                          (const Event            &event);
  bool                  has_pending_win_size                    ();
  /* --- GrabEntry --- */
  struct GrabEntry {
    ItemImpl *item;
    bool  unconfined;
    explicit            GrabEntry (ItemImpl *i, bool uc) : item (i), unconfined (uc) {}
  };
  vector<GrabEntry>     m_grab_stack;
  /* --- ButtonState --- */
  struct ButtonState {
    ItemImpl           *item;
    uint                button;
    explicit            ButtonState     (ItemImpl *i, uint b) : item (i), button (b) {}
    explicit            ButtonState     () : item (NULL), button (0) {}
    bool                operator< (const ButtonState &bs2) const
    {
      const ButtonState &bs1 = *this;
      return bs1.item < bs2.item || (bs1.item == bs2.item &&
                                     bs1.button < bs2.button);
    }
  };
  map<ButtonState,uint> m_button_state_map;
  /* --- EventLoop Source --- */
  class WindowSource : public EventLoop::Source {
    WindowImpl &window;
  public:
    explicit
    WindowSource  (WindowImpl &_window) :
      window (_window)
    {
      bool entered = rapicorn_thread_entered();
      if (!entered)
        rapicorn_thread_enter();
      RAPICORN_ASSERT (window.m_source == NULL);
      window.m_source = this;
      if (!entered)
        rapicorn_thread_leave();
    }
    virtual
    ~WindowSource ()
    {
      bool entered = rapicorn_thread_entered();
      if (!entered)
        rapicorn_thread_enter();
      RAPICORN_ASSERT (window.m_source != this);
      if (!entered)
        rapicorn_thread_leave();
    }
    virtual bool prepare    (uint64 current_time_usecs,
                             int64 *timeout_usecs_p)         { RAPICORN_ASSERT (rapicorn_thread_entered()); return window.prepare (current_time_usecs, timeout_usecs_p); }
    virtual bool check      (uint64 current_time_usecs)      { RAPICORN_ASSERT (rapicorn_thread_entered()); return window.check (current_time_usecs); }
    virtual bool dispatch   ()                               { RAPICORN_ASSERT (rapicorn_thread_entered()); return window.dispatch(); }
    virtual void
    destroy ()
    {
      RAPICORN_ASSERT (rapicorn_thread_entered());
      RAPICORN_ASSERT (window.m_source == this);
      window.m_source = NULL;
      window.destroy_viewp0rt();
    }
  };
};

} // Rapicorn

#endif  /* __RAPICORN_WINDOW_HH__ */
