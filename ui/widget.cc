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
  flags_ (VISIBLE),
  parent_ (NULL), acache_ (NULL), heritage_ (NULL),
  factory_context_ (ctor_factory_context()), sig_invalidate (Aida::slot (*this, &WidgetImpl::do_invalidate)),
  sig_hierarchy_changed (Aida::slot (*this, &WidgetImpl::hierarchy_changed))
{}

void
WidgetImpl::construct ()
{
  change_flags_silently (CONSTRUCTED, true);
  ObjectImpl::construct();
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

/// Return wether a widget is visible() and not offscreen, see ContainerImpl::change_unviewable()
bool
WidgetImpl::viewable() const
{
  return visible() && !test_any_flag (UNVIEWABLE | PARENT_UNVIEWABLE);
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

bool
WidgetImpl::change_flags_silently (uint64 mask, bool on)
{
  const uint64 old_flags = flags_;
  assert_return (mask != WidgetState::NORMAL, 0);
  if (old_flags & CONSTRUCTED)
    {
      // refuse to change constant flags
      assert_return ((mask & CONSTRUCTED) == 0, 0);
      assert_return ((mask & NEEDS_FOCUS_INDICATOR) == 0, 0);
    }
  if (old_flags & FINALIZING)
    assert_return ((mask & FINALIZING) == 0, 0);
  if (on)
    flags_ |= mask;
  else
    flags_ &= ~mask;
  if (parent() && uint64 (WidgetState::SELECTED) & (old_flags ^ flags_))
    {
      WidgetChain this_chain;
      this_chain.widget = this;
      this_chain.next = NULL;
      parent()->selectable_child_changed (this_chain);
    }
  // silently: omit change notification
  return old_flags != flags_;
}

void
WidgetImpl::propagate_state (bool notify_changed)
{
  ContainerImpl *container = as_container_impl();
  const bool self_is_window_impl = UNLIKELY (parent_ == NULL) && container && as_window_impl();
  const bool was_viewable = viewable();
  change_flags_silently (PARENT_INSENSITIVE, parent() && parent()->insensitive());
  change_flags_silently (PARENT_UNVIEWABLE, !self_is_window_impl && (!parent() || !parent()->viewable()));
  if (was_viewable != viewable())
    invalidate();       // changing viewable forces invalidation, regardless of notify_changed
  if (container)
    for (auto child : *container)
      child->propagate_state (notify_changed);
  if (notify_changed && !finalizing())
    sig_changed.emit (""); // changed() does not imply invalidate(), see above
}

void
WidgetImpl::set_flag (uint64 flag, bool on)
{
  assert ((flag & (flag - 1)) == 0); // single bit check
  const uint64 propagate_flag_mask = VISIBLE | uint64 (WidgetState::INSENSITIVE) | UNVIEWABLE |
                                     PARENT_INSENSITIVE | PARENT_UNVIEWABLE |
                                     WidgetState::HOVER | WidgetState::ACTIVE | HAS_DEFAULT;
  const uint64 repack_flag_mask = HSHRINK | VSHRINK | HEXPAND | VEXPAND |
                                  HSPREAD | VSPREAD | HSPREAD_CONTAINER | VSPREAD_CONTAINER |
                                  VISIBLE | UNVIEWABLE | PARENT_UNVIEWABLE;
  const bool fchanged = change_flags_silently (flag, on);
  if (fchanged)
    {
      if (flag & propagate_flag_mask)
        {
          expose();
          propagate_state (false);
        }
      if (flag & repack_flag_mask)
        {
          const PackInfo &pa = pack_info();
          repack (pa, pa); // includes invalidate();
          invalidate_parent(); // request resize even if flagged as invalid already
        }
      changed ("flags");
    }
}

StyleIfaceP
WidgetImpl::style () const
{
  return Factory::factory_context_style (factory_context_);
}

WidgetState
WidgetImpl::state () const
{
  constexpr WidgetState s0 = WidgetState (0); // WidgetState::NORMAL
  uint64 state = 0;
  state = state | (hover()       ? WidgetState::HOVER : s0);
  // WidgetState::PANEL
  // WidgetState::ACCELERATABLE
  state = state | (has_default() ? WidgetState::DEFAULT : s0);
  state = state | (insensitive() ? WidgetState::INSENSITIVE : s0);
  // WidgetState::SELECTED
  state = state | (has_focus()   ? WidgetState::FOCUSED : s0);
  state = state | (active()      ? WidgetState::ACTIVE : s0);
  // WidgetState::RETAINED
  return WidgetState (state);
}

bool
WidgetImpl::grab_default () const
{
  return false;
}

bool
WidgetImpl::allow_focus () const
{
  return test_any_flag (ALLOW_FOCUS);
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
  return test_all_flags (ALLOW_FOCUS | NEEDS_FOCUS_INDICATOR | HAS_FOCUS_INDICATOR) && can_focus();
}

bool
WidgetImpl::has_focus () const
{
  if (test_any_flag (uint64 (WidgetState::FOCUSED)))
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
  if (test_any_flag (uint64 (WidgetState::FOCUSED)))
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
  if (wself && imatcher.match (self, name()))
    return true;
  if (wparent)
    {
      WidgetImpl *pwidget = parent();
      while (pwidget)
        {
          if (imatcher.match (pwidget, pwidget->name()))
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
  if (heritage_)
    heritage_ = NULL;
  uint timer_id = get_data (&visual_update_key);
  if (timer_id)
    {
      remove_exec (timer_id);
      set_data (&visual_update_key, uint (0));
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
    throw Exception ("no such property: " + name() + "::" + property_name);
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

/// Get overriding widget size requisition width (-1 if unset).
double
WidgetImpl::width () const
{
  Requisition ovr = get_data (&override_requisition);
  return ovr.width >= 0 ? ovr.width : -1;
}

/// Override widget size requisition width (use -1 to unset).
void
WidgetImpl::width (double w)
{
  Requisition ovr = get_data (&override_requisition);
  ovr.width = w >= 0 ? w : -1;
  set_data (&override_requisition, ovr);
  invalidate_size();
}

/// Get overriding widget size requisition height (-1 if unset).
double
WidgetImpl::height () const
{
  Requisition ovr = get_data (&override_requisition);
  return ovr.height >= 0 ? ovr.height : -1;
}

/// Override widget size requisition height (use -1 to unset).
void
WidgetImpl::height (double h)
{
  Requisition ovr = get_data (&override_requisition);
  ovr.height = h >= 0 ? h : -1;
  set_data (&override_requisition, ovr);
  invalidate_size();
}

void
WidgetImpl::propagate_heritage ()
{
  ContainerImpl *container = this->as_container_impl();
  if (container)
    for (auto child : *container)
      child->heritage (heritage_);
}

void
WidgetImpl::heritage (HeritageP heritage)
{
  HeritageP old_heritage = heritage_;
  heritage_ = NULL;
  if (heritage)
    heritage_ = heritage->adapt_heritage (*this, color_scheme());
  if (heritage_ != old_heritage)
    {
      old_heritage = NULL;
      invalidate();
      propagate_heritage ();
    }
}

bool
WidgetImpl::process_event (const Event &event, bool capture) // widget coordinates relative
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

bool
WidgetImpl::point (Point p) /* widget coordinates relative */
{
  const Allocation a = clipped_allocation();
  return (drawable() &&
          p.x >= a.x && p.x < a.x + a.width &&
          p.y >= a.y && p.y < a.y + a.height);
}

void
WidgetImpl::set_parent (ContainerImpl *pcontainer)
{
  assert_return (this != as_window_impl());
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
      invalidate();
      if (heritage())
        heritage (NULL);
      old_parent->unparent_child (*this);
      parent_ = NULL;
      if (acache_)
        acache_reset (this);
      if (anchored())
        {
          assert_return (old_toplevel != NULL);
          sig_hierarchy_changed.emit (old_toplevel);
        }
      propagate_state (false); // propagate PARENT_VISIBLE, PARENT_INSENSITIVE
    }
  if (pcontainer)
    {
      assert_return (old_parent == NULL);
      parent_ = pcontainer;
      if (parent_->heritage())
        heritage (parent_->heritage());
      if (parent_->anchored() && !anchored())
        sig_hierarchy_changed.emit (NULL);
      invalidate();
      propagate_state (true);
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
WidgetImpl::invalidate_parent ()
{
  /* propagate (size) invalidation from children to parents */
  WidgetImpl *p = parent();
  if (p)
    p->invalidate_size();
}

void
WidgetImpl::invalidate (uint64 mask)
{
  mask &= INVALID_REQUISITION | INVALID_ALLOCATION | INVALID_CONTENT;
  return_unless (mask != 0);
  const bool had_invalid_content = test_any_flag (INVALID_CONTENT);
  const bool had_invalid_allocation = test_any_flag (INVALID_ALLOCATION);
  const bool had_invalid_requisition = test_any_flag (INVALID_REQUISITION);
  bool emit_invalidated = false;
  if (!had_invalid_content && (mask & INVALID_CONTENT))
    {
      expose();
      emit_invalidated = true;
    }
  change_flags_silently (mask, true);
  if ((!had_invalid_requisition && (mask & INVALID_REQUISITION)) ||
      (!had_invalid_allocation && (mask & INVALID_ALLOCATION)))
    {
      invalidate_parent(); // need new size-request from parent
      WidgetGroup::invalidate_widget (*this);
      emit_invalidated = true;
    }
  if (emit_invalidated && anchored())
    sig_invalidate.emit();
}

/// Determine "internal" size requisition of a widget, including overrides, excluding groupings.
Requisition
WidgetImpl::inner_size_request()
{
  /* loop until we gather a valid requisition, this explicitely allows for
   * requisition invalidation during the size_request phase, widget implementations
   * have to ensure we're not looping endlessly
   */
  while (test_any_flag (WidgetImpl::INVALID_REQUISITION))
    {
      change_flags_silently (WidgetImpl::INVALID_REQUISITION, false); // skip notification
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
                   Factory::factory_context_type (factory_context()).c_str(), name().c_str(),
                   inner.width, inner.height);
        }
      requisition_ = inner;
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

bool
WidgetImpl::tune_requisition (double new_width,
                            double new_height)
{
  Requisition req = requisition();
  if (new_width >= 0)
    req.width = new_width;
  if (new_height >= 0)
    req.height = new_height;
  return tune_requisition (req);
}

void
WidgetImpl::expose_internal (const Region &region)
{
  if (!region.empty())
    {
      // queue expose region on nextmost viewport
      ViewportImpl *vp = parent() ? parent()->get_viewport() : get_viewport();
      if (vp)
        vp->expose_child_region (region);
    }
}

/** Invalidate drawing contents of a widget
 *
 * Cause the given @a region of @a this widget to be rerendered.
 * The region is constrained to the clipped_allocation().
 * This method sets the #INVALID_CONTENT flag.
 */
void
WidgetImpl::expose (const Region &region) // widget relative
{
  if (drawable() && !test_any_flag (INVALID_CONTENT))
    {
      Region r (clipped_allocation());
      r.intersect (region);
      expose_internal (r);
    }
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
WidgetImpl::debug_name (const String &format)
{
  String s = format.empty() ? "%n" : format;
  const UserSource us = Factory::factory_context_source (factory_context());
  s = string_replace (s, "%f", us.filename);
  s = string_replace (s, "%l", string_format ("%u", us.line));
  s = string_replace (s, "%n", name());
  s = string_replace (s, "%r", string_format ("%gx%g", requisition_.width, requisition_.height));
  s = string_replace (s, "%a", allocation_.string());
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
  const Allocation *carea = clip_area();
  tstream.dump ("clip_area", carea ? carea->string() : "");
  tstream.dump ("factory_context", string_format ("{ name=%s, type=%s, impl=%s }",
                                                  Factory::factory_context_name (factory_context()),
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
WidgetImpl::repack (const PackInfo &orig,
              const PackInfo &pnew)
{
  if (parent())
    parent()->repack_child (*this, orig, pnew);
  invalidate();
}

WidgetImpl::PackInfo&
WidgetImpl::pack_info (bool create)
{
  static const PackInfo pack_info_defaults = {
    0,   1,   0, 1,     /* hposition, hspan, vposition, vspan */
    0,   0,   0, 0,     /* left_spacing, right_spacing, bottom_spacing, top_spacing */
    0.5, 1, 0.5, 1,     /* halign, hscale, valign, vscale */
  };
  static DataKey<PackInfo*> pack_info_key;
  PackInfo *pi = get_data (&pack_info_key);
  if (!pi)
    {
      if (create)
        {
          pi = new PackInfo (pack_info_defaults);
          set_data (&pack_info_key, pi);
        }
      else /* read-only access */
        pi = const_cast<PackInfo*> (&pack_info_defaults);
    }
  return *pi;
}

void
WidgetImpl::hposition (double d)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.hposition = d;
  repack (op, pa);
}

void
WidgetImpl::hspan (double d)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.hspan = MAX (1, d);
  repack (op, pa);
}

void
WidgetImpl::vposition (double d)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.vposition = d;
  repack (op, pa);
}

void
WidgetImpl::vspan (double d)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.vspan = MAX (1, d);
  repack (op, pa);
}

void
WidgetImpl::left_spacing (int s)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.left_spacing = MAX (0, s);
  repack (op, pa);
}

void
WidgetImpl::right_spacing (int s)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.right_spacing = MAX (0, s);
  repack (op, pa);
}

void
WidgetImpl::bottom_spacing (int s)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.bottom_spacing = MAX (0, s);
  repack (op, pa);
}

void
WidgetImpl::top_spacing (int s)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.top_spacing = MAX (0, s);
  repack (op, pa);
}

void
WidgetImpl::halign (double f)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.halign = CLAMP (f, 0, 1);
  repack (op, pa);
}

void
WidgetImpl::hscale (double f)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.hscale = f;
  repack (op, pa);
}

void
WidgetImpl::valign (double f)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.valign = CLAMP (f, 0, 1);
  repack (op, pa);
}

