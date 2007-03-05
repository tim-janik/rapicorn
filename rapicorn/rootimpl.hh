/* Rapicorn
 * Copyright (C) 2005-2006 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#ifndef __RAPICORN_ROOT_IMPL_HH__
#define __RAPICORN_ROOT_IMPL_HH__

#include <rapicorn/root.hh>
#include <rapicorn/containerimpl.hh>
#include <rapicorn/viewport.hh>

namespace Rapicorn {

class RootImpl : public virtual Root,
                 public virtual SingleContainerImpl,
                 public virtual Viewport::EventReceiver
{
  OwnedMutex            m_omutex;
  bool                  m_entered;
  Viewport             *m_viewport;
  Mutex                 m_async_mutex;
  std::list<Event*>     m_async_event_queue;
  MainLoop             *m_async_loop;
  uint                  m_asnyc_resize_draw_id;
  MainLoop::Source     *m_source;
  Region                m_expose_region;
  EventContext          m_last_event_context;
  vector<Item*>         m_last_entered_children;
  uint                  m_tunable_requisition_counter;
  Viewport::Config      m_config;
public:
  explicit              RootImpl                                ();
private:
  /*Des*/               ~RootImpl                               ();
  virtual void          dispose_item                            (Item                   &item);
  /* misc */
  virtual OwnedMutex&   owned_mutex                             ();
  vector<Item*>         item_difference                         (const vector<Item*>    &clist, /* preserves order of clist */
                                                                 const vector<Item*>    &cminus);
  /* sizing */
  virtual void          size_request                            (Requisition            &requisition);
  using                 Item::size_request;
  virtual void          size_allocate                           (Allocation              area);
  virtual bool          tunable_requisitions                    ();
  void                  resize_all                              (Allocation             *new_area);
  virtual void          do_invalidate                           ();
  void                  async_resize_draw                       ();
  virtual void          beep                                    ();
  /* rendering */
  virtual void          render                                  (Plane                  &plane);
  using                 Item::render;
  virtual void          expose_root_region                      (const Region           &region);
  virtual void          copy_area                               (const Rect             &src,
                                                                 const Point            &dest);
  void                  expose_now                              ();
  virtual void          draw_now                                ();
  /* grab handling */
  virtual void          remove_grab_item                        (Item                   &child);
  void                  grab_stack_changed                      ();
  virtual void          add_grab                                (Item                   &child,
                                                                 bool                    unconfined);
  virtual void          remove_grab                             (Item                   &child);
  virtual Item*         get_grab                                (bool                   *unconfined = NULL);
  /* main loop */
  virtual void          show                                    ();
  virtual bool          visible                                 ();
  virtual void          hide                                    ();
  virtual bool          closed                                  ();
  virtual void          close                                   ();
  void                  idle_show                               ();
  virtual Window        window                                  ();
  virtual bool          prepare                                 (uint64                  current_time_usecs,
                                                                 int64                  *timeout_usecs_p);
  virtual bool          check                                   (uint64                  current_time_usecs);
  virtual bool          dispatch                                ();
  virtual MainLoop*     get_loop                                ();
  /* event handling */
  virtual void          enqueue_async                           (Event                  *event);
  virtual void          cancel_item_events                      (Item                   *item);
  using                 Root::cancel_item_events;
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
  void                  handle_focus_key                        (const EventKey         &kevent);
  bool                  dispatch_key_event                      (const Event            &event);
  bool                  dispatch_scroll_event                   (const EventScroll      &sevent);
  bool                  dispatch_win_size_event                 (const Event            &event);
  bool                  dispatch_win_draw_event                 (const Event            &event);
  bool                  dispatch_win_delete_event               (const Event            &event);
  virtual bool          dispatch_event                          (const Event            &event);
  bool                  has_pending_win_size                    ();
  /* --- GrabEntry --- */
  struct GrabEntry {
    Item *item;
    bool  unconfined;
    explicit            GrabEntry (Item *i, bool uc) : item (i), unconfined (uc) {}
  };
  vector<GrabEntry>     m_grab_stack;
  /* --- ButtonState --- */
  struct ButtonState {
    Item               *item;
    uint                button;
    explicit            ButtonState     (Item *i, uint b) : item (i), button (b) {}
    explicit            ButtonState     () : item (NULL), button (0) {}
    bool                operator< (const ButtonState &bs2) const
    {
      const ButtonState &bs1 = *this;
      return bs1.item < bs2.item || (bs1.item == bs2.item &&
                                     bs1.button < bs2.button);
    }
  };
  map<ButtonState,uint> m_button_state_map;
  /* --- MainLoop Source --- */
  class RootSource : public MainLoop::Source {
    RootImpl &root;
  public:
    explicit            RootSource  (RootImpl &_root) : root (_root) { AutoLocker locker (root.m_omutex); root.m_source = this; }
    virtual             ~RootSource ()                               { AutoLocker locker (root.m_omutex); root.m_source = NULL; }
    virtual bool        prepare     (uint64 current_time_usecs,
                                     int64 *timeout_usecs_p)     { AutoLocker locker (root.m_omutex); return root.prepare (current_time_usecs, timeout_usecs_p); }
    virtual bool        check       (uint64 current_time_usecs)  { AutoLocker locker (root.m_omutex); return root.check (current_time_usecs); }
    virtual bool        dispatch    ()                           { AutoLocker locker (root.m_omutex); return root.dispatch(); }
  };
};


} // Rapicorn

#endif  /* __RAPICORN_ROOT_IMPL_HH__ */
