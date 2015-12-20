// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "platform.hh"
#include "main.hh"
#include "strings.hh"
#include "thread.hh"
#include "randomhash.hh"
#include <random>
#include <setjmp.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <glob.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/resource.h>
#include <linux/random.h>       // GRND_NONBLOCK
#include <sys/syscall.h>        // __NR_getrandom
#if defined (__i386__) || defined (__x86_64__)
#  include <x86intrin.h>        // __rdtsc
#endif

namespace Rapicorn {

// == CPUInfo ==
/// Acquire information about the runtime architecture and CPU type.
struct CPUInfo {
  // architecture name
  const char *machine;
  // CPU Vendor ID
  char cpu_vendor[13];
  // CPU features on X86
  uint x86_fpu : 1, x86_ssesys : 1, x86_tsc   : 1, x86_htt      : 1;
  uint x86_mmx : 1, x86_mmxext : 1, x86_3dnow : 1, x86_3dnowext : 1;
  uint x86_sse : 1, x86_sse2   : 1, x86_sse3  : 1, x86_ssse3    : 1;
  uint x86_cx16 : 1, x86_sse4_1 : 1, x86_sse4_2 : 1, x86_rdrand : 1;
};

/* figure architecture name from compiler */
static const char*
get_arch_name (void)
{
#if     defined  __alpha__
  return "Alpha";
#elif   defined __frv__
  return "frv";
#elif   defined __s390__
  return "s390";
#elif   defined __m32c__
  return "m32c";
#elif   defined sparc
  return "Sparc";
#elif   defined __m32r__
  return "m32r";
#elif   defined __x86_64__ || defined __amd64__
  return "AMD64";
#elif   defined __ia64__
  return "Intel Itanium";
#elif   defined __m68k__
  return "mc68000";
#elif   defined __powerpc__ || defined PPC || defined powerpc || defined __PPC__
  return "PPC";
#elif   defined __arc__
  return "arc";
#elif   defined __arm__
  return "Arm";
#elif   defined __mips__ || defined mips
  return "Mips";
#elif   defined __tune_i686__ || defined __i686__
  return "i686";
#elif   defined __tune_i586__ || defined __i586__
  return "i586";
#elif   defined __tune_i486__ || defined __i486__
  return "i486";
#elif   defined i386 || defined __i386__
  return "i386";
#else
  return "unknown-arch";
#warning platform.cc needs updating for this processor type
#endif
}

/* --- X86 detection via CPUID --- */
#if     defined __i386__
#  define x86_has_cpuid()       ({                              \
  unsigned int __eax, __ecx;                                    \
  __asm__ __volatile__                                          \
    (                                                           \
     /* copy EFLAGS into eax and ecx */                         \
     "pushf ; pop %0 ; mov %0, %1 \n\t"                         \
     /* toggle the ID bit and store back to EFLAGS */           \
     "xor $0x200000, %0 ; push %0 ; popf \n\t"                  \
     /* read back EFLAGS with possibly modified ID bit */       \
     "pushf ; pop %0 \n\t"                                      \
     : "=a" (__eax), "=c" (__ecx)                               \
     : /* no inputs */                                          \
     : "cc"                                                     \
     );                                                         \
  bool __result = (__eax ^ __ecx) & 0x00200000;                 \
  __result;                                                     \
})
/* save EBX around CPUID, because gcc doesn't like it to be clobbered with -fPIC */
#  define x86_cpuid(input, eax, ebx, ecx, edx)  \
  __asm__ __volatile__ (                        \
    /* save ebx in esi */                       \
    "mov %%ebx, %%esi \n\t"                     \
    /* get CPUID with eax=input */              \
    "cpuid \n\t"                                \
    /* swap ebx and esi */                      \
    "xchg %%ebx, %%esi"                         \
    : "=a" (eax), "=S" (ebx),                   \
      "=c" (ecx), "=d" (edx)                    \
    : "0" (input)                               \
    : "cc")
#elif   defined __x86_64__ || defined __amd64__
/* CPUID is always present on AMD64, see:
 * http://www.amd.com/us-en/assets/content_type/white_papers_and_tech_docs/24594.pdf
 * "AMD64 Architecture Programmer's Manual Volume 3",
 * "Appendix D: Instruction Subsets and CPUID Feature Sets"
 */
