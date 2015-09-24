// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "utilities.hh"
#include "inout.hh"
#include "main.hh"
#include "strings.hh"
#include "unicode.hh"
#include "thread.hh"
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <pwd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>  // gettid
#include <cxxabi.h> // abi::__cxa_demangle
#include <signal.h>
#include <glib.h>
#include <vector>
#include <algorithm>
#include <cstring>
#include <execinfo.h>
#include <sys/wait.h>
#include <stdexcept>

namespace Rapicorn {

/* --- limits.h & float.h checks --- */
/* assert several assumptions the code makes */
RAPICORN_STATIC_ASSERT (CHAR_BIT     == +8);
RAPICORN_STATIC_ASSERT (SCHAR_MIN    == -128);
RAPICORN_STATIC_ASSERT (SCHAR_MAX    == +127);
RAPICORN_STATIC_ASSERT (UCHAR_MAX    == +255);
RAPICORN_STATIC_ASSERT (SHRT_MIN     == -32768);
RAPICORN_STATIC_ASSERT (SHRT_MAX     == +32767);
RAPICORN_STATIC_ASSERT (USHRT_MAX    == +65535);
RAPICORN_STATIC_ASSERT (INT_MIN      == -2147483647 - 1);
RAPICORN_STATIC_ASSERT (INT_MAX      == +2147483647);
RAPICORN_STATIC_ASSERT (UINT_MAX     == +4294967295U);
RAPICORN_STATIC_ASSERT (INT64_MIN    == -9223372036854775807LL - 1);
RAPICORN_STATIC_ASSERT (INT64_MAX    == +9223372036854775807LL);
RAPICORN_STATIC_ASSERT (UINT64_MAX   == +18446744073709551615LLU);
RAPICORN_STATIC_ASSERT (LDBL_MIN     <= 1E-37);
RAPICORN_STATIC_ASSERT (LDBL_MAX     >= 1E+37);
RAPICORN_STATIC_ASSERT (LDBL_EPSILON <= 1E-9);
RAPICORN_STARTUP_ASSERT (FLT_MIN      <= 1E-37);
RAPICORN_STARTUP_ASSERT (FLT_MAX      >= 1E+37);
RAPICORN_STARTUP_ASSERT (FLT_EPSILON  <= 1E-5);
RAPICORN_STARTUP_ASSERT (DBL_MIN      <= 1E-37);
RAPICORN_STARTUP_ASSERT (DBL_MAX      >= 1E+37);
RAPICORN_STARTUP_ASSERT (DBL_EPSILON  <= 1E-9);

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

// == Timestamps ==
static clockid_t monotonic_clockid = CLOCK_REALTIME;
static uint64    monotonic_start = 0;
static uint64    monotonic_resolution = 1000;   // assume 1µs resolution for gettimeofday fallback
static uint64    realtime_start = 0;

static void
timestamp_init_ ()
{
  static const bool __used initialize = [] () {
    realtime_start = timestamp_realtime();
    struct timespec tp = { 0, 0 };
    if (clock_getres (CLOCK_REALTIME, &tp) >= 0)
      monotonic_resolution = tp.tv_sec * 1000000000ULL + tp.tv_nsec;
    uint64 mstart = realtime_start;
#ifdef CLOCK_MONOTONIC
    // CLOCK_MONOTONIC_RAW cannot slew, but doesn't measure SI seconds accurately
    // CLOCK_MONOTONIC may slew, but attempts to accurately measure SI seconds
    if (monotonic_clockid == CLOCK_REALTIME && clock_getres (CLOCK_MONOTONIC, &tp) >= 0)
      {
        monotonic_clockid = CLOCK_MONOTONIC;
        monotonic_resolution = tp.tv_sec * 1000000000ULL + tp.tv_nsec;
        mstart = timestamp_benchmark(); // here, monotonic_start=0 still
      }
#endif
    monotonic_start = mstart;
    return true;
  } ();
}
namespace { static struct Timestamper { Timestamper() { timestamp_init_(); } } realtime_startup; } // Anon

/** Provides the timestamp_realtime() value from program startup. */
uint64
timestamp_startup ()
{
  timestamp_init_();
  return realtime_start;
}

/** Return the current time as uint64 in µseconds. */
uint64
timestamp_realtime ()
{
  struct timespec tp = { 0, 0 };
  if (ISLIKELY (clock_gettime (CLOCK_REALTIME, &tp) >= 0))
    return tp.tv_sec * 1000000ULL + tp.tv_nsec / 1000;
  else
    {
      struct timeval now = { 0, 0 };
      gettimeofday (&now, NULL);
      return now.tv_sec * 1000000ULL + now.tv_usec;
    }
}

/** Provides resolution of timestamp_benchmark() in nano-seconds. */
uint64
timestamp_resolution ()
{
  timestamp_init_();
  return monotonic_resolution;
}

/** Returns benchmark timestamp in nano-seconds, clock starts around program startup. */
uint64
timestamp_benchmark ()
{
  struct timespec tp = { 0, 0 };
  uint64 stamp;
  if (ISLIKELY (clock_gettime (monotonic_clockid, &tp) >= 0))
    {
      stamp = tp.tv_sec * 1000000000ULL + tp.tv_nsec;
      stamp -= monotonic_start;                 // reduce number of significant bits
    }
  else
    {
      stamp = timestamp_realtime() * 1000;
      stamp -= MIN (stamp, monotonic_start);    // reduce number of significant bits
    }
  return stamp;
}

String
timestamp_format (uint64 stamp)
{
  const size_t fieldwidth = 8;
  const String fsecs = string_format ("%u", size_t (stamp) / 1000000);
  const String usecs = string_format ("%06u", size_t (stamp) % 1000000);
  String r = fsecs;
  if (r.size() < fieldwidth)
    r += '.';
  if (r.size() < fieldwidth)
    r += usecs.substr (0, fieldwidth - r.size());
  return r;
}

/// A monotonically increasing counter, increments are atomic and visible on all threads.
uint64
monotonic_counter ()
{
  static std::atomic<uint64> global_monotonic_counter { 4294967297 };
  return global_monotonic_counter++;
}

// == Logging ==
String
pretty_file (const char *file_dir, const char *file)
{
  if (!file)
    return "<" + String ("???") + ">";     // cannot use macros here
  if (RAPICORN_IS_ABSPATH (file) || !file_dir || !file_dir[0])
    return file;
  return String (file_dir) + "/" + file;
}

// == Sub-Process ==
struct PExec {
  String data_in, data_out, data_err;
  uint64 usec_timeout; // max child runtime
  StringVector args;
  StringVector evars;
public:
  inline int  execute (); // returns -errno
};

static String
exec_cmd (const String &cmd, bool witherr, StringVector fix_env, uint64 usec_timeout = 1000000 / 2)
{
  String btout;
  PExec sub;
  sub.data_in = "";
  sub.usec_timeout = usec_timeout;
  sub.args = string_split (cmd, " ");
  if (sub.args.size() < 1)
    sub.args.push_back ("");
  sub.evars = fix_env;
  int eret = sub.execute();
  if (eret < 0)
    critical ("executing '%s' failed: %s\n", sub.args[0].c_str(), strerror (-eret));
  if (!witherr)
    sub.data_err = "";
  return sub.data_err + sub.data_out;
}

static int // 0 on success
kill_child (int pid, int *status, int patience)
{
  int wr;
  if (patience >= 3)    // try graceful reap
    {
      if (waitpid (pid, status, WNOHANG) > 0)
        return 0;
    }
  if (patience >= 2)    // try SIGHUP
    {
      kill (pid, SIGHUP);
      if (waitpid (pid, status, WNOHANG) > 0)
        return 0;
      usleep (20 * 1000); // give it some scheduling/shutdown time
      if (waitpid (pid, status, WNOHANG) > 0)
        return 0;
      usleep (50 * 1000); // give it some scheduling/shutdown time
      if (waitpid (pid, status, WNOHANG) > 0)
        return 0;
      usleep (100 * 1000); // give it some scheduling/shutdown time
      if (waitpid (pid, status, WNOHANG) > 0)
        return 0;
    }
  if (patience >= 1)    // try SIGTERM
    {
      kill (pid, SIGTERM);
      if (waitpid (pid, status, WNOHANG) > 0)
        return 0;
      usleep (200 * 1000); // give it some scheduling/shutdown time
      if (waitpid (pid, status, WNOHANG) > 0)
        return 0;
      usleep (400 * 1000); // give it some scheduling/shutdown time
      if (waitpid (pid, status, WNOHANG) > 0)
        return 0;
    }
  // finish it off
  kill (pid, SIGKILL);
  do
    wr = waitpid (pid, status, 0);
  while (wr < 0 && errno == EINTR);
  return wr;
}

static inline ssize_t
string_must_read (String &string, int fd)
{
  ssize_t n_bytes;
  do
    {
      char buf[4096];
      n_bytes = read (fd, buf, sizeof (buf));
      if (n_bytes == 0)
        return 0; // EOF, calling this function assumes data is available
      else if (n_bytes > 0)
        {
          string.append (buf, n_bytes);
          return n_bytes;
        }
    }
  while (n_bytes < 0 && errno == EINTR);
  // n_bytes < 0
  critical ("failed to read() from child process: %s", strerror (errno));
  return -1;
}

#define RETRY_ON_EINTR(expr)    ({ ssize_t __r; do __r = ssize_t (expr); while (__r < 0 && errno == EINTR); __r; })

static inline size_t
string_must_write (const String &string, int outfd, size_t *stringpos)
{
  if (*stringpos < string.size())
    {
      ssize_t n = RETRY_ON_EINTR (write (outfd, string.data() + *stringpos, string.size() - *stringpos));
      *stringpos += MAX (n, 0);
    }
  return *stringpos < string.size(); // remainings
}

int
PExec::execute ()
{
  int fork_pid = -1;
  int stdin_pipe[2] = { -1, -1 };
  int stdout_pipe[2] = { -1, -1 };
  int stderr_pipe[2] = { -1, -1 };
  ssize_t i;
  const int parent_pid = getpid();
  if (pipe (stdin_pipe) < 0 || pipe (stdout_pipe) < 0 || pipe (stderr_pipe) < 0)
    goto return_errno;
  if (signal (SIGCHLD, SIG_DFL) == SIG_ERR)
    goto return_errno;
  char parent_pid_str[64];
  if (snprintf (parent_pid_str, sizeof (parent_pid_str), "%u", parent_pid) < 0)
    goto return_errno;
  char self_exe[1024]; // PATH_MAX
  i = readlink ("/proc/self/exe", self_exe, sizeof (self_exe));
  if (i < 0)
    goto return_errno;
  errno = ENOMEM;
  if (size_t (i) >= sizeof (self_exe))
    goto return_errno;
  fork_pid = fork ();
  if (fork_pid < 0)
    goto return_errno;
  if (fork_pid == 0)    // child
    {
      for (const String &evar : evars)
        {
          const char *var = evar.c_str(), *eq = strchr (var, '=');
          if (!eq)
            {
              unsetenv (var);
              continue;
            }
          setenv (evar.substr (0, eq - var).c_str(), eq + 1, true);
        }
      close (stdin_pipe[1]);
      close (stdout_pipe[0]);
      close (stderr_pipe[0]);
      if (RETRY_ON_EINTR (dup2 (stdin_pipe[0], 0)) < 0 ||
          RETRY_ON_EINTR (dup2 (stdout_pipe[1], 1)) < 0 ||
          RETRY_ON_EINTR (dup2 (stderr_pipe[1], 2)) < 0)
        {
          kill (getpid(), SIGSYS);
          while (1)
            _exit (-128);
        }
      if (stdin_pipe[0] >= 3)
        close (stdin_pipe[0]);
      if (stdout_pipe[1] >= 3)
        close (stdout_pipe[1]);
      if (stderr_pipe[1] >= 3)
        close (stderr_pipe[1]);
      const ssize_t exec_nargs = args.size() + 1;
      const char *exec_args[exec_nargs];
      for (i = 0; i < exec_nargs - 1; i++)
        if (args[i] == "\uFFFDexe\u001A")
          exec_args[i] = self_exe;
        else if (args[i] == "\uFFFDpid\u001A")
          exec_args[i] = parent_pid_str;
        else
          exec_args[i] = args[i].c_str();
      exec_args[exec_nargs - 1] = NULL;
      execvp (exec_args[0], (char**) exec_args);
      kill (getpid(), SIGSYS);
      while (1)
        _exit (-128);
    }
  else                  // parent
    {
      size_t data_in_pos = 0, need_wait = true;
      close (stdin_pipe[0]);
      close (stdout_pipe[1]);
      close (stderr_pipe[1]);
      uint last_status = 0;
      uint64 sstamp = timestamp_realtime();
      // read data until we get EOF on all pipes
      while (stdout_pipe[0] >= 0 || stderr_pipe[0] >= 0)
        {
          fd_set readfds, writefds;
          FD_ZERO (&readfds);
          FD_ZERO (&writefds);
          if (stdin_pipe[1] >= 0)
            FD_SET (stdin_pipe[1], &writefds);
          if (stdout_pipe[0] >= 0)
            FD_SET (stdout_pipe[0], &readfds);
          if (stderr_pipe[0] >= 0)
            FD_SET (stderr_pipe[0], &readfds);
          int maxfd = MAX (stdin_pipe[1], MAX (stdout_pipe[0], stderr_pipe[0]));
          struct timeval tv;
          tv.tv_sec = 0; // sleep at most 0.5 seconds to catch clock skews, etc.
          tv.tv_usec = MIN (usec_timeout ? usec_timeout : 1000000, 100 * 1000);
          int ret = select (maxfd + 1, &readfds, &writefds, NULL, &tv);
          if (ret < 0 && errno != EINTR)
            goto return_errno;
          if (stdin_pipe[1] >= 0 &&
              FD_ISSET (stdin_pipe[1], &writefds) &&
              string_must_write (data_in, stdin_pipe[1], &data_in_pos) == 0)
            {
              close (stdin_pipe[1]);
              stdin_pipe[1] = -1;
            }
          if (stdout_pipe[0] >= 0 && FD_ISSET (stdout_pipe[0], &readfds) && string_must_read (data_out, stdout_pipe[0]) == 0)
            {
              close (stdout_pipe[0]);
              stdout_pipe[0] = -1;
            }
          if (stderr_pipe[0] >= 0 && FD_ISSET (stderr_pipe[0], &readfds) && string_must_read (data_err, stderr_pipe[0]) == 0)
            {
              close (stderr_pipe[0]);
              stderr_pipe[0] = -1;
            }
          if (usec_timeout)
            {
              uint64 nstamp = timestamp_realtime();
              int status = 0;
              sstamp = MIN (sstamp, nstamp); // guard against backwards clock skews
              if (usec_timeout < nstamp - sstamp)
                {
                  // timeout reached, need to abort the child now
                  kill_child (fork_pid, &status, 3);
                  last_status = 1024; // timeout
                  if (WIFSIGNALED (status))
                    ; // debug_msg ("%s: child timed out and received: %s\n", __func__, strsignal (WTERMSIG (status)));
                  need_wait = false;
                  break;
                }
            }
        }
      close (stdout_pipe[0]);
      close (stderr_pipe[0]);
      if (need_wait)
        {
          int status = 0;
          pid_t wr;
          do
            wr = waitpid (fork_pid, &status, 0);
          while (wr < 0 && errno == EINTR);
          if (WIFEXITED (status)) // normal exit
            last_status = WEXITSTATUS (status); // 0..255
          else if (WIFSIGNALED (status))
            last_status = (WTERMSIG (status) << 12); // signalled
          else // WCOREDUMP (status)
            last_status = 512; // coredump
        }
      return last_status;
    }
 return_errno:
  const int error_errno = errno;
  if (fork_pid > 0) // child still alive?
    {
      int status = 0;
      kill_child (fork_pid, &status, 1);
    }
  close (stdin_pipe[0]);
  close (stdin_pipe[1]);
  close (stdout_pipe[0]);
  close (stdout_pipe[1]);
  close (stderr_pipe[0]);
  close (stderr_pipe[1]);
  errno = error_errno;
  return -error_errno;
}

// == Backtrace ==
struct Mapping { size_t addr, end; String exe; };
typedef std::vector<Mapping> MappingVector;

static MappingVector
read_maps ()
{
  MappingVector mv;
  FILE *fmaps = fopen ("/proc/self/maps", "r");
  if (!fmaps)
    return mv;
  int count = 0;
  while (!feof (fmaps))
    {
      size_t addr = 0, end = 0, offset, inode = 0;
      char perms[9 + 1], filename[1234 + 1] = "", device[9 + 1] = "";
      count = fscanf (fmaps, "%zx-%zx %9s %zx %9s %zu", &addr, &end, perms, &offset, device, &inode);
      if (count < 1)
        break;
      char c = fgetc (fmaps);
      while (c == ' ')
        c = fgetc (fmaps);
      if (c != '\n')
        {
          ungetc (c, fmaps);
          count = fscanf (fmaps, "%1234s\n", filename);
        }
      if (filename[0])
        mv.push_back (Mapping { addr, end, filename });
    }
  fclose (fmaps);
  return mv;
}

static StringVector
pretty_backtrace_symbols (void **pointers, const int nptrs)
{
  // fetch process maps to correlate pointers
  MappingVector maps = read_maps();
  // reduce mappings to DSOs
  char self_exe[1024]; // PATH_MAX
  ssize_t l = readlink ("/proc/self/exe", self_exe, sizeof (self_exe));
  if (l > 0)
    {
      // masquerade mappings for the argv[0] executable as addr2line uses absolute addresses for executables
      for (size_t i = 0; i < maps.size(); i++)
        if (maps[i].exe == self_exe ||                          // erase /proc/self/exe
            maps[i].exe.size() < 2 || maps[i].exe[0] != '/')    // erase "[heap]" and other non-dso
          {
            maps.erase (maps.begin() + i);
            i--;
          }
    }
  // resolve pointers to function, file and line
  StringVector symbols;
  const char *addr2line = "/usr/bin/addr2line";
  const bool have_addr2line = access (addr2line, X_OK) == 0;
  for (ssize_t i = 0; i < nptrs; i++)
    {
      const size_t addr = size_t (pointers[i]);
      String dso;
      size_t dso_offset = 0;
      // find DSO for pointer
      for (size_t j = 0; j < maps.size(); j++)
        if (maps[j].addr < addr && addr < maps[j].end)
          {
            dso = maps[j].exe;
            dso_offset = addr - maps[j].addr;
            break;
          }
      if (dso.empty())
        {
          // no DSO, resort to executable
          dso = self_exe;
          dso_offset = addr;
        }
      // resolve through addr2line
      String entry;
      if (have_addr2line)
        {
          // resolve to code location *before* return address
          const size_t caller_addr = dso_offset - 1;
          entry = exec_cmd (string_format ("%s -C -f -s -i -p -e %s 0x%zx", addr2line, dso.c_str(), caller_addr), true,
                            cstrings_to_vector ("LC_ALL=C", "LANGUAGE", NULL));
        }
      // polish entry
      while (entry.size() && strchr ("\n \t\r", entry[entry.size() - 1]))
        entry.resize (entry.size() - 1);
      if (string_endswith (entry, ":?"))
        entry.resize (entry.size() - 2);
      if (string_endswith (entry, " at ??"))
        entry.resize (entry.size() - 6);
      if (entry.compare (0, 2, "??") == 0)
        entry = "";
      if (entry.empty())
        entry = string_format ("%s@0x%zx", dso.c_str(), dso_offset);
      // format backtrace symbol output
      String symbol = string_format ("0x%016zx", addr);
      if (!entry.empty())
        symbol += " in " + entry;
      for (auto sym : string_split (symbol, "\n"))
        {
          if (sym.compare (0, 14, " (inlined by) ") == 0)
            sym = "           inlined by " + sym.substr (14);
          symbols.push_back (sym);
        }
    }
  return symbols;
}

#ifdef _EXECINFO_H
int (*backtrace_pointers) (void **buffer, int size) = &::backtrace; // from glibc
#else  // !_EXECINFO_H
static int
dummy_backtrace (void **buffer, int size)
{
  if (size)
    {
      buffer[0] = __builtin_return_address (0);
      return 1;
    }
  return 0;
}
int (*backtrace_pointers) (void **buffer, int size) = &dummy_backtrace;
#endif // !_EXECINFO_H

String
pretty_backtrace (void **ptrs, size_t nptrs, const char *file, int line, const char *func)
{
  void *fallback = __builtin_return_address (0);
  if (nptrs < 0)
    {
      ptrs = &fallback;
      nptrs = 1;
    }
  StringVector symbols = pretty_backtrace_symbols (ptrs, nptrs);
  String where;
  if (file && file[0])
    where += file;
  if (!where.empty() && line > 0)
    where += string_format (":%u", line);
  if (func && func[0])
    {
      if (!where.empty())
        where += ":";
      where += func;
    }
  if (!where.empty())
    where = " (from " + where + ")";
  return string_format ("Backtrace at 0x%016x%s:\n  %s\n", ptrs[0], where, string_join ("\n  ", symbols));
}

// == debug_backtrace_snapshot ==
struct BacktraceBuffer {
  void *ptrbuffer[RAPICORN_BACKTRACE_MAXDEPTH];
  int nptrs;
  BacktraceBuffer () :
    nptrs (0)
  {}
};
static Mutex                             backtrace_snapshot_mutex;
static std::map<size_t,BacktraceBuffer> *backtrace_snapshot_map = NULL;

void
debug_backtrace_snapshot (size_t key)
{
  BacktraceBuffer bbuffer;
  bbuffer.nptrs = backtrace (bbuffer.ptrbuffer, ARRAY_SIZE (bbuffer.ptrbuffer));
  ScopedLock<Mutex> locker (backtrace_snapshot_mutex);
  if (!backtrace_snapshot_map)
    backtrace_snapshot_map = new std::map<size_t,BacktraceBuffer>();
  (*backtrace_snapshot_map)[key] = bbuffer;
}

String
debug_backtrace_showshot (size_t key)
{
  BacktraceBuffer bbuffer;
  {
    ScopedLock<Mutex> locker (backtrace_snapshot_mutex);
    if (backtrace_snapshot_map)
      bbuffer = (*backtrace_snapshot_map)[key];
  }
  const StringVector syms = pretty_backtrace_symbols (bbuffer.ptrbuffer, bbuffer.nptrs);
  String btmsg;
  size_t addr = bbuffer.nptrs >= 1 ? size_t (bbuffer.ptrbuffer[1]) : 0;
  btmsg = string_format ("Backtrace at 0x%08x (stackframe at 0x%08x):\n", addr, size_t (__builtin_frame_address (0)));
  for (size_t i = 0; i < syms.size(); i++)
    btmsg += string_format ("  %s\n", syms[i].c_str());
  return btmsg;
}

// == CxxPasswd =
struct CxxPasswd {
  String pw_name, pw_passwd, pw_gecos, pw_dir, pw_shell;
  uid_t pw_uid;
  gid_t pw_gid;
  CxxPasswd (String username = "");
};

CxxPasswd::CxxPasswd (String username) :
  pw_uid (-1), pw_gid (-1)
{
  const int strbuf_size = 5 * 1024;
  char strbuf[strbuf_size + 256]; // work around Darwin getpwnam_r overwriting buffer boundaries
  struct passwd pwnambuf, *p = NULL;
  if (username.empty())
    {
      int ret = 0;
      errno = 0;
      do
        {
          if (1) // HAVE_GETPWUID_R
            ret = getpwuid_r (getuid(), &pwnambuf, strbuf, strbuf_size, &p);
          else   // HAVE_GETPWUID
            p = getpwuid (getuid());
        }
      while ((ret != 0 || p == NULL) && errno == EINTR);
      if (ret != 0)
        p = NULL;
    }
  else // !username.empty()
    {
      int ret = 0;
      errno = 0;
      do
        {
          if (1) // HAVE_GETPWNAM_R
            ret = getpwnam_r (username.c_str(), &pwnambuf, strbuf, strbuf_size, &p);
          else   // HAVE_GETPWNAM
            p = getpwnam (username.c_str());
        }
      while ((ret != 0 || p == NULL) && errno == EINTR);
      if (ret != 0)
        p = NULL;
    }
  if (p)
    {
      pw_name = p->pw_name;
      pw_passwd = p->pw_passwd;
      pw_uid = p->pw_uid;
      pw_gid = p->pw_gid;
      pw_gecos = p->pw_gecos;
      pw_dir = p->pw_dir;
      pw_shell = p->pw_shell;
    }
}

// == Development Helpers ==
/**
 * @def RAPICORN_STRLOC()
 * Expand to a string literal, describing the current code location.
 * Returns a string describing the current source code location, such as FILE and LINE number.
 */

/**
 * @def RAPICORN_RETURN_IF(expr [, rvalue])
 * Return if @a expr evaluates to true.
 * Silently return @a rvalue if expression @a expr evaluates to true. Returns void if @a rvalue was not specified.
 */

/**
 * @def RAPICORN_RETURN_UNLESS(expr [, rvalue])
 * Return if @a expr is false.
 * Silently return @a rvalue if expression @a expr evaluates to false. Returns void if @a rvalue was not specified.
 */

namespace Path {

/**
 * @param path  a filename path
 *
 * Return the directory part of a file name.
 */
String
dirname (const String &path)
{
  char *gdir = g_path_get_dirname (path.c_str());
  String dname = gdir;
  g_free (gdir);
  return dname;
}

/**
 * @param path  a filename path
 *
 * Strips all directory components from @a path and returns the resulting file name.
 */
String
basename (const String &path)
{
  char *gbase = g_path_get_basename (path.c_str());
  String bname = gbase;
  g_free (gbase);
  return bname;
}

/**
 * @param path a filename path
 *
 * Resolve links and directory references in @a path and provide
 * a canonicalized absolute pathname.
 */
String
realpath (const String &path)
{
  char *const cpath = ::realpath (path.c_str(), NULL);
  if (cpath)
    {
      const String result = cpath;
      free (cpath);
      errno = 0;
      return result;
    }
  // error case
  return path;
}

/**
 * @param path  a filename path
 * @param incwd optional current working directory
 *
 * Complete @a path to become an absolute file path. If neccessary, @a incwd or
 * the real current working directory is prepended.
 */
String
abspath (const String &path, const String &incwd)
{
  if (isabs (path))
    return path;
  if (!incwd.empty())
    return abspath (join (incwd, path), "");
  String pcwd = program_cwd();
  if (!pcwd.empty())
    return join (pcwd, path);
  return join (cwd(), path);
}

/**
 * @param path  a filename path
 *
 * Return wether @a path is an absolute pathname.
 */
bool
isabs (const String &path)
{
  return g_path_is_absolute (path.c_str());
}

/**
 * @param path  a filename path
 *
 * Return wether @a path is pointing to a directory component.
 */
bool
isdirname (const String &path)
{
  uint l = path.size();
  if (path == "." || path == "..")
    return true;
  if (l >= 1 && path[l-1] == RAPICORN_DIR_SEPARATOR)
    return true;
  if (l >= 2 && path[l-2] == RAPICORN_DIR_SEPARATOR && path[l-1] == '.')
    return true;
  if (l >= 3 && path[l-3] == RAPICORN_DIR_SEPARATOR && path[l-2] == '.' && path[l-1] == '.')
    return true;
  return false;
}

/// Get a @a user's home directory, uses $HOME if no @a username is given.
String
user_home (const String &username)
{
  if (username.empty())
    {
      // $HOME gets precedence over getpwnam(3), like '~/' vs '~username/' expansion
      const char *homedir = getenv ("HOME");
      if (homedir && isabs (homedir))
        return homedir;
    }
  CxxPasswd pwn (username);
  return pwn.pw_dir;
}

/// Get the $XDG_DATA_HOME directory, see: https://specifications.freedesktop.org/basedir-spec/latest
String
data_home ()
{
  const char *var = getenv ("XDG_DATA_HOME");
  if (var && isabs (var))
    return var;
  return expand_tilde ("~/.local/share");
}

/// Get the $XDG_CONFIG_HOME directory, see: https://specifications.freedesktop.org/basedir-spec/latest
String
config_home ()
{
  const char *var = getenv ("XDG_CONFIG_HOME");
  if (var && isabs (var))
    return var;
  return expand_tilde ("~/.config");
}

/// Get the $XDG_CACHE_HOME directory, see: https://specifications.freedesktop.org/basedir-spec/latest
String
cache_home ()
{
  const char *var = getenv ("XDG_CACHE_HOME");
  if (var && isabs (var))
    return var;
  return expand_tilde ("~/.cache");
}

/// Get the $XDG_RUNTIME_DIR directory, see: https://specifications.freedesktop.org/basedir-spec/latest
String
runtime_dir ()
{
  const char *var = getenv ("XDG_RUNTIME_DIR");
  if (var && isabs (var))
    return var;
  return string_format ("/run/user/%u", getuid());
}

/// Get the $XDG_CONFIG_DIRS directory list, see: https://specifications.freedesktop.org/basedir-spec/latest
String
config_dirs ()
{
  const char *var = getenv ("XDG_CONFIG_DIRS");
  if (var && var[0])
    return var;
  else
    return "/etc/xdg";
}

/// Get the $XDG_DATA_DIRS directory list, see: https://specifications.freedesktop.org/basedir-spec/latest
String
data_dirs ()
{
  const char *var = getenv ("XDG_DATA_DIRS");
  if (var && var[0])
    return var;
  else
    return "/usr/local/share:/usr/share";
}

static String
access_config_names (const String *newval)
{
  static Mutex mutex;
  ScopedLock<Mutex> locker (mutex);
  static String cfg_names;
  if (newval)
    cfg_names = *newval;
  if (cfg_names.empty())
    {
      String names = Path::basename (program_name());
      if (program_alias() != names)
        names = searchpath_join (names, program_alias());
      return names;
    }
  else
    return cfg_names;
}

/// Get config names as set with config_names(), if unset defaults to program_name() and program_alias().
String
config_names ()
{
  return access_config_names (NULL);
}

/// Set a colon separated list of names for this application to find configuration settings and files.
void
config_names (const String &names)
{
  access_config_names (&names);
}

/// Expand a "~/" or "~user/" @a path which refers to user home directories.
String
expand_tilde (const String &path)
{
  if (path[0] != '~')
    return path;
  const size_t dir = path.find (RAPICORN_DIR_SEPARATOR);
  String username;
  if (dir != String::npos)
    username = path.substr (1, dir - 1);
  else
    username = path.substr (1);
  const String userhome = user_home (username);
  return join (userhome, dir == String::npos ? "" : path.substr (dir));
}

String
skip_root (const String &path)
{
  const char *frag = g_path_skip_root (path.c_str());
  return frag;
}

String
join (const String &frag0, const String &frag1,
      const String &frag2, const String &frag3,
      const String &frag4, const String &frag5,
      const String &frag6, const String &frag7,
      const String &frag8, const String &frag9,
      const String &frag10, const String &frag11,
      const String &frag12, const String &frag13,
      const String &frag14, const String &frag15)
{
  char *cpath = g_build_path (RAPICORN_DIR_SEPARATOR_S, frag0.c_str(),
                              frag1.c_str(), frag2.c_str(), frag3.c_str(), frag4.c_str(),
                              frag5.c_str(), frag6.c_str(), frag7.c_str(), frag8.c_str(),
                              frag9.c_str(), frag10.c_str(), frag11.c_str(), frag12.c_str(),
                              frag13.c_str(), frag14.c_str(), frag15.c_str(), NULL);
  String path (cpath);
  g_free (cpath);
  return path;
}

static int
errno_check_file (const char *file_name,
                  const char *mode)
{
  uint access_mask = 0, nac = 0;
  
  if (strchr (mode, 'e'))       /* exists */
    nac++, access_mask |= F_OK;
  if (strchr (mode, 'r'))       /* readable */
    nac++, access_mask |= R_OK;
  if (strchr (mode, 'w'))       /* writable */
    nac++, access_mask |= W_OK;
  bool check_exec = strchr (mode, 'x') != NULL;
  if (check_exec)               /* executable */
    nac++, access_mask |= X_OK;
  
  /* on some POSIX systems, X_OK may succeed for root without any
   * executable bits set, so we also check via stat() below.
   */
  if (nac && access (file_name, access_mask) < 0)
    return -errno;
  
  bool check_file = strchr (mode, 'f') != NULL;     /* open as file */
  bool check_dir  = strchr (mode, 'd') != NULL;     /* open as directory */
  bool check_link = strchr (mode, 'l') != NULL;     /* open as link */
  bool check_char = strchr (mode, 'c') != NULL;     /* open as character device */
  bool check_block = strchr (mode, 'b') != NULL;    /* open as block device */
  bool check_pipe = strchr (mode, 'p') != NULL;     /* open as pipe */
  bool check_socket = strchr (mode, 's') != NULL;   /* open as socket */
  
  if (check_exec || check_file || check_dir || check_link || check_char || check_block || check_pipe || check_socket)
    {
      struct stat st;
      
      if (check_link)
        {
          if (lstat (file_name, &st) < 0)
            return -errno;
        }
      else if (stat (file_name, &st) < 0)
        return -errno;
      
      if (0)
        g_printerr ("file-check(\"%s\",\"%s\"): %s%s%s%s%s%s%s\n",
                    file_name, mode,
                    S_ISREG (st.st_mode) ? "f" : "",
                    S_ISDIR (st.st_mode) ? "d" : "",
                    S_ISLNK (st.st_mode) ? "l" : "",
                    S_ISCHR (st.st_mode) ? "c" : "",
                    S_ISBLK (st.st_mode) ? "b" : "",
                    S_ISFIFO (st.st_mode) ? "p" : "",
                    S_ISSOCK (st.st_mode) ? "s" : "");
      
      if (S_ISDIR (st.st_mode) && (check_file || check_link || check_char || check_block || check_pipe))
        return -EISDIR;
      if (check_file && !S_ISREG (st.st_mode))
        return -EINVAL;
      if (check_dir && !S_ISDIR (st.st_mode))
        return -ENOTDIR;
      if (check_link && !S_ISLNK (st.st_mode))
        return -EINVAL;
      if (check_char && !S_ISCHR (st.st_mode))
        return -ENODEV;
      if (check_block && !S_ISBLK (st.st_mode))
        return -ENOTBLK;
      if (check_pipe && !S_ISFIFO (st.st_mode))
        return -ENXIO;
      if (check_socket && !S_ISSOCK (st.st_mode))
        return -ENOTSOCK;
      if (check_exec && !(st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)))
        return -EACCES; /* for root executable, any +x bit is good enough */
    }
  
