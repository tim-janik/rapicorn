// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "window.hh"
#include "application.hh"
#include "factory.hh"

namespace Rapicorn {

struct ClassDoctor {
  static void item_set_flag       (ItemImpl &item, uint32 flag) { item.set_flag (flag, true); }
  static void item_unset_flag     (ItemImpl &item, uint32 flag) { item.unset_flag (flag); }
  static void set_window_heritage (WindowImpl &window, Heritage *heritage) { window.heritage (heritage); }
  static Heritage*
  window_heritage (WindowImpl &window, ColorSchemeType cst)
  {
    return Heritage::create_heritage (window, window, cst);
  }
};

WindowImpl&
Wind0wIface::impl ()
{
  WindowImpl *wimpl = dynamic_cast<WindowImpl*> (this);
  if (!wimpl)
    throw std::bad_cast();
  return *wimpl;
}

const WindowImpl&
Wind0wIface::impl () const
{
  const WindowImpl *wimpl = dynamic_cast<const WindowImpl*> (this);
  if (!wimpl)
    throw std::bad_cast();
  return *wimpl;
}

void
WindowImpl::set_parent (ContainerImpl *parent)
{
  if (parent)
    critical ("setting parent on toplevel Window item to: %p (%s)", parent, parent->typeid_pretty_name().c_str());
  return ContainerImpl::set_parent (parent);
}

bool
WindowImpl::custom_command (const String     &command_name,
                            const StringList &command_args)
{
  bool handled = false;
  if (!handled)
    handled = sig_commands.emit (command_name, command_args);
  return handled;
}

static DataKey<ItemImpl*> focus_item_key;

void
WindowImpl::uncross_focus (ItemImpl &fitem)
{
  assert (&fitem == get_data (&focus_item_key));
  cross_unlink (fitem, slot (*this, &WindowImpl::uncross_focus));
  ItemImpl *item = &fitem;
  while (item)
    {
      ClassDoctor::item_unset_flag (*item, FOCUS_CHAIN);
      ContainerImpl *fc = item->parent();
      if (fc)
        fc->set_focus_child (NULL);
      item = fc;
    }
  assert (&fitem == get_data (&focus_item_key));
  delete_data (&focus_item_key);
}

void
WindowImpl::set_focus (ItemImpl *item)
{
  ItemImpl *old_focus = get_data (&focus_item_key);
  if (item == old_focus)
    return;
  if (old_focus)
    uncross_focus (*old_focus);
  if (!item)
    return;
  /* set new focus */
  assert (item->has_ancestor (*this));
  set_data (&focus_item_key, item);
  cross_link (*item, slot (*this, &WindowImpl::uncross_focus));
  while (item)
    {
      ClassDoctor::item_set_flag (*item, FOCUS_CHAIN);
      ContainerImpl *fc = item->parent();
      if (fc)
        fc->set_focus_child (item);
      item = fc;
    }
}

ItemImpl*
WindowImpl::get_focus () const
{
  return get_data (&focus_item_key);
}

static ItemImpl*
container_find_item (ContainerImpl &container,
                     const String  &name)
{
  for (ContainerImpl::ChildWalker cw = container.local_children (); cw.has_next(); cw++)
    {
      ItemImpl &item = *cw;
      if (name == item.name())
        return &item;
      ContainerImpl *ct = dynamic_cast<ContainerImpl*> (&item);
      if (ct)
        {
          ItemImpl *it = container_find_item (*ct, name);
          if (it)
            return it;
        }
    }
  return NULL;
}

ItemImpl*
WindowImpl::find_item (const String &item_name)
{
  if (item_name == name())
    return this;
  return container_find_item (*this, item_name);
}

cairo_surface_t*
WindowImpl::create_snapshot (const Rect &subarea)
{
  const Allocation area = allocation();
  Display display;
  display.push_clip_rect (area.x, area.y, area.width, area.height);
  display.push_clip_rect (subarea.x, subarea.y, subarea.width, subarea.height);
  /* render display */
  if (!display.empty())
    render (display);
  cairo_surface_t *isurface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, subarea.width, subarea.height);
  /* comose area */
  cairo_t *cairo = cairo_create (isurface);
  cairo_translate (cairo, -subarea.x, -subarea.y);
  display.render_backing (cairo);
  cairo_destroy (cairo);
  return isurface;
}

