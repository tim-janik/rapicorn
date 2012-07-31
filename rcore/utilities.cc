// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "utilities.hh"
#include "main.hh"
#include "strings.hh"
#include "rapicornutf8.hh"
#include "thread.hh"
#include "rapicornmsg.hh"
#include "rapicorncpu.hh"
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>  // gettid
#include <cxxabi.h>
#include <signal.h>
#include <glib.h>
#include <vector>
#include <algorithm>
#include <cstring>
#include <syslog.h>
#include <execinfo.h>
#include <stdexcept>

#if !__GNUC_PREREQ (3, 4) || (__GNUC__ == 3 && __GNUC_MINOR__ == 4 && __GNUC_PATCHLEVEL__ < 6)
#error This GNU C++ compiler version is known to be broken - please consult ui/README
#endif

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

static clockid_t monotonic_clockid = CLOCK_REALTIME;
static uint64    monotonic_start = 0;
static uint64    monotonic_resolution = 1000;   // assume 1µs resolution for gettimeofday fallback
static uint64    realtime_start = 0;

static void
timestamp_init_ ()
{
  do_once
    {
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
    }
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
  const String fsecs = string_printf ("%zu", size_t (stamp) / 1000000);
  const String usecs = string_printf ("%06zu", size_t (stamp) % 1000000);
  String r = fsecs;
  if (r.size() < fieldwidth)
    r += '.';
  if (r.size() < fieldwidth)
    r += usecs.substr (0, fieldwidth - r.size());
  return r;
}

// == KeyConfig ==
struct KeyConfig {
  typedef std::map<String, String> StringMap;
  Mutex         m_mutex;
  StringMap     m_map;
public:
  void          assign    (const String &key, const String &val);
  String        slookup   (const String &key, const String &dfl = "");
  void          configure (const String &colon_options, bool &seen_debug_key);
};
void
KeyConfig::assign (const String &key, const String &val)
{
  ScopedLock<Mutex> locker (m_mutex);
  m_map[key] = val;
}
String
KeyConfig::slookup (const String &ckey, const String &dfl)
{
  // alias: "verbose" -> "debug"
  String key = ckey == "verbose" ? "debug" : ckey;
  ScopedLock<Mutex> locker (m_mutex);
  StringMap::iterator it = m_map.find (key);
  return it == m_map.end() ? dfl : it->second;
}
void
KeyConfig::configure (const String &colon_options, bool &seen_debug_key)
{
  ScopedLock<Mutex> locker (m_mutex);
  vector<String> onames, ovalues;
  string_options_split (colon_options, onames, ovalues, "1");
  for (size_t i = 0; i < onames.size(); i++)
    {
      String key = onames[i], val = ovalues[i];
      std::transform (key.begin(), key.end(), key.begin(), ::tolower);
      if (key.compare (0, 3, "no-") == 0)
        {
          key = key.substr (3);
          val = string_from_int (!string_to_bool (val));
        }
      if (key.compare (0, 6, "debug-") == 0 && key.size() > 6)
        seen_debug_key = seen_debug_key || string_to_bool (val);
      // alias: "verbose" -> "debug", "V=1" -> "debug", "V=0" -> "no-debug"
      if (key == "verbose" || (key == "v" && isdigit (string_lstrip (val).c_str()[0])))
        key = "debug";
      m_map[key] = val;
    }
}

// === Logging ===
bool                        _debug_flag = true; // bootup default before _init()
bool                        _devel_flag = RAPICORN_DEVEL_VERSION; // bootup default before _init()
static Atomic<char>         conftest_general_debugging = false;
static Atomic<char>         conftest_key_debugging = false;
static Atomic<KeyConfig*>   conftest_map = NULL;
static const char   * const conftest_defaults = "fatal-syslog=1:syslog=0:fatal-warnings=0";

static inline KeyConfig&
conftest_procure ()
{
  KeyConfig *kconfig = conftest_map.load();
  if (UNLIKELY (kconfig == NULL))
    {
      kconfig = new KeyConfig();
      bool dummy = 0;
      kconfig->configure (conftest_defaults, dummy);
      if (conftest_map.cas (NULL, kconfig))
        {
          const char *env_rapicorn = getenv ("RAPICORN");
          // configure with support for aliases, caches, etc
          debug_configure (env_rapicorn ? env_rapicorn : "");
        }
      else
        {
          delete kconfig; // some other thread was faster
          kconfig = conftest_map.load();
        }
    }
  return *kconfig;
}

void
debug_configure (const String &options)
{
  bool seen_debug_key = false;
  conftest_procure().configure (options, seen_debug_key);
  if (seen_debug_key)
    conftest_key_debugging.store (true);
  conftest_general_debugging.store (debug_confbool ("verbose") || debug_confbool ("debug-all"));
  Lib::atomic_store (&_debug_flag, bool (conftest_key_debugging.load() | conftest_general_debugging.load())); // update "cached" configuration
  Lib::atomic_store (&_devel_flag, debug_confbool ("devel", RAPICORN_DEVEL_VERSION)); // update "cached" configuration
  if (debug_confbool ("help"))
    do_once { printerr ("%s", debug_help().c_str()); }
}

static std::list<DebugEntry*> *debug_entries;

void
DebugEntry::dbe_list (DebugEntry *e, int what)
{
  do_once {
    static uint64 space[sizeof (*debug_entries) / sizeof (uint64)];
    debug_entries = new (space) std::list<DebugEntry*>();
  }
  if (what > 0)
    debug_entries->push_back (e);
  else if (what < 0)
    debug_entries->remove (e);
}

String
debug_help ()
{
  String s;
  s += "$RAPICORN - Environment variable for debugging and configuration.\n";
  s += "Colon seperated options can be listed here, listing an option enables it,\n";
  s += "prefixing it with 'no-' disables it. Option assignments are supported with\n";
  s += "the syntax 'option=VALUE'. The supported options are:\n";
  s += "  :help:            Provide a brief description for this environment variable.\n";
  s += "  :debug:           Enable general debugging messages, similar to :verbose:.\n";
  s += "  :debug-KEY:       Enable special purpose debugging, denoted by 'KEY'.\n";
  s += "  :debug-all:       Enable general and all special purpose debugging messages.\n";
  s += "  :devel:           Enable development version behavior.\n";
  s += "  :verbose:         Enable general information and diagnostic messages.\n";
  s += "  :no-fatal-syslog: Disable logging of fatal conditions through syslog(3).\n";
  s += "  :syslog:          Enable logging of general messages through syslog(3).\n";
  s += "  :fatal-warnings:  Cast all warning messages into fatal conditions.\n";
  for (auto it : *debug_entries)
    {
      String k = string_printf ("  :%s:", it->key);
      if (!it->blurb)
        s += k + "\n";
      else if (k.size() >= 20)
        s += k + "\n" + string_multiply (" ", 20) + it->blurb + "\n";
      else
        s += k + string_multiply (" ", 20 - k.size()) + it->blurb + "\n";
    }
  return s;
}

