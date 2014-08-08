// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "bindable.hh"
#include "thread.hh"
#include <unordered_map>

namespace Rapicorn {

// == BindablePath ==
bool
BindablePath::match (const std::string &name) const
{
  if (path.empty())
    {
      BindablePath &mpath = const_cast<BindablePath&> (*this);
      mpath.plist.push_back (name);
      return false;
    }
  if (name == path)
    return true;
  if (name.size() < path.size() && path.data()[name.size()] == '.' &&
      path.compare (0, name.size(), name) == 0)
    return true;
  return false;
}

// == BindableIface ==
static std::unordered_map<const BindableIface*, BindableIface::BindableNotifySignal> bindable_signal_map;
static Spinlock                                                                      bindable_signal_mutex;

BindableIface::~BindableIface ()
{
  ScopedLock<Spinlock> sig_map_locker (bindable_signal_mutex);
  auto it = bindable_signal_map.find (this);
  if (it != bindable_signal_map.end())
    bindable_signal_map.erase (it);
}

BindableIface::BindableNotifySignal::Connector
BindableIface::sig_bindable_notify () const
{
  ScopedLock<Spinlock> sig_map_locker (bindable_signal_mutex);
  return bindable_signal_map[this]();
}

void
BindableIface::bindable_notify (const std::string &name) const
{
  ScopedLock<Spinlock> sig_map_locker (bindable_signal_mutex);
  BindableNotifySignal &sig = bindable_signal_map[this];
  sig_map_locker.unlock();
  sig.emit (name);
}

// == BinadableAccessor ==
void
BinadableAccessor::ctor()
{
  notify_id_ = bindable_.sig_bindable_notify() += slot (this, &BinadableAccessor::notify_property);
}

BinadableAccessor::~BinadableAccessor ()
{
  /* depending on how users manage memory, bindable_ *might* have been destroyed already.
   * just in case, we're *not* using bindable_.sig_bindable_notify() but instead go through
   * disconnecting from bindable_signal_map directly.
   */
  {
    ScopedLock<Spinlock> sig_map_locker (bindable_signal_mutex);
    auto it = bindable_signal_map.find (&bindable_); // using bindable_ pointer but not the object
    if (it != bindable_signal_map.end())
      it->second() -= notify_id_;
  }
  if (adaptor_)
    delete adaptor_;
}

BinadableAccessor::StringList
BinadableAccessor::list_propertis ()
{
  BindablePath bpath;
  Any dummy;
  bindable_.bindable_get (bpath, dummy);
  return bpath.plist;
}

Any
BinadableAccessor::get_property (const String &name)
{
  BindablePath bpath;
  const_cast<String&> (bpath.path) = name;
  Any any;
  bindable_.bindable_get (bpath, any);
  return any;
}

void
BinadableAccessor::set_property (const String &name, const Any &any)
{
  fatal ("FIXME: unimplemented");
}

void
BinadableAccessor::notify_property (const String &name)
{
  fatal ("FIXME: unimplemented");
}

} // Rapicorn
