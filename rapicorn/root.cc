/* Rapicorn
 * Copyright (C) 2005 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "root.hh"
#include "containerimpl.hh"
using namespace std;

namespace Rapicorn {

Root::Root() :
  sig_expose (*this)
{
  change_flags_silently (ANCHORED, true);       /* root is always anchored */
}

class RootImpl : public Root, public SingleContainerImpl {
  bool m_entered;
public:
  RootImpl() :
    m_entered (false)
  {
    Appearance *appearance = Appearance::create_default();
    style (appearance->create_style ("normal"));
    unref (appearance);
    set_flag (PARENT_SENSITIVE, true);
  }
  ~RootImpl()
  {
    /* make sure all children are removed while this is still of type Root.
     * necessary because C++ alters the object type during constructors and destructors
     */
    if (has_children())
      remove (get_child());
  }
  virtual void
  size_request (Requisition &requisition)
  {
    if (has_visible_child())
      {
        Item &child = get_child();
        requisition = child.size_request();
      }
  }
  virtual void
  size_allocate (Allocation area)
  {
    allocation (area);
    if (!has_visible_child())
      return;
    Item &child = get_child();
    Requisition rq = child.size_request();
    child.set_allocation (area);
  }
  virtual void
  expose (const Allocation &area)
  {
    sig_expose.emit (area);
  }
protected:
  vector<Item*>
  item_difference (const vector<Item*> &clist, /* preserve order of clist */
                   const vector<Item*> &cminus)
  {
    map<Item*,bool> mminus;
    for (uint i = 0; i < cminus.size(); i++)
      mminus[cminus[i]] = true;
    vector<Item*> result;
    for (uint i = 0; i < clist.size(); i++)
      if (!mminus[clist[i]])
        result.push_back (clist[i]);
    return result;
  }
  EventContext last_event_context;
  vector<Item*> last_entered_children;
  bool
  dispatch_mouse_movement (const Event &event)
  {
    last_event_context = event;
    vector<Item*> pierced;
    /* figure all entered children */
    bool unconfined;
    Item *grab_item = get_grab (&unconfined);
    if (grab_item)
      {
        if (unconfined or grab_item->point (event.x, event.y, Affine()))
          {
            pierced.push_back (ref (grab_item));        /* grab-item receives all mouse events */
            Container *container = grab_item->interface<Container*>();
            if (container)                              /* deliver to hovered grab-item children as well */
              container->point_children (event.x, event.y, Affine(), pierced);
          }
      }
    else if (drawable())
      {
        pierced.push_back (ref (this)); /* root receives all mouse events */
        if (m_entered)
          point_children (event.x, event.y, Affine(), pierced);
      }
    /* send leave events */
    vector<Item*> left_children = item_difference (last_entered_children, pierced);
    EventMouse *mevent = create_event_mouse (MOUSE_LEAVE, EventContext (event));
    for (vector<Item*>::reverse_iterator it = left_children.rbegin(); it != left_children.rend(); it++)
      (*it)->process_event (*mevent);
    /* send enter events */
    vector<Item*> entered_children = item_difference (pierced, last_entered_children);
    mevent->type = MOUSE_ENTER;
    for (vector<Item*>::reverse_iterator it = entered_children.rbegin(); it != entered_children.rend(); it++)
      (*it)->process_event (*mevent);
    /* send actual move event */
    bool handled = false;
    mevent->type = MOUSE_MOVE;
    for (vector<Item*>::reverse_iterator it = pierced.rbegin(); it != pierced.rend(); it++)
      if (!handled && (*it)->sensitive())
        handled = (*it)->process_event (*mevent);
    /* cleanup */
    mevent->type = MOUSE_LEAVE;
    delete mevent;
    for (vector<Item*>::reverse_iterator it = last_entered_children.rbegin(); it != last_entered_children.rend(); it++)
      (*it)->unref();
    last_entered_children = pierced;
    return handled;
  }
  bool
  dispatch_event_to_pierced_or_grab (const Event &event)
  {
    vector<Item*> pierced;
    /* figure all entered children */
    Item *grab_item = get_grab();
    if (grab_item)
      pierced.push_back (ref (grab_item));
    else if (drawable())
      {
        pierced.push_back (ref (this)); /* root receives all events */
        point_children (event.x, event.y, Affine(), pierced);
      }
    /* send actual event */
    bool handled = false;
    for (vector<Item*>::reverse_iterator it = pierced.rbegin(); it != pierced.rend(); it++)
      {
        if (!handled && (*it)->sensitive())
          handled = (*it)->process_event (event);
        (*it)->unref();
      }
    return handled;
  }
private:
  struct ButtonState {
    Item *item;
    uint button;
    ButtonState (Item *i, uint b) : item (i), button (b) {}
    ButtonState () : item (NULL), button (0) {}
    bool operator< (const ButtonState &bs2) const
    {
      const ButtonState &bs1 = *this;
      return bs1.item < bs2.item || (bs1.item == bs2.item &&
                                     bs1.button < bs2.button);
    }
  };
  map<ButtonState,uint> button_state_map;
  bool
  dispatch_button_press (const EventButton &bevent)
  {
    uint press_count = bevent.type - BUTTON_PRESS + 1;
    assert (press_count >= 1 && press_count <= 3);
    /* figure all entered children */
    const vector<Item*> &pierced = last_entered_children;
    /* send actual event */
    bool handled = false;
    for (vector<Item*>::const_reverse_iterator it = pierced.rbegin(); it != pierced.rend(); it++)
      if (!handled && (*it)->sensitive())
        {
          ButtonState bs (*it, bevent.button);
          if (button_state_map[bs] == 0)                /* no press delivered for <button> on <item> yet */
            {
              button_state_map[bs] = press_count;       /* record single press */
              handled = (*it)->process_event (bevent);
            }
        }
    return handled;
  }
  bool
  dispatch_button_release (const EventButton &bevent)
  {
    bool handled = false;
    for (map<ButtonState,uint>::iterator it = button_state_map.begin(); it != button_state_map.end();)
      {
        const ButtonState &bs = it->first;
        // uint press_count = it->second;
        map<ButtonState,uint>::iterator current = it++;
        if (bs.button == bevent.button)
          {
#if 0 // FIXME
            if (press_count == 3)
              bevent.type = BUTTON_3RELEASE;
            else if (press_count == 2)
              bevent.type = BUTTON_2RELEASE;
#endif
            handled |= bs.item->process_event (bevent);
            button_state_map.erase (current);
          }
      }
    // bevent.type = BUTTON_RELEASE;
    return handled;
  }
  void
  cancel_item_events (Item *item)
  {
    /* cancel enter events */
    for (int i = last_entered_children.size(); i > 0;)
      {
        Item *current = last_entered_children[--i]; /* walk backwards */
        if (item == current || !item)
          {
            EventMouse *mevent = create_event_mouse (MOUSE_LEAVE, last_event_context);
            current->process_event (*mevent);
            delete mevent;
            current->unref();
            last_entered_children.erase (last_entered_children.begin() + i);
          }
      }
    /* cancel button press events */
    for (map<ButtonState,uint>::iterator it = button_state_map.begin(); it != button_state_map.end();)
      {
        const ButtonState &bs = it->first;
        map<ButtonState,uint>::iterator current = it++;
        if (bs.item == item || !item)
          {
            EventButton *bevent = create_event_button (BUTTON_CANCELED, last_event_context, bs.button);
            bs.item->process_event (*bevent);
            delete bevent;
            button_state_map.erase (current);
          }
      }
  }
  virtual void
  cancel_item_events (Item &item)
  {
    cancel_item_events (&item);
  }
  bool
  dispatch_cancel_event (const Event &event)
  {
    cancel_item_events (NULL);
    return false;
  }
  bool
  dispatch_enter_event (const EventMouse &mevent)
  {
    m_entered = true;
    dispatch_mouse_movement (mevent);
    return false;
  }
  virtual bool
  dispatch_move_event (const EventMouse &mevent)
  {
    dispatch_mouse_movement (mevent);
    return false;
  }
  bool
  dispatch_leave_event (const EventMouse &mevent)
  {
    dispatch_mouse_movement (mevent);
    m_entered = false;
    if (get_grab ())
      ; /* leave events in grab mode are automatically calculated */
    else
      {
        /* send leave events */
        while (last_entered_children.size())
          {
            Item *item = last_entered_children.back();
            last_entered_children.pop_back();
            item->process_event (mevent);
            item->unref();
          }
      }
    return false;
  }
  bool
  dispatch_button_event (const Event &event)
  {
    bool handled = false;
    const EventButton *bevent = dynamic_cast<const EventButton*> (&event);
    if (bevent)
      {
        dispatch_mouse_movement (*bevent);
        if (bevent->type >= BUTTON_PRESS && bevent->type <= BUTTON_3PRESS)
          handled = dispatch_button_press (*bevent);
        else
          handled = dispatch_button_release (*bevent);
        dispatch_mouse_movement (*bevent);
      }
    return handled;
  }
  bool
  dispatch_focus_event (const EventFocus &fevent)
  {
    bool handled = false;
    // dispatch_event_to_pierced_or_grab (*fevent);
    dispatch_mouse_movement (fevent);
    return handled;
  }
  bool
  dispatch_key_event (const Event &event)
  {
    bool handled = false;
    const EventKey *kevent = dynamic_cast<const EventKey*> (&event);
    if (kevent)
      {
        dispatch_mouse_movement (*kevent);
        Item *grab_item = get_grab();
        grab_item = grab_item ? grab_item : this;
        handled = grab_item->process_event (*kevent);
      }
    return handled;
  }
  bool
  dispatch_scroll_event (const EventScroll &sevent)
  {
    bool handled = false;
    if (sevent.type == SCROLL_UP || sevent.type == SCROLL_RIGHT ||
        sevent.type == SCROLL_DOWN || sevent.type == SCROLL_LEFT)
      {
        dispatch_mouse_movement (sevent);
        handled = dispatch_event_to_pierced_or_grab (sevent);
      }
    return handled;
  }
  bool
  dispatch_win_size_event (const Event &event)
  {
    bool handled = false;
    const EventWinSize *wevent = dynamic_cast<const EventWinSize*> (&event);
    if (wevent)
      {
        Allocation allocation (0, 0, wevent->width, wevent->height);
        set_allocation (allocation);
        handled = true;
      }
    return handled;
  }
private:
  struct GrabEntry {
    Item *item;
    bool  unconfined;
    GrabEntry (Item *i, bool uc) :
      item (i),
      unconfined (uc)
    {}
  };
  vector<GrabEntry> grab_stack;
  virtual void
  remove_grab_item (Item &child)
  {
    bool stack_changed = false;
    for (int i = grab_stack.size() - 1; i >= 0; i--)
      if (grab_stack[i].item == &child)
        {
          grab_stack.erase (grab_stack.begin() + i);
          stack_changed = true;
        }
    if (stack_changed)
      grab_stack_changed();
  }
  void
  grab_stack_changed()
  {
    // FIXME: use idle handler for event synthesis
    EventMouse *mevent = create_event_mouse (MOUSE_LEAVE, last_event_context);
    dispatch_mouse_movement (*mevent);
    /* synthesize neccessary leaves after grabbing */
    if (!grab_stack.size() && !m_entered)
      dispatch_event (*mevent);
    delete mevent;
  }
public:
  virtual void
  add_grab (Item &child,
            bool  unconfined)
  {
    if (!child.has_ancestor (*this))
      throw Exception ("child is not descendant of container \"", name(), "\": ", child.name());
    /* for unconfined==true grabs, the mouse pointer is always considered to
     * be contained by the grab-item, and only by the grab-item. events are
     * delivered to the grab-item and its children.
     */
    grab_stack.push_back (GrabEntry (&child, unconfined));
    // grab_stack_changed(); // FIXME: re-enable this, once grab_stack_changed() synthesizes from idler
  }
  virtual void
  remove_grab (Item &child)
  {
    for (int i = grab_stack.size() - 1; i >= 0; i--)
      if (grab_stack[i].item == &child)
        {
          grab_stack.erase (grab_stack.begin() + i);
          grab_stack_changed();
          return;
        }
    throw Exception ("no such child in grab stack: ", child.name());
  }
  virtual Item*
  get_grab (bool *unconfined = NULL)
  {
    for (int i = grab_stack.size() - 1; i >= 0; i--)
      if (grab_stack[i].item->visible())
        {
          if (unconfined)
            *unconfined = grab_stack[i].unconfined;
          return grab_stack[i].item;
        }
    return NULL;
  }
  virtual void
  dispose_item (Item &item)
  {
    remove_grab_item (item);
    cancel_item_events (item);
    SingleContainerImpl::dispose_item (item);
  }
  using Item::render;
  virtual void
  render (Plane &plane)
  {
    plane.fill (background());
    Display display;
    const Allocation &a = allocation();
    display.push_clip_rect (a.x, a.y, a.width, a.height);
    display.push_clip_rect (plane.rect());
    if (!display.empty())
      render (display);
    display.render_combined (plane);
    display.pop_clip_rect();
    display.pop_clip_rect();
  }
  virtual bool
  dispatch_event (const Event &event)
  {
    switch (event.type)
      {
      case EVENT_NONE:          return false;
      case MOUSE_ENTER:         return dispatch_enter_event (event);
      case MOUSE_MOVE:          return dispatch_move_event (event);
      case MOUSE_LEAVE:         return dispatch_leave_event (event);
      case BUTTON_PRESS:
      case BUTTON_2PRESS:
      case BUTTON_3PRESS:
      case BUTTON_CANCELED:
      case BUTTON_RELEASE:
      case BUTTON_2RELEASE:
      case BUTTON_3RELEASE:     return dispatch_button_event (event);
      case FOCUS_IN:
      case FOCUS_OUT:           return dispatch_focus_event (event);
      case KEY_PRESS:
      case KEY_CANCELED:
      case KEY_RELEASE:         return dispatch_key_event (event);
      case SCROLL_UP:          /* button4 */
      case SCROLL_DOWN:        /* button5 */
      case SCROLL_LEFT:        /* button6 */
      case SCROLL_RIGHT:       /* button7 */
        /**/                    return dispatch_scroll_event (event);
      case CANCEL_EVENTS:       return dispatch_cancel_event (event);
      case WIN_SIZE:            return dispatch_win_size_event (event);
      default:
      case EVENT_LAST:          return false;
      }
  }
  /* event queue */
  Mutex m_loop_mutex;
  std::list<Event*> m_event_queue;
  virtual bool
  prepare (uint64 current_time_usecs,
           int   *timeout_msecs_p)
  {
    AutoLocker locker (m_loop_mutex);
    return m_event_queue.size() > 0;
  }
  virtual bool
  check (uint64 current_time_usecs)
  {
    AutoLocker locker (m_loop_mutex);
    return m_event_queue.size() > 0;
  }
  virtual bool
  dispatch ()
  {
    AutoLocker locker (m_loop_mutex);
    if (m_event_queue.size())
      {
        Event *event = m_event_queue.front();
        m_event_queue.pop_front();
        dispatch_event (*event);
        delete event;
      }
    return true;
  }
  virtual void
  enqueue_async (Event *event)
  {
    AutoLocker locker (m_loop_mutex);
    m_event_queue.push_back (event);
  }
};

Handle<Root>
Root::handle ()
{
  Handle<Root> handle (*this, m_omutex);
  return handle;
}

static const ItemFactory<RootImpl> root_factory ("Rapicorn::Root");

} // Rapicorn