WindowImpl::WindowImpl() :
  m_loop (*ref_sink (EventLoop::create())),
  m_source (NULL),
  m_viewp0rt (NULL),
  m_tunable_requisition_counter (0),
  m_entered (false), m_auto_close (false),
  sig_displayed (*this)
{
  Heritage *hr = ClassDoctor::window_heritage (*this, color_scheme());
  ref_sink (hr);
  ClassDoctor::set_window_heritage (*this, hr);
  unref (hr);
  set_flag (PARENT_SENSITIVE, true);
  set_flag (PARENT_VISIBLE, true);
  /* adjust default Viewp0rt config */
  m_config.min_width = 13;
  m_config.min_height = 7;
  /* create event loop (auto-starts) */
  RAPICORN_ASSERT (m_source == NULL);
  EventLoop::Source *source = new WindowSource (*this);
  RAPICORN_ASSERT (m_source == source);
  m_loop.add_source (m_source, EventLoop::PRIORITY_NORMAL);
  m_source->primary (false);
  ApplicationImpl::the().add_wind0w (*this);
  change_flags_silently (ANCHORED, true);       /* window is always anchored */
}

void
WindowImpl::dispose ()
{
  ApplicationImpl::the().remove_wind0w (*this);
}

WindowImpl::~WindowImpl()
{
  if (m_viewp0rt)
    {
      delete m_viewp0rt;
      m_viewp0rt = NULL;
    }
  /* make sure all children are removed while this is still of type WindowImpl.
   * necessary because C++ alters the object type during constructors and destructors
   */
  if (has_children())
    remove (get_child());
  /* shutdown event loop */
  m_loop.kill_sources();
  RAPICORN_ASSERT (m_source == NULL); // should have been destroyed with loop
  /* this should be done last */
  unref (&m_loop);
}

bool
WindowImpl::self_visible () const
{
  return true;
}

void
WindowImpl::size_request (Requisition &requisition)
{
  if (has_allocatable_child())
    {
      ItemImpl &child = get_child();
      requisition = child.size_request();
    }
}

void
WindowImpl::size_allocate (Allocation area)
{
  allocation (area);
  if (has_allocatable_child())
    {
      ItemImpl &child = get_child();
      Requisition rq = child.size_request();
      child.set_allocation (area);
    }
}

bool
WindowImpl::tunable_requisitions ()
{
  return m_tunable_requisition_counter > 0;
}

void
WindowImpl::resize_all (Allocation *new_area)
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
  else if (!new_area && m_viewp0rt)
    {
      Viewp0rt::State state = m_viewp0rt->get_state();
      if (state.width > 0 && state.height > 0)
        {
          area.width = state.width;
          area.height = state.height;
          have_allocation = true;
        }
    }
  if (!test_flags (INVALID_REQUISITION | INVALID_ALLOCATION))
    return;
  /* this is the rcore of the resizing loop. via Item.tune_requisition(), we
   * allow items to adjust the requisition from within size_allocate().
   * whether the tuned requisition is honored at all, depends on
   * m_tunable_requisition_counter.
   * currently, we simply freeze the allocation after 3 iterations. for the
   * future it's possible to honor the tuned requisition only partially or
   * proportionally as m_tunable_requisition_counter decreases, so to mimick
   * a simulated annealing process yielding the final layout.
   */
  m_tunable_requisition_counter = 3;
  Requisition req;
  while (test_flags (INVALID_REQUISITION | INVALID_ALLOCATION))
    {
      req = size_request(); /* unsets INVALID_REQUISITION */
      if (!have_allocation)
        {
          /* fake initial allocation */
          area.width = req.width;
          area.height = req.height;
        }
      set_allocation (area); /* unsets INVALID_ALLOCATION, may re-::invalidate_size() */
      if (m_tunable_requisition_counter)
        m_tunable_requisition_counter--;
    }
  m_tunable_requisition_counter = 0;
  if (!have_allocation ||
      (!new_area &&
       (req.width > allocation().width || req.height > allocation().height)))
    {
      m_config.request_width = req.width;
      m_config.request_height = req.height;
      if (m_viewp0rt)
        {
          /* set_config() will request a WIN_SIZE and WIN_DRAW now, so there's no point in handling further exposes */
          m_viewp0rt->enqueue_win_draws();
          m_expose_region.clear();
          m_viewp0rt->set_config (m_config, true);
        }
    }
}

