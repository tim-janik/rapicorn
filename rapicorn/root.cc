/* Rapicorn
 * Copyright (C) 2005-2006 Tim Janik
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
#include "rootimpl.hh"

namespace Rapicorn {
using namespace std;

Root::Root()
{
  change_flags_silently (ANCHORED, true);       /* root is always anchored */
}

RootImpl::RootImpl() :
  m_entered (false), m_viewport (NULL),
  m_loop (NULL), m_source (NULL),
  m_expose_queue_stamp (0),
  m_tunable_requisition_counter (0)
{
  BIRNET_ASSERT (m_loop == NULL);
  m_loop = glib_loop_create();
  Appearance *appearance = Appearance::create_default();
  style (appearance->create_style ("normal"));
  unref (appearance);
  set_flag (PARENT_SENSITIVE, true);
  /* adjust default Viewport config */
  m_config.min_width = 13;
  m_config.min_height = 7;
}

RootImpl::~RootImpl()
{
  if (m_loop)
    {
      m_loop->quit();
      m_loop->unref();
      m_loop = NULL;
    }
  BIRNET_ASSERT (m_source == NULL); // should have been destroyed with loop
  if (m_viewport)
    {
      delete m_viewport;
      m_viewport = NULL;
    }
  /* make sure all children are removed while this is still of type Root.
   * necessary because C++ alters the object type during constructors and destructors
   */
  if (has_children())
    remove (get_child());
}

OwnedMutex&
RootImpl::owned_mutex ()
{
  return m_omutex;
}

void
RootImpl::size_request (Requisition &requisition)
{
  if (has_visible_child())
    {
      Item &child = get_child();
      requisition = child.size_request();
    }
}

void
RootImpl::size_allocate (Allocation area)
{
  allocation (area);
  if (has_visible_child())
    {
      Item &child = get_child();
      Requisition rq = child.size_request();
      child.set_allocation (area);
    }
}

bool
RootImpl::tunable_requisitions ()
{
  return m_tunable_requisition_counter > 0;
}

void
RootImpl::resize_all (Allocation *new_area)
{
  assert (m_tunable_requisition_counter == 0);
  bool have_allocation = false;
  Allocation area;
  if (new_area)
    {
      area.width = new_area->width;
      area.height = new_area->height;
      have_allocation = true;
      change_flags_silently (INVALID_ALLOCATION, true);
    }
  else if (m_viewport)
    {
      Viewport::State state = m_viewport->get_state();
      if (state.width > 0 && state.height > 0)
        {
          area.width = state.width;
          area.height = state.height;
          have_allocation = true;
        }
    }
  m_tunable_requisition_counter = 3;
  while (test_flags (INVALID_REQUISITION | INVALID_ALLOCATION))
    {
      Requisition req = size_request();
      if (!have_allocation)
        {
          /* fake an allocation */
          area.width = req.width;
          area.height = req.height;
        }
      set_allocation (area);
      if (m_tunable_requisition_counter)
        m_tunable_requisition_counter--;
    }
  m_tunable_requisition_counter = 0;
}

vector<Item*>
RootImpl::item_difference (const vector<Item*> &clist, /* preserves order of clist */
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

bool
RootImpl::dispatch_mouse_movement (const Event &event)
{
  m_last_event_context = event;
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
  vector<Item*> left_children = item_difference (m_last_entered_children, pierced);
  EventMouse *mevent = create_event_mouse (MOUSE_LEAVE, EventContext (event));
  for (vector<Item*>::reverse_iterator it = left_children.rbegin(); it != left_children.rend(); it++)
    (*it)->process_event (*mevent);
  /* send enter events */
  vector<Item*> entered_children = item_difference (pierced, m_last_entered_children);
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
  for (vector<Item*>::reverse_iterator it = m_last_entered_children.rbegin(); it != m_last_entered_children.rend(); it++)
    (*it)->unref();
  m_last_entered_children = pierced;
  return handled;
}

bool
RootImpl::dispatch_event_to_pierced_or_grab (const Event &event)
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

bool
RootImpl::dispatch_button_press (const EventButton &bevent)
{
  uint press_count = bevent.type - BUTTON_PRESS + 1;
  assert (press_count >= 1 && press_count <= 3);
  /* figure all entered children */
  const vector<Item*> &pierced = m_last_entered_children;
  /* send actual event */
  bool handled = false;
  for (vector<Item*>::const_reverse_iterator it = pierced.rbegin(); it != pierced.rend(); it++)
    if (!handled && (*it)->sensitive())
      {
        ButtonState bs (*it, bevent.button);
        if (m_button_state_map[bs] == 0)                /* no press delivered for <button> on <item> yet */
          {
            m_button_state_map[bs] = press_count;       /* record single press */
            handled = (*it)->process_event (bevent);
          }
      }
  return handled;
}

bool
RootImpl::dispatch_button_release (const EventButton &bevent)
{
  bool handled = false;
  for (map<ButtonState,uint>::iterator it = m_button_state_map.begin(); it != m_button_state_map.end();)
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
          m_button_state_map.erase (current);
        }
    }
  // bevent.type = BUTTON_RELEASE;
  return handled;
}