String
debug_confstring (const String &option, const String &vdefault)
{
  String key = option;
  std::transform (key.begin(), key.end(), key.begin(), ::tolower);
  return conftest_procure().slookup (key, vdefault);
}

bool
debug_confbool (const String &option, bool vdefault)
{
  return string_to_bool (debug_confstring (option, string_from_int (vdefault)));
}

int64
debug_confnum (const String &option, int64 vdefault)
{
  return string_to_int (debug_confstring (option, string_from_int (vdefault)));
}

bool
debug_key_enabled (const char *key)
{
  String dkey = key ? key : "";
  std::transform (dkey.begin(), dkey.end(), dkey.begin(), ::tolower);
  const bool keycheck = !key || debug_confbool ("debug-" + dkey) || // key selected
                        (debug_confbool ("debug-all") &&            // all keys enabled by default
                         debug_confbool ("debug-" + dkey, true));   // ensure key was not deselected
  return keycheck;
}

static int
thread_pid()
{
  int tid = -1;
#if     defined (__linux__) && defined (__NR_gettid)    /* present on linux >= 2.4.20 */
  tid = syscall (__NR_gettid);
#endif
  if (tid < 0)
    tid = getpid();
  return tid;
}

static void
ensure_openlog()
{
  do_once { openlog (NULL, LOG_PID, LOG_USER); } // force pid logging
}

static inline int
logtest (const char **kindp, const char *mode, int advance, int flags)
{
  if (strcmp (*kindp, mode) == 0)
    {
      *kindp += advance;
      return flags;
    }
  return 0;
}

static String
dbg_prefix (const String &fileline)
{
  // reduce "foo/bar.c:77" to "bar"
  String cxxstring = fileline;
  char *string = &cxxstring[0];
  char *d = strrchr (string, '.');
  if (d)
    {
      char *s = strrchr (string, DIR_SEPARATOR);
      if (d > s) // strip ".c:77"
        {
          *d = 0;
          return s ? s + 1 : string;
        }
    }
  return fileline;
}

// export debug_handler() symbol for debugging stacktraces and gdb
extern void debug_handler (const char, const String&, const String&, const char *key = NULL) __attribute__ ((noinline));

void // internal function, this + caller are skipped in backtraces
debug_handler (const char dkind, const String &file_line, const String &message, const char *key)
{
  /* The logging system must work before Rapicorn is initialized, and possibly even during
   * global_ctor phase. So any initialization needed here needs to be handled on demand.
   */
  String msg = message;
  const char kind = toupper (dkind);
  enum { DO_STDERR = 1, DO_SYSLOG = 2, DO_ABORT = 4, DO_DEBUG = 8, DO_ERRNO = 16,
         DO_STAMP = 32, DO_LOGFILE = 64, DO_FIXIT = 128, DO_BACKTRACE = 256, };
  static int conftest_logfd = 0;
  const String conftest_logfile = conftest_logfd == 0 ? conftest_procure().slookup ("logfile") : "";
  const int FATAL_SYSLOG = debug_confbool ("fatal-syslog") ? DO_SYSLOG : 0;
  const int MAY_SYSLOG = debug_confbool ("syslog") ? DO_SYSLOG : 0;
  const int MAY_ABORT  = debug_confbool ("fatal-warnings") ? DO_ABORT  : 0;
  const char *what = "DEBUG";
  int f = islower (dkind) ? DO_ERRNO : 0;                       // errno checks for lower-letter kinds
  switch (kind)
    {
    case 'F': what = "FATAL";    f |= DO_STDERR | FATAL_SYSLOG | DO_ABORT;  break;      // fatal, assertions
    case 'C': what = "CRITICAL"; f |= DO_STDERR | MAY_SYSLOG   | MAY_ABORT; break;      // critical
    case 'X': what = "FIX""ME";  f |= DO_FIXIT  | DO_STAMP;                 break;      // fixing needed
    case 'D': what = "DEBUG";    f |= DO_DEBUG  | DO_STAMP;                 break;      // debug
    case 'K': what = "DEBUG";    f |= DO_DEBUG  | DO_STAMP;                 break;      // debug with key
    }
  f |= conftest_logfd > 0 || !conftest_logfile.empty() ? DO_LOGFILE : 0;
  f |= (f & DO_ABORT) ? DO_BACKTRACE : 0;
  const String where = file_line + (file_line.empty() ? "" : ": ");
  const String wherewhat = where + what + ": ";
  String emsg = "\n"; // (f & DO_ERRNO ? ": " + string_from_errno (saved_errno) : "")
  if (kind == 'A')
    msg = "assertion failed: " + msg;
  else if (kind == 'U')
    msg = "assertion must not be reached" + String (msg.empty() ? "" : ": ") + msg;
  else if (kind == 'I')
    msg = "condition failed: " + msg;
  const uint64 start = timestamp_startup(), delta = max (timestamp_realtime(), start) - start;
  if (f & DO_STAMP)
    do_once {
      printerr ("[%s] %s[%u]: program started at: %.6f\n",
                timestamp_format (delta).c_str(),
                program_alias().c_str(), thread_pid(),
                start / 1000000.0);
    }
  if (f & DO_DEBUG)
    {
      String intro, prefix = key ? key : dbg_prefix (file_line);
      if (prefix.size())
        prefix = prefix + ": ";
      if (f & DO_STAMP)
        intro = string_printf ("[%s]", timestamp_format (delta).c_str());
      printerr ("%s %s%s%s", intro.c_str(), prefix.c_str(), msg.c_str(), emsg.c_str());
    }
  if (f & DO_FIXIT)
    {
      printerr ("%s: %s%s%s", what, where.c_str(), msg.c_str(), emsg.c_str());
    }
  if (f & DO_STDERR)
    {
      printerr ("%s[%u]:%s%s%s", program_alias().c_str(), thread_pid(), wherewhat.c_str(), msg.c_str(), emsg.c_str());
      if (f & DO_ABORT)
        printerr ("Aborting...\n");
    }
  if (f & DO_SYSLOG)
    {
      ensure_openlog();
      String end = emsg;
      if (!end.empty() && end[end.size()-1] == '\n')
        end.resize (end.size()-1);
      const int level = f & DO_ABORT ? LOG_ERR : LOG_WARNING;
      const String severity = (f & DO_ABORT) ? "ABORTING: " : "";
      syslog (level, "%s%s%s", severity.c_str(), msg.c_str(), end.c_str());
    }
  String btmsg;
  if (f & DO_BACKTRACE)
    {
      size_t addr;
      const vector<String> syms = pretty_backtrace (2, &addr);
      btmsg = string_printf ("%sBacktrace at 0x%08zx (stackframe at 0x%08zx):\n", where.c_str(),
                             addr, size_t (__builtin_frame_address (0)) /*size_t (&addr)*/);
      for (size_t i = 0; i < syms.size(); i++)
        btmsg += string_printf ("  %s\n", syms[i].c_str());
    }
  if (f & DO_LOGFILE)
    {
      String out;
      do_once
        {
          int fd;
          do
            fd = open (Path::abspath (conftest_logfile).c_str(), O_WRONLY | O_CREAT | O_APPEND | O_NOCTTY, 0666);
          while (conftest_logfd < 0 && errno == EINTR);
          if (fd == 0) // invalid initialization value
            {
              fd = dup (fd);
              close (0);
            }
          out = string_printf ("[%s] %s[%u]: program started at: %s\n",
                               timestamp_format (delta).c_str(), program_alias().c_str(), thread_pid(),
                               timestamp_format (start).c_str());
          conftest_logfd = fd;
        }
      out += string_printf ("[%s] %s[%u]:%s%s%s",
                            timestamp_format (delta).c_str(), program_alias().c_str(), thread_pid(),
                            wherewhat.c_str(), msg.c_str(), emsg.c_str());
      if (f & DO_ABORT)
        out += "aborting...\n";
      int err;
      do
        err = write (conftest_logfd, out.data(), out.size());
      while (err < 0 && (errno == EINTR || errno == EAGAIN));
      if (f & DO_BACKTRACE)
        do
          err = write (conftest_logfd, btmsg.data(), btmsg.size());
        while (err < 0 && (errno == EINTR || errno == EAGAIN));
    }
  if (f & DO_BACKTRACE)
    printerr ("%s", btmsg.c_str());
  if (f & DO_ABORT)
    {
      ::abort();
    }
}

