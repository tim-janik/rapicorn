// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "window.hh"
#include "factory.hh"
#include "application.hh"
#include "uithread.hh"
#include <algorithm>

namespace Rapicorn {

WindowImpl&
WindowIface::impl ()
{
  WindowImpl *wimpl = dynamic_cast<WindowImpl*> (this);
  if (!wimpl)
    throw std::bad_cast();
  return *wimpl;
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


WindowImpl::WindowImpl () :
  loop_ (uithread_main_loop()->create_sub_loop()),
  commands_emission_ (NULL)
{
  // create event loop (auto-starts)
  loop_->exec_dispatcher (Aida::slot (*this, &WindowImpl::command_dispatcher), EventLoop::PRIORITY_NOW);
  loop_->flag_primary (false);
}

void
WindowImpl::construct()
{
  ViewportImpl::construct();
  assert_return (has_children() == false);      // must be anchored before becoming parent
  assert_return (get_window() && get_viewport());
  WindowTrail::wenter (this);
  ApplicationImpl::WindowImplFriend::add_window (*this);
  assert_return (anchored() == false);
  sig_hierarchy_changed.emit (NULL);
  assert_return (anchored() == true);
}

void
WindowImpl::dispose()
{
  assert_return (anchored() == true);
  const bool was_listed = ApplicationImpl::WindowImplFriend::remove_window (*this);
  assert_return (was_listed);
  sig_hierarchy_changed.emit (this);
  assert_return (anchored() == false);
  ViewportImpl::dispose();
}

WindowImpl::~WindowImpl ()
{
  WindowTrail::wleave (this);
  assert_return (anchored() == false);
  // shutdown event loop
  loop_->destroy_loop();
  /* make sure all children are removed while this is still of type ViewportImpl.
   * necessary because C++ alters the object type during constructors and destructors
   */
  if (has_children())
    remove (get_child());
  AncestryCache *ancestry_cache = const_cast<AncestryCache*> (ViewportImpl::fetch_ancestry_cache());
  ancestry_cache->window = NULL;
}

const WidgetImpl::AncestryCache*
WindowImpl::fetch_ancestry_cache ()
{
  AncestryCache *ancestry_cache = const_cast<AncestryCache*> (ViewportImpl::fetch_ancestry_cache());
  ancestry_cache->window = this;
  return ancestry_cache;
}

void
WindowImpl::set_parent (ContainerImpl *parent)
{
  if (parent)
    critical ("setting parent on toplevel Window widget to: %p (%s)", parent, parent->typeid_name().c_str());
  return ContainerImpl::set_parent (parent);
}

void
WindowImpl::create_display_window ()
{
  ViewportImpl::create_display_window();
  if (has_display_window())
    loop_->flag_primary (true);
}

void
WindowImpl::destroy_display_window ()
{
  ViewportImpl::destroy_display_window();
  loop_->flag_primary (false);
}

void
WindowImpl::forcefully_close_all()
{
  vector<WindowImpl*> wl = WindowTrail::wlist();
  for (auto it : wl)
    it->close();
}

bool
WindowImpl::widget_is_anchored (WidgetImpl &widget, Internal)
{
  if (widget.parent() && widget.parent()->anchored())
    return true;
  WindowImpl *window = widget.as_window_impl(); // fast cast
  if (window && ApplicationImpl::WindowImplFriend::has_window (*window))
    return true;
  return false;
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
WindowImpl::command_dispatcher (const LoopState &state)
{
  if (state.phase == state.PREPARE || state.phase == state.CHECK)
    return commands_emission_ && commands_emission_->pending();
  else if (state.phase == state.DISPATCH)
    {
      WindowImplP guard_this = shared_ptr_cast<WindowImpl> (this);
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

bool
WindowImpl::synthesize_enter (double xalign, double yalign)
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
WindowImpl::synthesize_leave ()
{
  if (!has_display_window())
    return false;
  EventContext ec;
  push_immediate_event (create_event_mouse (MOUSE_LEAVE, ec));
  return true;
}

bool
WindowImpl::synthesize_click (WidgetIface &widgeti, int button, double xalign, double yalign)
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
WindowImpl::synthesize_delete ()
{
  if (!has_display_window())
    return false;
  EventContext ec;
  push_immediate_event (create_event_win_delete (ec));
  return true;
}

static const WidgetFactory<WindowImpl> window_factory ("Rapicorn::Window");

} // Rapicorn
