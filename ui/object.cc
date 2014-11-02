// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#include "object.hh"

namespace Rapicorn {

ObjectImpl::ObjectImpl () :
  sig_changed (slot (*this, &ObjectImpl::do_changed))
{}

ObjectImpl::~ObjectImpl()
{}

void
ObjectImpl::dispose ()
{}

void
ObjectImpl::changed (const String &name)
{
  sig_changed.emit (name);
}

void
ObjectImpl::do_changed (const String &name)
{}

Aida::ImplicitBaseP
exception_safe_shared_from_this (Aida::ImplicitBase *iface, int mode)
{
  if (mode == 0)
    return iface->shared_from_this();
  // mode == 1
  try {
    return iface->shared_from_this();
  } catch(const std::bad_weak_ptr& e) {
    return NULL;
  }
}

} // Rapicorn
