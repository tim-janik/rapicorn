// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_CPUASM_HH__
#define __RAPICORN_CPUASM_HH__

#include <rcore/cxxaux.hh>

// == Memory Barriers ==
#define RAPICORN_CACHE_LINE_ALIGNMENT   128
#if defined __x86_64__ || defined __amd64__
#define RAPICORN_MFENCE __sync_synchronize()
#define RAPICORN_SFENCE __asm__ __volatile__ ("sfence" ::: "memory")
#define RAPICORN_LFENCE __asm__ __volatile__ ("lfence" ::: "memory")
#else // !x86/64
#define RAPICORN_SFENCE __sync_synchronize() ///< Store Fence - prevent processor (and compiler) from reordering stores (write barrier).
#define RAPICORN_LFENCE __sync_synchronize() ///< Load Fence - prevent processor (and compiler) from reordering loads (read barrier).
/// Memory Fence - prevent processor (and compiler) from reordering loads/stores (read/write barrier), see also atomic_thread_fence().
#define RAPICORN_MFENCE __sync_synchronize()
#endif
/// Compiler Fence, prevent compiler from reordering non-volatile loads/stores, see also atomic_signal_fence().
#define RAPICORN_CFENCE __asm__ __volatile__ ("" ::: "memory")

// == Atomic Memory Access ==
/// Atomically load and return the current value of @a *p;
template<typename T> T    atomic_load     (const T volatile *p)     { RAPICORN_CFENCE; T t = *p; RAPICORN_LFENCE; return t; }
/// Atomically store @a v into @a *p;
template<typename T> void atomic_store    (T volatile *p, T v)      { RAPICORN_SFENCE; *p = v;  RAPICORN_CFENCE; }
/// Atomically load the current value of @a *p and add @a v. Returns the old value from before the addition.
template<typename T> T    atomic_fetch_add (T volatile *p, T v)     { return __sync_fetch_and_add (p, v); }
/// Atomically store @a n in @a *p if it equals @a o and return true, otherwise don't store and return false.
template<typename T> bool atomic_bool_cas (T volatile *p, T o, T n) { return __sync_bool_compare_and_swap (p, o, n); }
/// Atomically store @a n in @a *p if it equals @a o, return the value of @a *p from before the operation.
template<typename T> T    atomic_val_cas  (T volatile *p, T o, T n) { return __sync_val_compare_and_swap (p, o, n); }
/// Atomically store @a n in @a *p and return the value of @a *p from before the operation.
template<typename T> T    atomic_swap     (T volatile *p, T n)
{
  T o;
  do
    o = atomic_load (p);
  while (!__sync_bool_compare_and_swap (p, o, n));
  return o;
}
/// Atomically store @a node in @a *headp and update @a *nextp to point to the previous value of @a *headp.
template<typename T> void atomic_push_link (T*volatile *headp, T*volatile *nextp, T *node)
{
  do
    atomic_store (nextp, atomic_load (headp));
  while (!atomic_bool_cas (headp, *nextp, node));
}


// == RDTSC ==
#if defined __i386__ || defined __x86_64__ || defined __amd64__
#define RAPICORN_HAVE_X86_RDTSC  1
#define RAPICORN_X86_RDTSC()     ({ Rapicorn::uint32 __l_, __h_, __s_; \
                                    __asm__ __volatile__ ("rdtsc" : "=a" (__l_), "=d" (__h_));  \
                                    __s_ = __l_ + (Rapicorn::uint64 (__h_) << 32); __s_; })
#else
#define RAPICORN_HAVE_X86_RDTSC  0
#define RAPICORN_X86_RDTSC()    (0)
#endif

#endif // __RAPICORN_CPUASM_HH__