  return 0;
}

/**
 * @param file  possibly relative filename
 * @param mode  feature string
 * @return      true if @a file adhears to @a mode
 *
 * Perform various checks on @a file and return whether all
 * checks passed. On failure, errno is set appropriately, and
 * FALSE is returned. Available features to be checked for are:
 * @li @c e - @a file must exist
 * @li @c r - @a file must be readable
 * @li @c w - @a file must be writable
 * @li @c x - @a file must be executable
 * @li @c f - @a file must be a regular file
 * @li @c d - @a file must be a directory
 * @li @c l - @a file must be a symbolic link
 * @li @c c - @a file must be a character device
 * @li @c b - @a file must be a block device
 * @li @c p - @a file must be a named pipe
 * @li @c s - @a file must be a socket.
 */
bool
check (const String &file,
       const String &mode)
{
  int err = file.size() && mode.size() ? errno_check_file (file.c_str(), mode.c_str()) : -EFAULT;
  errno = err < 0 ? -err : 0;
  return errno == 0;
}

/**
 * @param file1  possibly relative filename
 * @param file2  possibly relative filename
 * @return       TRUE if @a file1 and @a file2 are equal
 *
 * Check whether @a file1 and @a file2 are pointing to the same inode
 * in the same file system on the same device.
 */
bool
equals (const String &file1,
        const String &file2)
{
  if (!file1.size() || !file2.size())
    return file1.size() == file2.size();
  struct stat st1 = { 0, }, st2 = { 0, };
  int err1 = 0, err2 = 0;
  errno = 0;
  if (stat (file1.c_str(), &st1) < 0 && stat (file1.c_str(), &st1) < 0)
    err1 = errno;
  errno = 0;
  if (stat (file2.c_str(), &st2) < 0 && stat (file2.c_str(), &st2) < 0)
    err2 = errno;
  if (err1 || err2)
    return err1 == err2;
  return (st1.st_dev  == st2.st_dev &&
          st1.st_ino  == st2.st_ino &&
          st1.st_rdev == st2.st_rdev);
}

/// Return the current working directoy, including symlinks used in $PWD if available.
String
cwd ()
{
#ifdef  _GNU_SOURCE
  {
    char *dir = get_current_dir_name();
    if (dir)
      {
        const String result = dir;
        free (dir);
        return result;
      }
  }
#endif
  size_t size = 512;
  do
    {
      char *buf = (char*) malloc (size);
      if (!buf)
        break;
      const char *const dir = getcwd (buf, size);
      if (dir)
        {
          const String result = dir;
          free (buf);
          return result;
        }
      free (buf);
      size *= 2;
    }
  while (errno == ERANGE);
  // system must be in a bad shape if we get here...
  return "./";
}

StringVector
searchpath_split (const String &searchpath)
{
  StringVector sv;
  uint i, l = 0;
  for (i = 0; i < searchpath.size(); i++)
    if (searchpath[i] == RAPICORN_SEARCHPATH_SEPARATOR || // ':' or ';'
        searchpath[i] == ';') // make ';' work under windows and unix
      {
        if (i > l)
          sv.push_back (searchpath.substr (l, i - l));
        l = i + 1;
      }
  if (i > l)
    sv.push_back (searchpath.substr (l, i - l));
  return sv;
}

static bool
is_searchpath_separator (const char c)
{
  return c == RAPICORN_SEARCHPATH_SEPARATOR || (c == ':' || c == ';');
}

/// Check if @a searchpath contains @a element, a trailing slash searches for directories.
bool
searchpath_contains (const String &searchpath, const String &element)
{
  const bool dirsearch = string_endswith (element, RAPICORN_DIR_SEPARATOR_S);
  const String needle = dirsearch && element.size() > 1 ? element.substr (0, element.size() - 1) : element; // strip trailing slash
  size_t pos = searchpath.find (needle);
  while (pos != String::npos)
    {
      size_t end = pos + needle.size();
      if (pos == 0 || is_searchpath_separator (searchpath[pos - 1]))
        {
          if (dirsearch && searchpath[end] == RAPICORN_DIR_SEPARATOR)
            end++; // skip trailing slash in searchpath segment
          if (searchpath[end] == 0 || is_searchpath_separator (searchpath[end]))
            return true;
        }
      pos = searchpath.find (needle, end);
    }
  return false;
}

/// Find the first @a file in @a searchpath which matches @a mode (see check()).
String
searchpath_find (const String &searchpath, const String &file, const String &mode)
{
  if (isabs (file))
    return check (file, mode) ? file : "";
  StringVector sv = searchpath_split (searchpath);
  for (size_t i = 0; i < sv.size(); i++)
    if (check (join (sv[i], file), mode))
      return join (sv[i], file);
  return "";
}

/// Find all @a searchpath entries matching @a mode (see check()).
StringVector
searchpath_list (const String &searchpath, const String &mode)
{
  StringVector v;
  for (const auto &file : searchpath_split (searchpath))
    if (check (file, mode))
      v.push_back (file);
  return v;
}

static String
searchpath_join1 (const String &a, const String &b)
{
  if (a.empty())
    return b;
  if (b.empty())
    return a;
  if (a[a.size()-1] == RAPICORN_SEARCHPATH_SEPARATOR ||
      b[0] == RAPICORN_SEARCHPATH_SEPARATOR)
    return a + b;
  return a + RAPICORN_SEARCHPATH_SEPARATOR_S + b;
}

/// Yield a new searchpath by combining each element of @a searchpath with each element of @a postfixes.
String
searchpath_multiply (const String &searchpath, const String &postfixes)
{
  String newpath;
  for (const auto &e : searchpath_split (searchpath))
    for (const auto &p : searchpath_split (postfixes))
      newpath = searchpath_join1 (newpath, join (e, p));
  return newpath;
}

String
searchpath_join (const String &frag0, const String &frag1, const String &frag2, const String &frag3,
                 const String &frag4, const String &frag5, const String &frag6, const String &frag7,
                 const String &frag8, const String &frag9, const String &frag10, const String &frag11,
                 const String &frag12, const String &frag13, const String &frag14, const String &frag15)
{
  String result = searchpath_join1 (frag0, frag1);
  result = searchpath_join1 (result, frag2);
  result = searchpath_join1 (result, frag3);
  result = searchpath_join1 (result, frag4);
  result = searchpath_join1 (result, frag5);
  result = searchpath_join1 (result, frag6);
  result = searchpath_join1 (result, frag7);
  result = searchpath_join1 (result, frag8);
  result = searchpath_join1 (result, frag9);
  result = searchpath_join1 (result, frag10);
  result = searchpath_join1 (result, frag11);
  result = searchpath_join1 (result, frag12);
  result = searchpath_join1 (result, frag13);
  result = searchpath_join1 (result, frag14);
  result = searchpath_join1 (result, frag15);
  return result;
}

String
vpath_find (const String &file, const String &mode)
{
  String result = searchpath_find (".", file, mode);
  if (!result.empty())
    return result;
  const char *vpath = getenv ("VPATH");
  if (vpath)
    {
      result = searchpath_find (vpath, file, mode);
      if (!result.empty())
        return result;
    }
  return file;
}

const String dir_separator = RAPICORN_DIR_SEPARATOR_S;
const String searchpath_separator = RAPICORN_SEARCHPATH_SEPARATOR_S;

static char* /* return malloc()-ed buffer containing a full read of FILE */
file_memread (FILE   *stream,
              size_t *lengthp)
{
  size_t sz = 256;
  char *malloc_string = (char*) malloc (sz);
  if (!malloc_string)
    return NULL;
  char *start = malloc_string;
  errno = 0;
  while (!feof (stream))
    {
      size_t bytes = fread (start, 1, sz - (start - malloc_string), stream);
      if (bytes <= 0 && ferror (stream) && errno != EAGAIN)
        {
          start = malloc_string; // error/0-data
          break;
        }
      start += bytes;
      if (start == malloc_string + sz)
        {
          bytes = start - malloc_string;
          sz *= 2;
          char *newstring = (char*) realloc (malloc_string, sz);
          if (!newstring)
            {
              start = malloc_string; // error/0-data
              break;
            }
          malloc_string = newstring;
          start = malloc_string + bytes;
        }
    }
  int savederr = errno;
  *lengthp = start - malloc_string;
  if (!*lengthp)
    {
      free (malloc_string);
      malloc_string = NULL;
    }
  errno = savederr;
  return malloc_string;
}

char*
memread (const String &filename,
         size_t       *lengthp)
{
  FILE *file = fopen (filename.c_str(), "r");
  if (!file)
    {
      *lengthp = 0;
      return strdup ("");
    }
  char *contents = file_memread (file, lengthp);
  int savederr = errno;
  fclose (file);
  errno = savederr;
  return contents;
}

void
memfree (char *memread_mem)
{
  if (memread_mem)
    free (memread_mem);
}

bool
memwrite (const String &filename, size_t len, const uint8 *bytes)
{
  FILE *file = fopen (filename.c_str(), "w");
  if (!file)
    return false;
  const size_t nbytes = fwrite (bytes, 1, len, file);
  bool success = ferror (file) == 0 && nbytes == len;
  success = fclose (file) == 0 && success;
  if (!success)
    unlink (filename.c_str());
  return success;
}

} // Path

