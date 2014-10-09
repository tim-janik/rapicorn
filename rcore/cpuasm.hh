// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_CPUASM_HH__
#define __RAPICORN_CPUASM_HH__

#include <rcore/cxxaux.hh>

// == Memory Fences/Barriers ==
#define RAPICORN_CACHE_LINE_ALIGNMENT   128
#if defined __x86_64__ || defined __amd64__
#define RAPICORN_MFENCE __sync_synchronize()
#define RAPICORN_SFENCE __asm__ __volatile__ ("sfence" ::: "memory")
#define RAPICORN_LFENCE __asm__ __volatile__ ("lfence" ::: "memory")
#else // !x86/64
#define RAPICORN_MFENCE __sync_synchronize() ///< Memory Fence - prevent processor (and compiler) from reordering loads/stores (read/write barrier).
#define RAPICORN_SFENCE __sync_synchronize() ///< Store Fence - prevent processor (and compiler) from reordering stores (write barrier).
#define RAPICORN_LFENCE __sync_synchronize() ///< Load Fence - prevent processor (and compiler) from reordering loads (read barrier).
#endif
#define RAPICORN_CFENCE __asm__ __volatile__ ("" ::: "memory") ///< Compiler Fence, prevent compiler from reordering non-volatile loads/stores.

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