void
WindowImpl::do_invalidate ()
{
  SingleContainerImpl::do_invalidate();
  // we just need to make sure to be woken up, since flags are set appropriately already
  m_loop.wakeup();
}

void
WindowImpl::beep()
{
  if (m_viewp0rt)
    m_viewp0rt->beep();
}

vector<ItemImpl*>
WindowImpl::item_difference (const vector<ItemImpl*> &clist, /* preserves order of clist */
                             const vector<ItemImpl*> &cminus)
{
  map<ItemImpl*,bool> mminus;
  for (uint i = 0; i < cminus.size(); i++)
    mminus[cminus[i]] = true;
  vector<ItemImpl*> result;
  for (uint i = 0; i < clist.size(); i++)
    if (!mminus[clist[i]])
      result.push_back (clist[i]);
  return result;
}

bool
WindowImpl::dispatch_mouse_movement (const Event &event)
{
  m_last_event_context = event;
  vector<ItemImpl*> pierced;
  /* figure all entered children */
  bool unconfined;
  ItemImpl *grab_item = get_grab (&unconfined);
  if (grab_item)
    {
      if (unconfined or grab_item->viewp0rt_point (Point (event.x, event.y)))
        {
          pierced.push_back (ref (grab_item));        /* grab-item receives all mouse events */
          ContainerImpl *container = grab_item->interface<ContainerImpl*>();
          if (container)                              /* deliver to hovered grab-item children as well */
            container->viewp0rt_point_children (Point (event.x, event.y), pierced);
        }
    }
  else if (drawable())
    {
      pierced.push_back (ref (this)); /* window receives all mouse events */
      if (m_entered)
        viewp0rt_point_children (Point (event.x, event.y), pierced);
    }
  /* send leave events */
  vector<ItemImpl*> left_children = item_difference (m_last_entered_children, pierced);
  EventMouse *leave_event = create_event_mouse (MOUSE_LEAVE, EventContext (event));
  for (vector<ItemImpl*>::reverse_iterator it = left_children.rbegin(); it != left_children.rend(); it++)
    (*it)->process_event (*leave_event);
  delete leave_event;
  /* send enter events */
  vector<ItemImpl*> entered_children = item_difference (pierced, m_last_entered_children);
  EventMouse *enter_event = create_event_mouse (MOUSE_ENTER, EventContext (event));
  for (vector<ItemImpl*>::reverse_iterator it = entered_children.rbegin(); it != entered_children.rend(); it++)
    (*it)->process_event (*enter_event);
  delete enter_event;
  /* send actual move event */
  bool handled = false;
  EventMouse *move_event = create_event_mouse (MOUSE_MOVE, EventContext (event));
  for (vector<ItemImpl*>::reverse_iterator it = pierced.rbegin(); it != pierced.rend(); it++)
    if (!handled && (*it)->sensitive())
      handled = (*it)->process_event (*move_event);
  delete move_event;
  /* cleanup */
  for (vector<ItemImpl*>::reverse_iterator it = m_last_entered_children.rbegin(); it != m_last_entered_children.rend(); it++)
    (*it)->unref();
  m_last_entered_children = pierced;
  return handled;
}