void
WidgetImpl::vscale (double f)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.vscale = f;
  repack (op, pa);
}

void
WidgetImpl::do_changed (const String &name)
{
  ObjectImpl::do_changed (name);
}

void
WidgetImpl::do_invalidate()
{}

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

static DataKey<String> widget_name_key;

String
WidgetImpl::name () const
{
  String s = get_data (&widget_name_key);
  if (s.empty())
    return Factory::factory_context_name (factory_context());
  else
    return s;
}

void
WidgetImpl::name (const String &str)
{
  if (str.empty())
    delete_data (&widget_name_key);
  else
    set_data (&widget_name_key, str);
  changed ("name");
}

UserSource
WidgetImpl::user_source () const
{
  return Factory::factory_context_source (factory_context());
}

static DataKey<ColorScheme> widget_color_scheme_key;

ColorScheme
WidgetImpl::color_scheme () const
{
  return get_data (&widget_color_scheme_key);
}

void
WidgetImpl::color_scheme (ColorScheme cst)
{
  if (cst != get_data (&widget_color_scheme_key))
    {
      if (cst == 0)
        delete_data (&widget_color_scheme_key);
      else
        set_data (&widget_color_scheme_key, cst);
      heritage (heritage()); // forces recalculation/adaption
    }
}

ThemeInfo&
WidgetImpl::theme_info () const
{
  return *ThemeInfo::fallback_theme();
}

