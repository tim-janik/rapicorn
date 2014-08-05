// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "binding.hh"

namespace Rapicorn {

Binding::Binding (ObjectIface &instance, const String &instance_property, const String &binding_path) :
  instance_ (instance), instance_property_ (instance_property), binding_path_ (binding_path)
{}

Binding::~Binding ()
{}

void
Binding::bind_context (ObjectIfaceP binding_context)
{
  assert_return (binding_context_ == NULL);
  binding_context_ = binding_context;
}

void
Binding::reset()
{
  binding_context_.reset();
}

} // Rapicorn
