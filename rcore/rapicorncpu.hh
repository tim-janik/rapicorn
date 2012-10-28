// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_CPU_HH__
#define __RAPICORN_CPU_HH__

#include <rcore/utilities.hh>

namespace Rapicorn {

struct CPUInfo {
  // architecture name
  const char *machine;
  // CPU Vendor ID
  const char *cpu_vendor;
  // CPU features on X86
  uint x86_fpu : 1, x86_ssesys : 1, x86_tsc   : 1, x86_htt      : 1;
  uint x86_mmx : 1, x86_mmxext : 1, x86_3dnow : 1, x86_3dnowext : 1;
  uint x86_sse : 1, x86_sse2   : 1, x86_sse3  : 1, x86_ssse3    : 1;
  uint x86_cx16 : 1, x86_sse4_1 : 1, x86_sse4_2 : 1;
};

CPUInfo cpu_info	(void);
String  cpu_info_string	(const CPUInfo &cpu_info);

} // Rapicorn

#endif /* __RAPICORN_CPU_HH__ */