Color
WidgetImpl::foreground ()
{
  return heritage()->foreground (state());
}

Color
WidgetImpl::background ()
{
  return heritage()->background (state());
}

/// Return clipping area for rendering and event processing if one is set.
const Allocation*
WidgetImpl::clip_area () const
{
  return test_any_flag (HAS_CLIP_AREA) ? &clip_area_ : NULL;
}

/// Assign clipping area for rendering and event processing.
void
WidgetImpl::clip_area (const Allocation *clip)
{
  const bool has_clip_area = clip != NULL;
  clip_area_ = has_clip_area ? *clip : Rect();
  if (has_clip_area != test_any_flag (HAS_CLIP_AREA))
    change_flags_silently (HAS_CLIP_AREA, has_clip_area);
}

/** Return widget allocation area accounting for clip_area().
 *
 * For any rendering or event processing purposes, clipped_allocation() should be used over allocation().
 * The unclipped size allocation is just used by @a this widget internally for layouting purposes, see also set_allocation().
 */
Allocation
WidgetImpl::clipped_allocation () const
{
  Allocation area = allocation();
  const Allocation *clip = clip_area();
  if (clip)
    area.intersect (*clip);
  return area;
}

bool
WidgetImpl::tune_requisition (Requisition requisition)
{
  WidgetImpl *p = parent();
  if (p && !test_any_flag (INVALID_REQUISITION))
    {
      ResizeContainerImpl *rc = p->get_resize_container();
      if (rc && rc->requisitions_tunable())
        {
          Requisition ovr (width(), height());
          requisition.width = ovr.width >= 0 ? ovr.width : MAX (requisition.width, 0);
          requisition.height = ovr.height >= 0 ? ovr.height : MAX (requisition.height, 0);
          if (requisition.width != requisition_.width || requisition.height != requisition_.height)
            {
              requisition_ = requisition;
              invalidate_parent(); // need new size-request on parent
              return true;
            }
        }
    }
  return false;
}

