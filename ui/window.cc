// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "window.hh"
#include "application.hh"
#include "factory.hh"
#include "uithread.hh"
#include <string.h> // memcpy
#include <algorithm>

#define EDEBUG(...)     RAPICORN_KEY_DEBUG ("Events", __VA_ARGS__)

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
WindowIface::impl ()
{
  WindowImpl *wimpl = dynamic_cast<WindowImpl*> (this);
  if (!wimpl)
    throw std::bad_cast();
  return *wimpl;
}

const WindowImpl&
WindowIface::impl () const
{
  const WindowImpl *wimpl = dynamic_cast<const WindowImpl*> (this);
  if (!wimpl)
    throw std::bad_cast();
  return *wimpl;
}

const PropertyList&
WindowImpl::_property_list ()
{
  static Property *properties[] = {
    MakeProperty (WindowImpl, title,   _("Title"),   _("User visible title to be displayed in the window title bar"), "rw"),
  };
  static const PropertyList property_list (properties, ViewportImpl::_property_list());
  return property_list;
}

String
WindowImpl::title () const
{
  return m_config.title;
}

void
WindowImpl::title (const String &window_title)
{
  if (m_config.title != window_title)
    {
      m_config.title = window_title;
      if (m_screen_window)
        m_screen_window->configure (m_config, false);
    }
}

void
WindowImpl::set_parent (ContainerImpl *parent)
{
  if (parent)
    critical ("setting parent on toplevel Window item to: %p (%s)", parent, parent->typeid_name().c_str());
  return ContainerImpl::set_parent (parent);
}

bool
WindowImpl::custom_command (const String &command_name, const StringSeq &command_args)
{
  assert_return (commands_emission_ == NULL, false);
  last_command_ = command_name;
  commands_emission_ = sig_commands.emission (command_name, command_args);
  return true;
}

bool
WindowImpl::command_dispatcher (const EventLoop::State &state)
{
  if (state.phase == state.PREPARE || state.phase == state.CHECK)
    return commands_emission_ && commands_emission_->pending();
  else if (state.phase == state.DISPATCH)
    {
      ref (this);
      commands_emission_->dispatch();                   // invoke signal handlers
      bool handled = false;
      if (commands_emission_->has_value())
        handled = commands_emission_->get_value();      // value returned from signal handler
      if (handled || commands_emission_->done())
        {
          if (!handled)                                 // all handlers returned false
            critical ("Command unhandled: %s", last_command_.c_str());
          Signal_commands::Emission *emi = commands_emission_;
          commands_emission_ = NULL;
          delete emi;
          last_command_ = "";
        }
      unref (this);
      return true;
    }
  else if (state.phase == state.DESTROY)
    {
      if (commands_emission_)
        {
          Signal_commands::Emission *emi = commands_emission_;
          commands_emission_ = NULL;
          delete emi;
          last_command_ = "";
        }
    }
  return false;
}

struct CurrentFocus {
  ItemImpl *focus_item;
  size_t    uncross_id;
  CurrentFocus (ItemImpl *f = NULL, size_t i = 0) : focus_item (f), uncross_id (i) {}
};
static DataKey<CurrentFocus> focus_item_key;

ItemImpl*
WindowImpl::get_focus () const
{
  return get_data (&focus_item_key).focus_item;
}

void
WindowImpl::uncross_focus (ItemImpl &fitem)
{
  CurrentFocus cfocus = get_data (&focus_item_key);
  assert_return (&fitem == cfocus.focus_item);
  if (cfocus.uncross_id)
    {
      set_data (&focus_item_key, CurrentFocus (cfocus.focus_item, 0)); // reset cfocus.uncross_id
      cross_unlink (fitem, cfocus.uncross_id);
      ItemImpl *item = &fitem;
      while (item)
        {
          ClassDoctor::item_unset_flag (*item, FOCUS_CHAIN);
          ContainerImpl *fc = item->parent();
          if (fc)
            fc->set_focus_child (NULL);
          item = fc;
        }
      cfocus = get_data (&focus_item_key);
      assert_return (&fitem == cfocus.focus_item && cfocus.uncross_id == 0);
      delete_data (&focus_item_key);
    }
}

