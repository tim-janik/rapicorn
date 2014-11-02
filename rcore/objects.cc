// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#include "objects.hh"
#include "thread.hh"
#include <typeinfo>
#include <cxxabi.h> // abi::__cxa_demangle

namespace Rapicorn {

/** Demangle a std::typeinfo.name() string into a proper C++ type name.
 * This function uses abi::__cxa_demangle() from <cxxabi.h> to demangle C++ type names,
 * which works for g++, libstdc++, clang++, libc++.
 */
String
cxx_demangle (const char *mangled_identifier)
{
  int status = 0;
  char *malloced_result = abi::__cxa_demangle (mangled_identifier, NULL, NULL, &status);
  String result = malloced_result && !status ? malloced_result : mangled_identifier;
  if (malloced_result)
    free (malloced_result);
  return result;
}

// == ReferenceCountable ==
static __attribute__ ((noinline)) size_t
stack_ptrdiff (const void *stackvariable, const void *data)
{
  size_t d = size_t (data);
  size_t s = size_t (stackvariable);
  return MAX (d, s) - MIN (d, s);
}

ReferenceCountable::ReferenceCountable (uint allow_stack_magic) :
  ref_field (FLOATING_FLAG + 1)
{
  if (allow_stack_magic != 0xbadbad)
    {
      // check for stack allocations
      const void *data = this;
      static const size_t stack_proximity_threshold = 4096; ///< page_size on most systems
      int stackvariable = 0;
      /* this check for ReferenceCountable instance allocations on the stack isn't
       * perfect, but should catch the most common cases for growing and shrinking stacks
       */
      if (stack_ptrdiff (&stackvariable, data) < stack_proximity_threshold)
        fatal ("ReferenceCountable object allocated on stack instead of heap: %u > %u (%p - %p)",
               stack_proximity_threshold, stack_ptrdiff (&stackvariable, data), data, &stackvariable);
    }
}

void
ReferenceCountable::ref_sink() const
{
  ref();
  uint32 old_ref = ref_get();
  uint32 new_ref = old_ref & ~FLOATING_FLAG;
  if (RAPICORN_UNLIKELY (old_ref != new_ref))
    {
      while (RAPICORN_UNLIKELY (!ref_cas (old_ref, new_ref)))
        {
          old_ref = ref_get();
          new_ref = old_ref & ~FLOATING_FLAG;
        }
      if (old_ref & FLOATING_FLAG)
        unref();
    }
}

void
ReferenceCountable::real_unref () const
{
  uint32 old_ref = ref_get();
  if (RAPICORN_UNLIKELY (1 == (old_ref & ~FLOATING_FLAG)))
    {
      ReferenceCountable *self = const_cast<ReferenceCountable*> (this);
      self->pre_finalize();
      old_ref = ref_get();
    }
  RAPICORN_ASSERT (old_ref & ~FLOATING_FLAG);         // old_ref > 1 ?
  while (RAPICORN_UNLIKELY (!ref_cas (old_ref, old_ref - 1)))
    {
      old_ref = ref_get();
      RAPICORN_ASSERT (old_ref & ~FLOATING_FLAG);     // catch underflow
    }
  if (RAPICORN_UNLIKELY (1 == (old_ref & ~FLOATING_FLAG)))
    {
      ReferenceCountable *self = const_cast<ReferenceCountable*> (this);
      self->finalize();
      self->delete_this();                            // usually: delete this;
    }
}

void
ReferenceCountable::ref_diag (const char *msg) const
{
  fprintf (stderr, "%s: this=%p ref_count=%d floating=%d", msg ? msg : "ReferenceCountable", this, ref_count(), floating());
}

void
ReferenceCountable::pre_finalize ()
{}

void
ReferenceCountable::finalize ()
{}

void
ReferenceCountable::delete_this ()
{
  delete this;
}

ReferenceCountable::~ReferenceCountable ()
{
  RAPICORN_ASSERT (ref_count() == 0);
}

} // Rapicorn