bool
WindowImpl::dispatch_event_to_pierced_or_grab (const Event &event)
{
  vector<ItemImpl*> pierced;
  /* figure all entered children */
  ItemImpl *grab_item = get_grab();
  if (grab_item)
    pierced.push_back (ref (grab_item));
  else if (drawable())
    {
      pierced.push_back (ref (this)); /* window receives all events */
      viewp0rt_point_children (Point (event.x, event.y), pierced);
    }
  /* send actual event */
  bool handled = false;
  for (vector<ItemImpl*>::reverse_iterator it = pierced.rbegin(); it != pierced.rend(); it++)
    {
      if (!handled && (*it)->sensitive())
        handled = (*it)->process_event (event);
      (*it)->unref();
    }
  return handled;
}

bool
WindowImpl::dispatch_button_press (const EventButton &bevent)
{
  uint press_count = bevent.type - BUTTON_PRESS + 1;
  assert (press_count >= 1 && press_count <= 3);
  /* figure all entered children */
  const vector<ItemImpl*> &pierced = m_last_entered_children;
  /* send actual event */
  bool handled = false;
  for (vector<ItemImpl*>::const_reverse_iterator it = pierced.rbegin(); it != pierced.rend(); it++)
    if (!handled && (*it)->sensitive())
      {
        ButtonState bs (*it, bevent.button);
        if (m_button_state_map[bs] == 0)                /* no press delivered for <button> on <item> yet */
          {
            m_button_state_map[bs] = press_count;       /* record single press */
            handled = (*it)->process_event (bevent);    // modifies m_last_entered_children + this
          }
      }
  return handled;
}

bool
WindowImpl::dispatch_button_release (const EventButton &bevent)
{
  bool handled = false;
 restart:
  for (map<ButtonState,uint>::iterator it = m_button_state_map.begin();
       it != m_button_state_map.end(); it++)
    {
      const ButtonState bs = it->first;
      // uint press_count = it->second;
      if (bs.button == bevent.button)
        {
#if 0 // FIXME
          if (press_count == 3)
            bevent.type = BUTTON_3RELEASE;
          else if (press_count == 2)
            bevent.type = BUTTON_2RELEASE;
#endif
          m_button_state_map.erase (it);
          handled |= bs.item->process_event (bevent); // modifies m_button_state_map + this
          goto restart; // restart bs.button search
        }
    }
  // bevent.type = BUTTON_RELEASE;
  return handled;
}

void
WindowImpl::cancel_item_events (ItemImpl *item)
{
  /* cancel enter events */
  for (int i = m_last_entered_children.size(); i > 0;)
    {
      ItemImpl *current = m_last_entered_children[--i]; /* walk backwards */
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
  while (m_button_state_map.begin() != m_button_state_map.end())
    {
      map<ButtonState,uint>::iterator it = m_button_state_map.begin();
      const ButtonState bs = it->first;
      m_button_state_map.erase (it);
      if (bs.item == item || !item)
        {
          EventButton *bevent = create_event_button (BUTTON_CANCELED, m_last_event_context, bs.button);
          bs.item->process_event (*bevent); // modifies m_button_state_map + this
          delete bevent;
        }
    }
}

bool
WindowImpl::dispatch_cancel_event (const Event &event)
{
  cancel_item_events (NULL);
  return false;
}

bool
WindowImpl::dispatch_enter_event (const EventMouse &mevent)
{
  m_entered = true;
  dispatch_mouse_movement (mevent);
  return false;
}

bool
WindowImpl::dispatch_move_event (const EventMouse &mevent)
{
  dispatch_mouse_movement (mevent);
  return false;
}

bool
WindowImpl::dispatch_leave_event (const EventMouse &mevent)
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
          ItemImpl *item = m_last_entered_children.back();
          m_last_entered_children.pop_back();
          item->process_event (mevent);
          item->unref();
        }
    }
  return false;
}

bool
WindowImpl::dispatch_button_event (const Event &event)
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
WindowImpl::dispatch_focus_event (const EventFocus &fevent)
{
  bool handled = false;
  // dispatch_event_to_pierced_or_grab (*fevent);
  dispatch_mouse_movement (fevent);
  return handled;
}

