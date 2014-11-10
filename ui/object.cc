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

String
ObjectImpl::typeid_name ()
{
  return cxx_demangle (typeid (*this).name());
}

} // Rapicorn