void
debug_assert (const char *file, const int line, const char *message)
{
  debug_handler ('C', string_printf ("%s:%d", file, line), string_printf ("assertion failed: %s", message));
}

void
debug_fassert (const char *file, const int line, const char *message)
{
  debug_handler ('F', string_printf ("%s:%d", file, line), string_printf ("assertion failed: %s", message));
  ::abort();
}

void
debug_fatal (const char *file, const int line, const char *format, ...)
{
  va_list vargs;
  va_start (vargs, format);
  String msg = string_vprintf (format, vargs);
  va_end (vargs);
  debug_handler ('F', string_printf ("%s:%d", file, line), msg);
  ::abort();
}

void
debug_critical (const char *file, const int line, const char *format, ...)
{
  va_list vargs;
  va_start (vargs, format);
  String msg = string_vprintf (format, vargs);
  va_end (vargs);
  debug_handler ('C', string_printf ("%s:%d", file, line), msg);
}

void
debug_fixit (const char *file, const int line, const char *format, ...)
{
  va_list vargs;
  va_start (vargs, format);
  String msg = string_vprintf (format, vargs);
  va_end (vargs);
  debug_handler ('X', string_printf ("%s:%d", file, line), msg);
}

void
debug_general (const char *file, const int line, const char *format, ...)
{
  if (!conftest_general_debugging)
    return;
  va_list vargs;
  va_start (vargs, format);
  String msg = string_vprintf (format, vargs);
  va_end (vargs);
  debug_handler ('D', string_printf ("%s:%d", file, line), msg);
}

void
debug_keymsg (const char *file, const int line, const char *key, const char *format, ...)
{
  if (!debug_enabled (key))
    return;
  va_list vargs;
  va_start (vargs, format);
  String msg = string_vprintf (format, vargs);
  va_end (vargs);
  debug_handler ('K', string_printf ("%s:%d", file, line), msg, key);
}

const char*
strerror (void)
{
  return strerror (errno);
}

const char*
strerror (int errnum)
{
  return ::strerror (errnum);
}

std::vector<std::string>
pretty_backtrace (uint level, size_t *parent_addr)
{
  void *ptrbuffer[1024];
  const int nptrs = backtrace (ptrbuffer, ARRAY_SIZE (ptrbuffer));
  if (parent_addr)
    *parent_addr = nptrs >= 1 + int (level) ? size_t (ptrbuffer[1 + level]) : 0;
  char **btsymbols = backtrace_symbols (ptrbuffer, nptrs);
  std::vector<std::string> symbols;
  if (btsymbols)
    {
      for (int i = 1 + level; i < nptrs; i++) // skip current function plus some
        {
          char dso[1234+1] = "???", fsym[1234+1] = "???"; size_t addr = size_t (ptrbuffer[i]);
          // parse: ./executable(_MangledFunction+0x00b33f) [0x00deadbeef]
          sscanf (btsymbols[i], "%1234[^(]", dso);
          sscanf (btsymbols[i], "%*[^(]%*[(]%1234[^)+-]", fsym);
          std::string func;
          if (fsym[0])
            {
              int status = 0;
              char *demang = abi::__cxa_demangle (fsym, NULL, NULL, &status);
              func = status == 0 && demang ? demang : fsym + std::string (" ()");
              if (demang)
                free (demang);
            }
          const char *dsof = strrchr (dso, '/');
          dsof = dsof ? dsof + 1 : dso;
          char buffer[2048] = { 0 };
          snprintf (buffer, sizeof (buffer), "0x%08zx %-26s %s", addr, dsof, func.c_str());
          symbols.push_back (buffer);
          if (strcmp (fsym, "main") == 0 || strcmp (fsym, "__libc_start_main") == 0)
            break;
        }
      free (btsymbols);
    }
  return symbols;
}

// === User Messages ==
/// Capture the filename and line number of a user provided resource file.
UserSource::UserSource (String _filename, int _lineno) :
  filename (_filename), lineno (_lineno)
{}

static void
user_message (const UserSource &source, const String &kind, const String &message)
{
  String fname, mkind, pname = program_alias();
  if (!pname.empty())
    pname += ":";
  if (!kind.empty())
    mkind = " " + kind + ":";
  if (!source.filename.empty())
    fname = source.filename + ":";
  if (source.lineno)
    fname = fname + string_printf ("%d:", source.lineno);
  // obey GNU warning style to allow automated location parsing
  printerr ("%s%s%s %s\n", pname.c_str(), fname.c_str(), mkind.c_str(), message.c_str());
}

/// Issue a notice about user resources.
void
user_notice (const UserSource &source, const char *format, ...)
{
  va_list vargs;
  va_start (vargs, format);
  String msg = string_vprintf (format, vargs);
  va_end (vargs);
  user_message (source, "", msg);
}

/// Issue a warning about user resources that likely need fixing.
void
user_warning (const UserSource &source, const char *format, ...)
{
  va_list vargs;
  va_start (vargs, format);
  String msg = string_vprintf (format, vargs);
  va_end (vargs);
  user_message (source, "warning", msg);
}