bool
WindowImpl::move_focus_dir (FocusDirType focus_dir)
{
  ItemImpl *new_focus = NULL, *old_focus = get_focus();
  if (old_focus)
    ref (old_focus);

  switch (focus_dir)
    {
    case FOCUS_UP:   case FOCUS_DOWN:
    case FOCUS_LEFT: case FOCUS_RIGHT:
      new_focus = old_focus;
      break;
    default: ;
    }
  if (focus_dir && !move_focus (focus_dir))
    {
      if (new_focus && new_focus->get_window() != this)
        new_focus = NULL;
      if (new_focus)
        new_focus->grab_focus();
      else
        set_focus (NULL);
      if (old_focus == new_focus)
        return false; // should have moved focus but failed
    }
  if (old_focus)
    unref (old_focus);
  return true;
}

bool
WindowImpl::dispatch_key_event (const Event &event)
{
  bool handled = false;
  dispatch_mouse_movement (event);
  ItemImpl *item = get_focus();
  if (item && item->process_viewp0rt_event (event))
    return true;
  const EventKey *kevent = dynamic_cast<const EventKey*> (&event);
  if (kevent && kevent->type == KEY_PRESS)
    {
      FocusDirType fdir = key_value_to_focus_dir (kevent->key);
      if (fdir)
        {
          if (!move_focus_dir (fdir))
            notify_key_error();
          handled = true;
        }
      if (0)
        {
          ItemImpl *grab_item = get_grab();
          grab_item = grab_item ? grab_item : this;
          handled = grab_item->process_event (*kevent);
        }
    }
  return handled;
}

bool
WindowImpl::dispatch_scroll_event (const EventScroll &sevent)
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
WindowImpl::dispatch_win_size_event (const Event &event)
{
  bool handled = false;
  const EventWinSize *wevent = dynamic_cast<const EventWinSize*> (&event);
  if (wevent)
    {
      Allocation allocation (0, 0, wevent->width, wevent->height);
      resize_all (&allocation);
      /* discard all expose requests, we'll get a new WIN_DRAW event */
      m_expose_region.clear();
      if (0)
        DEBUG ("win-size: %f %f", wevent->width, wevent->height);
      handled = true;
    }
  return handled;
}

void
WindowImpl::collapse_expose_region ()
{
  /* check for excess expose fragment scenarios */
  uint n_erects = m_expose_region.count_rects();
  if (n_erects > 999)           /* workable limit, considering O(n^2) collision complexity */
    {
      /* aparently the expose fragments we're combining are too small,
       * so we can end up with spending too much time on expose rectangle
       * compression (much more than needed for rendering).
       * as a workaround, we simply force everything into a single expose
       * rectangle which is good enough to avoid worst case explosion.
       * note that this is only triggered in seldom pathological cases.
       */
      m_expose_region.add (m_expose_region.extents());
      // printerr ("collapsing due to too many expose rectangles: %u -> %u\n", n_erects, m_expose_region.count_rects());
    }
}

bool
WindowImpl::dispatch_win_draw_event (const Event &event)
{
  bool handled = false;
  const EventWinDraw *devent = dynamic_cast<const EventWinDraw*> (&event);
  if (devent)
    {
      for (uint i = 0; i < devent->rectangles.size(); i++)
        {
          m_expose_region.add (devent->rectangles[i]);
          if (0)
            DEBUG ("win-draw: %f %f (at: %f %f)", devent->rectangles[i].width, devent->rectangles[i].height,
                   devent->rectangles[i].x, devent->rectangles[i].y);
          /* check for excess expose fragment scenarios */
          if (i % 11 == 0)              /* infrequent checks are good enough */
            collapse_expose_region();   /* collapse pathological expose regions */
        }
      handled = true;
    }
  return handled;
}