#  define x86_has_cpuid()                       (1)
/* save EBX around CPUID, because gcc doesn't like it to be clobbered with -fPIC */
#  define x86_cpuid(input, eax, ebx, ecx, edx)  \
  __asm__ __volatile__ (                        \
    /* save ebx in esi */                       \
    "mov %%rbx, %%rsi \n\t"                     \
    /* get CPUID with eax=input */              \
    "cpuid \n\t"                                \
    /* swap ebx and esi */                      \
    "xchg %%rbx, %%rsi"                         \
    : "=a" (eax), "=S" (ebx),                   \
      "=c" (ecx), "=d" (edx)                    \
    : "0" (input)                               \
    : "cc")
#else
#  define x86_has_cpuid()                       (false)
#  define x86_cpuid(input, eax, ebx, ecx, edx)  do {} while (0)
#endif


static jmp_buf cpu_info_jmp_buf;

static void RAPICORN_NORETURN
cpu_info_sigill_handler (int dummy)
{
  longjmp (cpu_info_jmp_buf, 1);
}

static bool
get_x86_cpu_features (CPUInfo *ci)
{
  memset (ci, 0, sizeof (*ci));
  /* check if the CPUID instruction is supported */
  if (!x86_has_cpuid ())
    return false;

  /* query intel CPUID range */
  unsigned int eax, ebx, ecx, edx;
  x86_cpuid (0, eax, ebx, ecx, edx);
  unsigned int v_ebx = ebx, v_ecx = ecx, v_edx = edx;
  char *vendor = ci->cpu_vendor;
  *((unsigned int*) &vendor[0]) = ebx;
  *((unsigned int*) &vendor[4]) = edx;
  *((unsigned int*) &vendor[8]) = ecx;
  vendor[12] = 0;
  if (eax >= 1)                 /* may query version and feature information */
    {
      x86_cpuid (1, eax, ebx, ecx, edx);
      if (ecx & (1 << 0))
        ci->x86_sse3 = true;
      if (ecx & (1 << 9))
        ci->x86_ssse3 = true;
      if (ecx & (1 << 13))
        ci->x86_cx16 = true;
      if (ecx & (1 << 19))
        ci->x86_sse4_1 = true;
      if (ecx & (1 << 20))
        ci->x86_sse4_2 = true;
      if (ecx & (1 << 30))
        ci->x86_rdrand = true;
      if (edx & (1 << 0))
        ci->x86_fpu = true;
      if (edx & (1 << 4))
        ci->x86_tsc = true;
      if (edx & (1 << 23))
        ci->x86_mmx = true;
      if (edx & (1 << 25))
        {
          ci->x86_sse = true;
          ci->x86_mmxext = true;
        }
      if (edx & (1 << 26))
        ci->x86_sse2 = true;
      if (edx & (1 << 28))
        ci->x86_htt = true;
      /* http://www.intel.com/content/www/us/en/processors/processor-identification-cpuid-instruction-note.html
       * "Intel Processor Identification and the CPUID Instruction"
       */
    }

  /* query extended CPUID range */
  x86_cpuid (0x80000000, eax, ebx, ecx, edx);
  if (eax >= 0x80000001 &&      /* may query extended feature information */
      v_ebx == 0x68747541 &&    /* AuthenticAMD */
      v_ecx == 0x444d4163 && v_edx == 0x69746e65)
    {
      x86_cpuid (0x80000001, eax, ebx, ecx, edx);
      if (edx & (1 << 31))
        ci->x86_3dnow = true;
      if (edx & (1 << 22))
        ci->x86_mmxext = true;
      if (edx & (1 << 30))
        ci->x86_3dnowext = true;
      /* www.amd.com/us-en/assets/content_type/white_papers_and_tech_docs/25481.pdf
       * "AMD CPUID Specification"
       */
    }

  /* check system support for SSE */
  if (ci->x86_sse)
    {
      struct sigaction action, old_action;
      action.sa_handler = cpu_info_sigill_handler;
      sigemptyset (&action.sa_mask);
      action.sa_flags = SA_NOMASK;
      sigaction (SIGILL, &action, &old_action);
      if (setjmp (cpu_info_jmp_buf) == 0)
        {
#if     defined __i386__ || defined __x86_64__ || defined __amd64__
          unsigned int mxcsr;
          __asm__ __volatile__ ("stmxcsr %0 ; sfence ; emms" : "=m" (mxcsr));
          /* executed SIMD instructions without exception */
          ci->x86_ssesys = true;
#endif // x86
        }
      else
        {
          /* signal handler jumped here */
          // g_printerr ("caught SIGILL\n");
        }
      sigaction (SIGILL, &old_action, NULL);
    }

  return true;
}