// == AssertionError ==
static String
construct_error_msg (const String &file, size_t line, const char *error, const String &expr)
{
  String s;
  if (!file.empty())
    {
      s += file;
      if (line > 0)
        s += ":" + string_from_int (line);
      s += ": ";
    }
  s += error;
  if (!expr.empty())
    {
      s += ": ";
      s += expr;
    }
  return s;
}

AssertionError::AssertionError (const String &expr, const String &file, size_t line) :
  m_msg (construct_error_msg (file, line, "assertion failed", expr))
{}

AssertionError::~AssertionError () throw()
{}

const char*
AssertionError::what () const throw()
{
  return m_msg.c_str();
}

// == Development Helpers ==
/**
 * @def RAPICORN_STRLOC()
 * Returns a string describing the current source code location, such as FILE and LINE number.
 */
/**
 * @def STRLOC()
 * Shorthand for RAPICORN_STRLOC() if RAPICORN_CONVENIENCE is defined.
 */

/**
 * @def RAPICORN_RETURN_IF(expr [, rvalue])
 * Silently return @a rvalue if expression @a expr evaluates to true. Returns void if @a rvalue was not specified.
 */
/**
 * @def return_if(expr [, rvalue])
 * Shorthand for RAPICORN_RETURN_IF() if RAPICORN_CONVENIENCE is defined.
 */

/**
 * @def RAPICORN_RETURN_UNLESS(expr [, rvalue])
 * Silently return @a rvalue if expression @a expr evaluates to false. Returns void if @a rvalue was not specified.
 */
/**
 * @def return_unless(expr [, rvalue])
 * Shorthand for RAPICORN_RETURN_UNLESS() if RAPICORN_CONVENIENCE is defined.
 */

/**
 * @def RAPICORN_FATAL(format,...)
 * Issues an error message, and aborts the program. The error message @a format uses printf-like syntax.
 */
/**
 * @def fatal(format,...)
 * Shorthand for RAPICORN_FATAL() if RAPICORN_CONVENIENCE is defined.
 */

/**
 * @def RAPICORN_ASSERT(cond)
 * Issue an error and abort the program if expression @a cond evaluates to false.
 */
/**
 * @def assert(cond)
 * Shorthand for RAPICORN_ASSERT() if RAPICORN_CONVENIENCE is defined.
 */

/**
 * @def RAPICORN_ASSERT_RETURN(cond [, rvalue])
 * Issue an error and return @a rvalue if expression @a cond evaluates to false. Returns void if @a rvalue was not specified.
 * This is normally used as function entry condition.
 */
/**
 * @def assert_return(cond [, rvalue])
 * Shorthand for RAPICORN_ASSERT_RETURN() if RAPICORN_CONVENIENCE is defined.
 */

/**
 * @def RAPICORN_ASSERT_UNREACHED()
 * Issues an error message, and aborts the program if it is reached at runtime.
 * This is normally used to label code conditions intended to be unreachable.
 */
/**
 * @def assert_unreached
 * Shorthand for RAPICORN_ASSERT_UNREACHED() if RAPICORN_CONVENIENCE is defined.
 */

/**
 * @def RAPICORN_CRITICAL(format,...)
 * Issues an error message, and aborts the program if it was started with RAPICORN=fatal-criticals.
 * The error message @a format uses printf-like syntax.
 */
/**
 * @def critical(format,...)
 * Shorthand for RAPICORN_CRITICAL() if RAPICORN_CONVENIENCE is defined.
 */

/**
 * @def RAPICORN_CRITICAL_UNLESS(cond)
 * Behaves like RAPICORN_CRITICAL() if expression @a cond evaluates to false.
 * This is normally used as a non-fatal assertion.
 */
/**
 * @def critical_unless(cond)
 * Shorthand for RAPICORN_CRITICAL_UNLESS() if RAPICORN_CONVENIENCE is defined.
 */

/**
 * @def RAPICORN_DEBUG(format,...)
 * Issues an debugging message if the program was started with RAPICORN=debug.
 * The message @a format uses printf-like syntax.
 */
/**
 * @def DEBUG(format,...)
 * Shorthand for RAPICORN_DEBUG() if RAPICORN_CONVENIENCE is defined.
 */

/**
 * @def RAPICORN_KEY_DEBUG(key, format,...)
 * Issues an debugging message if the program was started with RAPICORN=debug-<KEY>, where <KEY> is given as @a key.
 * The message @a format uses printf-like syntax.
 */
/**
 * @def KEY_DEBUG(format,...)
 * Shorthand for RAPICORN_KEY_DEBUG() if RAPICORN_CONVENIENCE is defined.
 */

/**
 * @def RAPICORN_THROW_IF_FAIL(expr)
 * Throws an AssertionError exception with error message if expression @a expr evaluates to false.
 * This is normally used as function entry condition.
 */
/**
 * @def throw_if_fail
 * Shorthand for RAPICORN_THROW_IF_FAIL() if RAPICORN_CONVENIENCE is defined.
 */


// == utilities ==
void
printerr (const char *format, ...)
{
  va_list args;
  va_start (args, format);
  String ers = string_vprintf (format, args);
  va_end (args);
  fflush (stdout);
  fputs (ers.c_str(), stderr);
  fflush (stderr);
}

void
printout (const char *format, ...)
{
  va_list args;
  va_start (args, format);
  String ers = string_vprintf (format, args);
  va_end (args);
  fflush (stderr);
  fputs (ers.c_str(), stdout);
  fflush (stdout);
}

/* --- process handle --- */
static struct {
  pid_t pid, ppid;
  uid_t uid;
  struct timeval tv;
  struct timezone tz;
  struct utsname uts;
} process_info = { 0, };
static bool
seed_buffer (vector<uint8> &randbuf)
{
  bool have_random = false;

  static const char *partial_files[] = { "/dev/urandom", "/dev/urandom$", };
  for (uint i = 0; i < ARRAY_SIZE (partial_files) && !have_random; i++)
    {
      FILE *file = fopen (partial_files[i], "r");
      if (file)
        {
          const uint rsz = 16;
          size_t sz = randbuf.size();
          randbuf.resize (randbuf.size() + rsz);
          have_random = rsz == fread (&randbuf[sz], 1, rsz, file);
          fclose (file);
        }
    }

  static const char *full_files[] = { "/proc/stat", "/proc/interrupts", "/proc/uptime", };
  for (uint i = 0; i < ARRAY_SIZE (full_files) && !have_random; i++)
    {
      size_t len = 0;
      char *mem = Path::memread (full_files[i], &len);
      if (mem)
        {
          randbuf.insert (randbuf.end(), mem, mem + len);
          free (mem);
          have_random = len > 0;
        }
    }

#ifdef __OpenBSD__
  if (!have_random)
    {
      randbuf.insert (randbuf.end(), arc4random());
      randbuf.insert (randbuf.end(), arc4random() >> 8);
      randbuf.insert (randbuf.end(), arc4random() >> 16);
      randbuf.insert (randbuf.end(), arc4random() >> 24);
      have_random = true;
    }
#endif

  return have_random;
}