/* --- DataList --- */
DataList::NodeBase::~NodeBase ()
{}

void
DataList::set_data (NodeBase *node)
{
  /* delete old node */
  NodeBase *it = rip_data (node->key);
  if (it)
    delete it;
  /* prepend node */
  node->next = nodes;
  nodes = node;
}

DataList::NodeBase*
DataList::get_data (DataKey<void> *key) const
{
  NodeBase *it;
  for (it = nodes; it; it = it->next)
    if (it->key == key)
      return it;
  return NULL;
}

DataList::NodeBase*
DataList::rip_data (DataKey<void> *key)
{
  NodeBase *last = NULL, *it;
  for (it = nodes; it; last = it, it = last->next)
    if (it->key == key)
      {
        /* unlink existing node */
        if (last)
          last->next = it->next;
        else
          nodes = it->next;
        it->next = NULL;
        return it;
      }
  return NULL;
}

void
DataList::clear_like_destructor()
{
  while (nodes)
    {
      NodeBase *it = nodes;
      nodes = it->next;
      it->next = NULL;
      delete it;
    }
}

DataList::~DataList()
{
  clear_like_destructor();
}

/* --- url handling --- */
bool
url_show (const char *url)
{
  static struct {
    const char   *prg, *arg1, *prefix, *postfix;
    bool          asyncronous; /* start asyncronously and check exit code to catch launch errors */
    volatile bool disabled;
  } www_browsers[] = {
    /* program */               /* arg1 */      /* prefix+URL+postfix */
    /* configurable, working browser launchers */
    { "gnome-open",             NULL,           "", "", 0 }, /* opens in background, correct exit_code */
    { "exo-open",               NULL,           "", "", 0 }, /* opens in background, correct exit_code */
    /* non-configurable working browser launchers */
    { "kfmclient",              "openURL",      "", "", 0 }, /* opens in background, correct exit_code */
    { "gnome-moz-remote",       "--newwin",     "", "", 0 }, /* opens in background, correct exit_code */
#if 0
    /* broken/unpredictable browser launchers */
    { "browser-config",         NULL,            "", "", 0 }, /* opens in background (+ sleep 5), broken exit_code (always 0) */
    { "xdg-open",               NULL,            "", "", 0 }, /* opens in foreground (first browser) or background, correct exit_code */
    { "sensible-browser",       NULL,            "", "", 0 }, /* opens in foreground (first browser) or background, correct exit_code */
    { "htmlview",               NULL,            "", "", 0 }, /* opens in foreground (first browser) or background, correct exit_code */
#endif
    /* direct browser invocation */
    { "x-www-browser",          NULL,           "", "", 1 }, /* opens in foreground, browser alias */
    { "firefox",                NULL,           "", "", 1 }, /* opens in foreground, correct exit_code */
    { "mozilla-firefox",        NULL,           "", "", 1 }, /* opens in foreground, correct exit_code */
    { "mozilla",                NULL,           "", "", 1 }, /* opens in foreground, correct exit_code */
    { "konqueror",              NULL,           "", "", 1 }, /* opens in foreground, correct exit_code */
    { "opera",                  "-newwindow",   "", "", 1 }, /* opens in foreground, correct exit_code */
    { "galeon",                 NULL,           "", "", 1 }, /* opens in foreground, correct exit_code */
    { "epiphany",               NULL,           "", "", 1 }, /* opens in foreground, correct exit_code */
    { "amaya",                  NULL,           "", "", 1 }, /* opens in foreground, correct exit_code */
    { "dillo",                  NULL,           "", "", 1 }, /* opens in foreground, correct exit_code */
  };
  uint i;
  for (i = 0; i < ARRAY_SIZE (www_browsers); i++)
    if (!www_browsers[i].disabled)
      {
        char *args[128] = { 0, };
        uint n = 0;
        args[n++] = (char*) www_browsers[i].prg;
        if (www_browsers[i].arg1)
          args[n++] = (char*) www_browsers[i].arg1;
        char *string = g_strconcat (www_browsers[i].prefix, url, www_browsers[i].postfix, NULL);
        args[n] = string;
        GError *error = NULL;
        char fallback_error[64] = "Ok";
        bool success;
        if (!www_browsers[i].asyncronous) /* start syncronously and check exit code */
          {
            int exit_status = -1;
            success = g_spawn_sync (NULL, /* cwd */
                                    args,
                                    NULL, /* envp */
                                    G_SPAWN_SEARCH_PATH,
                                    NULL, /* child_setup() */
                                    NULL, /* user_data */
                                    NULL, /* standard_output */
                                    NULL, /* standard_error */
                                    &exit_status,
                                    &error);
            success = success && !exit_status;
            if (exit_status)
              g_snprintf (fallback_error, sizeof (fallback_error), "exitcode: %u", exit_status);
          }
        else
          success = g_spawn_async (NULL, /* cwd */
                                   args,
                                   NULL, /* envp */
                                   G_SPAWN_SEARCH_PATH,
                                   NULL, /* child_setup() */
                                   NULL, /* user_data */
                                   NULL, /* child_pid */
                                   &error);
        g_free (string);
        RAPICORN_KEY_DEBUG ("URL", "show \"%s\": %s: %s", url, args[0], error ? error->message : fallback_error);
        g_clear_error (&error);
        if (success)
          return true;
        www_browsers[i].disabled = true;
      }
  /* reset all disabled states if no browser could be found */
  for (i = 0; i < ARRAY_SIZE (www_browsers); i++)
    www_browsers[i].disabled = false;
  return false;
}