void
RootImpl::cancel_item_events (Item *item)
{
  /* cancel enter events */
  for (int i = m_last_entered_children.size(); i > 0;)
    {
      Item *current = m_last_entered_children[--i]; /* walk backwards */
      if (item == current || !item)
        {
          EventMouse *mevent = create_event_mouse (MOUSE_LEAVE, m_last_event_context);
          current->process_event (*mevent);
          delete mevent;
          current->unref();
          m_last_entered_children.erase (m_last_entered_children.begin() + i);
        }
    }
  /* cancel button press events */
  for (map<ButtonState,uint>::iterator it = m_button_state_map.begin(); it != m_button_state_map.end();)
    {
      const ButtonState &bs = it->first;
      map<ButtonState,uint>::iterator current = it++;
      if (bs.item == item || !item)
        {
          EventButton *bevent = create_event_button (BUTTON_CANCELED, m_last_event_context, bs.button);
          bs.item->process_event (*bevent);
          delete bevent;
          m_button_state_map.erase (current);
        }
    }
}

bool
RootImpl::dispatch_cancel_event (const Event &event)
{
  cancel_item_events (NULL);
  return false;
}

bool
RootImpl::dispatch_enter_event (const EventMouse &mevent)
{
  m_entered = true;
  dispatch_mouse_movement (mevent);
  return false;
}

bool
RootImpl::dispatch_move_event (const EventMouse &mevent)
{
  dispatch_mouse_movement (mevent);
  return false;
}

bool
RootImpl::dispatch_leave_event (const EventMouse &mevent)
{
  dispatch_mouse_movement (mevent);
  m_entered = false;
  if (get_grab ())
    ; /* leave events in grab mode are automatically calculated */
  else
    {
      /* send leave events */
      while (m_last_entered_children.size())
        {
          Item *item = m_last_entered_children.back();
          m_last_entered_children.pop_back();
          item->process_event (mevent);
          item->unref();
        }
    }
  return false;
}

bool
RootImpl::dispatch_button_event (const Event &event)
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
RootImpl::dispatch_focus_event (const EventFocus &fevent)
{
  bool handled = false;
  // dispatch_event_to_pierced_or_grab (*fevent);
  dispatch_mouse_movement (fevent);
  return handled;
}

bool
RootImpl::dispatch_key_event (const Event &event)
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
RootImpl::dispatch_scroll_event (const EventScroll &sevent)
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
RootImpl::dispatch_win_size_event (const Event &event)
{
  bool handled = false;
  const EventWinSize *wevent = dynamic_cast<const EventWinSize*> (&event);
  if (wevent)
    {
      Allocation allocation (0, 0, wevent->width, wevent->height);
      resize_all (&allocation);
      /* discard all expose requests, we'll get a WIN_DRAW event */
      m_expose_queue.clear();
      handled = true;
    }
  return handled;
}

bool
RootImpl::dispatch_win_draw_event (const Event &event)
{
  bool handled = false;
  const EventWinDraw *devent = dynamic_cast<const EventWinDraw*> (&event);
  if (devent)
    {
      for (uint i = 0; i < devent->rectangles.size(); i++)
        {
          if (m_viewport->last_draw_stamp() != devent->draw_stamp)
            break;    // discard outdated expose rectangles
          const Rect &rect = devent->rectangles[i];
          /* render area */
          Plane *plane = new Plane (rect.ll.x, rect.ll.y, rect.width(), rect.height());
          render (*plane);
          /* blit to screen */
          m_viewport->blit_plane (plane, devent->draw_stamp); // takes over plane
        }
      handled = true;
    }
  return handled;
}

bool
RootImpl::dispatch_win_delete_event (const Event &event)
{
  bool handled = false;
  const EventWinDelete *devent = dynamic_cast<const EventWinDelete*> (&event);
  if (devent)
    {
      stop_async();
      handled = true;
    }
  return handled;
}

void
RootImpl::flush_expose_queue()
{
  m_viewport->invalidate_plane (m_expose_queue, m_expose_queue_stamp);
  m_expose_queue.clear();
}

void
RootImpl::expose (const Allocation &area)
{
  /* this function is expected to *queue* events, and not render immediately */
  if (m_viewport && area.width && area.height)
    {
      if (!m_expose_queue.size())
        m_loop->idle_next (slot (*this, &RootImpl::flush_expose_queue)); // FIXME: prio should be lower than the prio for input event processing
      uint stamp = m_viewport->last_draw_stamp();
      if (stamp != m_expose_queue_stamp)
        m_expose_queue.clear(); /* discard outdated exposes */
      m_expose_queue.push_back (Rect (Point (area.x, area.y), area.width, area.height));
      m_expose_queue_stamp = stamp;
    }
}