/** Retrieve string identifying the runtime CPU type.
 * The returned string contains: number of online CPUs, a string
 * describing the CPU architecture, the vendor and finally
 * a number of flag words describing CPU features plus a trailing space.
 * This allows checks for CPU features via a simple string search for
 * " FEATURE ".
 * @return Example: "4 AMD64 GenuineIntel FPU TSC HTT CMPXCHG16B MMX MMXEXT SSESYS SSE SSE2 SSE3 SSSE3 SSE4.1 SSE4.2 "
 */
String
cpu_info()
{
  static String cpu_info_string = []() {
    CPUInfo cpu_info = { 0, };
    if (!get_x86_cpu_features (&cpu_info))
      strcat (cpu_info.cpu_vendor, "Unknown");
    cpu_info.machine = get_arch_name();
    String info;
    // cores
    info += string_format ("%d", sysconf (_SC_NPROCESSORS_ONLN));
    // architecture
    info += String (" ") + cpu_info.machine;
    // vendor
    info += String (" ") + cpu_info.cpu_vendor;
    // processor flags
    if (cpu_info.x86_fpu)
      info += " FPU";
    if (cpu_info.x86_tsc)
      info += " TSC";
    if (cpu_info.x86_htt)
      info += " HTT";
    if (cpu_info.x86_cx16)
      info += " CMPXCHG16B";
    // MMX flags
    if (cpu_info.x86_mmx)
      info += " MMX";
    if (cpu_info.x86_mmxext)
      info += " MMXEXT";
    // SSE flags
    if (cpu_info.x86_ssesys)
      info += " SSESYS";
    if (cpu_info.x86_sse)
      info += " SSE";
    if (cpu_info.x86_sse2)
      info += " SSE2";
    if (cpu_info.x86_sse3)
      info += " SSE3";
    if (cpu_info.x86_ssse3)
      info += " SSSE3";
    if (cpu_info.x86_sse4_1)
      info += " SSE4.1";
    if (cpu_info.x86_sse4_2)
      info += " SSE4.2";
    if (cpu_info.x86_rdrand)
      info += " rdrand";
    // 3DNOW flags
    if (cpu_info.x86_3dnow)
      info += " 3DNOW";
    if (cpu_info.x86_3dnowext)
      info += " 3DNOWEXT";
    info += " ";
    return String (info.c_str());
  }();
  return cpu_info_string;
}

// == TaskStatus ==
TaskStatus::TaskStatus (int pid, int tid) :
  process_id (pid), task_id (tid >= 0 ? tid : pid), state (UNKNOWN), processor (-1), priority (0),
  utime (0), stime (0), cutime (0), cstime (0),
  ac_stamp (0), ac_utime (0), ac_stime (0), ac_cutime (0), ac_cstime (0)
{}

static bool
update_task_status (TaskStatus &self)
{
  static long clk_tck = 0;
  if (!clk_tck)
    {
      clk_tck = sysconf (_SC_CLK_TCK);
      if (clk_tck <= 0)
        clk_tck = 100;
    }
  int pid = -1, ppid = -1, pgrp = -1, session = -1, tty_nr = -1, tpgid = -1;
  int exit_signal = 0, processor = 0;
  long cutime = 0, cstime = 0, priority = 0, nice = 0, dummyld = 0;
  long itrealvalue = 0, rss = 0;
  unsigned long flags = 0, minflt = 0, cminflt = 0, majflt = 0, cmajflt = 0;
  unsigned long utime = 0, stime = 0, vsize = 0, rlim = 0, startcode = 0;
  unsigned long endcode = 0, startstack = 0, kstkesp = 0, kstkeip = 0;
  unsigned long signal = 0, blocked = 0, sigignore = 0, sigcatch = 0;
  unsigned long wchan = 0, nswap = 0, cnswap = 0, rt_priority = 0, policy = 0;
  unsigned long long starttime = 0;
  char state = 0, command[8192 + 1] = { 0 };
  String filename = string_format ("/proc/%u/task/%u/stat", self.process_id, self.task_id);
  FILE *file = fopen (filename.c_str(), "r");
  if (!file)
    return false;
  int n = fscanf (file,
                  "%d %8192s %c "
                  "%d %d %d %d %d "
                  "%lu %lu %lu %lu %lu %lu %lu "
                  "%ld %ld %ld %ld %ld %ld "
                  "%llu %lu %ld "
                  "%lu %lu %lu %lu %lu "
                  "%lu %lu %lu %lu %lu "
                  "%lu %lu %lu %d %d "
                  "%lu %lu",
                  &pid, command, &state, // n=3
                  &ppid, &pgrp, &session, &tty_nr, &tpgid, // n=8
                  &flags, &minflt, &cminflt, &majflt, &cmajflt, &utime, &stime, // n=15
                  &cutime, &cstime, &priority, &nice, &dummyld, &itrealvalue, // n=21
                  &starttime, &vsize, &rss, // n=24
                  &rlim, &startcode, &endcode, &startstack, &kstkesp, // n=29
                  &kstkeip, &signal, &blocked, &sigignore, &sigcatch, // n=34
                  &wchan, &nswap, &cnswap, &exit_signal, &processor, // n=39
                  &rt_priority, &policy // n=41
                  );
  fclose (file);
  const double jiffies_to_usecs = 1000000.0 / clk_tck;
  if (n >= 3)
    self.state = TaskStatus::State (state);
  if (n >= 15)
    {
      self.ac_utime = utime * jiffies_to_usecs;
      self.ac_stime = stime * jiffies_to_usecs;
    }
  if (n >= 17)
    {
      self.ac_cutime = cutime * jiffies_to_usecs;
      self.ac_cstime = cstime * jiffies_to_usecs;
    }
  if (n >= 18)
    self.priority = priority;
  if (n >= 39)
    self.processor = 1 + processor;
  return true;
}