void
WindowImpl::set_focus (ItemImpl *item)
{
  CurrentFocus cfocus = get_data (&focus_item_key);
  if (item == cfocus.focus_item)
    return;
  if (cfocus.focus_item)
    uncross_focus (*cfocus.focus_item);
  if (!item)
    return;
  // set new focus
  assert_return (item->has_ancestor (*this));
  cfocus.focus_item = item;
  cfocus.uncross_id = cross_link (*cfocus.focus_item, Aida::slot (*this, &WindowImpl::uncross_focus));
  set_data (&focus_item_key, cfocus);
  while (item)
    {
      ClassDoctor::item_set_flag (*item, FOCUS_CHAIN);
      ContainerImpl *fc = item->parent();
      if (fc)
        fc->set_focus_child (item);
      item = fc;
    }
}

cairo_surface_t*
WindowImpl::create_snapshot (const Rect &subarea)
{
  const Allocation area = allocation();
  Region region = area;
  region.intersect (subarea);
  cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, subarea.width, subarea.height);
  critical_unless (cairo_surface_status (surface) == CAIRO_STATUS_SUCCESS);
  cairo_surface_set_device_offset (surface, -subarea.x, -subarea.y);
  cairo_t *cr = cairo_create (surface);
  critical_unless (CAIRO_STATUS_SUCCESS == cairo_status (cr));
  render_into (cr, region);
  cairo_destroy (cr);
  return surface;
}

namespace WindowTrail {
static Mutex               wmutex;
static vector<WindowImpl*> windows;
static vector<WindowImpl*> wlist  ()               { ScopedLock<Mutex> slock (wmutex); return windows; }
static void wenter (WindowImpl *wi) { ScopedLock<Mutex> slock (wmutex); windows.push_back (wi); }
static void wleave (WindowImpl *wi)
{
  ScopedLock<Mutex> slock (wmutex);
  auto it = find (windows.begin(), windows.end(), wi);
  assert_return (it != windows.end());
  windows.erase (it);
};
} // WindowTrail

void
WindowImpl::forcefully_close_all ()
{
  vector<WindowImpl*> wl = WindowTrail::wlist();
  for (auto it : wl)
    it->close();
}

WindowImpl::WindowImpl() :
  m_loop (*ref_sink (uithread_main_loop()->new_slave())),
  m_screen_window (NULL), commands_emission_ (NULL),
  m_notify_displayed_id (0), m_entered (false), m_auto_close (false), m_pending_win_size (false), m_pending_expose (true)
{
  const_cast<AnchorInfo*> (force_anchor_info())->window = this;
  WindowTrail::wenter (this);
  Heritage *hr = ClassDoctor::window_heritage (*this, color_scheme());
  ref_sink (hr);
  ClassDoctor::set_window_heritage (*this, hr);
  unref (hr);
  set_flag (PARENT_SENSITIVE, true);
  set_flag (PARENT_VISIBLE, true);
  /* create event loop (auto-starts) */
  m_loop.exec_dispatcher (Aida::slot (*this, &WindowImpl::event_dispatcher), EventLoop::PRIORITY_NORMAL);
  m_loop.exec_dispatcher (Aida::slot (*this, &WindowImpl::resizing_dispatcher), PRIORITY_RESIZE);
  m_loop.exec_dispatcher (Aida::slot (*this, &WindowImpl::drawing_dispatcher), EventLoop::PRIORITY_UPDATE);
  m_loop.exec_dispatcher (Aida::slot (*this, &WindowImpl::command_dispatcher), EventLoop::PRIORITY_NOW);
  m_loop.flag_primary (false);
  ApplicationImpl::the().add_window (*this);
  change_flags_silently (ANCHORED, true);       /* window is always anchored */
}

void
WindowImpl::dispose ()
{
  ApplicationImpl::the().remove_window (*this);
}

WindowImpl::~WindowImpl()
{
  WindowTrail::wleave (this);
  if (m_notify_displayed_id)
    {
      m_loop.try_remove (m_notify_displayed_id);
      m_notify_displayed_id = 0;
    }
  if (m_screen_window)
    {
      m_screen_window->destroy();
      m_screen_window = NULL;
    }
  /* make sure all children are removed while this is still of type WindowImpl.
   * necessary because C++ alters the object type during constructors and destructors
   */
  if (has_children())
    remove (get_child());
  /* shutdown event loop */
  m_loop.kill_sources();
  /* this should be done last */
  unref (&m_loop);
  const_cast<AnchorInfo*> (force_anchor_info())->window = NULL;
}

