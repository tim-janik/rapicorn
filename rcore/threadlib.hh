// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_THREADLIB_HH__
#define __RAPICORN_THREADLIB_HH__

#include <condition_variable>

namespace Rapicorn {
namespace Lib {

#define RAPICORN_CACHE_LINE_ALIGNMENT   128
#define RAPICORN_MFENCE    __sync_synchronize() ///< Memory Fence - prevent processor (and compiler) from reordering loads/stores (read/write barrier).
#define RAPICORN_SFENCE    RAPICORN_X86SFENCE   ///< Store Fence - prevent processor (and compiler) from reordering stores (read barrier).
#define RAPICORN_LFENCE    RAPICORN_X86LFENCE   ///< Load Fence - prevent processor (and compiler) from reordering loads (read barrier).
#define RAPICORN_CFENCE    __asm__ __volatile__ ("" ::: "memory") ///< Compiler Fence, prevent compiler from reordering non-volatiles loads/stores.
#define RAPICORN_X86LFENCE __asm__ __volatile__ ("lfence" ::: "memory") ///< X86 lfence - prevent processor from reordering loads (read barrier).
#define RAPICORN_X86SFENCE __asm__ __volatile__ ("sfence" ::: "memory") ///< X86 sfence - prevent processor from reordering stores (write barrier).

template<typename T> T    atomic_load  (T volatile *p)      { RAPICORN_CFENCE; T t = *p; RAPICORN_LFENCE; return t; }
template<typename T> void atomic_store (T volatile *p, T i) { RAPICORN_SFENCE; *p = i;  RAPICORN_CFENCE; }

template<typename T>
class Atomic {
  T volatile v;
  /*ctor*/  Atomic    () = delete;
protected:
  constexpr Atomic    (T i) : v (i) {}
  Atomic<T>& operator=(Atomic<T> &o) { store (o.load()); return *this; }
  Atomic<T> volatile& operator=(Atomic<T> &o) volatile { store (o.load()); return *this; }
public:
  T         load      () const volatile { return atomic_load (&v); }
  void      store     (T i)    volatile { atomic_store (&v, i); }
  bool      cas  (T o, T n)    volatile { return __sync_bool_compare_and_swap (&v, o, n); }
  T         operator+=(T i)    volatile { return __sync_add_and_fetch (&v, i); }
  T         operator-=(T i)    volatile { return __sync_sub_and_fetch (&v, i); }
  T         operator&=(T i)    volatile { return __sync_and_and_fetch (&v, i); }
  T         operator^=(T i)    volatile { return __sync_xor_and_fetch (&v, i); }
  T         operator|=(T i)    volatile { return __sync_or_and_fetch  (&v, i); }
  T         operator++()       volatile { return __sync_add_and_fetch (&v, 1); }
  T         operator++(int)    volatile { return __sync_fetch_and_add (&v, 1); }
  T         operator--()       volatile { return __sync_sub_and_fetch (&v, 1); }
  T         operator--(int)    volatile { return __sync_fetch_and_sub (&v, 1); }
  void      operator= (T i)    volatile { store (i); }
  operator  T         () const volatile { return load(); }
};

#define RAPICORN_NEW_ONCE(pointer_variable)       \
  ({ typeof (pointer_variable) *___vp = &pointer_variable;      \
    while (once_enter (___vp)) { once_leave (___vp, new typeof (**___vp)); } *___vp; })

void once_list_enter  ();
bool once_list_bounce (volatile void *ptr);
bool once_list_leave  (volatile void *ptr);

template<class Value> inline bool
once_enter (volatile Value *value_location)
{
  if (RAPICORN_LIKELY (atomic_load (value_location) != 0))
    return false;
  else
    {
      once_list_enter();
      const bool initialized = atomic_load (value_location) != 0;
      const bool needs_init = once_list_bounce (initialized ? NULL : value_location);
      return needs_init;
    }
}

template<class Value> inline void
once_leave (volatile Value *value_location,
            Value           initialization_value)
{
  RAPICORN_RETURN_UNLESS (atomic_load (value_location) == 0);
  RAPICORN_RETURN_UNLESS (initialization_value != 0);

  atomic_store (value_location, initialization_value);
  const bool found_and_removed = once_list_leave (value_location);
  RAPICORN_RETURN_UNLESS (found_and_removed == true);
}

} // Lib
} // Rapicorn


#endif // __RAPICORN_THREADLIB_HH__