bool
WindowImpl::dispatch_win_delete_event (const Event &event)
{
  bool handled = false;
  const EventWinDelete *devent = dynamic_cast<const EventWinDelete*> (&event);
  if (devent)
    {
      destroy_viewp0rt();
      dispose();
      handled = true;
    }
  return handled;
}

void
WindowImpl::expose_window_region (const Region &region)
{
  /* this function is expected to *queue* exposes, and not render immediately */
  if (m_viewp0rt && !region.empty())
    {
      m_expose_region.add (region);
      collapse_expose_region();
    }
}

void
WindowImpl::copy_area (const Rect  &src,
                       const Point &dest)
{
  /* need to copy pixel regions and expose regions */
  if (m_viewp0rt && src.width && src.height)
    {
      /* force delivery of any pending exposes */
      expose_now();
      /* intersect copied expose region */
      Region exr = m_expose_region;
      exr.intersect (Region (src));
      /* pixel-copy area */
      m_viewp0rt->copy_area (src.x, src.y, // may queue new exposes
                             src.width, src.height,
                             dest.x, dest.y);
      /* shift expose region */
      exr.affine (AffineTranslate (Point (dest.x - src.x, dest.y - src.y)));
      /* expose copied area */
      expose_window_region (exr);
      // FIXME: synthesize 0-dist pointer movement from idle handler
    }
}

void
WindowImpl::expose_now ()
{
  if (m_viewp0rt)
    {
      /* force delivery of any pending update events */
      m_viewp0rt->enqueue_win_draws();
      /* collect all WIN_DRAW events */
      m_async_mutex.lock();
      std::list<Event*> events;
      std::list<Event*>::iterator it = m_async_event_queue.begin();
      while (it != m_async_event_queue.end())
        {
          if ((*it)->type == WIN_DRAW)
            {
              std::list<Event*>::iterator current = it++;
              events.push_back (*current);
              m_async_event_queue.erase (current);
            }
          else
            it++;
        }
      m_async_mutex.unlock();
      /* invalidate areas from all collected WIN_DRAW events */
      while (!events.empty())
        {
          Event *event = events.front();
          events.pop_front();
          dispatch_event (*event);
          delete event;
        }
    }
  else
    m_expose_region.clear(); // nuke stale exposes
}

void
WindowImpl::draw_now ()
{
  if (m_viewp0rt)
    {
      /* force delivery of any pending exposes */
      expose_now();
      /* render invalidated contents */
      m_expose_region.intersect (Region (allocation()));
      std::vector<Rect> rects;
      m_expose_region.list_rects (rects);
      m_expose_region.clear();
      Viewp0rt::State state = m_viewp0rt->get_state();
      for (uint i = 0; i < rects.size(); i++)
        {
          const IRect &ir = rects[i];
          Display display;
          display.push_clip_rect (ir.x, ir.y, ir.width, ir.height);
          /* render display */
          if (!display.empty())
            render (display);
          /* avoid unnecessary plane transfers for slow remote
           * displays, for local displays it'd cause resizing lags.
           */
          if (!state.local_blitting && has_pending_win_size())
            {
              break;
            }
          /* blit to screen */
          m_viewp0rt->blit_display (display);
          display.pop_clip_rect();
        }
      if (!has_pending_win_size())
        sig_displayed.emit();
    }
  else
    m_expose_region.clear(); // nuke stale exposes
}

void
WindowImpl::render (Display &display)
{
  const IRect ia = allocation();
  display.push_clip_rect (ia.x, ia.y, ia.width, ia.height);
  // paint background
  cairo_t *cr = display.create_cairo (background());
  cairo_destroy (cr);
  // paint children
  SingleContainerImpl::render (display);
  display.pop_clip_rect();
}

void
WindowImpl::remove_grab_item (ItemImpl &child)
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
WindowImpl::grab_stack_changed()
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
WindowImpl::add_grab (ItemImpl *child,
                      bool      unconfined)
{
  RAPICORN_CHECK (child != NULL);
  add_grab (*child, unconfined);
}