static uint32 process_hash = 0;
static uint64 locatable_process_hash64 = 0;

static void
process_handle_and_seed_init (const StringVector &args)
{
  /* function should be called only once */
  assert (process_info.pid == 0);

  /* gather process info */
  process_info.pid = getpid();
  process_info.ppid = getppid();
  process_info.uid = getuid();
  gettimeofday (&process_info.tv, &process_info.tz);
  uname (&process_info.uts);

  /* gather random system entropy */
  vector<uint8> randbuf;
  randbuf.insert (randbuf.end(), (uint8*) &process_info, (uint8*) (&process_info + 1));

  /* mangle entropy into system specific hash */
  uint64 phash64 = 0;
  for (uint i = 0; i < randbuf.size(); i++)
    phash64 = (phash64 << 5) - phash64 + randbuf[i];
  process_hash = phash64 % 4294967291U;
  locatable_process_hash64 = (uint64 (process_hash) << 32) + 0xa0000000;

  /* gather random seed entropy, this should include the entropy
   * from process_hash, but not be predictable from it.
   */
  seed_buffer (randbuf);

  /* mangle entropy into random seed */
  uint64 rand_seed = phash64;
  for (uint i = 0; i < randbuf.size(); i++)
    rand_seed = (rand_seed << 5) + rand_seed + randbuf[i];

  /* seed random generators */
  union { uint64 seed64; unsigned short us[3]; } u;
  u.seed64 = rand_seed % 0x1000000000015ULL;    // hash entropy into 48bits
  u.seed64 ^= u.seed64 << 48;                   // keep upper bits (us[0]) filled for big-endian
  srand48 (rand_seed); // will be overwritten by seed48() on sane systems
  seed48 (u.us);
  srand (lrand48());
}
static InitHook _process_handle_and_seed_init ("core/01 Init Process Handle and Seeds", process_handle_and_seed_init);

String
process_handle ()
{
  return string_printf ("%s/%08x", process_info.uts.nodename, process_hash);
}

/* --- VirtualTypeid --- */
VirtualTypeid::~VirtualTypeid ()
{ /* virtual destructor ensures vtable */ }

String
VirtualTypeid::typeid_name ()
{
  return typeid (*this).name();
}

String
VirtualTypeid::typeid_pretty_name ()
{
  return cxx_demangle (typeid (*this).name());
}

String
VirtualTypeid::cxx_demangle (const char *mangled_identifier)
{
  int status = 0;
  char *malloced_result = abi::__cxa_demangle (mangled_identifier, NULL, NULL, &status);
  String result = malloced_result && !status ? malloced_result : mangled_identifier;
  if (malloced_result)
    free (malloced_result);
  return result;
}

/// @namespace Rapicorn::Path The Rapicorn::Path namespace provides functions for file path manipulation and testing.
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
 * @param path  a filename path
 * @param incwd optional current working directory
 *
 * Complete @a path to become an absolute file path. If neccessary, @a incwd or
 * the real current working directory is prepended.
 */
