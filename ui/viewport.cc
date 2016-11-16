// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "viewport.hh"
#include "factory.hh"
#include "uithread.hh"
#include "rcore/cairoutils.hh"

#define EDEBUG(...)           RAPICORN_KEY_DEBUG ("Events", __VA_ARGS__)
#define DEBUG_RESIZE(...)     RAPICORN_KEY_DEBUG ("Resize", __VA_ARGS__)
#define DEBUG_RENDER(...)     RAPICORN_KEY_DEBUG ("Render", __VA_ARGS__)

namespace Rapicorn {

ViewportImpl::ViewportImpl() :
  loop_ (uithread_main_loop()->create_slave()),
  display_window_ (NULL), commands_emission_ (NULL), immediate_event_hash_ (0),
  tunable_requisition_counter_ (0),
  auto_focus_ (true), entered_ (false), pending_win_size_ (false), pending_expose_ (true),
  need_resize_ (false)
{
  inherited_state_ = 0;
  config_.title = application_name();
  // create event loop (auto-starts)
  loop_->exec_dispatcher (Aida::slot (*this, &ViewportImpl::event_dispatcher), EventLoop::PRIORITY_NORMAL);
  loop_->exec_dispatcher (Aida::slot (*this, &ViewportImpl::drawing_dispatcher), EventLoop::PRIORITY_UPDATE);
  loop_->exec_dispatcher (Aida::slot (*this, &ViewportImpl::command_dispatcher), EventLoop::PRIORITY_NOW);
  loop_->flag_primary (false);
}

ViewportImpl::~ViewportImpl()
{
  assert_return (anchored() == false);
  if (has_display_window())
    {
      clear_immediate_event();
      display_window_->destroy();
      display_window_ = NULL;
    }
  // shutdown event loop
  loop_->destroy_loop();
  AncestryCache *ancestry_cache = const_cast<AncestryCache*> (ResizeContainerImpl::fetch_ancestry_cache());
  ancestry_cache->viewport = NULL;
}

const WidgetImpl::AncestryCache*
ViewportImpl::fetch_ancestry_cache ()
{
  AncestryCache *ancestry_cache = const_cast<AncestryCache*> (ResizeContainerImpl::fetch_ancestry_cache());
  ancestry_cache->viewport = this;
  return ancestry_cache;
}

void
ViewportImpl::beep()
{
  if (has_display_window())
    display_window_->beep();
}

String
ViewportImpl::title () const
{
  return config_.title;
}

void
ViewportImpl::title (const String &viewport_title)
{
  if (config_.title != viewport_title)
    {
      config_.title = viewport_title;
      if (has_display_window())
        display_window_->configure (config_, false);
      changed ("title");
    }
}

bool
ViewportImpl::auto_focus () const
{
  return auto_focus_;
}

void
ViewportImpl::auto_focus (bool afocus)
{
  if (afocus != auto_focus_)
    {
      auto_focus_ = afocus;
      changed ("auto_focus");
    }
}

bool
ViewportImpl::custom_command (const String &command_name, const StringSeq &command_args)
{
  assert_return (commands_emission_ == NULL, false);
  last_command_ = command_name;
  commands_emission_ = sig_commands.emission (command_name, command_args);
  return true;
}

bool
ViewportImpl::command_dispatcher (const LoopState &state)
{
  if (state.phase == state.PREPARE || state.phase == state.CHECK)
    return commands_emission_ && commands_emission_->pending();
  else if (state.phase == state.DISPATCH)
    {
      ViewportImplP guard_this = shared_ptr_cast<ViewportImpl> (this);
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

WidgetIfaceP
ViewportImpl::get_entered ()
{
  WidgetImplP widget = last_entered_children_.empty() ? NULL : last_entered_children_.back();
  if (widget && widget->anchored())
    return widget;
  return NULL;
}

struct CurrentFocus {
  WidgetImpl *focus_widget;
  size_t      uncross_id;
  CurrentFocus (WidgetImpl *f = NULL, size_t i = 0) : focus_widget (f), uncross_id (i) {}
};
static DataKey<CurrentFocus> focus_widget_key;

WidgetIfaceP
ViewportImpl::get_focus ()
{
  return shared_ptr_cast<WidgetIface> (get_data (&focus_widget_key).focus_widget);
}

void
ViewportImpl::uncross_focus (WidgetImpl &fwidget)
{
  CurrentFocus cfocus = get_data (&focus_widget_key);
  assert_return (&fwidget == cfocus.focus_widget);
  if (cfocus.uncross_id)
    {
      set_data (&focus_widget_key, CurrentFocus (cfocus.focus_widget, 0));      // reset cfocus.uncross_id
      cross_unlink (fwidget, cfocus.uncross_id);
      WidgetImpl *widget = &fwidget;
      while (widget)
        {
          widget->set_flag (FOCUS_CHAIN, false);
          ContainerImpl *fc = widget->parent();
          if (fc)
            fc->focus_lost();
          widget = fc;
        }
      cfocus = get_data (&focus_widget_key);
      assert_return (&fwidget == cfocus.focus_widget && cfocus.uncross_id == 0);
      delete_data (&focus_widget_key);                                          // reset cfocus.focus_widget
      fwidget.widget_adjust_state (WidgetState::FOCUSED, false);
    }
}

void
ViewportImpl::set_focus (WidgetImpl *widget)
{
  CurrentFocus cfocus = get_data (&focus_widget_key);
  if (widget == cfocus.focus_widget)
    return;
  if (cfocus.focus_widget)
    uncross_focus (*cfocus.focus_widget);
  if (!widget)
    return;
  // set new focus
  assert_return (widget->has_ancestor (*this));
  cfocus.focus_widget = widget;
  cfocus.uncross_id = cross_link (*cfocus.focus_widget, Aida::slot (*this, &ViewportImpl::uncross_focus));
  set_data (&focus_widget_key, cfocus);
  while (widget)
    {
      widget->set_flag (FOCUS_CHAIN, true);
      ContainerImpl *fc = widget->parent();
      if (fc)
        fc->set_focus_child (widget);
      widget = fc;
    }
  cfocus.focus_widget->widget_adjust_state (WidgetState::FOCUSED, true);
}

cairo_surface_t*
ViewportImpl::create_snapshot (const IRect &subarea)
{
  const IRect rect = subarea.intersection (allocation());
  cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, subarea.width, subarea.height);
  CAIRO_CHECK_STATUS (surface);
  cairo_surface_set_device_offset (surface, -subarea.x, -subarea.y);
  cairo_t *cr = cairo_create (surface);
  CAIRO_CHECK_STATUS (cr);
  vector<IRect> irects;
  irects.push_back (rect);
  compose_into (cr, irects);
  cairo_destroy (cr);
  return surface;
}

void
ViewportImpl::expose_region (const Region &region)
{
  if (!region.empty())
    {
      expose_region_.add (region);
      collapse_expose_region();
    }
}

void
ViewportImpl::collapse_expose_region ()
{
  // check for excess expose fragment scenarios
  uint n_erects = expose_region_.count_rects();
  /* considering O(n^2) collision computation complexity, but also focus frame
   * exposures which easily consist of 4+ fragments, a hundred rectangles turn
   * out to be an emperically suitable threshold.
   */
  if (n_erects > 99)
    {
      /* aparently the expose fragments we're combining are too small,
       * so we can end up with spending too much time on expose rectangle
       * compression (more time than needed for actual rendering).
       * as a workaround, we simply force everything into a single expose
       * rectangle which is good enough to avoid worst case explosion.
       */
      expose_region_.add (expose_region_.extents());
      DEBUG_RENDER ("collapsing expose rectangles due to overflow: %u -> %u", n_erects, expose_region_.count_rects());
    }
}

// Return @a widgets without any of @a removes, preserving the original order.
vector<WidgetImplP>
ViewportImpl::widget_difference (const vector<WidgetImplP> &widgets, const vector<WidgetImplP> &removes)
{
  std::set<WidgetImpl*> mminus;
  for (auto &delme : removes)
    mminus.insert (&*delme);
  vector<WidgetImplP> remainings;
  if (widgets.size() > removes.size())
    remainings.reserve (widgets.size() - removes.size());
  for (auto &candidate : widgets)
    if (mminus.find (&*candidate) == mminus.end())
      remainings.push_back (candidate);
  return remainings;
}

bool
ViewportImpl::dispatch_mouse_movement (const Event &event)
{
  last_event_context_ = event;
  vector<WidgetImplP> pierced; // list of entered widgets
  // construct list of widgets eligible to receive events
  bool constrained = false;
  WidgetImpl *grab_widget = get_grab (&constrained);
  if (grab_widget && grab_widget->drawable())
    {
      pierced.push_back (shared_ptr_cast<WidgetImpl> (grab_widget));    // grab_widget always receives events
      if (grab_widget->point (grab_widget->point_from_event (event)))
        {
          ContainerImpl *container = grab_widget->as_container_impl();
          if (container)                                                // deliver events to grab-widget children
            container->point_descendants (container->point_from_event (event), pierced);
        }
    }
  else if (!grab_widget && drawable() && entered_)
    {
      pierced.push_back (shared_ptr_cast<WidgetImpl> (this));           // deliver to entered viewport
      point_descendants (point_from_event (event), pierced);
    }
  // send leave events - not "stolen" by grab_widget, so other widgets are notified about the grab through leave-event
  vector<WidgetImplP> left_children = widget_difference (last_entered_children_, pierced);
  EventMouse *leave_event = create_event_mouse (MOUSE_LEAVE, EventContext (event));
  for (vector<WidgetImplP>::reverse_iterator it = left_children.rbegin(); it != left_children.rend(); it++)
    (*it)->process_event (*leave_event);
  delete leave_event;
  left_children.clear();
  // send enter events - for grabs, 'pierced' was setup accordingly above
  vector<WidgetImplP> insensitive_widgets;
  vector<WidgetImplP> entered_children = widget_difference (pierced, last_entered_children_);
  EventMouse *enter_event = create_event_mouse (MOUSE_ENTER, EventContext (event));
  for (vector<WidgetImplP>::reverse_iterator it = entered_children.rbegin(); it != entered_children.rend(); it++)
    if ((*it)->sensitive())
      (*it)->process_event (*enter_event);
    else
      insensitive_widgets.push_back (*it);
  delete enter_event;
  entered_children.clear();
  // filter insensitive widgets, which we kept from getting enter-event
  if (insensitive_widgets.size())
    pierced = widget_difference (pierced, insensitive_widgets);
  insensitive_widgets.clear();
  // send actual move event - for grabs, 'pierced' was setup accordingly above
  bool handled = false;
  EventMouse *move_event = create_event_mouse (MOUSE_MOVE, EventContext (event));
  for (vector<WidgetImplP>::reverse_iterator it = pierced.rbegin(); it != pierced.rend(); it++)
    if (!handled)
      handled = (*it)->process_event (*move_event);
  delete move_event;
  // update entered children
  last_entered_children_.swap (pierced);
  if (last_entered_children_.size() != pierced.size() ||
      (last_entered_children_.size() > 0 && last_entered_children_.back() != pierced.back()))
    changed ("get_entered");
  return handled;
}

/// Dispatch event to previously entered widgets.
bool
ViewportImpl::dispatch_event_to_entered (const Event &event)
{
  const vector<WidgetImplP> &pierced = last_entered_children_;
  // send actual event
  bool handled = false;
  for (auto it = pierced.rbegin(); it != pierced.rend() && !handled; it++)
    if ((*it)->sensitive())
      handled = (*it)->process_event (event);
  return handled;
}

bool
ViewportImpl::dispatch_button_press (const EventButton &bevent)
{
  uint press_count = bevent.type - BUTTON_PRESS + 1;
  assert (press_count >= 1 && press_count <= 3);
  // figure all entered children
  const vector<WidgetImplP> pierced = last_entered_children_; // use copy to beware of modifications
  // event propagation, capture phase - capture_event(press_event) is always paired with capture_event(release_event) or reset/CANCELED
  bool handled = false;
  for (vector<WidgetImplP>::const_iterator it = pierced.begin(); !handled && it != pierced.end(); it++)
    {
      WidgetImpl &widget = **it;
      if (widget.anchored() && widget.sensitive())
        {
          ButtonState bs (monotonic_counter(), widget, bevent.button, true);
          if (button_state_map_[bs] == 0)                               // no press delivered for <button> on <widget> yet
            {
              button_state_map_[bs] = press_count;                      // record press of bevent.button for pairing
              handled = widget.process_event (bevent, true);    // may modify last_entered_children_ + this
            }
        }
    }
  // event propagation, bubble phase - handle_event(press_event) is always paired with handle_event(release_event) or reset/CANCELED
  for (vector<WidgetImplP>::const_reverse_iterator it = pierced.rbegin(); !handled && it != pierced.rend(); it++)
    {
      WidgetImpl &widget = **it;
      if (widget.anchored() && widget.sensitive())
        {
          ButtonState bs (monotonic_counter(), widget, bevent.button);
          if (button_state_map_[bs] == 0)                               // no press delivered for <button> on <widget> yet
            {
              button_state_map_[bs] = press_count;                      // record press of bevent.button for pairing
              handled = widget.process_event (bevent);  // may modify last_entered_children_ + this
            }
        }
    }
  return handled;
}

ViewportImpl::ButtonStateMap::iterator
ViewportImpl::button_state_map_find_earliest (const uint button, const bool captured)
{
  ButtonStateMap::iterator earliest_it = button_state_map_.end();
  uint64 earliest_serializer = ~uint64 (0);
  // find first iterator matching arguments ordered by serializer
  for (ButtonStateMap::iterator it = button_state_map_.begin(); it != button_state_map_.end(); ++it)
    {
      const ButtonState &bs = it->first;
      if (bs.button == button && bs.captured == captured && bs.serializer < earliest_serializer)
        {
          earliest_it = it;
          earliest_serializer = bs.serializer;
        }
    }
  return earliest_it;
}

bool
ViewportImpl::dispatch_button_release (const EventButton &bevent)
{
  bool handled = false;
  // event propagation, bubble phase - handle_event(release/CANCELED) for previous handle_event(press_event)
  for (ButtonStateMap::iterator       it = button_state_map_find_earliest (bevent.button, false); // ordering implied by ButtonState.serializer
       it != button_state_map_.end(); it = button_state_map_find_earliest (bevent.button, false))
    {
      const ButtonState bs = it->first; // uint press_count = it->second;
#if 0 // FIXME
      if (press_count == 3)
        bevent.type = BUTTON_3RELEASE;
      else if (press_count == 2)
        bevent.type = BUTTON_2RELEASE;
      // bevent.type = BUTTON_RELEASE;
#endif
      button_state_map_.erase (it);
      handled |= bs.widget.process_event (bevent, bs.captured); // modifies button_state_map_ + this
    }
  // event propagation, capture phase - capture_event(release/CANCELED) for previous capture_event(press_event)
  for (ButtonStateMap::iterator       it = button_state_map_find_earliest (bevent.button, true); // ordering implied by ButtonState.serializer
       it != button_state_map_.end(); it = button_state_map_find_earliest (bevent.button, true))
    {
      const ButtonState bs = it->first; // uint press_count = it->second;
#if 0 // FIXME
      if (press_count == 3)
        bevent.type = BUTTON_3RELEASE;
      else if (press_count == 2)
        bevent.type = BUTTON_2RELEASE;
      // bevent.type = BUTTON_RELEASE;
#endif
      button_state_map_.erase (it);
      handled |= bs.widget.process_event (bevent, bs.captured); // modifies button_state_map_ + this
    }
  return handled;
}

void
ViewportImpl::cancel_widget_events (WidgetImpl *widget)
{
  // cancel enter events
  for (int i = last_entered_children_.size(); i > 0;)
    {
      WidgetImplP current = last_entered_children_[--i]; /* walk backwards */
      if (widget == &*current || !widget)
        {
          last_entered_children_.erase (last_entered_children_.begin() + i);
          EventMouse *mevent = create_event_mouse (MOUSE_LEAVE, last_event_context_);
          current->process_event (*mevent);
          delete mevent;
        }
    }
  // cancel button press events
  while (button_state_map_.begin() != button_state_map_.end())
    {
      ButtonStateMap::iterator it = button_state_map_.begin();
      const ButtonState bs = it->first;
      button_state_map_.erase (it);
      if (&bs.widget == widget || !widget)
        {
          EventButton *bevent = create_event_button (BUTTON_CANCELED, last_event_context_, bs.button);
          bs.widget.process_event (*bevent, bs.captured); // modifies button_state_map_ + this
          delete bevent;
        }
    }
}

bool
ViewportImpl::dispatch_cancel_event (const Event &event)
{
  cancel_widget_events (NULL);
  return false;
}

bool
ViewportImpl::dispatch_enter_event (const EventMouse &mevent)
{
  entered_ = true;
  dispatch_mouse_movement (mevent);
  return false;
}

bool
ViewportImpl::dispatch_move_event (const EventMouse &mevent)
{
  dispatch_mouse_movement (mevent);
  return false;
}

bool
ViewportImpl::dispatch_leave_event (const EventMouse &mevent)
{
  dispatch_mouse_movement (mevent);
  entered_ = false;
  return false;
}

bool
ViewportImpl::dispatch_button_event (const Event &event)
{
  bool handled = false;
  const EventButton *bevent = dynamic_cast<const EventButton*> (&event);
  if (bevent)
    {
      ensure_resized(); // ensure coordinates are interpreted with correct allocations
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
ViewportImpl::dispatch_focus_event (const EventFocus &fevent)
{
  bool handled = false;
  // dispatch_event_to_entered (*fevent);
  dispatch_mouse_movement (fevent);
  return handled;
}

bool
ViewportImpl::move_container_focus (ContainerImpl &container, FocusDir focus_dir)
{
  assert_return (container.has_ancestor (*this), false);
  WidgetImplP keep_focus = NULL, old_focus = shared_ptr_cast<WidgetImpl> (get_focus());
  switch (focus_dir)
    {
    case FocusDir::UP:   case FocusDir::DOWN:
    case FocusDir::LEFT: case FocusDir::RIGHT:
      keep_focus = old_focus;
      break;
    default: ;
    }
  if (focus_dir != FocusDir::NONE && !container.move_focus (focus_dir))
    {
      if (keep_focus && keep_focus->get_viewport() != this)
        keep_focus = NULL;
      if (keep_focus)
        keep_focus->grab_focus();        // ensure to keep directional focus widget (usually arrows)
      else
        set_focus (NULL);
      if (old_focus == get_focus())
        return false;                   // should have moved focus but failed
    }
  if (old_focus && !get_focus() && (focus_dir == FocusDir::NEXT || focus_dir == FocusDir::PREV))
    {
      // wrap around for ordered focus moves (usually Tab)
      return container.move_focus (focus_dir);
    }
  return true;
}

bool
ViewportImpl::dispatch_key_event (const Event &event)
{
  ensure_resized(); // ensure coordinates are interpreted with correct allocations
  dispatch_mouse_movement (event);
  // find target widget
  WidgetImpl *target, *grab_widget = get_grab(), *focus_widget = get_focus_widget();
  if (focus_widget && (!grab_widget || focus_widget->has_ancestor (*grab_widget)))
    target = focus_widget;
  else if (grab_widget)
    target = grab_widget;
  else
    target = this;
  // try delivery
  if (target->key_sensitive())
    {
      if (target->process_event (event) == true)
        return true;
      grab_widget = get_grab();
      focus_widget = get_focus_widget();
    }
  // some key presses need further processing
  const EventKey *kevent = dynamic_cast<const EventKey*> (&event);
  if (!kevent || kevent->type != KEY_PRESS)
    return false;
  // activate focus / default
  const ActivateKeyType activate = key_value_to_activation (kevent->key);
  if (activate == ACTIVATE_FOCUS || activate == ACTIVATE_DEFAULT)
    {
      if (focus_widget && focus_widget->key_sensitive() && (!grab_widget || focus_widget->has_ancestor (*grab_widget)))
        {
          if (!focus_widget->activate())
            notify_key_error();
          return true;
        }
    }
  // move focus
  const FocusDir fdir = key_value_to_focus_dir (kevent->key);
  if (fdir != FocusDir::NONE)
    {
      ContainerImpl *container = grab_widget ? grab_widget->as_container_impl() : this;
      if (container && container->key_sensitive())
        {
          if (!move_container_focus (*container, fdir))
            notify_key_error();
          return true;
        }
    }
  return false;
}

bool
ViewportImpl::dispatch_data_event (const Event &event)
{
  dispatch_mouse_movement (event);
  WidgetImpl *focus_widget = get_focus_widget();
  if (focus_widget && focus_widget->key_sensitive() && focus_widget->process_event (event))
    return true;
  else if (event.type == CONTENT_REQUEST)
    {
      // CONTENT_REQUEST events must be answered
      const EventData *devent = dynamic_cast<const EventData*> (&event);
      provide_content ("", "", devent->request_id); // no-type, i.e. reject request
      return true;
    }
  else
    return false;
}

bool
ViewportImpl::dispatch_scroll_event (const EventScroll &sevent)
{
  bool handled = false;
  if (sevent.type == SCROLL_UP || sevent.type == SCROLL_RIGHT ||
      sevent.type == SCROLL_DOWN || sevent.type == SCROLL_LEFT)
    {
      dispatch_mouse_movement (sevent);
      handled = dispatch_event_to_entered (sevent);
    }
  return handled;
}
bool
ViewportImpl::dispatch_win_size_event (const Event &event)
{
  bool handled = false;
  const EventWinSize *wevent = dynamic_cast<const EventWinSize*> (&event);
  if (wevent)
    {
      pending_win_size_ = wevent->intermediate || has_queued_win_size();
      EDEBUG ("%s: %.0fx%.0f intermediate=%d pending=%d", string_from_event_type (event.type),
              wevent->width, wevent->height, wevent->intermediate, pending_win_size_);
      DEBUG_RESIZE ("%s: %.0fx%.0f intermediate=%d pending=%d", string_from_event_type (event.type),
                    wevent->width, wevent->height, wevent->intermediate, pending_win_size_);
      const Allocation area = allocation();
      if (wevent->width != area.width || wevent->height != area.height)
        {
          Allocation new_area = Allocation (0, 0, wevent->width, wevent->height);
          if (!pending_win_size_)
            {
#if 0 // excessive resizing costs
              Allocation zzz = new_area;
              resize_redraw (&zzz);
              for (int i = 0; i < 37; i++)
                {
                  zzz.width += 1;
                  resize_redraw (&zzz);
                  zzz.height += 1;
                  resize_redraw (&zzz);
                }
#endif
              resize_redraw (&new_area);
            }
          else
            discard_expose_region(); // we'll get more WIN_SIZE events
        }
      if (!pending_win_size_ && pending_expose_)
        {
          expose();
          pending_expose_ = false;
        }
      handled = true;
    }
  return handled;
}

bool
ViewportImpl::dispatch_win_delete_event (const Event &event)
{
  bool handled = false;
  const EventWinDelete *devent = dynamic_cast<const EventWinDelete*> (&event);
  if (devent)
    handled = dispatch_win_destroy();
  return handled;
}

bool
ViewportImpl::dispatch_win_destroy ()
{
  destroy();
  return true;
}

void
ViewportImpl::draw_child (WidgetImpl &child)
{
  // FIXME: this should be optimized to just redraw the child in question
  ViewportImpl *child_viewport = child.get_viewport();
  assert_return (child_viewport == this);
  draw_now();
}

void
ViewportImpl::draw_now ()
{
  if (has_display_window())
    {
      const uint64 start = timestamp_realtime();
      IRect area = allocation();
      assert_return (area.x == 0 && area.y == 0);
      // determine invalidated rendering region
      Region region = area;
      region.intersect (peek_expose_region());
      discard_expose_region();
      // create compose surface extents
      IRect rrect = region.extents();
      const int x1 = rrect.x, y1 = rrect.y, x2 = rrect.x + rrect.width, y2 = rrect.y + rrect.height;
      cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, x2 - x1, y2 - y1);
      cairo_surface_set_device_offset (surface, -x1, -y1);
      CAIRO_CHECK_STATUS (surface);
      cairo_t *cr = cairo_create (surface);
      CAIRO_CHECK_STATUS (cr);
      // test code
      cairo_rectangle (cr, area.x, area.y, area.width, area.height);
      cairo_set_source_rgb (cr, 0, 0, 1);
      cairo_paint (cr);
      // compose into region rectangles
      vector<IRect> irects;
      region.list_rects (irects);
      compose_into (cr, irects);
      // and blit contents onto the screen
      display_window_->blit_surface (surface, region);
      cairo_destroy (cr);
      cairo_surface_destroy (surface);
      // notify "displayed" at PRIORITY_UPDATE, so other high priority handlers run first
      loop_->exec_callback ([this] () {
          if (has_display_window())
            sig_displayed.emit();
        }, EventLoop::PRIORITY_UPDATE);
      const uint64 stop = timestamp_realtime();
      DEBUG_RENDER ("%+d%+d%+dx%d coverage=%.1f%% elapsed=%.3fms",
                    x1, y1, x2 - x1, y2 - y1, ((x2 - x1) * (y2 - y1)) * 100.0 / (area.width*area.height),
                    (stop - start) / 1000.0);
    }
  else
    discard_expose_region(); // nuke stale exposes
}

void
ViewportImpl::render (RenderContext &rcontext)
{
  // paint background
  Color col = background();
  cairo_t *cr = cairo_context (rcontext);
  cairo_set_source_rgba (cr, col.red1(), col.green1(), col.blue1(), col.alpha1());
  vector<DRect> drects;
  rendering_region (rcontext).list_rects (drects);
  for (size_t i = 0; i < drects.size(); i++)
    cairo_rectangle (cr, drects[i].x, drects[i].y, drects[i].width, drects[i].height);
  cairo_clip (cr);
  cairo_paint (cr);
}

void
ViewportImpl::remove_grab_widget (WidgetImpl &child)
{
  bool stack_changed = false;
  for (int i = grab_stack_.size() - 1; i >= 0; i--)
    if (grab_stack_[i].widget == &child)
      {
        grab_stack_.erase (grab_stack_.begin() + i);
        stack_changed = true;
      }
  if (stack_changed)
    grab_stack_changed();
}

void
ViewportImpl::grab_stack_changed()
{
  // FIXME: use idle handler for event synthesis
  EventMouse *mevent = create_event_mouse (MOUSE_LEAVE, last_event_context_);
  dispatch_mouse_movement (*mevent);
  // synthesize neccessary leaves after grabbing
  if (!grab_stack_.size() && !entered_)
    dispatch_event (*mevent);
  delete mevent;
}

void
ViewportImpl::add_grab (WidgetImpl &child, bool constrained)
{
  if (!child.has_ancestor (*this))
    throw Exception ("child is not descendant of container \"", id(), "\": ", child.id());
  /* for constrained==true grabs, the mouse pointer is always considered to
   * be contained by the grab-widget, and only by the grab-widget. events are
   * delivered to the grab-widget and its children.
   */
  grab_stack_.push_back (GrabEntry (&child, constrained));
  // grab_stack_changed(); // FIXME: re-enable this, once grab_stack_changed() synthesizes from idler
}

bool
ViewportImpl::remove_grab (WidgetImpl *child)
{
  assert_return (child != NULL, false);
  return remove_grab (*child);
}

bool
ViewportImpl::remove_grab (WidgetImpl &child)
{
  for (int i = grab_stack_.size() - 1; i >= 0; i--)
    if (grab_stack_[i].widget == &child)
      {
        grab_stack_.erase (grab_stack_.begin() + i);
        grab_stack_changed();
        return true;
      }
  return false;
}

WidgetImpl*
ViewportImpl::get_grab (bool *constrained)
{
  for (int i = grab_stack_.size() - 1; i >= 0; i--)
    if (grab_stack_[i].widget->visible())
      {
        if (constrained)
          *constrained = grab_stack_[i].constrained;
        return &*grab_stack_[i].widget;
      }
  return NULL;
}

bool
ViewportImpl::is_grabbing (WidgetImpl &descendant)
{
  for (size_t i = 0; i < grab_stack_.size(); i++)
    if (grab_stack_[i].widget == &descendant)
      return true;
  return false;
}

void
ViewportImpl::dispose_widget (WidgetImpl &widget)
{
  remove_grab_widget (widget);
  cancel_widget_events (widget);
  ResizeContainerImpl::dispose_widget (widget);
}

bool
ViewportImpl::has_queued_win_size ()
{
  return has_display_window() && display_window_->peek_events ([] (Event *e) { return e->type == WIN_SIZE; });
}

bool
ViewportImpl::dispatch_event (const Event &event)
{
  if (!has_display_window())
    return false;       // we can only handle events on a display_window
  EDEBUG ("%s: w=%p", string_from_event_type (event.type), this);
  switch (event.type)
    {
    case EVENT_LAST:
    case EVENT_NONE:          return false;
    case MOUSE_ENTER:         return dispatch_enter_event (event);
    case MOUSE_MOVE:
      if (display_window_->peek_events ([] (Event *e) { return e->type == MOUSE_MOVE; }))
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
    case CONTENT_DATA:
    case CONTENT_CLEAR:
    case CONTENT_REQUEST:     return dispatch_data_event (event);
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
ViewportImpl::event_dispatcher (const LoopState &state)
{
  if (state.phase == state.PREPARE || state.phase == state.CHECK)
    return has_display_window() && display_window_->has_event();
  else if (state.phase == state.DISPATCH)
    {
      const WidgetImplP guard_this = shared_ptr_cast<WidgetImpl> (this);
      Event *event = has_display_window() ? display_window_->pop_event() : NULL;
      if (event)
        {
          if (immediate_event_hash_ == size_t (event))
            clear_immediate_event();
          dispatch_event (*event);
          delete event;
        }
      return true;
    }
  else if (state.phase == state.DESTROY)
    destroy_display_window();
  return false;
}

bool
ViewportImpl::immediate_event_dispatcher (const LoopState &state)
{
  switch (state.phase)
    {
    case LoopState::PREPARE:
    case LoopState::CHECK:
      return true;
    case LoopState::DISPATCH:
      {
        const WidgetImplP guard_this = shared_ptr_cast<WidgetImpl> (this);
        event_dispatcher (state);
        return immediate_event_hash_ != 0; // destroy if there are no more immediate events
      }
    default:
      return false;
    }
}

void
ViewportImpl::push_immediate_event (Event *event)
{
  assert_return (has_display_window() == true);
  assert_return (event != NULL);
  display_window_->push_event (event);
  if (immediate_event_hash_ == 0)
    {
      // force immediate (synthesized) event processing before further uithread main loop RPC handling
      loop_->exec_dispatcher (Aida::slot (*this, &ViewportImpl::immediate_event_dispatcher), EventLoop::PRIORITY_NOW);
      immediate_event_hash_ = size_t (event);
    }
}

void
ViewportImpl::clear_immediate_event ()
{
  return_unless (immediate_event_hash_ != 0);
  immediate_event_hash_ = 0;
}

void
ViewportImpl::queue_resize_redraw()
{
  if (!need_resize_)
    {
      need_resize_ = true;
      loop_->wakeup();
    }
}

void
ViewportImpl::ensure_resized()
{
  if (need_resize_)
    resize_redraw (NULL, true);
}

WidgetImpl::WidgetFlag
ViewportImpl::check_widget_requisition (WidgetImpl &widget, bool discard_tuned)
{
  if (discard_tuned)
    widget.invalidate_requisition();    // discard requisitions tuned to previous allocations
  ContainerImpl *container = widget.as_container_impl();
  uint64 invalidation_flags = 0;
  if (RAPICORN_LIKELY (container))
    for (auto &child : *container)
      if (RAPICORN_LIKELY (child->visible()))
        invalidation_flags |= check_widget_requisition (*child, discard_tuned);
  if (RAPICORN_UNLIKELY (widget.test_any (INVALID_REQUISITION)))
    widget.requisition();               // does size_request and clears INVALID_REQUISITION
  invalidation_flags |= widget.widget_flags_ & (INVALID_REQUISITION | INVALID_ALLOCATION | INVALID_CONTENT);
  return WidgetFlag (invalidation_flags);
}

WidgetImpl::WidgetFlag
ViewportImpl::check_widget_allocation (WidgetImpl &widget)
{
  return_unless (widget.visible(), WidgetFlag (0));
  if (need_resize_)
    {
      /* if *any* descendant needs size_request, we need to stop
       * allocating (forcing) inadequate sizes and start over.
       */
      return INVALID_REQUISITION;
    }
  if (widget.test_any (INVALID_ALLOCATION))
    widget.set_child_allocation (widget.child_allocation()); // clears INVALID_ALLOCATION
  uint64 invalidation_flags = 0;
  ContainerImpl *container = widget.as_container_impl();
  if (container)
    for (auto &child : *container)
      invalidation_flags |= check_widget_allocation (*child);
  invalidation_flags |= widget.widget_flags_ & (INVALID_REQUISITION | INVALID_ALLOCATION | INVALID_CONTENT);
  return WidgetFlag (invalidation_flags);
}

WidgetImpl::WidgetFlag
ViewportImpl::pop_need_resize()
{
  if (need_resize_)
    {
      need_resize_ = false;
      return INVALID_REQUISITION;
    }
  return WidgetFlag (0);
}

WidgetImpl::WidgetFlag
ViewportImpl::negotiate_sizes (const Allocation *new_viewport_area)
{
  const uint64 INVALID_SIZE = INVALID_REQUISITION | INVALID_ALLOCATION;
  uint64 invalidation_flags = pop_need_resize();
  // ensure all widgets have a valid size requisition
  while (invalidation_flags & INVALID_REQUISITION)              // widget implementations must avoid an endless requisition loop
    {
      invalidation_flags = check_widget_requisition (*this, false);
      invalidation_flags |= pop_need_resize();
    }
  // assign new size allocation if provided
  tunable_requisition_counter_ = 3;
  if (new_viewport_area)
    set_child_allocation (*new_viewport_area);
  /* negotiate and allocate sizes, initially allow tuning and re-allocations:
   * - widgets can tune_requisition() during size_allocate() to negotiate width
   *   for height or vice versa.
   * - whether the tuned requisition is honored at all, depends on
   *   tunable_requisition_counter_ which is adjusted after a few iterations
   *   to enforce a halt of the negotiation process.
   * - requisitions tuned to the current allocation are re-considered by the
   *   widget's ancestors, see WidgetImpl::tune_requisition().
   */
  for (/**/; tunable_requisition_counter_; tunable_requisition_counter_--)
    {
      if (test_any (INVALID_ALLOCATION))
        {
          if (new_viewport_area)
            set_child_allocation (child_allocation());
          else
            {
              const Requisition rsize = requisition();
              set_child_allocation (Allocation (0, 0, rsize.width, rsize.height));
            }
        }
      do
        invalidation_flags = check_widget_allocation (*this);   // aborts if need_resize_!=0
      while (INVALID_ALLOCATION == (invalidation_flags & INVALID_SIZE));
      invalidation_flags |= pop_need_resize();
      while (invalidation_flags & INVALID_REQUISITION)
        invalidation_flags = check_widget_requisition (*this, false);
      if (0 == (invalidation_flags & INVALID_ALLOCATION))
        break;                                                  // !INVALID_REQUISITION and !INVALID_ALLOCATION
    }
  tunable_requisition_counter_ = 0;
  // fixate last allocations (if still needed)
  while (invalidation_flags & INVALID_SIZE)
    {
      while (invalidation_flags & INVALID_REQUISITION)
        invalidation_flags = check_widget_requisition (*this, false);
      while (invalidation_flags & INVALID_ALLOCATION)
        invalidation_flags = check_widget_allocation (*this);
      invalidation_flags |= pop_need_resize();
    }
  return WidgetFlag (invalidation_flags);
}

void
ViewportImpl::negotiate_initial_size()
{
  Allocation area;
  // first, get a simple, untuned size requisition from all widgets
  invalidate_size();
  check_widget_requisition (*this, /* discard_tuned */ true);
  // second, negotiate ideal size based on requisition
  negotiate_sizes (NULL);
}

bool
ViewportImpl::can_resize_redraw()
{
  return !pending_win_size_ && has_display_window() && (need_resize_ || exposes_pending());
}

void
ViewportImpl::resize_redraw (const Allocation *new_viewport_area, bool resize_only)
{
  const uint64 resize_start = timestamp_realtime();
  if (new_viewport_area)
    invalidate_allocation();                            // sets need_resize_
  return_unless (can_resize_redraw());                  // check need_resize_
  // if we have a new allocation, previous tunings are useless
  const bool discard_previous_tunings = new_viewport_area != NULL;
  if (discard_previous_tunings)
    check_widget_requisition (*this, true);
  // negotiate ideal size within allocation
  const Allocation current_allocation = allocation();
  const uint64 invalidation_flags = negotiate_sizes (new_viewport_area ? new_viewport_area : &current_allocation);
  // check if we fit the display window
  maybe_resize_viewport();
  // redraw resized contents
  const uint64 redraw_start = timestamp_realtime();
  const String fixme_dbg = peek_expose_region().extents().string();
  if (resize_only)
    need_resize_ = true;                                // must redraw later
  else if (!need_resize_ && !pending_win_size_ && (exposes_pending() || invalidation_flags & INVALID_CONTENT))
    {
      render_widget();
      draw_now();
    }
  const uint64 stop = timestamp_realtime();
  DEBUG_RESIZE ("request=%s allocate=%s pws=%d expose=%s resize_elapsed=%.3fms redraw_elapsed=%.3fms nresize=%d",
                new_viewport_area ? "-" : string_format ("%.0fx%.0f", requisition().width, requisition().height),
                string_format ("%.0fx%.0f", allocation().width, allocation().height),
                pending_win_size_, fixme_dbg,
                (redraw_start - resize_start) / 1000.0,
                (!resize_only) * (stop - redraw_start) / 1000.0,
                need_resize_);
}

void
ViewportImpl::maybe_resize_viewport()
{
  const Requisition rsize = requisition();
  if (has_display_window())
    {
      // grow display window if needed
      const DisplayWindow::State state = display_window_->get_state();
      if (state.width <= 0 || state.height <= 0 ||
          ((rsize.width > state.width || rsize.height > state.height) &&
           (config_.request_width != rsize.width || config_.request_height != rsize.height)))
        {
          const int oldreq_width = config_.request_width, oldreq_height = config_.request_height;
          config_.request_width = rsize.width;
          config_.request_height = rsize.height;
          pending_win_size_ = true;
          discard_expose_region(); // we request a new WIN_SIZE event in configure
          display_window_->configure (config_, true);
          DEBUG_RESIZE ("resize-viewport: %s: old=%dx%d new_config=%dx%d", id(), oldreq_width, oldreq_height, config_.request_width, config_.request_height);
        }
    }
}

bool
ViewportImpl::drawing_dispatcher (const LoopState &state)
{
  if (state.phase == state.PREPARE || state.phase == state.CHECK)
    {
      return can_resize_redraw();
    }
  else if (state.phase == state.DISPATCH)
    {
      const WidgetImplP guard_this = shared_ptr_cast<WidgetImpl> (this);
      resize_redraw (NULL);
      return true;
    }
  return false;
}

EventLoop*
ViewportImpl::get_loop ()
{
  return &*loop_;
}

bool
ViewportImpl::screen_viewable ()
{
  return visible() && has_display_window() && display_window_->viewable();
}

static bool startup_viewport = true;

void
ViewportImpl::async_show()
{
  if (has_display_window())
    {
      // try to ensure initial focus
      if (auto_focus_ && !get_focus())
        move_focus (FocusDir::NEXT);
      // size request & show up
      display_window_->show();
      // figure if this is the first window triggered by the user startig an app
      const bool user_action = startup_viewport;
      startup_viewport = false;
      display_window_->present (user_action);
    }
}

DisplayWindow*
ViewportImpl::display_window (Internal) const
{
  return display_window_;
}

void
ViewportImpl::create_display_window ()
{
  if (anchored())
    {
      if (!has_display_window())
        {
          negotiate_initial_size(); // find and allocate initial size
          DisplayDriver *sdriver = DisplayDriver::retrieve_display_driver ("auto");
          if (sdriver)
            {
              DisplayWindow::Setup setup;
              setup.window_type = WindowType::NORMAL;
              uint64 flags = DisplayWindow::ACCEPT_FOCUS | DisplayWindow::DELETABLE |
                             DisplayWindow::DECORATED | DisplayWindow::MINIMIZABLE | DisplayWindow::MAXIMIZABLE;
              setup.request_flags = DisplayWindow::Flags (flags);
              setup.session_role = "RAPICORN:" + program_name();
              if (!id().empty())
                setup.session_role += ":" + id();
              setup.bg_average = background();
              const Allocation asize = allocation();
              config_.request_width = asize.width;
              config_.request_height = asize.height;
              if (config_.alias.empty())
                config_.alias = program_alias();
              pending_win_size_ = true;
              display_window_ = sdriver->create_display_window (setup, config_);
              display_window_->set_event_wakeup ([this] () { loop_->wakeup(); /* thread safe */ });
            }
          else
            fatal ("failed to find and open any display driver");
        }
      RAPICORN_ASSERT (has_display_window() == true);
      loop_->flag_primary (true); // FIXME: depends on WM-managable
      EventLoop::VoidSlot sl = Aida::slot (*this, &ViewportImpl::async_show);
      loop_->exec_callback (sl);
    }
}

void
ViewportImpl::destroy_display_window ()
{
  const WidgetImplP guard_this = shared_ptr_cast<WidgetImpl*> (this);
  if (!has_display_window())
    return; // during destruction, ref_count == 0
  clear_immediate_event();
  display_window_->destroy();
  display_window_ = NULL;
  loop_->flag_primary (false);
  // reset widget state where needed
  cancel_widget_events (NULL);
}

void
ViewportImpl::show ()
{
  create_display_window();
}

bool
ViewportImpl::closed ()
{
  return !has_display_window();
}

void
ViewportImpl::close ()
{
  destroy_display_window();
}

void
ViewportImpl::destroy ()
{
  const WidgetImplP guard_this = shared_ptr_cast<WidgetImpl*> (this);
  if (anchored())
    {
      destroy_display_window();
      dispose();
    }
}

void
ViewportImpl::query_idle ()
{
  if (has_display_window())
    {
      const ViewportImplP thisp = shared_ptr_cast<ViewportImpl*> (this);
      const int PRIORITY_FLOOR = 1; // lowest possible priority, must be truely idle to execute this
      loop_->exec_callback ([thisp] () { thisp->sig_notify_idle.emit(); }, PRIORITY_FLOOR);
    }
  else
    sig_notify_idle.emit();
}

bool
ViewportImpl::snapshot (const String &pngname)
{
  cairo_surface_t *isurface = this->create_snapshot (allocation());
  cairo_status_t wstatus = cairo_surface_write_to_png (isurface, pngname.c_str());
  cairo_surface_destroy (isurface);
  String err = CAIRO_STATUS_SUCCESS == wstatus ? "ok" : cairo_status_to_string (wstatus);
  RAPICORN_DIAG ("ViewportImpl:snapshot:%s: failed to create \"%s\": %s", id(), pngname, err);
  return CAIRO_STATUS_SUCCESS == wstatus;
}

bool
ViewportImpl::synthesize_enter (double xalign, double yalign)
{
  if (!has_display_window())
    return false;
  const Allocation &area = allocation();
  Point p (area.x + xalign * (max (1, area.width) - 1),
           area.y + yalign * (max (1, area.height) - 1));
  p = point_to_viewport (p);
  EventContext ec;
  ec.x = p.x;
  ec.y = p.y;
  push_immediate_event (create_event_mouse (MOUSE_ENTER, ec));
  return true;
}

bool
ViewportImpl::synthesize_leave ()
{
  if (!has_display_window())
    return false;
  EventContext ec;
  push_immediate_event (create_event_mouse (MOUSE_LEAVE, ec));
  return true;
}

bool
ViewportImpl::synthesize_click (WidgetIface &widgeti, int button, double xalign, double yalign)
{
  WidgetImpl &widget = *dynamic_cast<WidgetImpl*> (&widgeti);
  if (!has_display_window() || !&widget)
    return false;
  const Allocation &area = widget.allocation();
  Point p (area.x + xalign * (max (1, area.width) - 1),
           area.y + yalign * (max (1, area.height) - 1));
  p = widget.point_to_viewport (p);
  EventContext ec;
  ec.x = p.x;
  ec.y = p.y;
  push_immediate_event (create_event_button (BUTTON_RELEASE, ec, button));
  push_immediate_event (create_event_button (BUTTON_PRESS, ec, button));
  return true;
}

bool
ViewportImpl::synthesize_delete ()
{
  if (!has_display_window())
    return false;
  EventContext ec;
  push_immediate_event (create_event_win_delete (ec));
  return true;
}

static const WidgetFactory<ViewportImpl> viewport_factory ("Rapicorn::Viewport");

} // Rapicorn