bool
WindowImpl::self_visible () const
{
  return true;
}

void
WindowImpl::resize_window (const Allocation *new_area)
{
  const uint64 start = timestamp_realtime();
  assert_return (requisitions_tunable() == false);
  Requisition rsize;
  ScreenWindow::State state;
  bool allocated = false;

  // negotiate sizes (new_area==NULL) and ensures window is allocated
  negotiate_size (new_area);
  m_pending_expose = true;
  if (new_area)
    goto done;  // only called for reallocating
  rsize = requisition();

  // grow screen window if needed
  if (!m_screen_window)
    goto done;
  state = m_screen_window->get_state();
  if (state.width <= 0 || state.height <= 0 || rsize.width > state.width || rsize.height > state.height)
    {
      m_config.request_width = rsize.width;
      m_config.request_height = rsize.height;
      m_pending_win_size = true;
      discard_expose_region(); // we request a new WIN_SIZE event in configure
      m_screen_window->configure (m_config, true);
      goto done;
    }
  // screen window size is good, allocate it
  if (rsize.width != state.width || rsize.height != state.height)
    {
      Allocation area = Allocation (0, 0, state.width, state.height);
      negotiate_size (&area);
      allocated = true;
    }
 done:
  const uint64 stop = timestamp_realtime();
  Allocation area = new_area ? *new_area : allocated ? Allocation (0, 0, state.width, state.height) : Allocation (0, 0, rsize.width, rsize.height);
  EDEBUG ("RESIZE: request=%s allocate=%s elapsed=%.3fms",
          new_area ? "-" : string_printf ("%.0fx%.0f", rsize.width, rsize.height).c_str(),
          string_printf ("%.0fx%.0f", area.width, area.height).c_str(),
          (stop - start) / 1000.0);
}

void
WindowImpl::do_invalidate ()
{
  ViewportImpl::do_invalidate();
  // we just need to make sure to be woken up, since flags are set appropriately already
  m_loop.wakeup();
}

