// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "bindable.hh"
#include "thread.hh"
#include <unordered_map>

namespace Rapicorn {

// == BindableIface ==
typedef std::shared_ptr<BindableIface::BindableNotifySignal> BindableNotifySignalP;
static std::unordered_map<const BindableIface*, BindableNotifySignalP> bindable_signal_map;
static Spinlock                                                        bindable_signal_mutex;

BindableIface::~BindableIface ()
{
  ScopedLock<Spinlock> sig_map_locker (bindable_signal_mutex);
  auto it = bindable_signal_map.find (this);
  if (it != bindable_signal_map.end())
    bindable_signal_map.erase (it);
}

bool
BindableIface::bindable_match (const String &bpath, const String &name) const
{
  if (name == bpath)
    return true;
  if (name.size() < bpath.size() && bpath.data()[name.size()] == '.' &&
      bpath.compare (0, name.size(), name) == 0)
    return true;
  return false;
}

BindableIface::BindableNotifySignal::Connector
BindableIface::sig_bindable_notify () const
{
  ScopedLock<Spinlock> sig_map_locker (bindable_signal_mutex);
  BindableNotifySignalP &sp = bindable_signal_map[this];
  if (!sp)
    sp = std::make_shared<BindableNotifySignalP::element_type> ();
  return (*sp) ();
}

void
BindableIface::bindable_notify (const std::string &name) const
{
  ScopedLock<Spinlock> sig_map_locker (bindable_signal_mutex);
  BindableNotifySignalP sp = bindable_signal_map[this];
  if (sp)
    {
      sig_map_locker.unlock();
      sp->emit (name);
    }
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
      {
        BindableNotifySignalP sp = it->second;
        if (sp)
          (*sp) () -= notify_id_;
      }
  }
  if (adaptor_)
    delete adaptor_;
}

Any
BinadableAccessor::get_property (const String &name)
{
  Any any;
  bindable_.bindable_get (name, any);
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