/** Set size allocation of a widget.
 *
 * Allocate the given @a area to @a this widget.
 * The size allocation is used by the widget for layouting of its contents.
 * That is, its rendering contents and children of a container will be constrained to the allocation() area.
 * Normally, children are clipped to their parent's allocation, this can be overridden by
 * passing a different @a clip area which constrains the part of the allocation to be rendered
 * and is sensitive for input event processing, see clipped_allocation().
 * This method clears the #INVALID_ALLOCATION flag and calls expose() on the widget as needed.
 */
void
WidgetImpl::set_allocation (const Allocation &area, const Allocation *clip)
{
  Allocation sarea (iround (area.x), iround (area.y), iround (area.width), iround (area.height));
  const double smax = 4503599627370496.; // 52bit precision is maximum for doubles
  sarea.x      = CLAMP (sarea.x, -smax, smax);
  sarea.y      = CLAMP (sarea.y, -smax, smax);
  sarea.width  = CLAMP (sarea.width,  0, smax);
  sarea.height = CLAMP (sarea.height, 0, smax);
  /* remember old area */
  const Allocation old_allocation = clipped_allocation();
  const Rect *const old_clip_ptr = clip_area(), old_clip = old_clip_ptr ? *old_clip_ptr : old_allocation;
  // always reallocate to re-layout children
  change_flags_silently (INVALID_ALLOCATION, false); /* skip notification */
  if (!visible())
    sarea = Allocation (0, 0, 0, 0);
  Rect new_clip = clip ? *clip : area;
  if (!clip && parent())
    new_clip.intersect (parent()->clipped_allocation());
  const bool allocation_changed = allocation_ != sarea || new_clip != old_clip;
  allocation_ = sarea;
  clip_area (new_clip == area ? NULL : &new_clip); // invalidates old clip_area()
  size_allocate (allocation_, allocation_changed);
  Allocation a = allocation();
  const bool need_expose = allocation_changed || test_any_flag (INVALID_CONTENT);
  change_flags_silently (INVALID_CONTENT, false); // skip notification
  // expose old area
  if (need_expose)
    {
      Region region (old_allocation);
      expose_internal (region); // don't intersect with new allocation
    }
  /* expose new area */
  if (need_expose)
    expose();
  SZDEBUG ("size allocation: 0x%016x:%s: %s => %s", size_t (this),
           Factory::factory_context_type (factory_context()).c_str(), name().c_str(),
           a.string().c_str());
}