void
WindowImpl::beep()
{
  if (m_screen_window)
    m_screen_window->beep();
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
      if (unconfined or grab_item->screen_window_point (Point (event.x, event.y)))
        {
          pierced.push_back (ref (grab_item));        /* grab-item receives all mouse events */
          ContainerImpl *container = grab_item->interface<ContainerImpl*>();
          if (container)                              /* deliver to hovered grab-item children as well */
            container->screen_window_point_children (Point (event.x, event.y), pierced);
        }
    }
  else if (drawable())
    {
      pierced.push_back (ref (this)); /* window receives all mouse events */
      if (m_entered)
        screen_window_point_children (Point (event.x, event.y), pierced);
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
      screen_window_point_children (Point (event.x, event.y), pierced);
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
  if (item && item->process_screen_window_event (event))
    return true;
  const EventKey *kevent = dynamic_cast<const EventKey*> (&event);
  if (kevent && kevent->type == KEY_PRESS)
    {
      FocusDirType fdir = key_value_to_focus_dir (kevent->key);
      ActivateKeyType activate = key_value_to_activation (kevent->key);
      if (!handled && fdir)
        {
          if (!move_focus_dir (fdir))
            notify_key_error();
          handled = true;
        }
      if (!handled && (activate == ACTIVATE_FOCUS || activate == ACTIVATE_DEFAULT))
        {
          ItemImpl *focus_item = get_focus();
          if (focus_item && focus_item->sensitive())
            {
              if (!focus_item->activate())
                notify_key_error();
              handled = true;
            }
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
      m_pending_win_size = wevent->intermediate || has_queued_win_size();
      EDEBUG ("%s: %.0fx%.0f intermediate=%d pending=%d", string_from_event_type (event.type),
              wevent->width, wevent->height, wevent->intermediate, m_pending_win_size);
      const Allocation area = allocation();
      if (wevent->width != area.width || wevent->height != area.height)
        {
          Allocation new_area = Allocation (0, 0, wevent->width, wevent->height);
          if (!m_pending_win_size)
            {
#if 0 // excessive resizing costs
              Allocation zzz = new_area;
              resize_window (&zzz);
              for (int i = 0; i < 37; i++)
                {
                  zzz.width += 1;
                  resize_window (&zzz);
                  zzz.height += 1;
                  resize_window (&zzz);
                }
#endif
              resize_window (&new_area);
            }
          else
            discard_expose_region(); // we'll get more WIN_SIZE events
        }
      if (!m_pending_win_size && m_pending_expose)
        {
          expose();
          m_pending_expose = false;
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
    handled = dispatch_win_destroy();
  return handled;
}

bool
WindowImpl::dispatch_win_destroy ()
{
  destroy_screen_window();
  dispose();
  return true;
}

void
WindowImpl::notify_displayed()
{
  // emit updates at exec_update() priority, so other high priority handlers run first (exec_now)
  sig_displayed.emit();
}

void
WindowImpl::draw_child (ItemImpl &child)
{
  // FIXME: this should be optimized to just redraw the child in question
  WindowImpl *child_window = child.get_window();
  assert_return (child_window == this);
  draw_now();
}

void
WindowImpl::draw_now ()
{
  if (m_screen_window)
    {
      const uint64 start = timestamp_realtime();
      Rect area = allocation();
      assert_return (area.x == 0 && area.y == 0);
      // determine invalidated rendering region
      Region region = area;
      region.intersect (peek_expose_region());
      discard_expose_region();
      // rendering rectangle
      Rect rrect = region.extents();
      const int x1 = ifloor (rrect.x), y1 = ifloor (rrect.y), x2 = iceil (rrect.x + rrect.width), y2 = iceil (rrect.y + rrect.height);
      cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, x2 - x1, y2 - y1);
      cairo_surface_set_device_offset (surface, -x1, -y1);
      critical_unless (cairo_surface_status (surface) == CAIRO_STATUS_SUCCESS);
      cairo_t *cr = cairo_create (surface);
      critical_unless (CAIRO_STATUS_SUCCESS == cairo_status (cr));
      render_into (cr, region);
      m_screen_window->blit_surface (surface, region);
      cairo_destroy (cr);
      cairo_surface_destroy (surface);
      if (!m_notify_displayed_id)
        m_notify_displayed_id = m_loop.exec_update (Aida::slot (*this, &WindowImpl::notify_displayed));
      const uint64 stop = timestamp_realtime();
      EDEBUG ("RENDER: %+d%+d%+dx%d coverage=%.1f%% elapsed=%.3fms",
              x1, y1, x2 - x1, y2 - y1, ((x2 - x1) * (y2 - y1)) * 100.0 / (area.width*area.height),
              (stop - start) / 1000.0);
    }
  else
    discard_expose_region(); // nuke stale exposes
}

void
WindowImpl::render (RenderContext &rcontext, const Rect &rect)
{
  // paint background
  Color col = background();
  cairo_t *cr = cairo_context (rcontext, rect);
  cairo_set_source_rgba (cr, col.red1(), col.green1(), col.blue1(), col.alpha1());
  vector<Rect> rects;
  rendering_region (rcontext).list_rects (rects);
  for (size_t i = 0; i < rects.size(); i++)
    cairo_rectangle (cr, rects[i].x, rects[i].y, rects[i].width, rects[i].height);
  cairo_clip (cr);
  cairo_paint (cr);
  ViewportImpl::render (rcontext, rect);
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
  assert_return (child != NULL);
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
  assert_return (child != NULL);
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
  ViewportImpl::dispose_item (item);
}

bool
WindowImpl::has_queued_win_size ()
{
  return m_screen_window && m_screen_window->peek_events ([] (Event *e) { return e->type == WIN_SIZE; });
}

bool
WindowImpl::dispatch_event (const Event &event)
{
  if (!m_screen_window)
    return false;       // we can only handle events on a screen_window
  EDEBUG ("%s: w=%p", string_from_event_type (event.type), this);
  switch (event.type)
    {
    case EVENT_LAST:
    case EVENT_NONE:          return false;
    case MOUSE_ENTER:         return dispatch_enter_event (event);
    case MOUSE_MOVE:
      if (m_screen_window->peek_events ([] (Event *e) { return e->type == MOUSE_MOVE; }))
        return true; // coalesce multiple motion events
      else
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
    case SCROLL_UP:          // button4
    case SCROLL_DOWN:        // button5
    case SCROLL_LEFT:        // button6
    case SCROLL_RIGHT:       // button7
      ;                       return dispatch_scroll_event (event);
    case CANCEL_EVENTS:       return dispatch_cancel_event (event);
    case WIN_SIZE:            return dispatch_win_size_event (event);
    case WIN_DELETE:          return dispatch_win_delete_event (event);
    case WIN_DESTROY:         return dispatch_win_destroy();
    }
  return false;
}

bool
WindowImpl::event_dispatcher (const EventLoop::State &state)
{
  if (state.phase == state.PREPARE || state.phase == state.CHECK)
    return m_screen_window && m_screen_window->has_event();
  else if (state.phase == state.DISPATCH)
    {
      ref (this);
      Event *event = m_screen_window ? m_screen_window->pop_event() : NULL;
      if (event)
        {
          dispatch_event (*event);
          delete event;
        }
      unref (this);
      return true;
    }
  else if (state.phase == state.DESTROY)
    destroy_screen_window();
  return false;
}

bool
WindowImpl::resizing_dispatcher (const EventLoop::State &state)
{
  const bool can_resize = !m_pending_win_size && m_screen_window;
  const bool need_resize = can_resize && test_flags (INVALID_REQUISITION | INVALID_ALLOCATION);
  if (state.phase == state.PREPARE || state.phase == state.CHECK)
    return need_resize;
  else if (state.phase == state.DISPATCH)
    {
      ref (this);
      if (need_resize)
        resize_window();
      unref (this);
      return true;
    }
  return false;
}

void
WindowImpl::enable_auto_close ()
{
  m_auto_close = true;
}

bool
WindowImpl::drawing_dispatcher (const EventLoop::State &state)
{
  if (state.phase == state.PREPARE || state.phase == state.CHECK)
    return exposes_pending();
  else if (state.phase == state.DISPATCH)
    {
      ref (this);
      if (exposes_pending())
        {
          draw_now();
          if (m_auto_close)
            {
              EventLoop *loop = get_loop();
              if (loop)
                {
                  loop->exec_timer (0, Aida::slot (*this, &WindowImpl::destroy_screen_window), INT_MAX);
                  m_auto_close = false;
                }
            }
        }
      unref (this);
      return true;
    }
  return false;
}

bool
WindowImpl::prepare (const EventLoop::State &state,
                     int64                  *timeout_usecs_p)
{
  return false;
}

bool
WindowImpl::check (const EventLoop::State &state)
{
  return false;
}

bool
WindowImpl::dispatch (const EventLoop::State &state)
{
  return false;
}

EventLoop*
WindowImpl::get_loop ()
{
  return &m_loop;
}

bool
WindowImpl::viewable ()
{
  return visible() && m_screen_window && m_screen_window->viewable();
}

void
WindowImpl::idle_show()
{
  if (m_screen_window)
    {
      // size request & show up
      m_screen_window->show();
    }
}

void
WindowImpl::create_screen_window ()
{
  if (!finalizing())
    {
      if (!m_screen_window)
        {
          resize_window(); // ensure initial size requisition
          ScreenDriver *sdriver = ScreenDriver::retrieve_screen_driver ("auto");
          if (sdriver)
            {
              ScreenWindow::Setup setup;
              setup.window_type = WINDOW_TYPE_NORMAL;
              uint64 flags = ScreenWindow::ACCEPT_FOCUS | ScreenWindow::DELETABLE |
                             ScreenWindow::DECORATED | ScreenWindow::MINIMIZABLE | ScreenWindow::MAXIMIZABLE;
              setup.request_flags = ScreenWindow::Flags (flags);
              String prg = program_ident();
              if (prg.empty())
                prg = program_file();
              setup.session_role = "RAPICORN:" + prg;
              if (!name().empty())
                setup.session_role += ":" + name();
              setup.bg_average = background();
              const Requisition rsize = requisition();
              m_config.request_width = rsize.width;
              m_config.request_height = rsize.height;
              if (m_config.title.empty())
                {
                  // user_warning (this->user_source(), "window title is unset");
                  m_config.title = setup.session_role;
                }
              if (m_config.alias.empty())
                m_config.alias = program_alias();
              m_pending_win_size = true;
              m_screen_window = sdriver->create_screen_window (setup, m_config);
              m_screen_window->set_event_wakeup ([this] () { m_loop.wakeup(); /* thread safe */ });
            }
          else
            fatal ("failed to find and open any screen driver");
        }
      RAPICORN_ASSERT (m_screen_window != NULL);
      m_loop.flag_primary (true); // FIXME: depends on WM-managable
      EventLoop::VoidSlot sl = Aida::slot (*this, &WindowImpl::idle_show);
      m_loop.exec_now (sl);
    }
}

bool
WindowImpl::has_screen_window ()
{
  return !!m_screen_window;
}

void
WindowImpl::destroy_screen_window ()
{
  if (!m_screen_window)
    return; // during destruction, ref_count == 0
  ref (this);
  m_screen_window->destroy();
  m_screen_window = NULL;
  m_loop.flag_primary (false);
  m_loop.kill_sources();
  // reset item state where needed
  cancel_item_events (NULL);
  if (!finalizing())
    {
      m_loop.exec_dispatcher (Aida::slot (*this, &WindowImpl::event_dispatcher), EventLoop::PRIORITY_NORMAL);
      m_loop.exec_dispatcher (Aida::slot (*this, &WindowImpl::resizing_dispatcher), PRIORITY_RESIZE);
      m_loop.exec_dispatcher (Aida::slot (*this, &WindowImpl::drawing_dispatcher), EventLoop::PRIORITY_UPDATE);
      m_loop.exec_dispatcher (Aida::slot (*this, &WindowImpl::command_dispatcher), EventLoop::PRIORITY_NOW);
    }
  unref (this);
}

void
WindowImpl::show ()
{
  create_screen_window();
}

bool
WindowImpl::closed ()
{
  return !has_screen_window();
}

void
WindowImpl::close ()
{
  destroy_screen_window();
}

bool
WindowImpl::snapshot (const String &pngname)
{
  cairo_surface_t *isurface = this->create_snapshot (allocation());
  cairo_status_t wstatus = cairo_surface_write_to_png (isurface, pngname.c_str());
  cairo_surface_destroy (isurface);
  String err = CAIRO_STATUS_SUCCESS == wstatus ? "ok" : cairo_status_to_string (wstatus);
  DEBUG ("WindowImpl:snapshot:%s: failed to create \"%s\": %s", name().c_str(), pngname.c_str(), err.c_str());
  return CAIRO_STATUS_SUCCESS == wstatus;
}

bool
WindowImpl::synthesize_enter (double xalign,
                              double yalign)
{
  if (!has_screen_window())
    return false;
  const Allocation &area = allocation();
  Point p (area.x + xalign * (max (1, area.width) - 1),
           area.y + yalign * (max (1, area.height) - 1));
  p = point_to_screen_window (p);
  EventContext ec;
  ec.x = p.x;
  ec.y = p.y;
  m_screen_window->push_event (create_event_mouse (MOUSE_ENTER, ec));
  return true;
}

bool
WindowImpl::synthesize_leave ()
{
  if (!has_screen_window())
    return false;
  EventContext ec;
  m_screen_window->push_event (create_event_mouse (MOUSE_LEAVE, ec));
  return true;
}

bool
WindowImpl::synthesize_click (ItemIface &itemi,
                              int        button,
                              double     xalign,
                              double     yalign)
{
  ItemImpl &item = *dynamic_cast<ItemImpl*> (&itemi);
  if (!has_screen_window() || !&item)
    return false;
  const Allocation &area = item.allocation();
  Point p (area.x + xalign * (max (1, area.width) - 1),
           area.y + yalign * (max (1, area.height) - 1));
  p = item.point_to_screen_window (p);
  EventContext ec;
  ec.x = p.x;
  ec.y = p.y;
  m_screen_window->push_event (create_event_button (BUTTON_RELEASE, ec, button));
  m_screen_window->push_event (create_event_button (BUTTON_PRESS, ec, button));
  return true;
}

bool
WindowImpl::synthesize_delete ()
{
  if (!has_screen_window())
    return false;
  EventContext ec;
  m_screen_window->push_event (create_event_win_delete (ec));
  return true;
}

static const ItemFactory<WindowImpl> window_factory ("Rapicorn::Factory::Window");

} // Rapicorn
