// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "object.hh"

namespace Rapicorn {

static __thread FactoryContext *call_stack_ctor_factory_context = NULL;

void
ObjectImpl::ctor_factory_context (FactoryContext *fc)
{
  if (fc)
    assert_return (call_stack_ctor_factory_context == NULL);
  else
    assert_return (call_stack_ctor_factory_context != NULL);
  call_stack_ctor_factory_context = fc;
}

// Provides factory_context from call stack context during object construction only
FactoryContext&
ObjectImpl::ctor_factory_context ()
{
  assert_return (call_stack_ctor_factory_context != NULL, *(FactoryContext*) NULL);
  return *call_stack_ctor_factory_context;
}

ObjectImpl::ObjectImpl () :
  sig_changed (slot (*this, &ObjectImpl::do_changed))
{}

ObjectImpl::~ObjectImpl()
{}

void
ObjectImpl::construct ()
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