#define ACCOUNTING_MSECS        50

bool
TaskStatus::update ()
{
  const TaskStatus old (*this);
  const uint64 now = timestamp_realtime();              // usecs
  if (ac_stamp + ACCOUNTING_MSECS * 1000 >= now)
    return false;                                       // limit accounting to a few times per second
  if (!update_task_status (*this))
    return false;
  const double delta = 1000000.0 / MAX (1, now - ac_stamp);
  utime = uint64 (MAX (ac_utime - old.ac_utime, 0) * delta);
  stime = uint64 (MAX (ac_stime - old.ac_stime, 0) * delta);
  cutime = uint64 (MAX (ac_cutime - old.ac_cutime, 0) * delta);
  cstime = uint64 (MAX (ac_cstime - old.ac_cstime, 0) * delta);
  ac_stamp = now;
  return true;
}

String
TaskStatus::string ()
{
  return
    string_format ("pid=%d task=%d state=%c processor=%d priority=%d perc=%.2f%% utime=%.3fms stime=%.3fms cutime=%.3f cstime=%.3f",
                   process_id, task_id, state, processor, priority, (utime + stime) * 0.0001,
                   utime * 0.001, stime * 0.001, cutime * 0.001, cstime * 0.001);
}

// == Entropy ==
static Mutex      entropy_mutex;
static KeccakRng *entropy_global_pool = NULL;
static uint64     entropy_mix_simple = 0;

/** @class Entropy
 * To provide good quality random numbers, this class gathers entropy from a variety of sources.
 * Under Linux, this includes the runtime environment and (if present) devices, interrupts,
 * disk + network statistics, system load, execution + pipelining + scheduling latencies and of
 * course random number devices. In combination with well established techniques like
 * syscall timings (see Entropics13 @cite Entropics13) and a SHA3 algorithm derived random number
 * generator (KeccakRng) for the mixing, the entropy collection is designed to be good enough
 * to use as seeds for new PRNGs and to securely generate cryptographic tokens like session keys.
 */

KeccakRng&
Entropy::entropy_pool()
{
  if (RAPICORN_LIKELY (entropy_global_pool))
    return *entropy_global_pool;
  assert (entropy_mutex.try_lock() == false); // pool *must* be locked by caller
  // create pool and seed it with system details
  std::seed_seq seq { 1 };
  KeccakRng *kpool = new KeccakCryptoRng (seq); // prevent auto-seeding
  system_entropy (*kpool);
  // gather entropy from runtime information and mix into pool
  KeccakCryptoRng keccak (seq);
  runtime_entropy (keccak);
  uint64_t seed_data[25];
  keccak.generate (&seed_data[0], &seed_data[25]);
  kpool->seed (seed_data, 25);
  // establish global pool
  assert (entropy_global_pool == NULL);
  entropy_global_pool = kpool;
  return *entropy_global_pool;
}

void
Entropy::slow_reseed ()
{
  // gather and mangle entropy data
  std::seed_seq seq { 1 };
  KeccakCryptoRng keccak (seq); // prevent auto-seeding
  runtime_entropy (keccak);
  // mix entropy into global pool
  uint64_t seed_data[25];
  keccak.generate (&seed_data[0], &seed_data[25]);
  ScopedLock<Mutex> locker (entropy_mutex);
  entropy_pool().xor_seed (seed_data, 25);
}

