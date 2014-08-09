// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "object.hh"

namespace Rapicorn {

ObjectImpl::ObjectImpl () :
  sig_changed (slot (*this, &ObjectImpl::do_changed))
{}

ObjectImpl::~ObjectImpl()
{}

void
ObjectImpl::changed (const String &name)
{
  if (!finalizing())
    sig_changed.emit (name);
}

void
ObjectImpl::do_changed (const String &name)
{}

} // Rapicorn
