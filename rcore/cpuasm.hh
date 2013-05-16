// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_CPUASM_HH__
#define __RAPICORN_CPUASM_HH__

#include <rcore/cxxaux.hh>

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