static constexpr uint64_t
bytehash_fnv64a (const uint8_t *bytes, size_t n, uint64_t hash = 0xcbf29ce484222325)
{
  return n == 0 ? hash : bytehash_fnv64a (bytes + 1, n - 1, 0x100000001b3 * (hash ^ bytes[0]));
}

static uint64_t
stringhash_fnv64a (const String &string)
{
  return bytehash_fnv64a ((const uint8*) string.data(), string.size());
}

void
Entropy::add_data (const void *bytes, size_t n_bytes)
{
  const uint64_t bits = bytehash_fnv64a ((const uint8_t*) bytes, n_bytes);
  ScopedLock<Mutex> locker (entropy_mutex);
  entropy_mix_simple ^= bits + (entropy_mix_simple * 1664525);
}

uint64_t
Entropy::get_seed ()
{
  ScopedLock<Mutex> locker (entropy_mutex);
  KeccakRng &pool = entropy_pool();
  if (entropy_mix_simple)
    {
      pool.xor_seed (&entropy_mix_simple, 1);
      entropy_mix_simple = 0;
    }
  return pool();
}

static int
getrandom (void *buffer, size_t count, unsigned flags)
{
#ifdef __NR_getrandom
  const long ret = syscall (__NR_getrandom, buffer, count, flags);
  if (ret > 0)
    return ret;
#endif
  FILE *file = fopen ("/dev/urandom", "r");     // fallback
  if (file)
    {
      const size_t l = fread (buffer, 1, count, file);
      fclose (file);
      if (l > 0)
        return 0;
    }
  errno = ENOSYS;
  return 0;
}

static bool
hash_getrandom (KeccakRng &pool)
{
  uint64_t buffer[25];
  int flags = 0;
#ifdef GRND_NONBLOCK
  flags |= GRND_NONBLOCK;
#endif
  int l = getrandom (buffer, sizeof (buffer), flags);
  if (l > 0)
    {
      pool.xor_seed (buffer, l / sizeof (buffer[0]));
      return true;
    }
  return false;
}

template<class Data> static void
hash_anything (KeccakRng &pool, const Data &data)
{
  const uint64_t *d64 = (const uint64_t*) &data;
  uint len = sizeof (data);
  uint64_t dummy;
  if (sizeof (Data) < sizeof (uint64_t))
    {
      dummy = 0;
      memcpy (&dummy, &data, sizeof (Data));
      d64 = &dummy;
      len = 1;
    }
  pool.xor_seed (d64, len / sizeof (d64[0]));
}