void
RootImpl::render (Plane &plane)
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

void
RootImpl::remove_grab_item (Item &child)
{
  bool stack_changed = false;
  for (int i = m_grab_stack.size() - 1; i >= 0; i--)
    if (m_grab_stack[i].item == &child)
      {
        m_grab_stack.erase (m_grab_stack.begin() + i);
        stack_changed = true;
      }
  if (stack_changed)
    grab_stack_changed();
}

void
RootImpl::grab_stack_changed()
{
  // FIXME: use idle handler for event synthesis
  EventMouse *mevent = create_event_mouse (MOUSE_LEAVE, m_last_event_context);
  dispatch_mouse_movement (*mevent);
  /* synthesize neccessary leaves after grabbing */
  if (!m_grab_stack.size() && !m_entered)
    dispatch_event (*mevent);
  delete mevent;
}

void
RootImpl::add_grab (Item &child,
                    bool  unconfined)
{
  if (!child.has_ancestor (*this))
    throw Exception ("child is not descendant of container \"", name(), "\": ", child.name());
  /* for unconfined==true grabs, the mouse pointer is always considered to
   * be contained by the grab-item, and only by the grab-item. events are
   * delivered to the grab-item and its children.
   */
  m_grab_stack.push_back (GrabEntry (&child, unconfined));
  // grab_stack_changed(); // FIXME: re-enable this, once grab_stack_changed() synthesizes from idler
}

void
RootImpl::remove_grab (Item &child)
{
  for (int i = m_grab_stack.size() - 1; i >= 0; i--)
    if (m_grab_stack[i].item == &child)
      {
        m_grab_stack.erase (m_grab_stack.begin() + i);
        grab_stack_changed();
        return;
      }
  throw Exception ("no such child in grab stack: ", child.name());
}

Item*
RootImpl::get_grab (bool *unconfined)
{
  for (int i = m_grab_stack.size() - 1; i >= 0; i--)
    if (m_grab_stack[i].item->visible())
      {
        if (unconfined)
          *unconfined = m_grab_stack[i].unconfined;
        return m_grab_stack[i].item;
      }
  return NULL;
}

void
RootImpl::dispose_item (Item &item)
{
  remove_grab_item (item);
  cancel_item_events (item);
  SingleContainerImpl::dispose_item (item);
}

bool
RootImpl::dispatch_event (const Event &event)
{
  switch (event.type)
    {
    case EVENT_LAST:
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
    case WIN_DRAW:            return dispatch_win_draw_event (event);
    case WIN_DELETE:          return dispatch_win_delete_event (event);
    }
  return false;
}

bool
RootImpl::prepare (uint64 current_time_usecs,
                   int   *timeout_msecs_p)
{
  AutoLocker locker (m_omutex);
  return !m_event_queue.empty();
}

bool
RootImpl::check (uint64 current_time_usecs)
{
  AutoLocker locker (m_omutex);
  return !m_event_queue.empty();
}

bool
RootImpl::dispatch ()
{
  AutoLocker locker (m_omutex);
  if (!m_event_queue.empty())
    {
      Event *event = m_event_queue.front();
      m_event_queue.pop_front();
      dispatch_event (*event);
      delete event;
    }
  return true;
}

void
RootImpl::enqueue_async (Event *event)
{
  AutoLocker locker (m_omutex);
  m_event_queue.push_back (event);
  if (m_loop)
    m_loop->wakeup();
}

void
RootImpl::idle_show()
{
  resize_all();
  Requisition req = size_request();
  m_config.request_width = req.width;
  m_config.request_height = req.height;
  m_viewport->set_config (m_config);
  m_viewport->show();
}

void
RootImpl::run_async (void)
{
  AutoLocker monitor (owned_mutex());
  BIRNET_ASSERT (m_viewport == NULL);
  m_viewport = Viewport::create_viewport ("auto", WINDOW_TYPE_NORMAL, *this);
  BIRNET_ASSERT (m_loop != NULL);
  MainLoopPool::add_loop (m_loop);
  BIRNET_ASSERT (m_source == NULL);
  m_source = new RootSource (*this);
  m_loop->idle_now (slot (*this, &RootImpl::idle_show));
  m_loop->add_source (MainLoop::PRIORITY_NORMAL, m_source);
  // m_loop->idle_timed (250, timer);
}

void
RootImpl::stop_async (void)
{
  AutoLocker monitor (owned_mutex());
  if (m_viewport)
    m_viewport->hide();
  if (m_loop)
    m_loop->quit();
}

static const ItemFactory<RootImpl> root_factory ("Rapicorn::Root");

} // Rapicorn