// == rendering ==
class WidgetImpl::RenderContext {
  friend class WidgetImpl;
  vector<cairo_surface_t*> surfaces;
  Region                   render_area;
  Rect                    *hierarchical_clip;
  vector<cairo_t*>         cairos;
public:
  explicit      RenderContext() : hierarchical_clip (NULL) {}
  /*dtor*/     ~RenderContext();
  const Region& region() const { return render_area; }
};

void
WidgetImpl::render_into (cairo_t *cr, const Region &region)
{
  RenderContext rcontext;
  rcontext.render_area = clipped_allocation();
  rcontext.render_area.intersect (region);
  if (!rcontext.render_area.empty())
    {
      render_widget (rcontext);
      cairo_save (cr);
      vector<Rect> rects;
      rcontext.render_area.list_rects (rects);
      for (size_t i = 0; i < rects.size(); i++)
        cairo_rectangle (cr, rects[i].x, rects[i].y, rects[i].width, rects[i].height);
      cairo_clip (cr);
      for (size_t i = 0; i < rcontext.surfaces.size(); i++)
        {
          cairo_set_source_surface (cr, rcontext.surfaces[i], 0, 0);
          cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
          cairo_paint_with_alpha (cr, 1.0);
        }
      cairo_restore (cr);
    }
}