static bool
hash_macs (KeccakRng &pool)
{
  // query devices for the AF_INET family which might be the only one supported
  int sockfd = socket (AF_INET, SOCK_DGRAM, 0);         // open IPv4 UDP socket
  if (sockfd < 0)
    return false;
  // discover devices by index, might include devices that are 'down'
  String devices;
  int ret = 0;
  for (size_t j = 0; j <= 1 || ret >= 0; j++)           // try [0] and [1]
    {
      struct ifreq iftmp = { 0, };
      iftmp.ifr_ifindex = j;
      ret = ioctl (sockfd, SIOCGIFNAME, &iftmp);
      if (ret < 0)
        continue;
      if (!devices.empty())
        devices += ",";
      devices += iftmp.ifr_name;                        // found name
      devices += "//";                                  // no inet address
      if (ioctl (sockfd, SIOCGIFHWADDR, &iftmp) >= 0)   // query MAC
        {
          const uint8_t *mac = (const uint8_t*) iftmp.ifr_hwaddr.sa_data;
          devices += string_format ("%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        }
    }
  // discover devices that are 'up'
  char ifcbuffer[8192] = { 0, };                        // buffer space for returning interfaces
  struct ifconf ifc;
  ifc.ifc_len = sizeof (ifcbuffer);
  ifc.ifc_buf = ifcbuffer;
  ret = ioctl (sockfd, SIOCGIFCONF, &ifc);
  for (size_t i = 0; ret >= 0 && i < ifc.ifc_len / sizeof (struct ifreq); i++)
    {
      const struct ifreq *iface = &ifc.ifc_req[i];
      if (!devices.empty())
        devices += ",";
      devices += iface->ifr_name;                       // found name
      devices += "/";
      const uint8_t *addr = (const uint8_t*) &((struct sockaddr_in*) &iface->ifr_addr)->sin_addr;
      devices += string_format ("%u.%u.%u.%u", addr[0], addr[1], addr[2], addr[3]);
      devices += "/";                                   // added inet address
      if (ioctl (sockfd, SIOCGIFHWADDR, iface) >= 0)    // query MAC
        {
          const uint8_t *mac = (const uint8_t*) iface->ifr_hwaddr.sa_data;
          devices += string_format ("%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        }
    }
  close (sockfd);
  devices.resize (((devices.size() + 7) / 8) * 8); // align to uint64_t
  pool.xor_seed ((const uint64_t*) devices.data(), devices.size() / 8);
  // printout ("SEED(MACs): %s\n", devices.c_str());
  return !devices.empty();
}

static bool
hash_stat (KeccakRng &pool, const char *filename)
{
  struct {
    struct stat stat;
    uint64_t padding;
  } s = { 0, };
  if (stat (filename, &s.stat) == 0)
    {
      pool.xor_seed ((const uint64_t*) &s.stat, sizeof (s.stat) / sizeof (uint64_t));
      // printout ("SEED(%s): atime=%u mtime=%u ctime=%u size=%u...\n", filename, s.stat.st_atime, s.stat.st_atime, s.stat.st_atime, s.stat.st_size);
      return true;
    }
  return false;
}

static bool
hash_file (KeccakRng &pool, const char *filename, const size_t maxbytes = 16384)
{
  FILE *file = fopen (filename, "r");
  if (file)
    {
      uint64_t buffer[maxbytes / 8];
      const size_t l = fread (buffer, sizeof (buffer[0]), sizeof (buffer) / sizeof (buffer[0]), file);
      fclose (file);
      if (l > 0)
        {
          pool.xor_seed (buffer, l);
          // printout ("SEED(%s): %s\n", filename, String ((const char*) buffer, std::min (l * 8, size_t (48))));
          return true;
        }
    }
  return false;
}

static bool __attribute__ ((__unused__))
hash_glob (KeccakRng &pool, const char *fileglob, const size_t maxbytes = 16384)
{
  glob_t globbuf = { 0, };
  glob (fileglob, GLOB_NOSORT, NULL, &globbuf);
  bool success = false;
  for (size_t i = globbuf.gl_offs; i < globbuf.gl_pathc; i++)
    success |= hash_file (pool, globbuf.gl_pathv[i], maxbytes);
  globfree (&globbuf);
  return success;
}

struct HashStamp {
  uint64_t bstamp;
#ifdef  CLOCK_PROCESS_CPUTIME_ID
  uint64_t tcpu;
#endif
#if     defined (__i386__) || defined (__x86_64__)
  uint64_t tsc;
#endif
};

static void __attribute__ ((__noinline__))
hash_time (HashStamp *hstamp)
{
  asm (""); // enfore __noinline__
  hstamp->bstamp = timestamp_benchmark();
#ifdef  CLOCK_PROCESS_CPUTIME_ID
  {
    struct timespec ts;
    clock_gettime (CLOCK_PROCESS_CPUTIME_ID, &ts);
    hstamp->tcpu = ts.tv_sec * uint64_t (1000000000) + ts.tv_nsec;
  }
#endif
#if     defined (__i386__) || defined (__x86_64__)
  hstamp->tsc = __rdtsc();
#endif
}

static void
hash_cpu_usage (KeccakRng &pool)
{
  union {
    uint64_t      ui64[24];
    struct {
      struct rusage rusage;     // 144 bytes
      struct tms    tms;        //  32 bytes
      clock_t       clk;        //   8 bytes
    };
  } u = { 0, };
  getrusage (RUSAGE_SELF, &u.rusage);
  u.clk = times (&u.tms);
  pool.xor_seed (u.ui64, sizeof (u.ui64) / sizeof (u.ui64[0]));
}

static bool
get_rdrand (uint64 *u, uint count)
{
#if defined (__i386__) || defined (__x86_64__)
  if (strstr (cpu_info().c_str(), " rdrand"))
    for (uint i = 0; i < count; i++)
      __asm__ __volatile__ ("rdrand %0" : "=r" (u[i]));
  else
    for (uint i = 0; i < count; i++)
      {
        uint64_t d = __rdtsc();       // fallback
        u[i] = bytehash_fnv64a ((const uint8*) &d, 8);
      }
  return true;
#endif
  return false;
}

static void
runtime_entropy (KeccakRng &pool)
{
  HashStamp hash_stamps[64] = { 0, };
  HashStamp *stamp = &hash_stamps[0];
  hash_time (stamp++);
  uint64_t uint_array[64] = { 0, };
  uint64_t *uintp = &uint_array[0];
  hash_time (stamp++);  *uintp++ = timestamp_realtime();
  hash_time (stamp++);  hash_cpu_usage (pool);
  hash_time (stamp++);  *uintp++ = timestamp_benchmark();
  hash_time (stamp++);  get_rdrand (uintp, 8); uintp += 8;
  hash_time (stamp++);  *uintp++ = ThisThread::thread_pid();
  hash_time (stamp++);  *uintp++ = getuid();
  hash_time (stamp++);  *uintp++ = geteuid();
  hash_time (stamp++);  *uintp++ = getgid();
  hash_time (stamp++);  *uintp++ = getegid();
  hash_time (stamp++);  *uintp++ = getpid();
  hash_time (stamp++);  *uintp++ = getsid (0);
  int ppid;
  hash_time (stamp++);  *uintp++ = ppid = getppid();
  hash_time (stamp++);  *uintp++ = getsid (ppid);
  hash_time (stamp++);  *uintp++ = getpgrp();
  hash_time (stamp++);  *uintp++ = tcgetpgrp (0);
  hash_time (stamp++);  hash_getrandom (pool);
  hash_time (stamp++);  { *uintp++ = std::random_device()(); } // may open devices, so destroy early on
  hash_time (stamp++);  hash_anything (pool, std::chrono::high_resolution_clock::now().time_since_epoch().count());
  hash_time (stamp++);  hash_anything (pool, std::chrono::steady_clock::now().time_since_epoch().count());
  hash_time (stamp++);  hash_anything (pool, std::chrono::system_clock::now().time_since_epoch().count());
  hash_time (stamp++);  hash_anything (pool, std::this_thread::get_id());
  String compiletime = __DATE__ __TIME__ __FILE__ __TIMESTAMP__;
  hash_time (stamp++);  *uintp++ = stringhash_fnv64a (compiletime);     // compilation entropy
  hash_time (stamp++);  *uintp++ = size_t (compiletime.data());         // heap address
  hash_time (stamp++);  *uintp++ = size_t (&entropy_mutex);             // data segment
  hash_time (stamp++);  *uintp++ = size_t ("PATH");                     // const data segment
  hash_time (stamp++);  *uintp++ = size_t (getenv ("PATH"));            // a.out address
  hash_time (stamp++);  *uintp++ = size_t (&stamp);                     // stack segment
  hash_time (stamp++);  *uintp++ = size_t (&runtime_entropy);           // code segment
  hash_time (stamp++);  *uintp++ = size_t (&::fopen);                   // libc code segment
  hash_time (stamp++);  *uintp++ = size_t (&std::string::npos);         // stl address
  hash_time (stamp++);  *uintp++ = stringhash_fnv64a (cpu_info());      // CPU type influence
  hash_time (stamp++);  *uintp++ = timestamp_benchmark();
  hash_time (stamp++);  hash_cpu_usage (pool);
  hash_time (stamp++);  *uintp++ = timestamp_realtime();
  hash_time (stamp++);
  assert (uintp <= &uint_array[sizeof (uint_array) / sizeof (uint_array[0])]);
  assert (stamp <= &hash_stamps[sizeof (hash_stamps) / sizeof (hash_stamps[0])]);
  pool.xor_seed ((uint64_t*) &hash_stamps[0], (stamp - &hash_stamps[0]) * sizeof (hash_stamps[0]) / sizeof (uint64_t));
  pool.xor_seed (&uint_array[0], uintp - &uint_array[0]);
}

void
Entropy::runtime_entropy (KeccakRng &pool)
{
  Rapicorn::runtime_entropy (pool);
}

void
collect_runtime_entropy (uint64 *data, size_t n)
{
  std::seed_seq seq { 1 }; // provide seed_seq to avoid auto seeding
  KeccakFastRng pool (seq);
  runtime_entropy (pool);
  for (size_t i = 0; i < n; i++)
    data[i] = pool();
}

static void
system_entropy (KeccakRng &pool)
{
  HashStamp hash_stamps[64] = { 0, };
  HashStamp *stamp = &hash_stamps[0];
  hash_time (stamp++);
  uint64_t uint_array[64] = { 0, };
  uint64_t *uintp = &uint_array[0];
  hash_time (stamp++);  *uintp++ = timestamp_realtime();
  hash_time (stamp++);  *uintp++ = timestamp_benchmark();
  hash_time (stamp++);  *uintp++ = timestamp_startup();
  hash_time (stamp++);  hash_cpu_usage (pool);
  hash_time (stamp++);  hash_file (pool, "/proc/sys/kernel/random/boot_id");
  hash_time (stamp++);  hash_file (pool, "/proc/version");
  hash_time (stamp++);  hash_file (pool, "/proc/cpuinfo");
  hash_time (stamp++);  hash_file (pool, "/proc/devices");
  hash_time (stamp++);  hash_file (pool, "/proc/meminfo");
  hash_time (stamp++);  hash_file (pool, "/proc/buddyinfo");
  hash_time (stamp++);  hash_file (pool, "/proc/diskstats");
  hash_time (stamp++);  hash_file (pool, "/proc/1/stat");
  hash_time (stamp++);  hash_file (pool, "/proc/1/sched");
  hash_time (stamp++);  hash_file (pool, "/proc/1/schedstat");
  hash_time (stamp++);  hash_file (pool, "/proc/self/stat");
  hash_time (stamp++);  hash_file (pool, "/proc/self/sched");
  hash_time (stamp++);  hash_file (pool, "/proc/self/schedstat");
  hash_time (stamp++);  hash_macs (pool);
  // hash_glob: "/sys/devices/**/net/*/address", "/sys/devices/*/*/*/ieee80211/phy*/*address*"
  hash_time (stamp++);  hash_file (pool, "/proc/uptime");
  hash_time (stamp++);  hash_file (pool, "/proc/user_beancounters");
  hash_time (stamp++);  hash_file (pool, "/proc/driver/rtc");
  hash_time (stamp++);  hash_stat (pool, "/var/log/syslog");            // for mtime
  hash_time (stamp++);  hash_stat (pool, "/var/log/auth.log");          // for mtime
  hash_time (stamp++);  hash_stat (pool, "/var/tmp");                   // for mtime
  hash_time (stamp++);  hash_stat (pool, "/tmp");                       // for mtime
  hash_time (stamp++);  hash_stat (pool, "/dev");                       // for mtime
  hash_time (stamp++);  hash_stat (pool, "/var/lib/ntp/ntp.drift");     // for mtime
  hash_time (stamp++);  hash_stat (pool, "/var/run/utmp");              // for mtime & atime
  hash_time (stamp++);  hash_stat (pool, "/var/log/wtmp");              // for mtime & atime
  hash_time (stamp++);  hash_stat (pool, "/sbin/init");                 // for atime
  hash_time (stamp++);  hash_stat (pool, "/var/spool");                 // for atime
  hash_time (stamp++);  hash_stat (pool, "/var/spool/cron");            // for atime
  hash_time (stamp++);  hash_stat (pool, "/var/spool/anacron");         // for atime
  hash_time (stamp++);  hash_file (pool, "/dev/urandom", 400);
  hash_time (stamp++);  hash_file (pool, "/proc/sys/kernel/random/uuid");
  hash_time (stamp++);  hash_file (pool, "/proc/schedstat");
  hash_time (stamp++);  hash_file (pool, "/proc/sched_debug");
  hash_time (stamp++);  hash_file (pool, "/proc/fairsched");
  hash_time (stamp++);  hash_file (pool, "/proc/interrupts");
  hash_time (stamp++);  hash_file (pool, "/proc/loadavg");
  hash_time (stamp++);  hash_file (pool, "/proc/softirqs");
  hash_time (stamp++);  hash_file (pool, "/proc/stat");
  hash_time (stamp++);  hash_file (pool, "/proc/net/fib_triestat");
  hash_time (stamp++);  hash_file (pool, "/proc/net/netstat");
  hash_time (stamp++);  hash_file (pool, "/proc/net/dev");
  hash_time (stamp++);  hash_file (pool, "/proc/vz/vestat");
  hash_time (stamp++);  hash_cpu_usage (pool);
  hash_time (stamp++);  *uintp++ = timestamp_realtime();
  hash_time (stamp++);
  assert (uintp <= &uint_array[sizeof (uint_array) / sizeof (uint_array[0])]);
  assert (stamp <= &hash_stamps[sizeof (hash_stamps) / sizeof (hash_stamps[0])]);
  pool.xor_seed ((uint64_t*) &hash_stamps[0], (stamp - &hash_stamps[0]) * sizeof (hash_stamps[0]) / sizeof (uint64_t));
  pool.xor_seed (&uint_array[0], uintp - &uint_array[0]);
}

void
Entropy::system_entropy (KeccakRng &pool)
{
  Rapicorn::system_entropy (pool);
}

void
collect_system_entropy (uint64 *data, size_t n)
{
  std::seed_seq seq { 1 }; // provide seed_seq to avoid auto seeding
  KeccakCryptoRng pool (seq);
  system_entropy (pool);
  for (size_t i = 0; i < n; i++)
    data[i] = pool();
}

} // Rapicorn
