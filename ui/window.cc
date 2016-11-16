// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "window.hh"
#include "factory.hh"
#include "application.hh"
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


WindowImpl::WindowImpl ()
{}

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
}

WindowImpl::~WindowImpl ()
{
  WindowTrail::wleave (this);
  assert_return (anchored() == false);
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

static const WidgetFactory<WindowImpl> window_factory ("Rapicorn::Window");

} // Rapicorn