String
abspath (const String &path,
         const String &incwd)
{
  if (isabs (path))
    return path;
  if (!incwd.empty())
    return join (incwd, path);
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

/**
 * Return the current working directoy.
 */
String
cwd ()
{
  char *gpwd = g_get_current_dir();
  String wd = gpwd;
  g_free (gpwd);
  return wd;
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

String
vpath_find (const String &file, const String &mode)
{
  const char *vpath = getenv ("VPATH");
  return searchpath_find (vpath ? vpath : ".", file, mode);
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

} // Path

/**
 * @class ResourceBlob
 * A ResourceBlob provides access to binary data (BLOB = Binary Large OBject) which may be
 * preassembled and compiled into a program or located in a resource path directory.
 * Locators for resources should generally adhere to the form: @code
 *      res: [relative_path/] resource_name
 * @endcode
 * See also: RAPICORN_STATIC_RESOURCE_DATA(), RAPICORN_STATIC_RESOURCE_ENTRY().
 * Example: @SNIPPET{rcore/tests/multitest.cc, ResourceBlob-EXAMPLE}
 */
ResourceBlob::ResourceBlob (const String &name, size_t dsize, std::shared_ptr<const uint8> shdata) :
  m_name (name), m_size (dsize), m_data (shdata)
{}

static ResourceBlob::Entry *res_head = NULL;
static Mutex                res_mutex;

void
ResourceBlob::Entry::reg_add (ResourceBlob::Entry *entry)
{
  assert_return (entry && !entry->next);
  ScopedLock<Mutex> sl (res_mutex);
  entry->next = res_head;
  res_head = entry;
}

ResourceBlob::Entry::~Entry()
{
  ScopedLock<Mutex> sl (res_mutex);
  Entry **ptr = &res_head;
  while (*ptr != this)
    ptr = &(*ptr)->next;
  *ptr = next;
}

const ResourceBlob::Entry*
ResourceBlob::Entry::find_entry (const String &res_name)
{
  ScopedLock<Mutex> sl (res_mutex);
  for (const Entry *e = res_head; e; e = e->next)
    if (res_name == e->name)
      return e;
  return NULL;
}

ResourceBlob
ResourceBlob::load (const String &path)
{
  struct BBlob : ResourceBlob {
    BBlob (const String &name, size_t dsize, std::shared_ptr<const uint8> ddata) :
      ResourceBlob (name, dsize, ddata)
    {}
  };
  const Entry *entry = Entry::find_entry (path);
  struct NopDeleter { void operator() (const uint8*) {} }; // prevent delete on const data
  if (entry && (entry->dsize == entry->psize || entry->dsize + 1 == entry->psize))
    {
      if (entry->dsize + 1 == entry->psize)
        assert (entry->pdata[entry->dsize] == 0);       // 0-terminated char array
      else
        assert (entry->dsize == entry->psize);
      const uint8 *data = reinterpret_cast<const uint8*> (entry->pdata);
      return BBlob (path, entry->dsize, std::shared_ptr<const uint8> (data, NopDeleter()));
    }
  else if (entry && entry->psize && entry->dsize == 0)  // variable length array
    {
      const uint8 *data = reinterpret_cast<const uint8*> (entry->pdata);
      return BBlob (path, entry->psize, std::shared_ptr<const uint8> (data, NopDeleter()));
    }
  else if (entry && entry->psize < entry->dsize)        // compressed
    {
      const uint8 *data = zintern_decompress (entry->dsize, reinterpret_cast<const uint8*> (entry->pdata), entry->psize);
      struct ZinternDeleter { void operator() (const uint8 *d) { zintern_free (const_cast<uint8*> (d)); } };
      return BBlob (path, entry->dsize, std::shared_ptr<const uint8> (data, ZinternDeleter()));
    }
  // FIXME: handle file system lookups for ResourceBlob
  return BBlob (path, 0, std::shared_ptr<const uint8> (nullptr));
}

String
ResourceBlob::string () const
{
  size_t l = size();
  if (l && data()[l - 1] == 0)
    l--; // std::string automatically 0-terminates its contents.
  return std::string ((char*) data(), l);
}

/**
 * @class Deletable
 * Classes derived from @a Deletable need to have a virtual destructor.
 * Handlers can be registered with class instances that are called during
 * instance deletion. This is most useful for automated memory keeping of
 * custom resources attached to the lifetime of a particular instance.
 */
Deletable::~Deletable ()
{
  invoke_deletion_hooks();
}

/**
 * @param deletable     possible Deletable* handle
 * @return              TRUE if the hook was added
 *
 * Adds the deletion hook to @a deletable if it is non NULL.
 * The deletion hook is asserted to be so far uninstalled.
 * This function is MT-safe and may be called from any thread.
 */
bool
Deletable::DeletionHook::deletable_add_hook (Deletable *deletable)
{
  if (deletable)
    {
      deletable->add_deletion_hook (this);
      return true;
    }
  return false;
}

/**
 * @param deletable     possible Deletable* handle
 * @return              TRUE if the hook was removed
 *
 * Removes the deletion hook from @a deletable if it is non NULL.
 * The deletion hook is asserted to be installed on @a deletable.
 * This function is MT-safe and may be called from any thread.
 */
bool
Deletable::DeletionHook::deletable_remove_hook (Deletable *deletable)
{
  if (deletable)
    {
      deletable->remove_deletion_hook (this);
      return true;
    }
  return false;
}

Deletable::DeletionHook::~DeletionHook ()
{
  if (this->next || this->prev)
    fatal ("hook is being destroyed but not unlinked: %p", this);
}

#define DELETABLE_MAP_HASH (19) /* use prime size for hashing, sums up to roughly 1k (use 83 for 4k) */
struct DeletableAuxData {
  Deletable::DeletionHook* hooks;
  DeletableAuxData() : hooks (NULL) {}
  ~DeletableAuxData() { assert (hooks == NULL); }
};
struct DeletableMap {
  Mutex                                 mutex;
  std::map<Deletable*,DeletableAuxData> dmap;
};
typedef std::map<Deletable*,DeletableAuxData>::iterator DMapIterator;
static Atomic<DeletableMap*> deletable_maps = NULL;

static inline void
auto_init_deletable_maps (void)
{
  if (UNLIKELY (deletable_maps.load() == NULL))
    {
      DeletableMap *dmaps = new DeletableMap[DELETABLE_MAP_HASH];
      if (!deletable_maps.cas (NULL, dmaps))
        delete dmaps;
      // ensure threading works
      deletable_maps[0].mutex.lock();
      deletable_maps[0].mutex.unlock();
    }
}

/**
 * @param hook  valid deletion hook
 *
 * Add an uninstalled deletion hook to the deletable.
 * This function is MT-safe and may be called from any thread.
 */
void
Deletable::add_deletion_hook (DeletionHook *hook)
{
  auto_init_deletable_maps();
  const uint32 hashv = ((gsize) (void*) this) % DELETABLE_MAP_HASH;
  deletable_maps[hashv].mutex.lock();
  RAPICORN_ASSERT (hook);
  RAPICORN_ASSERT (!hook->next);
  RAPICORN_ASSERT (!hook->prev);
  DeletableAuxData &ad = deletable_maps[hashv].dmap[this];
  if (!ad.hooks)
    ad.hooks = hook->prev = hook->next = hook;
  else
    {
      hook->prev = ad.hooks->prev;
      hook->next = ad.hooks;
      hook->prev->next = hook;
      hook->next->prev = hook;
      ad.hooks = hook;
    }
  deletable_maps[hashv].mutex.unlock();
  hook->monitoring_deletable (*this);
  //g_printerr ("DELETABLE-ADD(%p,%p)\n", this, hook);
}

/**
 * @param hook  valid deletion hook
 *
 * Remove a previously added deletion hook.
 * This function is MT-safe and may be called from any thread.
 */
void
Deletable::remove_deletion_hook (DeletionHook *hook)
{
  auto_init_deletable_maps();
  const uint32 hashv = ((gsize) (void*) this) % DELETABLE_MAP_HASH;
  deletable_maps[hashv].mutex.lock();
  RAPICORN_ASSERT (hook);
  RAPICORN_ASSERT (hook->next && hook->prev);
  hook->next->prev = hook->prev;
  hook->prev->next = hook->next;
  DMapIterator it = deletable_maps[hashv].dmap.find (this);
  RAPICORN_ASSERT (it != deletable_maps[hashv].dmap.end());
  DeletableAuxData &ad = it->second;
  if (ad.hooks == hook)
    ad.hooks = hook->next != hook ? hook->next : NULL;
  hook->prev = NULL;
  hook->next = NULL;
  deletable_maps[hashv].mutex.unlock();
  //g_printerr ("DELETABLE-REM(%p,%p)\n", this, hook);
}

/**
 * Invoke all deletion hooks installed on this deletable.
 */
void
Deletable::invoke_deletion_hooks()
{
  /* upon program exit, we may get here without deletable maps or even
   * threading being initialized. to avoid calling into a NULL threading
   * table, we'll detect the case and return
   */
  if (NULL == deletable_maps.load())
    return;
  auto_init_deletable_maps();
  const uint32 hashv = ((gsize) (void*) this) % DELETABLE_MAP_HASH;
  while (TRUE)
    {
      /* lookup hook list */
      deletable_maps[hashv].mutex.lock();
      DeletionHook *hooks;
      DMapIterator it = deletable_maps[hashv].dmap.find (this);
      if (it != deletable_maps[hashv].dmap.end())
        {
          DeletableAuxData &ad = it->second;
          hooks = ad.hooks;
          ad.hooks = NULL;
          deletable_maps[hashv].dmap.erase (it);
        }
      else
        hooks = NULL;
      deletable_maps[hashv].mutex.unlock();
      /* we're done if all hooks have been procesed */
      if (!hooks)
        break;
      /* process hooks */
      while (hooks)
        {
          DeletionHook *hook = hooks;
          hook->next->prev = hook->prev;
          hook->prev->next = hook->next;
          hooks = hook->next != hook ? hook->next : NULL;
          hook->prev = hook->next = NULL;
          //g_printerr ("DELETABLE-DISMISS(%p,%p)\n", this, hook);
          hook->dismiss_deletable();
        }
    }
}

/* --- Id Allocator --- */
IdAllocator::IdAllocator ()
{}

IdAllocator::~IdAllocator ()
{}

class IdAllocatorImpl : public IdAllocator {
  Spinlock     mutex;
  const uint   counterstart, wbuffer_capacity;
  uint         counter, wbuffer_pos, wbuffer_size;
  uint        *wbuffer;
  vector<uint> free_ids;
public:
  explicit     IdAllocatorImpl (uint startval, uint wbuffercap);
  virtual     ~IdAllocatorImpl () { delete[] wbuffer; }
  virtual uint alloc_id        ();
  virtual void release_id      (uint unique_id);
  virtual bool seen_id         (uint unique_id);
};

IdAllocator*
IdAllocator::_new (uint startval)
{
  return new IdAllocatorImpl (startval, 97);
}

IdAllocatorImpl::IdAllocatorImpl (uint startval, uint wbuffercap) :
  counterstart (startval), wbuffer_capacity (wbuffercap),
  counter (counterstart), wbuffer_pos (0), wbuffer_size (0)
{
  wbuffer = new uint[wbuffer_capacity];
}

void
IdAllocatorImpl::release_id (uint unique_id)
{
  assert_return (unique_id >= counterstart && unique_id < counter);
  /* protect */
  mutex.lock();
  /* release oldest withheld id */
  if (wbuffer_size >= wbuffer_capacity)
    free_ids.push_back (wbuffer[wbuffer_pos]);
  /* withhold released id */
  wbuffer[wbuffer_pos++] = unique_id;
  wbuffer_size = MAX (wbuffer_size, wbuffer_pos);
  if (wbuffer_pos >= wbuffer_capacity)
    wbuffer_pos = 0;
  /* cleanup */
  mutex.unlock();
}

uint
IdAllocatorImpl::alloc_id ()
{
  uint64 unique_id;
  mutex.lock();
  if (free_ids.empty())
    unique_id = counter++;
  else
    {
      uint64 randomize = uint64 (this) + (uint64 (&randomize) >> 3); // cheap random data
      randomize += wbuffer[wbuffer_pos] + wbuffer_pos + counter; // add entropy
      uint random_pos = randomize % free_ids.size();
      unique_id = free_ids[random_pos];
      free_ids[random_pos] = free_ids.back();
      free_ids.pop_back();
    }
  mutex.unlock();
  return unique_id;
}

bool
IdAllocatorImpl::seen_id (uint unique_id)
{
  mutex.lock();
  bool inrange = unique_id >= counterstart && unique_id < counter;
  mutex.unlock();
  return inrange;
}


/* --- Locatable --- */
#define LOCATOR_ID_OFFSET 0xa0000000
static Spinlock           locatable_mutex;
static vector<Locatable*> locatable_objs;
static IdAllocatorImpl    locator_ids (1, 227); // has own mutex

Locatable::Locatable () :
  m_locatable_index (0)
{}

Locatable::~Locatable ()
{
  if (UNLIKELY (m_locatable_index))
    {
      ScopedLock<Spinlock> locker (locatable_mutex);
      assert_return (m_locatable_index <= locatable_objs.size());
      const uint index = m_locatable_index - 1;
      locatable_objs[index] = NULL;
      locator_ids.release_id (m_locatable_index);
      m_locatable_index = 0;
    }
}

uint64
Locatable::locatable_id () const
{
  if (UNLIKELY (m_locatable_index == 0))
    {
      m_locatable_index = locator_ids.alloc_id();
      const uint index = m_locatable_index - 1;
      ScopedLock<Spinlock> locker (locatable_mutex);
      if (index >= locatable_objs.size())
        locatable_objs.resize (index + 1);
      locatable_objs[index] = const_cast<Locatable*> (this);
    }
  return locatable_process_hash64 + m_locatable_index;
}

Locatable*
Locatable::from_locatable_id (uint64 locatable_id)
{
  if (UNLIKELY (locatable_id == 0))
    return NULL;
  if (locatable_id >> 32 != locatable_process_hash64 >> 32)
    return NULL; // id from wrong process
  const uint index = locatable_id - locatable_process_hash64  - 1;
  ScopedLock<Spinlock> locker (locatable_mutex);
  assert_return (index < locatable_objs.size(), NULL);
  Locatable *_this = locatable_objs[index];
  return _this;
}

/* --- ReferenceCountable --- */
static const size_t stack_proximity_threshold = 4096; // <= page_size on most systems

static __attribute__ ((noinline))
size_t
stack_ptrdiff (const void *stackvariable,
               const void *data)
{
  size_t d = size_t (data);
  size_t s = size_t (stackvariable);
  return MAX (d, s) - MIN (d, s);
}

void
ReferenceCountable::stackcheck (const void *data)
{
  int stackvariable = 0;
  /* this check for ReferenceCountable instance allocations on the stack isn't
   * perfect, but should catch the most common cases for growing and shrinking stacks
   */
  if (stack_ptrdiff (&stackvariable, data) < stack_proximity_threshold)
    fatal ("ReferenceCountable object allocated on stack instead of heap: %zu > %zu (%p - %p)",
           stack_proximity_threshold,
           stack_ptrdiff (&stackvariable, data),
           data, &stackvariable);
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
url_test_show (const char *url)
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
        DEBUG ("show \"%s\": %s: %s", url, args[0], error ? error->message : fallback_error);
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

static void
browser_launch_warning (const char *url)
{
  Msg::display (Msg::WARNING,
                Msg::Title (_("Launch Web Browser")),
                Msg::Text1 (_("Failed to launch a web browser executable")),
                Msg::Text2 (_("No suitable web browser executable could be found to be executed and to display the URL: %s"), url),
                Msg::Check (_("Show messages about web browser launch problems")));
}

void
url_show (const char *url)
{
  bool success = url_test_show (url);
  if (!success)
    browser_launch_warning (url);
}

static void
unlink_file_name (gpointer data)
{
  char *file_name = (char*) data;
  while (unlink (file_name) < 0 && errno == EINTR);
  g_free (file_name);
}

static const gchar*
url_create_redirect (const char    *url,
                     const char    *url_title,
                     const char    *cookie)
{
  const char *ver = "0.5";
  gchar *tname = NULL;
  gint fd = -1;
  while (fd < 0)
    {
      g_free (tname);
      tname = g_strdup_printf ("/tmp/Url%08X%04X.html", (int) lrand48(), getpid());
      fd = open (tname, O_WRONLY | O_CREAT | O_EXCL, 00600);
      if (fd < 0 && errno != EEXIST)
        {
          g_free (tname);
          return NULL;
        }
    }
  char *text = g_strdup_printf ("<!DOCTYPE HTML SYSTEM>\n"
                                "<html><head>\n"
                                "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n"
                                "<meta http-equiv=\"refresh\" content=\"0; URL=%s\">\n"
                                "<meta http-equiv=\"set-cookie\" content=\"%s\">\n"
                                "<title>%s</title>\n"
                                "</head><body>\n"
                                "<h1>%s</h1>\n"
                                "<b>Document Redirection</b><br>\n"
                                "Your browser is being redirected.\n"
                                "If it does not support automatic redirections, try <a href=\"%s\">%s</a>.\n"
                                "<hr>\n"
                                "<address>RapicornUrl/%s file redirect</address>\n"
                                "</body></html>\n",
                                url, cookie, url_title, url_title, url, url, ver);
  int w, c, l = strlen (text);
  do
    w = write (fd, text, l);
  while (w < 0 && errno == EINTR);
  g_free (text);
  do
    c = close (fd);
  while (c < 0 && errno == EINTR);
  if (w != l || c < 0)
    {
      while (unlink (tname) < 0 && errno == EINTR)
        {}
      g_free (tname);
      return NULL;
    }
  cleanup_add (60 * 1000, unlink_file_name, tname); /* free tname */
  return tname;
}

bool
url_test_show_with_cookie (const char *url,
                           const char *url_title,
                           const char *cookie)
{
  const char *redirect = url_create_redirect (url, url_title, cookie);
  if (redirect)
    return url_test_show (redirect);
  else
    return url_test_show (url);
}

void
url_show_with_cookie (const char *url,
                      const char *url_title,
                      const char *cookie)
{
  bool success = url_test_show_with_cookie (url, url_title, cookie);
  if (!success)
    browser_launch_warning (url);
}

/* --- cleanups --- */
typedef struct {
  uint           id;
  GDestroyNotify handler;
  void          *data;
} Cleanup;

static Mutex cleanup_mutex;
static GSList *cleanup_list = NULL;

static void
cleanup_exec_Lm (Cleanup *cleanup)
{
  cleanup_list = g_slist_remove (cleanup_list, cleanup);
  g_source_remove (cleanup->id);
  GDestroyNotify handler = cleanup->handler;
  void *data = cleanup->data;
  g_free (cleanup);
  cleanup_mutex.unlock();
  handler (data);
  cleanup_mutex.lock();
}

/**
 * Force all cleanup handlers (see rapicorn_cleanup_add()) to be immediately
 * executed. This function should be called at program exit to execute
 * cleanup handlers which have timeouts that have not yet expired.
 */
void
cleanup_force_handlers (void)
{
  cleanup_mutex.lock();
  while (cleanup_list)
    cleanup_exec_Lm ((Cleanup*) cleanup_list->data);
  cleanup_mutex.unlock();
}

static gboolean
cleanup_exec (gpointer data)
{
  cleanup_mutex.lock();
  cleanup_exec_Lm ((Cleanup*) data);
  cleanup_mutex.unlock();
  return FALSE;
}

/**
 * @param timeout_ms    timeout in milliseconds
 * @param handler       cleanup handler to run
 * @param data          cleanup handler data
 *
 * Register a cleanup handler, the @a handler is guaranteed to be run
 * asyncronously (i.e. not from within cleanup_add()). The cleanup
 * handler will be called as soon as @a timeout_ms has elapsed or
 * cleanup_force_handlers() is called.
 */
uint
cleanup_add (guint          timeout_ms,
             GDestroyNotify handler,
             void          *data)
{
  Cleanup *cleanup = g_new0 (Cleanup, 1);
  cleanup->handler = handler;
  cleanup->data = data;
  cleanup->id = g_timeout_add (timeout_ms, cleanup_exec, cleanup);
  cleanup_mutex.lock();
  cleanup_list = g_slist_prepend (cleanup_list, cleanup);
  cleanup_mutex.unlock();
  return cleanup->id;
}

/* --- BaseObject --- */
static Mutex                        plor_mutex;
static std::map<String,BaseObject*> plor_map; // Process Local Object Repository

static bool
plor_remove (const String &plor_name)
{
  ScopedLock<Mutex> locker (plor_mutex);
  std::map<String,BaseObject*>::iterator mit = plor_map.find (plor_name);
  if (mit != plor_map.end())
    {
      plor_map.erase (mit);
      return true;
    }
  return false;
}

static const char *plor_subs[] = { // FIXME: need registry
  "models",
  NULL,
};

static bool
plor_add (BaseObject   &object,
          const String &name)
{
  ScopedLock<Mutex> locker (plor_mutex);
  size_t p = name.find ('/');
  if (p != name.npos && p < name.size())
    {
      String s = name.substr (0, p);
      for (uint i = 0; plor_subs[i]; i++)
        if (s == plor_subs[i])
          {
            plor_map[name] = &object;
            return true;
          }
    }
  return false;
}

BaseObject*
BaseObject::plor_get (const String &plor_url)
{
  ScopedLock<Mutex> locker (plor_mutex);
  std::map<String,BaseObject*>::iterator mit = plor_map.find (plor_url);
  if (mit != plor_map.end())
    return mit->second;
  return NULL;
}

static class PlorDataKey : public DataKey<String> {
  virtual void destroy (String data) { plor_remove (data); }
} plor_name_key;

String
BaseObject::plor_name () const
{
  return get_data (&plor_name_key);
}

void
BaseObject::plor_name (const String &_plor_name)
{
  assert_return (plor_name() == "");
  if (plor_add (*this, _plor_name))
    set_data (&plor_name_key, _plor_name);
  else
    critical ("invalid plor name for object (%p): %s", this, _plor_name.c_str());
}

void
BaseObject::dispose ()
{}

/* --- memory utils --- */
void*
malloc_aligned (gsize	  total_size,
                gsize	  alignment,
                guint8	**free_pointer)
{
  uint8 *aligned_mem = (uint8*) g_malloc (total_size);
  *free_pointer = aligned_mem;
  if (!alignment || !size_t (aligned_mem) % alignment)
    return aligned_mem;
  g_free (aligned_mem);
  aligned_mem = (uint8*) g_malloc (total_size + alignment - 1);
  *free_pointer = aligned_mem;
  if (size_t (aligned_mem) % alignment)
    aligned_mem += alignment - size_t (aligned_mem) % alignment;
  return aligned_mem;
}

/**
 * The fmsb() function returns the position of the most significant bit set in the word @a val.
 * The least significant bit is position 1 and the most significant position is, for example, 32 or 64.
 * @returns The position of the most significant bit set is returned, or 0 if no bits were set.
 */
int // 0 or 1..64
fmsb (uint64 val)
{
  if (val >> 32)
    return 32 + fmsb (val >> 32);
  int nb = 32;
  do
    {
      nb--;
      if (val & (1U << nb))
        return nb + 1;  /* 1..32 */
    }
  while (nb > 0);
  return 0; /* none found */
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
 * with the rapicorn-zintern utility, so no errors should occour during
 * decompression.
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