/// Render widget's clipped allocation area contents into the rendering context provided.
void
WidgetImpl::render_widget (RenderContext &rcontext)
{
  size_t n_cairos = rcontext.cairos.size();
  Rect area = clipped_allocation();
  Rect *saved_hierarchical_clip = rcontext.hierarchical_clip;
  Rect newclip;
  const Rect *clip = clip_area();
  if (clip)
    {
      newclip = *clip;
      if (saved_hierarchical_clip)
        newclip.intersect (*saved_hierarchical_clip);
      rcontext.hierarchical_clip = &newclip;
    }
  area.intersect (rendering_region (rcontext).extents());
  render (rcontext, area);
  render_recursive (rcontext);
  rcontext.hierarchical_clip = saved_hierarchical_clip;
  while (rcontext.cairos.size() > n_cairos)
    {
      cairo_destroy (rcontext.cairos.back());
      rcontext.cairos.pop_back();
    }
}

void
WidgetImpl::render (RenderContext &rcontext, const Rect &rect)
{}

void
WidgetImpl::render_recursive (RenderContext &rcontext)
{}

const Region&
WidgetImpl::rendering_region (RenderContext &rcontext) const
{
  return rcontext.render_area;
}

cairo_t*
WidgetImpl::cairo_context (RenderContext &rcontext, const Allocation &area)
{
  Rect rect = area;
  if (area == Allocation (-1, -1, 0, 0))
    rect = clipped_allocation();
  if (rcontext.hierarchical_clip)
    rect.intersect (*rcontext.hierarchical_clip);
  const bool empty_dummy = rect.width < 1 || rect.height < 1; // we allow cairo_contexts with 0x0 pixels
  if (empty_dummy)
    rect.width = rect.height = 1;
  assert_return (rect.width > 0 && rect.height > 0, NULL);
  cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, iceil (rect.width), iceil (rect.height));
  if (cairo_surface_status (surface) != CAIRO_STATUS_SUCCESS)
    critical ("%s: failed to create ARGB32 cairo surface with %dx%d pixels: %s\n", __func__, iceil (rect.width), iceil (rect.height),
              cairo_status_to_string (cairo_surface_status (surface)));
  cairo_surface_set_device_offset (surface, -rect.x, -rect.y);
  cairo_t *cr = cairo_create (surface);
  rcontext.cairos.push_back (cr);
  if (empty_dummy)
    cairo_surface_destroy (surface);
  else
    rcontext.surfaces.push_back (surface);
  return cr;
}

WidgetImpl::RenderContext::~RenderContext()
{
  critical_unless (cairos.size() == 0);
  while (!cairos.empty())
    {
      cairo_destroy (cairos.back());
      cairos.pop_back();
    }
  while (!surfaces.empty())
    {
      cairo_surface_destroy (surfaces.back());
      surfaces.pop_back();
    }
}

/// Return wether a widget is viewable() and has a non-0 clipped allocation
bool
WidgetImpl::drawable () const
{
  if (viewable() && allocation_.width > 0 && allocation_.height > 0)
    {
      const Allocation *clip = clip_area();
      if (clip)
        {
          Allocation carea = allocation_;
          carea.intersect (*clip);
          if (carea.width <= 0 || carea.height <= 0)
            return false;
        }
      return true;
    }
  return false;
}

} // Rapicorn