void
WindowImpl::add_grab (ItemImpl &child,
                      bool      unconfined)
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
WindowImpl::remove_grab (ItemImpl *child)
{
  RAPICORN_CHECK (child != NULL);
  remove_grab (*child);
}

void
WindowImpl::remove_grab (ItemImpl &child)
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

ItemImpl*
WindowImpl::get_grab (bool *unconfined)
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
WindowImpl::dispose_item (ItemImpl &item)
{
  remove_grab_item (item);
  cancel_item_events (item);
  SingleContainerImpl::dispose_item (item);
}

bool
WindowImpl::has_pending_win_size ()
{
  bool found_one = false;
  m_async_mutex.lock();
  for (std::list<Event*>::iterator it = m_async_event_queue.begin();
       it != m_async_event_queue.end();
       it++)
    if ((*it)->type == WIN_SIZE)
      {
        found_one = true;
        break;
      }
  m_async_mutex.unlock();
  return found_one;
}

bool
WindowImpl::dispatch_event (const Event &event)
{
  if (!m_viewp0rt)
    return false;       // we can only handle events on viewp0rts
  if (0)
    DEBUG ("Window: event: %s", string_from_event_type (event.type));
  switch (event.type)
    {
    case EVENT_LAST:
    case EVENT_NONE:          return false;
    case MOUSE_ENTER:         return dispatch_enter_event (event);
    case MOUSE_MOVE:
      if (true) // coalesce multiple motion events
        {
          m_async_mutex.lock();
          std::list<Event*>::iterator it = m_async_event_queue.begin();
          bool found_event = it != m_async_event_queue.end() && (*it)->type == MOUSE_MOVE;
          m_async_mutex.unlock();
          if (found_event)
            return true;
        }
      return dispatch_move_event (event);
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
    case WIN_SIZE:            return has_pending_win_size() ? true : dispatch_win_size_event (event);
    case WIN_DRAW:            return has_pending_win_size() ? true : dispatch_win_draw_event (event);
    case WIN_DELETE:          return dispatch_win_delete_event (event);
    }
  return false;
}

bool
WindowImpl::prepare (uint64 current_time_usecs,
                     int64 *timeout_usecs_p)
{
  ScopedLock<Mutex> aelocker (m_async_mutex);
  return !m_async_event_queue.empty() || !m_expose_region.empty() || (m_viewp0rt && test_flags (INVALID_REQUISITION | INVALID_ALLOCATION));
}

bool
WindowImpl::check (uint64 current_time_usecs)
{
  ScopedLock<Mutex> aelocker (m_async_mutex);
  return !m_async_event_queue.empty() || !m_expose_region.empty() || (m_viewp0rt && test_flags (INVALID_REQUISITION | INVALID_ALLOCATION));
}

void
WindowImpl::enable_auto_close ()
{
  m_auto_close = true;
}

bool
WindowImpl::dispatch ()
{
  ref (this);
  {
    m_async_mutex.lock();
    Event *event = NULL;
    if (!m_async_event_queue.empty())
      {
        event = m_async_event_queue.front();
        m_async_event_queue.pop_front();
      }
    m_async_mutex.unlock();
    if (event)
      {
        dispatch_event (*event);
        delete event;
      }
    else if (m_viewp0rt && test_flags (INVALID_REQUISITION | INVALID_ALLOCATION))
      resize_all (NULL);
    else if (!m_expose_region.empty())
      {
        draw_now();
        if (m_auto_close)
          {
            EventLoop *loop = get_loop();
            if (loop)
              {
                loop->exec_timer (0, slot (*this, &WindowImpl::destroy_viewp0rt), INT_MAX);
                m_auto_close = false;
              }
          }
      }
  }
  unref();
  return true;
}

EventLoop*
WindowImpl::get_loop ()
{
  ScopedLock<Mutex> aelocker (m_async_mutex);
  return &m_loop;
}

void
WindowImpl::enqueue_async (Event *event)
{
  ScopedLock<Mutex> aelocker (m_async_mutex);
  m_async_event_queue.push_back (event);
  m_loop.wakeup(); /* thread safe */
}

