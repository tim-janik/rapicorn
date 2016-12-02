// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "widget.hh"
#include "binding.hh"
#include "container.hh"
#include "adjustment.hh"
#include "window.hh"
#include "cmdlib.hh"
#include "sizegroup.hh"
#include "factory.hh"
#include "selector.hh"
#include "selob.hh"
#include "rcore/cairoutils.hh"
#include "uithread.hh"  // uithread_main_loop
#include <algorithm>

#define SZDEBUG(...)    RAPICORN_KEY_DEBUG ("Sizing", __VA_ARGS__)

namespace Rapicorn {

EventHandler::EventHandler() :
  sig_event (Aida::slot (*this, &EventHandler::handle_event))
{}

bool
EventHandler::capture_event (const Event &event)
{
  return false;
}

bool
EventHandler::handle_event (const Event &event)
{
  return false;
}

WidgetImpl&
WidgetIface::impl ()
{
  WidgetImpl *iimpl = dynamic_cast<WidgetImpl*> (this);
  if (!iimpl)
    throw std::bad_cast();
  return *iimpl;
}

const WidgetImpl&
WidgetIface::impl () const
{
  const WidgetImpl *iimpl = dynamic_cast<const WidgetImpl*> (this);
  if (!iimpl)
    throw std::bad_cast();
  return *iimpl;
}

WidgetImpl::WidgetImpl () :
  widget_flags_ (VISIBLE), widget_state_ (uint64 (WidgetState::NORMAL)), inherited_state_ (uint64 (WidgetState::STASHED)),
  parent_ (NULL), acache_ (NULL), factory_context_ (ctor_factory_context()), pack_info_ (NULL), cached_surface_ (NULL),
  sig_hierarchy_changed (Aida::slot (*this, &WidgetImpl::hierarchy_changed))
{}

void
WidgetImpl::construct ()
{
  ObjectImpl::construct();
  change_flags_silently (CONSTRUCTED, true);
}

void
WidgetImpl::fabricated ()
{
  // must chain and propagate to children
}

void
WidgetImpl::foreach_recursive (const std::function<void (WidgetImpl&)> &f)
{
  f (*this);
}

bool
WidgetImpl::ancestry_visible () const
{
  const WidgetImpl *widget = this;
  do
    {
      if (!widget->visible())
        return false;
      widget = widget->parent_;
    }
  while (widget);
  return true;
}

bool
WidgetImpl::ancestry_hover () const
{
  const WidgetImpl *widget = this;
  do
    {
      if (widget->hover())
        return true;
      widget = widget->parent();
    }
  while (widget);
  return false;
}

bool
WidgetImpl::ancestry_active () const
{
  const WidgetImpl *widget = this;
  do
    {
      if (widget->active())
        return true;
      widget = widget->parent();
    }
  while (widget);
  return false;
}

/// Return wether a widget is visible() and not offscreen (#STASHED), see ContainerImpl::change_unviewable()
bool
WidgetImpl::viewable() const
{
  return visible() && !stashed();
}

/// Return wether a widget is sensitive() and can process key events.
bool
WidgetImpl::key_sensitive () const
{
  return sensitive() && ancestry_visible();
}

/// Return wether a widget is sensitive() and can process pointer events.
bool
WidgetImpl::pointer_sensitive () const
{
  return sensitive() && drawable();
}

WidgetImpl::WidgetFlag
WidgetImpl::change_invalidation (WidgetChange type, uint64 bits)
{
  if (type == WidgetChange::VIEWABLE ||    // changing viewable() usually requires resizing and re-rendering
      type == WidgetChange::CHILD_ADDED || // same for parent/child changes
      type == WidgetChange::CHILD_REMOVED)
    return INVALID_REQUISITION | INVALID_ALLOCATION | INVALID_CONTENT;
  if (type == WidgetChange::STATE)
    {
      const WidgetState state_repaint_mask =
        WidgetState::ACCELERATABLE | WidgetState::HOVER | WidgetState::PANEL | WidgetState::DEFAULT |
        WidgetState::SELECTED | WidgetState::FOCUSED | WidgetState::INSENSITIVE |
        WidgetState::ACTIVE | WidgetState::TOGGLED | WidgetState::RETAINED;
      const WidgetState state_resize_mask =
        WidgetState::ACCELERATABLE | WidgetState::DEFAULT | WidgetState::FOCUSED |
        WidgetState::ACTIVE | WidgetState::TOGGLED | WidgetState::RETAINED;
      WidgetFlag invalidation_flags = WidgetFlag (0);
      if (bits & state_repaint_mask)
        invalidation_flags = invalidation_flags | INVALID_CONTENT;
      if (bits & state_resize_mask)
        invalidation_flags = invalidation_flags | INVALID_REQUISITION;
      return invalidation_flags;
    }
  if (type == WidgetChange::FLAGS)
    {
      const WidgetFlag flag_repaint_mask = ALLOW_FOCUS | FOCUS_CHAIN | HAS_FOCUS_INDICATOR | HAS_DEFAULT;
      if (bits & flag_repaint_mask)
        return INVALID_CONTENT;
      // repacking flags are handled elsewhere
    }
  return WidgetFlag (0);
}

/// Unconditionally propagate widget state changes like WidgetState bits, ANCHORED, etc
void
WidgetImpl::widget_propagate_state (const WidgetState prev_state)
{
  ContainerImpl *const container = as_container_impl();
  const bool was_viewable = viewable();
  const WidgetState stash_invisible = visible() ? WidgetState (0) : WidgetState::STASHED;
  if (parent())
    inherited_state_ = uint64 (parent()->state() | stash_invisible);
  else
    {
      const bool self_is_window_impl = UNLIKELY (parent_ == NULL) && container && as_window_impl();
      inherited_state_ = uint64 (self_is_window_impl ? stash_invisible : WidgetState::STASHED);
    }
  const uint64 bits_changed = uint64 (prev_state) ^ state();
  return_unless (bits_changed != 0);
  uint64 invalidation_flags = 0;
  if (was_viewable != viewable())
    invalidation_flags = change_invalidation (WidgetChange::VIEWABLE, 0);
  invalidation_flags |= change_invalidation (WidgetChange::STATE, bits_changed);
  if (invalidation_flags)
    widget_invalidate (WidgetFlag (invalidation_flags));
  if (!finalizing())
    {
      changed ("state");
      if (bits_changed & WidgetState::ACCELERATABLE)
        changed ("acceleratable");
      if (bits_changed & WidgetState::HOVER)
        changed ("hover");
      if (bits_changed & WidgetState::PANEL)
        changed ("panel");
      if (bits_changed & WidgetState::DEFAULT)
        changed ("receives_default");
      if (bits_changed & WidgetState::SELECTED)
        changed ("selected");
      if (bits_changed & WidgetState::FOCUSED)
        changed ("focused");
      if (bits_changed & WidgetState::INSENSITIVE)
        changed ("insensitive");
      if (bits_changed & WidgetState::ACTIVE)
        changed ("active");
      if (bits_changed & WidgetState::TOGGLED)
        changed ("toggled");
      if (bits_changed & WidgetState::RETAINED)
        changed ("retained");
      if (bits_changed & WidgetState::STASHED)
        changed ("stashed");
    }
  if (container)
    for (auto child : *container)
      child->widget_propagate_state (child->state());
}

/// Toggle @a mask to be @a on by changing widget_state_ and propagating to children.
void
WidgetImpl::widget_adjust_state (WidgetState mask, bool on)
{
  assert_return ((mask & (mask - 1)) == 0);     // single bit check
  const WidgetState old_state = state();        // widget_state_ | inherited_state_
  const uint64 old_widget_state = widget_state_;
  if (on)
    widget_state_ = widget_state_ | mask;
  else
    widget_state_ &= ~uint64 (mask);
  const uint64 bits_changed = (old_state ^ state()) | (old_widget_state ^ widget_state_);
  if (bits_changed)
    widget_propagate_state (old_state);
  if (parent() && (WidgetState::SELECTED & bits_changed))
    {
      WidgetChain this_chain;
      this_chain.widget = this;
      this_chain.next = NULL;
      parent()->selectable_child_changed (this_chain);
    }
  if (((state() ^ old_state) & WidgetState::TOGGLED) != 0)
    WidgetGroup::toggled_widget (*this);
}

/// Apply @a mask to widget_flags_ and return if flags changed.
bool
WidgetImpl::change_flags_silently (WidgetFlag mask, bool on)
{
  const uint64 old_flags = widget_flags_;
  if (old_flags & CONSTRUCTED)
    {
      // refuse to change constant flags after construction
      assert_return ((mask & CONSTRUCTED) == 0, false);
      assert_return ((mask & NEEDS_FOCUS_INDICATOR) == 0, false);
    }
  if (old_flags & FINALIZING) // cannot toggle FINALIZING
    assert_return ((mask & FINALIZING) == 0, 0);
  if (on)
    widget_flags_ |= mask;
  else
    widget_flags_ &= ~mask;
  return old_flags != widget_flags_;
}

void
WidgetImpl::set_flag (WidgetFlag flag, bool on)
{
  assert ((flag & (flag - 1)) == 0);            // single bit check
  const WidgetState old_state = state();        // widget_state_ | inherited_state_
  const bool flags_changed = change_flags_silently (flag, on);
  const uint64 packing_flags = HSHRINK | HEXPAND | HSPREAD | HSPREAD_CONTAINER |
                               VSHRINK | VEXPAND | VSPREAD | VSPREAD_CONTAINER | VISIBLE;
  if (flags_changed)
    {
      const uint64 invalidation_flags = change_invalidation (WidgetChange::FLAGS, flag);
      if (invalidation_flags)
        widget_invalidate (WidgetFlag (invalidation_flags));
      if (parent() && (flag & packing_flags))   // for flags affecting parent
        {
          parent()->invalidate_requisition();   // it must check requisition
          invalidate_requisition();
        }
      changed ("flags");
      widget_propagate_state (old_state);       // mirrors !VISIBLE as STASHED
    }
}

StyleIfaceP
WidgetImpl::style () const
{
  return Factory::factory_context_style (factory_context_);
}

bool
WidgetImpl::grab_default () const
{
  return false;
}

bool
WidgetImpl::allow_focus () const
{
  return test_any (ALLOW_FOCUS);
}

void
WidgetImpl::allow_focus (bool b)
{
  set_flag (ALLOW_FOCUS, b);
}

bool
WidgetImpl::can_focus () const
{
  return true;
}

bool
WidgetImpl::focusable () const
{
  return test_all (ALLOW_FOCUS | NEEDS_FOCUS_INDICATOR | HAS_FOCUS_INDICATOR) && can_focus();
}

bool
WidgetImpl::has_focus () const
{
  if (test_any (FOCUS_CHAIN))
    {
      WindowImpl *rwidget = get_window();
      if (rwidget && rwidget->get_focus() == this)
        return true;
    }
  return false;
}

void
WidgetImpl::unset_focus ()
{
  if (test_any (FOCUS_CHAIN))
    {
      WindowImpl *rwidget = get_window();
      if (rwidget && rwidget->get_focus() == this)
        WindowImpl::WidgetImplFriend::set_focus (*rwidget, NULL);
    }
}

bool
WidgetImpl::grab_focus ()
{
  if (has_focus())
    return true;
  if (!focusable() || !sensitive() || !ancestry_visible())
    return false;
  // unset old focus
  WindowImpl *rwidget = get_window();
  if (rwidget)
    WindowImpl::WidgetImplFriend::set_focus (*rwidget, NULL);
  // set new focus
  rwidget = get_window();
  if (rwidget && rwidget->get_focus() == NULL)
    WindowImpl::WidgetImplFriend::set_focus (*rwidget, this);
  return rwidget && rwidget->get_focus() == this;
}

bool
WidgetImpl::move_focus (FocusDir fdir)
{
  if (!has_focus() && focusable())
    return grab_focus();
  return false;
}

bool
WidgetImpl::acceleratable () const
{
  return test_state (WidgetState::ACCELERATABLE);
}

void
WidgetImpl::acceleratable (bool b)
{
  widget_adjust_state (WidgetState::ACCELERATABLE, b);
}

bool
WidgetImpl::hover () const
{
  return test_state (WidgetState::HOVER);
}

void
WidgetImpl::hover (bool b)
{
  widget_adjust_state (WidgetState::HOVER, b);
}

bool
WidgetImpl::panel () const
{
  return test_state (WidgetState::PANEL);
}

void
WidgetImpl::panel (bool b)
{
  widget_adjust_state (WidgetState::PANEL, b);
}

bool
WidgetImpl::receives_default () const
{
  return test_state (WidgetState::DEFAULT);
}

void
WidgetImpl::receives_default (bool b)
{
  widget_adjust_state (WidgetState::DEFAULT, b);
}

bool
WidgetImpl::widget_maybe_selected () const
{
  return false;
}

bool
WidgetImpl::selected () const
{
  return test_state (WidgetState::SELECTED);
}

void
WidgetImpl::selected (bool b)
{
  widget_adjust_state (WidgetState::SELECTED, b);
}

bool
WidgetImpl::focused () const
{
  return test_state (WidgetState::FOCUSED);
}

void
WidgetImpl::focused (bool b)
{
  assert_unreached();
}

bool
WidgetImpl::insensitive () const
{
  return test_state (WidgetState::INSENSITIVE);
}

void
WidgetImpl::insensitive (bool b)
{
  widget_adjust_state (WidgetState::INSENSITIVE, b);
}

/// Return if this widget and all its ancestors are not insensitive.
bool
WidgetImpl::sensitive () const
{
  return (state() & WidgetState::INSENSITIVE) == 0;
}

/// Adjust the @a insensitive property.
void
WidgetImpl::sensitive (bool b)
{
  insensitive (!b);
}

bool
WidgetImpl::active () const
{
  return test_state (WidgetState::ACTIVE);
}

void
WidgetImpl::active (bool b)
{
  widget_adjust_state (WidgetState::ACTIVE, b);
}

bool
WidgetImpl::widget_maybe_toggled () const
{
  return false;
}

bool
WidgetImpl::toggled () const
{
  return test_state (WidgetState::TOGGLED);
}

void
WidgetImpl::toggled (bool b)
{
  if (widget_maybe_toggled())
    widget_adjust_state (WidgetState::TOGGLED, b);
}

bool
WidgetImpl::retained () const
{
  return test_state (WidgetState::RETAINED);
}

void
WidgetImpl::retained (bool b)
{
  if (widget_maybe_toggled())
    widget_adjust_state (WidgetState::RETAINED, b);
}

bool
WidgetImpl::stashed () const
{
  return test_state (WidgetState::STASHED);
}

void
WidgetImpl::stashed (bool b)
{
  widget_adjust_state (WidgetState::STASHED, b);
}

bool
WidgetImpl::activate_widget ()
{
  return false;
}

bool
WidgetImpl::activate ()
{
  if (!sensitive())
    return false;
  ContainerImpl *pcontainer = parent();
  while (pcontainer)
    {
      pcontainer->scroll_to_child (*this);
      pcontainer = pcontainer->parent();
    }
  return activate_widget();
}

void
WidgetImpl::notify_key_error ()
{
  WindowImpl *rwidget = get_window();
  if (rwidget)
    {
      DisplayWindow *display_window = WindowImpl::WidgetImplFriend::display_window (*rwidget);
      if (display_window)
        display_window->beep();
    }
}

/** Request content data.
 * Request content data from clipboard or selection, etc. This request will result in the delivery of a
 * CONTENT_DATA event with a @a data_type possibly different from the requested @a data_type.
 * The event's @a data_type will be empty if no content data is available.
 * The @a nonce argument is an ID that should be unique for each new request, it is provided by the
 * CONTENT_DATA event to associate the originating request.
 */
bool
WidgetImpl::request_content (ContentSourceType csource, uint64 nonce, const String &data_type)
{
  WindowImpl *rwidget = get_window();
  if (rwidget)
    {
      DisplayWindow *display_window = WindowImpl::WidgetImplFriend::display_window (*rwidget);
      if (display_window)
        {
          display_window->request_content (csource, nonce, data_type);
          return true;
        }
    }
  return false;
}

/** Claim ownership for content requests.
 * Make this widget the owner for future @a content_source requests (delivered as CONTENT_REQUEST events).
 * The @a data_types contain a list of acceptable types to retrieve contents.
 * A CONTENT_CLEAR event may be sent to indicate ownership loss, e.g. because another widget or process took on ownership.
 */
bool
WidgetImpl::own_content (ContentSourceType content_source, uint64 nonce, const StringVector &data_types)
{
  assert_return (data_types.size() >= 1, false);
  WindowImpl *rwidget = get_window();
  if (rwidget)
    {
      DisplayWindow *display_window = WindowImpl::WidgetImplFriend::display_window (*rwidget);
      if (display_window)
        {
          display_window->set_content_owner (content_source, nonce, data_types);
          return true;
        }
    }
  return false;
}

/** Reject ownership for content requests.
 * Counterpart to WidgetImpl::own_content(), gives up ownership if it was previously acquired.
 */
bool
WidgetImpl::disown_content (ContentSourceType content_source, uint64 nonce)
{
  WindowImpl *rwidget = get_window();
  if (rwidget)
    {
      DisplayWindow *display_window = WindowImpl::WidgetImplFriend::display_window (*rwidget);
      if (display_window)
        {
          display_window->set_content_owner (content_source, nonce, StringVector());
          return true;
        }
    }
  return false;
}

/** Provide reply data for CONTENT_REQUEST events.
 * All CONTENT_REQUEST events need to be honored with provide_content() replies,
 * the @a nonce argument is provided by the event and must be passed along.
 * To reject a content request, pass an empty string as @a data_type.
 */
bool
WidgetImpl::provide_content (const String &data_type, const String &data, uint64 request_id)
{
  WindowImpl *rwidget = get_window();
  if (rwidget)
    {
      DisplayWindow *display_window = WindowImpl::WidgetImplFriend::display_window (*rwidget);
      if (display_window)
        {
          display_window->provide_content (data_type, data, request_id);
          return true;
        }
    }
  return false;
}

static ContainerImpl*
container_cast (WidgetImpl *widget)
{
  return widget ? widget->as_container_impl() : NULL;
}

size_t
WidgetImpl::cross_link (WidgetImpl &link, const WidgetSlot &uncross)
{
  assert_return (this != &link, 0);
  ContainerImpl *common_container = container_cast (common_ancestor (link));
  assert_return (common_container != NULL, 0);
  return common_container->widget_cross_link (*this, link, uncross);
}

void
WidgetImpl::cross_unlink (WidgetImpl &link, size_t link_id)
{
  assert_return (this != &link);
  ContainerImpl *common_container = container_cast (common_ancestor (link));
  assert_return (common_container != NULL);
  common_container->widget_cross_unlink (*this, link, link_id);
}

void
WidgetImpl::uncross_links (WidgetImpl &link)
{
  assert (this != &link);
  ContainerImpl *common_container = container_cast (common_ancestor (link));
  assert (common_container != NULL);
  common_container->widget_uncross_links (*this, link);
}

bool
WidgetImpl::match_interface (bool wself, bool wparent, bool children, InterfaceMatcher &imatcher) const
{
  WidgetImpl *self = const_cast<WidgetImpl*> (this);
  if (wself && imatcher.match (self, id()))
    return true;
  if (wparent)
    {
      WidgetImpl *pwidget = parent();
      while (pwidget)
        {
          if (imatcher.match (pwidget, pwidget->id()))
            return true;
          pwidget = pwidget->parent();
        }
    }
  if (children)
    {
      ContainerImpl *container = self->as_container_impl();
      if (container)
        for (auto child : *container)
          if (child->match_interface (1, 0, 1, imatcher))
            return true;
    }
  return false;
}

bool
WidgetImpl::match_selector (const String &selector)
{
  Selector::SelobAllocator sallocator;
  return Selector::Matcher::query_selector_bool (selector, *sallocator.widget_selob (*this));
}

WidgetIfaceP
WidgetImpl::query_selector (const String &selector)
{
  Selector::SelobAllocator sallocator;
  Selector::Selob *selob = Selector::Matcher::query_selector_first (selector, *sallocator.widget_selob (*this));
  return shared_ptr_cast<WidgetIface*> (selob ? sallocator.selob_widget (*selob) : NULL);
}

WidgetSeq
WidgetImpl::query_selector_all (const String &selector)
{
  Selector::SelobAllocator sallocator;
  vector<Selector::Selob*> result = Selector::Matcher::query_selector_all (selector, *sallocator.widget_selob (*this));
  WidgetSeq widgets;
  for (vector<Selector::Selob*>::const_iterator it = result.begin(); it != result.end(); it++)
    {
      WidgetImpl *widget = sallocator.selob_widget (**it);
      if (widget)
        widgets.push_back (shared_ptr_cast<WidgetIface*> (widget));
    }
  return widgets;
}

WidgetIfaceP
WidgetImpl::query_selector_unique (const String &selector)
{
  Selector::SelobAllocator sallocator;
  Selector::Selob *selob = Selector::Matcher::query_selector_unique (selector, *sallocator.widget_selob (*this));
  return shared_ptr_cast<WidgetIface*> (selob ? sallocator.selob_widget (*selob) : NULL);
}

uint
WidgetImpl::exec_slow_repeater (const EventLoop::BoolSlot &sl)
{
  WindowImpl *rwidget = get_window();
  if (rwidget)
    {
      EventLoop *loop = rwidget->get_loop();
      if (loop)
        return loop->exec_timer (sl, 250, 50, EventLoop::PRIORITY_NORMAL);
    }
  return 0;
}

uint
WidgetImpl::exec_fast_repeater (const EventLoop::BoolSlot &sl)
{
  WindowImpl *rwidget = get_window();
  if (rwidget)
    {
      EventLoop *loop = rwidget->get_loop();
      if (loop)
        return loop->exec_timer (sl, 200, 20, EventLoop::PRIORITY_NORMAL);
    }
  return 0;
}

uint
WidgetImpl::exec_key_repeater (const EventLoop::BoolSlot &sl)
{
  WindowImpl *rwidget = get_window();
  if (rwidget)
    {
      EventLoop *loop = rwidget->get_loop();
      if (loop)
        return loop->exec_timer (sl, 250, 33, EventLoop::PRIORITY_NORMAL);
    }
  return 0;
}

uint
WidgetImpl::exec_now (const EventLoop::VoidSlot &sl)
{
  /* queue arbitrary code for asynchornous execution, i.e. this function pretty much guarantees
   * slot execution if there's *any* main loop running, so fallback to the UIThread main loop if needed.
   */
  WindowImpl *rwidget = get_window();
  EventLoop *loop = rwidget ? rwidget->get_loop() : &*uithread_main_loop();
  return loop ? loop->exec_now (sl) : 0;
}

bool
WidgetImpl::remove_exec (uint exec_id)
{
  WindowImpl *rwidget = get_window();
  if (rwidget)
    {
      EventLoop *loop = rwidget->get_loop();
      if (loop)
        return loop->try_remove (exec_id);
    }
  return false;
}

bool
WidgetImpl::clear_exec (uint *exec_id)
{
  assert_return (exec_id != NULL, false);
  bool removed = false;
  if (*exec_id)
    removed = remove_exec (*exec_id);
  *exec_id = 0;
  return removed;
}

static DataKey<uint> visual_update_key;

void
WidgetImpl::queue_visual_update ()
{
  uint timer_id = get_data (&visual_update_key);
  if (!timer_id)
    {
      WindowImpl *rwidget = get_window();
      if (rwidget)
        {
          EventLoop *loop = rwidget->get_loop();
          if (loop)
            {
              timer_id = loop->exec_timer (Aida::slot (*this, &WidgetImpl::force_visual_update), 20);
              set_data (&visual_update_key, timer_id);
            }
        }
    }
}

void
WidgetImpl::force_visual_update ()
{
  uint timer_id = get_data (&visual_update_key);
  if (timer_id)
    {
      remove_exec (timer_id);
      set_data (&visual_update_key, uint (0));
    }
  visual_update();
}

void
WidgetImpl::visual_update ()
{}

WidgetImpl::~WidgetImpl()
{
  critical_unless (isconstructed());
  change_flags_silently (FINALIZING, true);
  WidgetGroup::delete_widget (*this);
  if (parent())
    parent()->remove (this);
  uint timer_id = get_data (&visual_update_key);
  if (timer_id)
    {
      remove_exec (timer_id);
      set_data (&visual_update_key, uint (0));
    }
  if (pack_info_)
    {
      PackInfo *delme = pack_info_;
      pack_info_ = NULL;
      delete delme;
    }
}

Command*
WidgetImpl::lookup_command (const String &command_name)
{
  typedef std::map<const String, Command*> CommandMap;
  static std::map<const CommandList*,CommandMap*> clist_map;
  /* find/construct command map */
  const CommandList &clist = list_commands();
  CommandMap *cmap = clist_map[&clist];
  if (!cmap)
    {
      cmap = new CommandMap;
      for (auto cmdp : clist)
        (*cmap)[cmdp->ident] = cmdp.get();
      clist_map[&clist] = cmap;
    }
  CommandMap::iterator it = cmap->find (command_name);
  if (it != cmap->end())
    return it->second;
  else
    return NULL;
}

bool
WidgetImpl::exec_command (const String &command_call_string)
{
  String cmd_name;
  StringSeq args;
  if (!command_scan (command_call_string, &cmd_name, &args))
    {
      critical ("Invalid command syntax: %s", command_call_string.c_str());
      return false;
    }
  cmd_name = string_strip (cmd_name);
  if (cmd_name == "")
    return true;

  WidgetImpl *widget = this;
  while (widget)
    {
      Command *cmd = widget->lookup_command (cmd_name);
      if (cmd)
        return cmd->exec (widget, args);
      widget = widget->parent();
    }

  if (command_lib_exec (*this, cmd_name, args))
    return true;

  widget = this;
  while (widget)
    {
      if (widget->custom_command (cmd_name, args))
        return true;
      widget = widget->parent();
    }
  critical ("toplevel failed to handle command"); // ultimately Window *always* handles commands
  return false;
}

bool
WidgetImpl::custom_command (const String    &command_name,
                          const StringSeq &command_args)
{
  return false;
}

const CommandList&
WidgetImpl::list_commands ()
{
  static CommandP commands[] = {
  };
  static const CommandList command_list (commands);
  return command_list;
}

struct NamedAny { String name; Any any; };
typedef vector<NamedAny> NamedAnyVector;

class UserDataKey : public DataKey<NamedAnyVector*> {
  virtual void destroy (NamedAnyVector *nav) override
  {
    delete nav;
  }
};
static UserDataKey user_data_key;

void
WidgetImpl::set_user_data (const String &name, const Any &any)
{
  return_unless (!name.empty());
  NamedAnyVector *nav = get_data (&user_data_key);
  if (!nav)
    {
      nav = new NamedAnyVector;
      set_data (&user_data_key, nav);
    }
  for (size_t i = 0; i < nav->size(); i++)
    if ((*nav)[i].name == name)
      {
        if (any.kind() == Aida::UNTYPED)
          nav->erase (nav->begin() + i);
        else
          (*nav)[i].any = any;
        return;
      }
  NamedAny nany;
  nany.name = name;
  nany.any = any;
  nav->push_back (nany);
}

Any
WidgetImpl::get_user_data (const String &name)
{
  NamedAnyVector *nav = get_data (&user_data_key);
  if (nav)
    for (auto it : *nav)
      if (it.name == name)
        return it.any;
  return Any();
}

Property*
WidgetImpl::lookup_property (const String &property_name)
{
  return __aida_lookup__ (property_name);
}

String
WidgetImpl::get_property (const String &property_name)
{
  return __aida_getter__ (property_name);
}

void
WidgetImpl::set_property (const String &property_name, const String &value)
{
  if (!__aida_setter__ (property_name, value))
    throw Exception ("no such property: " + id() + "::" + property_name);
}

bool
WidgetImpl::try_set_property (const String &property_name, const String &value)
{
  return __aida_setter__ (property_name, value);
}

static DataKey<ObjectIfaceP> data_context_key;

void
WidgetImpl::data_context (ObjectIface &dcontext)
{
  ObjectIfaceP oip = get_data (&data_context_key);
  if (oip.get() != &dcontext)
    {
      oip = shared_ptr_cast<ObjectIface*> (&dcontext);
      if (oip)
        set_data (&data_context_key, oip);
      else
        delete_data (&data_context_key);
      if (anchored())
        foreach_recursive ([] (WidgetImpl &widget) { widget.data_context_changed(); });
    }
}

ObjectIfaceP
WidgetImpl::data_context () const
{
  ObjectIfaceP oip = get_data (&data_context_key);
  if (!oip)
    {
      WidgetImpl *p = parent();
      if (p)
        return p->data_context();
    }
  return oip;
}

typedef vector<BindingP> BindingVector;
class BindingVectorKey : public DataKey<BindingVector*> {
  virtual void destroy (BindingVector *data) override
  {
    for (auto b : *data)
      b->reset();
    delete data;
  }
};
static BindingVectorKey binding_key;

void
WidgetImpl::data_context_changed ()
{
  BindingVector *bv = get_data (&binding_key);
  if (bv)
    {
      ObjectIfaceP dc = data_context();
      for (BindingP b : *bv)
        if (dc)
          b->bind_context (dc);
        else
          b->reset();
    }
}

void
WidgetImpl::add_binding (const String &property, const String &binding_path)
{
  assert_return (false == anchored());
  BindingP bp = Binding::make_shared (*this, property, binding_path);
  BindingVector *bv = get_data (&binding_key);
  if (!bv)
    {
      bv = new BindingVector;
      set_data (&binding_key, bv);
    }
  bv->push_back (bp);
}

void
WidgetImpl::remove_binding (const String &property)
{
  BindingVector *bv = get_data (&binding_key);
  if (bv)
    for (auto bi = bv->begin(); bi != bv->end(); ++bi)
      if ((*bi)->instance_property() == property)
        {
          (*bi)->reset();
          bv->erase (bi);
          return;
      }
}

static class OvrKey : public DataKey<Requisition> {
  Requisition
  fallback()
  {
    return Requisition (-1, -1);
  }
} override_requisition;

bool
WidgetImpl::process_event (const Event &event, bool capture) // viewport coordinates
{
  bool handled = false;
  EventHandler *controller = dynamic_cast<EventHandler*> (this);
  if (controller)
    {
      if (capture)
        handled = controller->capture_event (event);
      else
        handled = controller->sig_event.emit (event);
    }
  return handled;
}

/// Check if Point @a widget_point lies within widget.
bool
WidgetImpl::point (Point widget_point) const
{
  const Allocation a = allocation();
  return (visible() &&
          widget_point.x >= a.x && widget_point.x < a.x + a.width &&
          widget_point.y >= a.y && widget_point.y < a.y + a.height);
}

/// Convert Point from ViewportImpl to WidgetImpl coordinates.
Point
WidgetImpl::point_from_viewport (Point viewport_point) const
{
  // translate ViewportImpl to parent
  const Point ppoint = parent_ ? parent_->point_from_viewport (viewport_point) : viewport_point;
  // translate parent to widget
  return Point (ppoint.x - child_allocation_.x, ppoint.y - child_allocation_.y);
}

/// Convert Point from WidgetImpl to ViewportImpl coordinates.
Point
WidgetImpl::point_to_viewport (Point widget_point) const
{
  // translate widget to parent
  const Point ppoint (widget_point.x + child_allocation_.x, widget_point.y + child_allocation_.y);
  // translate parent to ViewportImpl
  return parent_ ? parent_->point_to_viewport (ppoint) : ppoint;
}

/// Convert IRect from ViewportImpl to WidgetImpl coordinates.
IRect
WidgetImpl::rect_from_viewport (IRect viewport_rect) const
{
  const Point widget_point = point_from_viewport (Point (viewport_rect.x, viewport_rect.y));
  return IRect (iround (widget_point.x), iround (widget_point.y), viewport_rect.width, viewport_rect.height);
}

/// Convert IRect from WidgetImpl to ViewportImpl coordinates.
IRect
WidgetImpl::rect_to_viewport (IRect widget_rect) const
{
  const Point viewport_point = point_to_viewport (Point (widget_rect.x, widget_rect.y));
  return IRect (iround (viewport_point.x), iround (viewport_point.y), widget_rect.width, widget_rect.height);
}

void
WidgetImpl::set_parent (ContainerImpl *pcontainer)
{
  assert_return (this != as_window_impl());
  const WidgetState old_state = state();        // widget_state_ | inherited_state_
  EventHandler *controller = dynamic_cast<EventHandler*> (this);
  if (controller)
    controller->reset();
  ContainerImpl* old_parent = parent();
  const WidgetImplP guard_child = shared_ptr_cast<WidgetImpl*> (this);
  const ContainerImplP guard_parent = shared_ptr_cast<ContainerImpl*> (old_parent);
  static void (*const acache_reset) (WidgetImpl*) =
    [] (WidgetImpl *widget)
    {
      widget->acache_ = NULL;
      ContainerImpl *container = widget->as_container_impl();
      if (container)
        for (auto child : *container)
          if (child->acache_)
            acache_reset (&*child);
    };
  if (old_parent)
    {
      assert_return (pcontainer == NULL);
      WindowImpl *old_toplevel = get_window();
      invalidate_all();
      old_parent->unparent_child (*this);
      parent_ = NULL;
      if (acache_)
        acache_reset (this);
      inherited_state_ = uint64 (WidgetState::STASHED);
      widget_propagate_state (old_state); // propagate PARENT_VISIBLE, PARENT_INSENSITIVE
      if (anchored())
        {
          assert_return (old_toplevel != NULL);
          sig_hierarchy_changed.emit (old_toplevel);
        }
    }
  if (pcontainer)
    {
      assert_return (old_parent == NULL);
      parent_ = pcontainer;
      inherited_state_ = 0;
      widget_propagate_state (old_state);
      if (parent_->anchored() && !anchored())
        sig_hierarchy_changed.emit (NULL);
      invalidate_all();
    }
}

ContainerImplP
WidgetImpl::parentp () const
{
  return shared_ptr_cast<ContainerImpl> (parent()); // throws bad_weak_ptr during parent's dtor
}

bool
WidgetImpl::has_ancestor (const WidgetImpl &ancestor) const
{
  const WidgetImpl *widget = this;
  while (widget)
    {
      if (widget == &ancestor)
        return true;
      widget = widget->parent();
    }
  return false;
}

WidgetImpl*
WidgetImpl::common_ancestor (const WidgetImpl &other) const
{
  WidgetImpl *widget1 = const_cast<WidgetImpl*> (this);
  do
    {
      WidgetImpl *widget2 = const_cast<WidgetImpl*> (&other);
      do
        {
          if (widget1 == widget2)
            return widget1;
          widget2 = widget2->parent();
        }
      while (widget2);
      widget1 = widget1->parent();
    }
  while (widget1);
  return NULL;
}

ContainerImpl*
WidgetImpl::root () const
{
  ContainerImpl *root = parent_;
  if (!root)
    root = const_cast<WidgetImpl*> (this)->as_container_impl();
  else
    while (root->parent_)
      root = root->parent_;
  return root;
}

WindowImpl*
WidgetImpl::get_window () const
{
  const AncestryCache *acache = ancestry_cache();
  if (acache)
    return acache->window;
  WindowImpl *window = NULL;
  for (WidgetImpl *widget = const_cast<WidgetImpl*> (this); widget && !window; widget = widget->parent())
    window = widget->as_window_impl();
  return window;
}

ViewportImpl*
WidgetImpl::get_viewport () const
{
  const AncestryCache *acache = ancestry_cache();
  if (acache)
    return acache->viewport;
  ViewportImpl *viewport = NULL;
  for (WidgetImpl *widget = const_cast<WidgetImpl*> (this); widget && !viewport; widget = widget->parent())
    viewport = dynamic_cast<ViewportImpl*> (widget);
  return viewport;
}

ResizeContainerImpl*
WidgetImpl::get_resize_container () const
{
  const AncestryCache *acache = ancestry_cache();
  if (acache)
    return acache->resize_container;
  ResizeContainerImpl *resize_container = NULL;
  for (WidgetImpl *widget = const_cast<WidgetImpl*> (this); widget && !resize_container; widget = widget->parent())
    resize_container = dynamic_cast<ResizeContainerImpl*> (widget);
  return resize_container;
}

void
WidgetImpl::acache_check () const
{
  WidgetImpl *self = const_cast<WidgetImpl*> (this);
  if (!acache_ && anchored())
    self->acache_ = self->fetch_ancestry_cache();
  else if (acache_ && !anchored())
    self->acache_ = NULL;
}

const WidgetImpl::AncestryCache*
WidgetImpl::fetch_ancestry_cache ()
{
  return parent_ ? parent_->ancestry_cache() : NULL;
}

/// WidgetGroup sprouts are turned into widget group objects for ANCHORED widgets
struct WidgetGroupSprout {
  String          group_name;
  WidgetGroupType group_type;
  WidgetGroupSprout() : group_type (WidgetGroupType (0)) {}
  WidgetGroupSprout (const String &name, WidgetGroupType gt) : group_name (name), group_type (gt) {}
  bool operator== (const WidgetGroupSprout &other) const { return other.group_name == group_name && other.group_type == group_type; }
};
class WidgetGroupSproutsKey : public DataKey<vector<WidgetGroupSprout>*> {
  virtual void destroy (vector<WidgetGroupSprout> *widget_group_sprouts) override
  {
    delete widget_group_sprouts;
  }
};
static WidgetGroupSproutsKey widget_group_sprouts_key;

void
WidgetImpl::enter_widget_group (const String &group_name, WidgetGroupType group_type)
{
  assert_return (false == anchored());
  assert_return (false == group_name.empty());
  auto *sprouts = get_data (&widget_group_sprouts_key);
  if (!sprouts)
    {
      sprouts = new vector<WidgetGroupSprout>;
      set_data (&widget_group_sprouts_key, sprouts);
    }
  const WidgetGroupSprout sprout (group_name, group_type);
  auto it = std::find (sprouts->begin(), sprouts->end(), sprout);
  const bool widget_group_exists = it != sprouts->end();
  assert_return (false == widget_group_exists);
  sprouts->push_back (sprout);
}

void
WidgetImpl::leave_widget_group (const String &group_name, WidgetGroupType group_type)
{
  assert_return (false == anchored());
  auto *sprouts = get_data (&widget_group_sprouts_key);
  const WidgetGroupSprout sprout (group_name, group_type);
  auto it = find (sprouts->begin(), sprouts->end(), sprout);
  const bool widget_group_exists = it != sprouts->end();
  assert_return (true == widget_group_exists);
  sprouts->erase (it);
}

void
WidgetImpl::sync_widget_groups (const String &group_list, WidgetGroupType group_type)
{
  StringVector add = string_split_any (group_list, ",");
  string_vector_strip (add);
  string_vector_erase_empty (add);
  std::stable_sort (add.begin(), add.end());
  StringVector del = list_widget_groups (group_type);
  std::stable_sort (del.begin(), del.end());
  for (size_t a = 0, d = 0; a < add.size() || d < del.size();)
    if (d >= del.size() || (a < add.size() && add[a] < del[d]))
      enter_widget_group (add[a++], group_type);
    else if (a >= add.size() || add[a] > del[d]) // (d < del.size())
      leave_widget_group (del[d++], group_type);
    else // (a < add.size() && d < del.size() && add[a] == del[d])
      a++, d++;
}

StringVector
WidgetImpl::list_widget_groups (WidgetGroupType group_type) const
{
  auto *sprouts = get_data (&widget_group_sprouts_key);
  StringVector strings;
  if (sprouts)
    for (auto it : *sprouts)
      if (it.group_type == group_type)
        strings.push_back (it.group_name);
  return strings;
}

WidgetGroup*
WidgetImpl::find_widget_group (const String &group_name, WidgetGroupType group, bool force_create)
{
  assert_return (force_create == false || anchored(), NULL);
  return_if (group_name.empty(), NULL);
  ContainerImpl *container = as_container_impl();
  for (auto c = container ? container : parent(); c; c = c->parent())
    {
      WidgetGroup *widget_group = c->retrieve_widget_group (group_name, group, false);
      if (widget_group)
        return widget_group;
    }
  if (!force_create)
    return NULL;
  // fallback to create on root
  user_warning (this->user_source(), "creating undefined widget group \"%s\"", group_name);
  ContainerImpl *r = root();
  assert (r); // we asserted anchored() earlier
  return r->retrieve_widget_group (group_name, group, true);
}

void
WidgetImpl::changed (const String &name)
{
  if (!finalizing())
    ObjectImpl::changed (name);
}

void
WidgetImpl::widget_invalidate (WidgetFlag mask)
{
  return_unless (0 == (mask & ~(INVALID_REQUISITION | INVALID_ALLOCATION | INVALID_CONTENT)));
  return_unless (mask != 0);
  const bool had_invalid_requisition = test_any (INVALID_REQUISITION);
  change_flags_silently (mask, true);
  if (test_any (INVALID_CONTENT))
    expose();
  if (!had_invalid_requisition && test_any (INVALID_REQUISITION))
    WidgetGroup::invalidate_widget (*this);
  WindowImpl *window = get_window();
  if (window)
    window->queue_resize_redraw();
}

/// Determine "internal" size requisition of a widget, including overrides, excluding groupings.
Requisition
WidgetImpl::inner_size_request()
{
  /* loop until we gather a valid requisition, this explicitely allows for
   * requisition invalidation during the size_request phase, widget implementations
   * have to ensure we're not looping endlessly
   */
  while (test_any (INVALID_REQUISITION))
    {
      change_flags_silently (INVALID_REQUISITION, false);
      Requisition inner; // 0,0
      if (visible())
        {
          size_request (inner);
          inner.width = MAX (inner.width, 0);
          inner.height = MAX (inner.height, 0);
          Requisition ovr (width(), height());
          if (ovr.width >= 0)
            inner.width = ovr.width;
          if (ovr.height >= 0)
            inner.height = ovr.height;
          SZDEBUG ("size requesting: 0x%016x:%s: %s => %.17gx%.17g", size_t (this),
                   Factory::factory_context_type (factory_context()), id(),
                   inner.width, inner.height);
        }
      if (requisition_ != inner)
        {
          requisition_ = inner;
          widget_invalidate (INVALID_ALLOCATION);
          WidgetImpl *p = parent();
          if (p)
            p->invalidate_requisition(); // parent needs to recheck size once a child changed size
        }
    }
  return visible() ? requisition_ : Requisition();
}

/** Get the size requisition of a widget.
 *
 * Determines the size requisition of a widget if its not already calculated.
 * Changing widget properties like fonts or children of a container may cause
 * size requisition to be recalculated.
 * The widget size requisition is determined by the actual widget type or
 * possibly the container type which takes children into account, see size_request().
 * Overridden width() and height() are taken into account if specified.
 * The resulting size can also be affected by size group settings if this widget
 * has been included into any.
 * The #INVALID_REQUISITION flag is unset after this method has been called.
 * @returns The size requested by this widget for layouting.
 */
Requisition
WidgetImpl::requisition ()
{
  return SizeGroup::widget_requisition (*this);
}

/// Variant of expose() that allows exposing areas outside of the current allocation().
void
WidgetImpl::expose_unclipped (const Region &region)
{
  return_unless (region.empty() == false);
  return_unless (anchored());
  // clip expose region extents against ancestry (and translate)
  IRect vextents = region.extents();
  Point delta = Point (vextents.x, vextents.y);
  WidgetImpl *last = this;
  for (WidgetImpl *p = last->parent(); p; last = p, p = last->parent())
    {
      const Allocation child_allocation = last->child_allocation();
      // translate into parent
      delta.x += child_allocation.x;
      delta.y += child_allocation.y;
      vextents.x += child_allocation.x;
      vextents.y += child_allocation.y;
      // clip extents to parent
      vextents.intersect (p->allocation());
    }
  return_unless (vextents.empty() == false);
  // translate region into ancestor coordinates
  ViewportImpl &viewport = *last->get_viewport(); // never NULL when anchored()
  Region viewport_region = region;
  viewport_region.translate (delta.x, delta.y);
  // clip region against ancestry
  viewport_region.intersect (Region (vextents));
  viewport.expose_region (viewport_region);
}

/** Invalidate drawing contents of a widget
 *
 * Cause the given @a region of @a this widget to be recomposed.
 * If the widget needs to be redrawn, use invalidate_content() instead.
 * The expose region is constrained to the allocation().
 */
void
WidgetImpl::expose (const Region &region) // widget relative
{
  Region r (allocation());
  r.intersect (region);
  expose_unclipped (r);
}

/// Signal emitted when a widget ancestry is added to or removed from a Window
void
WidgetImpl::hierarchy_changed (WidgetImpl *old_toplevel)
{
  if (anchored())
    {
      // leave anchored
      assert_return (old_toplevel != NULL);
      const WidgetGroup::GroupVector widget_groups = WidgetGroup::list_groups (*this);
      for (auto wgroup : widget_groups)
        wgroup->remove_widget (*this);
      data_context_changed();
    }
  // assign anchored
  const bool was_anchored = anchored();
  set_flag (ANCHORED, WindowImpl::WidgetImplFriend::widget_is_anchored (*this)); // anchored = true/false
  const bool hierarchy_anchor_changed = was_anchored != anchored();
  assert_return (hierarchy_anchor_changed == true);
  if (anchored())
    {
      // enter anchored
      assert_return (old_toplevel == NULL);
      assert_return (get_window() != NULL);
      auto *sprouts = get_data (&widget_group_sprouts_key);
      if (sprouts)
        for (const auto sprout : *sprouts)
          {
            WidgetGroup *wgroup = find_widget_group (sprout.group_name, sprout.group_type, true);
            if (wgroup)
              wgroup->add_widget (*this);
          }
      data_context_changed();
    }
  else
    acache_ = NULL; // reinforce acache_reset in case of intermittent assignment
}

String
WidgetImpl::debug_name (const String &format) const
{
  String s = format.empty() ? "%n|%f:%l" : format;
  const UserSource us = Factory::factory_context_source (factory_context());
  s = string_replace (s, "%f", Path::basename (us.filename));
  s = string_replace (s, "%p", us.filename);
  s = string_replace (s, "%l", string_format ("%u", us.line));
  s = string_replace (s, "%n", id());
  s = string_replace (s, "%r", string_format ("%gx%g", requisition_.width, requisition_.height));
  s = string_replace (s, "%a", child_allocation_.string());
  return s;
}

static WidgetImpl *global_debug_dump_marker = NULL;

String
WidgetImpl::debug_dump (const String &flags)
{
  WidgetImpl *saved_debug_dump_marker = global_debug_dump_marker;
  global_debug_dump_marker = this;
  WidgetImpl *asroot = root();
  if (!asroot)
    asroot = this;
  String dump = asroot->test_dump();
  global_debug_dump_marker = saved_debug_dump_marker;
  return dump;
}

String
WidgetImpl::test_dump ()
{
  TestStream *tstream = TestStream::create_test_stream();
  make_test_dump (*tstream);
  String s = tstream->string();
  delete tstream;
  return s;
}

void
WidgetImpl::make_test_dump (TestStream &tstream)
{
  tstream.push_node (Factory::factory_context_impl_type (factory_context()));
  if (this == global_debug_dump_marker)
    tstream.dump ("debug_dump", String ("1"));
  const PropertyList &plist = __aida_properties__();
  size_t n_properties = 0;
  Aida::Property **properties = plist.list_properties (&n_properties);
  for (uint i = 0; i < n_properties; i++)
    {
      Property *property = dynamic_cast<Property*> (properties[i]);
      if (property && property->readable())
        {
          String value = get_property (property->ident);
          tstream.dump (property->ident, value);
        }
    }
  dump_private_data (tstream);
  tstream.push_indent();
  dump_test_data (tstream);
  tstream.pop_indent();
  tstream.pop_node ();
}

void
WidgetImpl::dump_test_data (TestStream &tstream)
{}

void
WidgetImpl::dump_private_data (TestStream &tstream)
{
  tstream.dump ("requisition", string_format ("(%.17g, %.17g)", requisition().width, requisition().height));
  tstream.dump ("allocation", allocation().string());
  tstream.dump ("factory_context", string_format ("{ id=%s, type=%s, impl=%s }",
                                                  Factory::factory_context_id (factory_context()),
                                                  Factory::factory_context_type (factory_context()),
                                                  Factory::factory_context_impl_type (factory_context())));
}

void
WidgetImpl::find_adjustments (AdjustmentSourceType adjsrc1,
                              Adjustment         **adj1,
                              AdjustmentSourceType adjsrc2,
                              Adjustment         **adj2,
                              AdjustmentSourceType adjsrc3,
                              Adjustment         **adj3,
                              AdjustmentSourceType adjsrc4,
                              Adjustment         **adj4)
{
  for (WidgetImpl *pwidget = this->parent(); pwidget; pwidget = pwidget->parent())
    {
      AdjustmentSource *adjustment_source = pwidget->interface<AdjustmentSource*>();
      if (!adjustment_source)
        continue;
      if (adj1 && !*adj1)
        *adj1 = adjustment_source->get_adjustment (adjsrc1);
      if (adj2 && !*adj2)
        *adj2 = adjustment_source->get_adjustment (adjsrc2);
      if (adj3 && !*adj3)
        *adj3 = adjustment_source->get_adjustment (adjsrc3);
      if (adj4 && !*adj4)
        *adj4 = adjustment_source->get_adjustment (adjsrc4);
      if ((!adj1 || *adj1) &&
          (!adj2 || *adj2) &&
          (!adj3 || *adj3) &&
          (!adj4 || *adj4))
        break;
    }
}

void
WidgetImpl::repack (const PackInfo &orig, const PackInfo &pnew)
{
  if (parent())
    parent()->repack_child (*this, orig, pnew);
}

const PackInfo WidgetImpl::default_pack_info = {
  0,   1,   0, 1,       // hposition, hspan, vposition, vspan
  0.5, 1, 0.5, 1,       // halign, hscale, valign, vscale
  0,   0,   0, 0,       // left_spacing, right_spacing, bottom_spacing, top_spacing
  -1, -1,               // ovr_width, ovr_height
};

PackInfo&
WidgetImpl::widget_pack_info()
{
  if (!pack_info_)
    pack_info_ = new PackInfo (default_pack_info);
  return *pack_info_;
}

void
WidgetImpl::hposition (double d)
{
  PackInfo &pa = widget_pack_info(), op = pa;
  pa.hposition = d;
  repack (op, pa);
}

void
WidgetImpl::hspan (double d)
{
  PackInfo &pa = widget_pack_info(), op = pa;
  pa.hspan = MAX (1, d);
  repack (op, pa);
}

void
WidgetImpl::vposition (double d)
{
  PackInfo &pa = widget_pack_info(), op = pa;
  pa.vposition = d;
  repack (op, pa);
}

void
WidgetImpl::vspan (double d)
{
  PackInfo &pa = widget_pack_info(), op = pa;
  pa.vspan = MAX (1, d);
  repack (op, pa);
}

void
WidgetImpl::left_spacing (int s)
{
  PackInfo &pa = widget_pack_info(), op = pa;
  pa.left_spacing = CLAMP (s, 0, 32767);
  repack (op, pa);
}

void
WidgetImpl::right_spacing (int s)
{
  PackInfo &pa = widget_pack_info(), op = pa;
  pa.right_spacing = CLAMP (s, 0, 32767);
  repack (op, pa);
}

void
WidgetImpl::bottom_spacing (int s)
{
  PackInfo &pa = widget_pack_info(), op = pa;
  pa.bottom_spacing = CLAMP (s, 0, 32767);
  repack (op, pa);
}

void
WidgetImpl::top_spacing (int s)
{
  PackInfo &pa = widget_pack_info(), op = pa;
  pa.top_spacing = CLAMP (s, 0, 32767);
  repack (op, pa);
}

/// Override widget size requisition width (use -1 to unset).
void
WidgetImpl::width (int w)
{
  PackInfo &pa = widget_pack_info(), op = pa;
  pa.ovr_width = w >= 0 ? MIN (w, 32767) : -1;
  repack (op, pa);
}

/// Override widget size requisition height (use -1 to unset).
void
WidgetImpl::height (int h)
{
  PackInfo &pa = widget_pack_info(), op = pa;
  pa.ovr_height = h >= 0 ? MIN (h, 32767) : -1;
  repack (op, pa);
}

void
WidgetImpl::halign (double f)
{
  PackInfo &pa = widget_pack_info(), op = pa;
  pa.halign = CLAMP (f, 0, 1);
  repack (op, pa);
}

void
WidgetImpl::hscale (double f)
{
  PackInfo &pa = widget_pack_info(), op = pa;
  pa.hscale = f;
  repack (op, pa);
}

void
WidgetImpl::valign (double f)
{
  PackInfo &pa = widget_pack_info(), op = pa;
  pa.valign = CLAMP (f, 0, 1);
  repack (op, pa);
}

void
WidgetImpl::vscale (double f)
{
  PackInfo &pa = widget_pack_info(), op = pa;
  pa.vscale = f;
  repack (op, pa);
}

void
WidgetImpl::do_changed (const String &name)
{
  ObjectImpl::do_changed (name);
}

bool
WidgetImpl::do_event (const Event &event)
{
  return false;
}

String
WidgetImpl::hsize_group () const
{
  return string_join (",", list_widget_groups (WIDGET_GROUP_HSIZE));
}

void
WidgetImpl::hsize_group (const String &group_list)
{
  sync_widget_groups (group_list, WIDGET_GROUP_HSIZE);
}

String
WidgetImpl::vsize_group () const
{
  return string_join (",", list_widget_groups (WIDGET_GROUP_VSIZE));
}

void
WidgetImpl::vsize_group (const String &group_list)
{
  sync_widget_groups (group_list, WIDGET_GROUP_VSIZE);
}

static DataKey<String> widget_id_key;

String
WidgetImpl::id () const
{
  String s = get_data (&widget_id_key);
  if (s.empty())
    return Factory::factory_context_id (factory_context());
  else
    return s;
}

void
WidgetImpl::id (const String &str)
{
  if (str.empty())
    delete_data (&widget_id_key);
  else
    set_data (&widget_id_key, str);
  changed ("id");
}

UserSource
WidgetImpl::user_source () const
{
  return Factory::factory_context_source (factory_context());
}

Color
WidgetImpl::current_color (StyleColor color_type, const String &detail)
{
  return state_color (state(), color_type, detail);
}

Color
WidgetImpl::state_color (WidgetState state, StyleColor color_type, const String &detail)
{
  return style()->state_color (state, color_type, detail);
}

Color
WidgetImpl::foreground ()
{
  return state_color (state(), StyleColor::FOREGROUND);
}

Color
WidgetImpl::background ()
{
  return state_color (state(), StyleColor::BACKGROUND);
}

Color
WidgetImpl::dark_color ()
{
  return state_color (state(), StyleColor::DARK);
}

Color
WidgetImpl::dark_clinch ()
{
  return state_color (state(), StyleColor::DARK_CLINCH);
}

Color
WidgetImpl::light_color ()
{
  return state_color (state(), StyleColor::LIGHT);
}

Color
WidgetImpl::light_clinch ()
{
  return state_color (state(), StyleColor::LIGHT_CLINCH);
}

Color
WidgetImpl::focus_color ()
{
  return state_color (state(), StyleColor::FOCUS_COLOR);
}

bool
WidgetImpl::tune_requisition (int new_width, int new_height)
{
  Requisition req = requisition();
  if (new_width >= 0)
    req.width = new_width;
  if (new_height >= 0)
    req.height = new_height;
  return tune_requisition (req);
}

bool
WidgetImpl::tune_requisition (Requisition requisition)
{
  WidgetImpl *p = parent();
  if (p && !test_any (INVALID_REQUISITION))
    {
      WindowImpl *rc = get_window();
      if (rc && rc->requisitions_tunable())
        {
          Requisition ovr (width(), height());
          requisition.width = ovr.width >= 0 ? ovr.width : MAX (requisition.width, 0);
          requisition.height = ovr.height >= 0 ? ovr.height : MAX (requisition.height, 0);
          if (requisition.width != requisition_.width || requisition.height != requisition_.height)
            {
              requisition_ = requisition;
              /* once a child changes size, its parent needs to:
               * - check the parent size, it might have changed
               * - reallocate the child, possibly changing the previous allocation
               */
              p->invalidate_size();
              return true;
            }
        }
    }
  return false;
}

/** Set parent-relative size allocation of a widget.
 *
 * Allocate the given @a area to @a this widget.
 * The size allocation is used by the widget for layouting of its contents.
 * That is, its rendering contents and children of a container will be constrained to the allocation() area.
 * Normally, children are clipped to their parent's allocation, which constrains the part of the
 * allocation that is to be rendered and is sensitive for input event processing, see clipped_allocation().
 * This method clears the #INVALID_ALLOCATION flag and calls expose() on the widget as needed.
 */
void
WidgetImpl::set_child_allocation (const Allocation &area)
{
  Allocation sarea = area;
  // capture old allocation area
  const Allocation old_allocation = allocation();
  // determine new allocation area
  if (!visible())
    sarea = Allocation (0, 0, 0, 0);
  const bool allocation_changed = child_allocation_ != sarea;
  if (test_any (INVALID_ALLOCATION) || allocation_changed)
    {
      change_flags_silently (INVALID_ALLOCATION, false);
      // expose old area
      if (allocation_changed)
        {
          Region region (old_allocation);
          expose_unclipped (region);            // skip intersecting with allocation
        }
      // move and resize new allocation
      child_allocation_ = sarea;
      size_allocate (allocation());             // causes re-layout of immediate children
      // re-render new area
      if (allocation_changed)
        invalidate_content();
      SZDEBUG ("size allocation: 0x%016x:%s: %s => %s", size_t (this),
               Factory::factory_context_type (factory_context()), id(),
               allocation().string());
    }
}

// == rendering ==
class WidgetImpl::RenderContext {
  friend class WidgetImpl;
  cairo_surface_t *surface;
  cairo_t         *cairo;
  IRect            area;
public:
  explicit      RenderContext (IRect rect) :
    surface (NULL), cairo (NULL), area (rect)
  {}
  /*dtor*/     ~RenderContext()
  {
    if (surface)
      cairo_surface_destroy (surface);
    if (cairo)
      cairo_destroy (cairo);
  }
};

void
WidgetImpl::widget_compose_into (cairo_t *cr, const vector<IRect> &view_rects, int x_offset, int y_offset)
{
  ContainerImpl *container = as_container_impl();
  const Allocation view_area = Allocation (x_offset + child_allocation_.x, y_offset + child_allocation_.y, child_allocation_.width, child_allocation_.height);
  // clipping rectangles for this and descendants
  vector<IRect> clip_rects;
  if (cached_surface_ || (container && container->has_children()))
    for (size_t i = 0; i < view_rects.size(); i++)
      {
        IRect rect = view_rects[i];
        rect.intersect (view_area);
        if (!rect.empty())
          clip_rects.push_back (rect);
      }
  if (cached_surface_ && clip_rects.size())
    {
      cairo_save (cr);
      // clip to rectangles
      for (size_t i = 0; i < clip_rects.size(); i++)
        cairo_rectangle (cr, clip_rects[i].x, clip_rects[i].y, clip_rects[i].width, clip_rects[i].height);
      cairo_clip (cr);
      // compose widget surface OVER
      cairo_set_source_surface (cr, cached_surface_, view_area.x, view_area.y);
      cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
      cairo_paint_with_alpha (cr, 1.0);
      // done
      cairo_restore (cr);
    }
  if (container && clip_rects.size())
    for (auto &child : *container)
      child->widget_compose_into (cr, clip_rects, view_area.x, view_area.y);
}

void
WidgetImpl::compose_into (cairo_t *cr, const vector<IRect> &view_rects)
{
  widget_compose_into (cr, view_rects, 0, 0);
}

/// Render widget contents and contents of all viewable descendants.
void
WidgetImpl::render_widget ()
{
  widget_render_recursive (child_allocation_);
}

void
WidgetImpl::widget_render_recursive (const IRect &ancestry_clip)
{
  if (test_any (INVALID_REQUISITION))
    critical ("%s: rendering widget with invalid %s: %s", debug_name(), "requisition", debug_name ("%r"));
  if (test_any (INVALID_ALLOCATION))
    critical ("%s: rendering widget with invalid %s: %s", debug_name(), "allocation", debug_name ("%a"));
  // abort if we're clipped
  const IRect clip_rect = allocation().intersection (ancestry_clip);
  return_unless (clip_rect.empty() == false);
  // first render descandants recursively
  ContainerImpl *container = as_container_impl();
  if (container)
    for (auto &child : *container)
      child->widget_render_recursive (clip_rect);
  // render contents on demand only
  return_unless (test_any (INVALID_CONTENT) == true);
  // dispose of previous scene...
  if (cached_surface_)
    {
      cairo_surface_t *const surface = cached_surface_;
      assert_return (cairo_image_surface_get_format (surface) == CAIRO_FORMAT_ARGB32);
      const uint width = cairo_image_surface_get_width (surface);
      const uint height = cairo_image_surface_get_height (surface);
      double xoff = 0, yoff = 0;
      cairo_surface_get_device_offset (surface, &xoff, &yoff);
      IRect rect = IRect (-iround (xoff), -iround (yoff), width, height);
      expose_unclipped (rect);
      cairo_surface_destroy (cached_surface_);
      cached_surface_ = NULL;
    }
  // render contents
  RenderContext rcontext (allocation());
  render (rcontext);
  assert_return (cached_surface_ == NULL);
  cached_surface_ = rcontext.surface;
  rcontext.surface = NULL;
  set_flag (INVALID_CONTENT, false);
  // ensure display of new scene...
  if (cached_surface_)
    {
      cairo_surface_t *const surface = cached_surface_;
      assert_return (cairo_image_surface_get_format (surface) == CAIRO_FORMAT_ARGB32);
      const uint width = cairo_image_surface_get_width (surface);
      const uint height = cairo_image_surface_get_height (surface);
      double xoff = 0, yoff = 0;
      cairo_surface_get_device_offset (surface, &xoff, &yoff);
      IRect rect = IRect (-iround (xoff), -iround (yoff), width, height);
      expose_unclipped (rect);
    }
  assert_return (test_any (INVALID_REQUISITION | INVALID_ALLOCATION | INVALID_CONTENT) == 0);
}

Region
WidgetImpl::rendering_region (RenderContext &rcontext) const
{
  return Region (rcontext.area);
}

cairo_t*
WidgetImpl::cairo_context (RenderContext &rcontext)
{
  if (rcontext.cairo)
    return rcontext.cairo;
  bool empty_dummy = false;
  if (!rcontext.surface)
    {
      IRect rect = rcontext.area;
      empty_dummy = rect.width < 1 || rect.height < 1;
      if (empty_dummy)
        rect.width = rect.height = 1; // simulate cairo_context with 0x0 pixels
      rcontext.surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, rect.width, rect.height);
      CAIRO_CHECK_STATUS (rcontext.surface);
      cairo_surface_set_device_offset (rcontext.surface, -rect.x, -rect.y);
    }
  if (!rcontext.cairo)
    {
      rcontext.cairo = cairo_create (rcontext.surface);
      CAIRO_CHECK_STATUS (rcontext.cairo);
      if (empty_dummy)
        {
          cairo_surface_destroy (rcontext.surface);
          rcontext.surface = NULL;
        }
    }
  return rcontext.cairo;
}

/// Return wether a widget is viewable() and not #STASHED.
bool
WidgetImpl::drawable () const
{
  if (viewable() && child_allocation_.width > 0 && child_allocation_.height > 0)
    return true;
  return false;
}

} // Rapicorn