/* --- zintern support --- */
#include <zlib.h>

/**
 * @param decompressed_size exact size of the decompressed data to be returned
 * @param cdata             compressed data block
 * @param cdata_size        exact size of the compressed data block
 * @returns                 decompressed data block or NULL in low memory situations
 *
 * Decompress the data from @a cdata of length @a cdata_size into a newly
 * allocated block of size @a decompressed_size which is returned.
 * The returned block needs to be freed with g_free().
 * This function is intended to decompress data which has been compressed
 * with the rapidres utility, so no errors should occour during decompression.
 * Consequently, if any error occours during decompression or if the resulting
 * data block is of a size other than @a decompressed_size, the program will
 * abort with an appropriate error message.
 * If not enough memory could be allocated for decompression, NULL is returned.
 */
uint8*
zintern_decompress (unsigned int          decompressed_size,
                    const unsigned char  *cdata,
                    unsigned int          cdata_size)
{
  uLongf dlen = decompressed_size;
  uint64 len = dlen + 1;
  uint8 *text = (uint8*) g_try_malloc (len);
  if (!text)
    return NULL;        /* handle ENOMEM gracefully */
  
  int64 result = uncompress (text, &dlen, cdata, cdata_size);
  const char *err;
  switch (result)
    {
    case Z_OK:
      if (dlen == decompressed_size)
        {
          err = NULL;
          break;
        }
      /* fall through */
    case Z_DATA_ERROR:
      err = "internal data corruption";
      break;
    case Z_MEM_ERROR:
      err = "out of memory";
      g_free (text);
      return NULL;      /* handle ENOMEM gracefully */
      break;
    case Z_BUF_ERROR:
      err = "insufficient buffer size";
      break;
    default:
      err = "unknown error";
      break;
    }
  if (err)
    fatal ("failed to decompress (%p, %u): %s", cdata, cdata_size, err);

  text[dlen] = 0;
  return text;          /* success */
}

void
zintern_free (uint8 *dc_data)
{
  g_free (dc_data);
}

} // Rapicorn