bool
WindowImpl::viewable ()
{
  return visible() && m_viewp0rt && m_viewp0rt->visible();
}

void
WindowImpl::idle_show()
{
  if (m_viewp0rt)
    {
      /* request size, WIN_SIZE and WIN_DRAW */
      if (!m_viewp0rt->visible())
        resize_all (NULL);
      /* size requested, show up */
      m_viewp0rt->show();
    }
}

void
WindowImpl::create_viewp0rt ()
{
  if (m_source)
    {
      if (!m_viewp0rt)
        {
          m_viewp0rt = Viewp0rt::create_viewp0rt ("auto", WINDOW_TYPE_NORMAL, *this);
          resize_all (NULL);
        }
      RAPICORN_ASSERT (m_viewp0rt != NULL);
      m_source->primary (true); // FIXME: depends on WM-managable
      VoidSlot sl = slot (*this, &WindowImpl::idle_show);
      m_loop.exec_now (sl);
    }
}

bool
WindowImpl::has_viewp0rt ()
{
  return m_source && m_viewp0rt && m_viewp0rt->visible();
}

void
WindowImpl::destroy_viewp0rt ()
{
  if (!m_viewp0rt)
    return; // during destruction, ref_count == 0
  ref (this);
  m_viewp0rt->hide();
  delete m_viewp0rt;
  m_viewp0rt = NULL;
  if (m_source)
    {
      m_loop.kill_sources(); // calls m_source methods
      RAPICORN_ASSERT (m_source == NULL);
      EventLoop::Source *source = new WindowSource (*this);
      RAPICORN_ASSERT (m_source == source);
      m_loop.add_source (m_source, EventLoop::PRIORITY_NORMAL);
      m_source->primary (false);
    }
  // reset item state where needed
  cancel_item_events (NULL);
  unref (this);
}

void
WindowImpl::show ()
{
  create_viewp0rt();
}

bool
WindowImpl::closed ()
{
  return !has_viewp0rt();
}

void
WindowImpl::close ()
{
  destroy_viewp0rt();
}

bool
WindowImpl::synthesize_enter (double xalign,
                              double yalign)
{
  if (!has_viewp0rt())
    return false;
  const Allocation &area = allocation();
  Point p (area.x + xalign * (max (1, area.width) - 1),
           area.y + yalign * (max (1, area.height) - 1));
  p = point_to_viewp0rt (p);
  EventContext ec;
  ec.x = p.x;
  ec.y = p.y;
  enqueue_async (create_event_mouse (MOUSE_ENTER, ec));
  return true;
}

bool
WindowImpl::synthesize_leave ()
{
  if (!has_viewp0rt())
    return false;
  EventContext ec;
  enqueue_async (create_event_mouse (MOUSE_LEAVE, ec));
  return true;
}

bool
WindowImpl::synthesize_click (ItemIface &itemi,
                              int        button,
                              double     xalign,
                              double     yalign)
{
  ItemImpl &item = *dynamic_cast<ItemImpl*> (&itemi);
  if (!has_viewp0rt() || !&item)
    return false;
  const Allocation &area = item.allocation();
  Point p (area.x + xalign * (max (1, area.width) - 1),
           area.y + yalign * (max (1, area.height) - 1));
  p = item.point_to_viewp0rt (p);
  EventContext ec;
  ec.x = p.x;
  ec.y = p.y;
  enqueue_async (create_event_button (BUTTON_PRESS, ec, button));
  enqueue_async (create_event_button (BUTTON_RELEASE, ec, button));
  return true;
}

bool
WindowImpl::synthesize_delete ()
{
  if (!has_viewp0rt())
    return false;
  EventContext ec;
  enqueue_async (create_event_win_delete (ec));
  return true;
}

Wind0wIface&
WindowImpl::wind0w ()
{
  return *this;
}

static const ItemFactory<WindowImpl> window_factory ("Rapicorn::Factory::Window");

} // Rapicorn
