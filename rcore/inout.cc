// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "inout.hh"
#include "utilities.hh"
#include "strings.hh"
#include "thread.hh"
#include "main.hh"
#include "../configure.h"
#include <unistd.h>
#include <syslog.h>
#include <sys/types.h>
#include <fcntl.h>
#include <cstring>

static constexpr int UNCHECKED = -1;    // undetermined bool option

namespace Rapicorn {

// == Basic I/O ==
void
printout_string (const String &string)
{
  fflush (stderr);
  fputs (string.c_str(), stdout);
  fflush (stdout);
}

void
printerr_string (const String &string)
{
  fflush (stdout);
  fputs (string.c_str(), stderr);
  fflush (stderr);
}

// === User Messages ==
/// Capture the module, filename and line number of a user provided resource file.
UserSource::UserSource (const String &_module, const String &_filename, int _line) :
  module (_module), filename (_filename), line (_line)
{}

static void
user_message (const UserSource &source, const String &kind, const String &message)
{
  String fname, mkind, pname = program_alias();
  if (!pname.empty())
    pname += ":";
  if (!kind.empty())
    mkind = " " + kind + ":";
  if (!source.module.empty())
    mkind += " " + source.module + ":";
  if (!source.filename.empty())
    fname = source.filename + ":";
  if (source.line)
    fname = fname + string_format ("%d:", source.line);
  // obey GNU warning style to allow automated location parsing
  printerr ("%s%s%s %s\n", pname.c_str(), fname.c_str(), mkind.c_str(), message.c_str());
}

void
user_notice_string (const UserSource &source, const String &string)
{
  user_message (source, "", string);
}

void
user_warning_string (const UserSource &source, const String &string)
{
  user_message (source, "warning", string);
}

// == debug_handler ==
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

#if DOXYGEN
static void docextract_definition = RAPICORN_DEBUG_OPTION ("all", "Enable all available debugging keys and generate lots of output.");
#endif
static DebugOption dbe_syslog = RAPICORN_DEBUG_OPTION ("syslog", "Enable logging of general purpose messages through syslog(3).");
static DebugOption dbe_fatal_syslog = RAPICORN_DEBUG_OPTION ("fatal-syslog", "Enable logging of fatal conditions through syslog(3).");
static DebugOption dbe_fatal_warnings = RAPICORN_DEBUG_OPTION ("fatal-warnings", "Cast all warning messages into fatal errors.");

static __attribute__ ((noinline)) void // internal function, this + caller are skipped in backtraces
debug_handler (const char dkind, const String &file_line, const String &message, const char *key = NULL)
{
  /* The logging system must work before Rapicorn is initialized, and possibly even during
   * global_ctor phase. So any initialization needed here needs to be handled on demand.
   */
  String msg = message;
  const char kind = toupper (dkind);
  enum { DO_STDERR = 1, DO_SYSLOG = 2, DO_ABORT = 4, DO_DEBUG = 8, DO_ERRNO = 16,
         DO_STAMP = 32, DO_LOGFILE = 64, DO_DIAG = 128, DO_BACKTRACE = 256, };
  static int conftest_logfd = 0;
  const String conftest_logfile = conftest_logfd == 0 ? debug_config_get ("logfile") : "";
  const int FATAL_SYSLOG = dbe_fatal_syslog ? DO_SYSLOG : 0;
  const int MAY_SYSLOG = dbe_syslog ? DO_SYSLOG : 0;
  const int MAY_ABORT  = dbe_fatal_warnings ? DO_ABORT : 0;
  const int MAY_DIAG   = debug_devel_check() ? DO_DIAG : 0;
  const char *what = "DEBUG";
  int f = islower (dkind) ? DO_ERRNO : 0;                       // errno checks for lower-letter kinds
  switch (kind)
    {
    case 'F': what = "FATAL";    f |= DO_STDERR | FATAL_SYSLOG | DO_ABORT;  break;      // fatal, assertions
    case 'C': what = "CRITICAL"; f |= DO_STDERR | MAY_SYSLOG   | MAY_ABORT; break;      // critical
    case 'G': what = "DIAG";     f |= MAY_DIAG  | DO_STAMP;                 break;      // diagnostics
    case 'X': what = "FIX""ME";  f |= DO_DIAG   | DO_STAMP;                 break;      // fixing needed
    case 'D': what = "DEBUG";    f |= DO_DEBUG  | DO_STAMP;                 break;      // debug
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
                program_alias().c_str(), ThisThread::thread_pid(),
                start / 1000000.0);
    }
  if (f & DO_DEBUG)
    {
      String intro, prefix = key ? key : dbg_prefix (file_line);
      if (prefix.size())
        prefix = prefix + ": ";
      if (f & DO_STAMP)
        intro = string_format ("[%s]", timestamp_format (delta).c_str());
      printerr ("%s %s%s%s", intro.c_str(), prefix.c_str(), msg.c_str(), emsg.c_str());
    }
  if (f & DO_DIAG)
    {
      printerr ("%s: %s%s%s", what, where.c_str(), msg.c_str(), emsg.c_str());
    }
  if (f & DO_STDERR)
    {
      using namespace AnsiColors;
      const char *cy1 = color (FG_CYAN), *cy0 = color (FG_DEFAULT);
      const char *bo1 = color (BOLD), *bo0 = color (BOLD_OFF);
      const char *fgw = color (FG_WHITE), *fgc1 = fgw, *fgc0 = color (FG_DEFAULT), *fbo1 = "", *fbo0 = "";
      switch (kind)
        {
        case 'F':
          fgc1 = color (FG_RED);
          fbo1 = bo1;                   fbo0 = bo0;
          break;
        case 'C':
          fgc1 = color (FG_YELLOW);
          break;
        }
      printerr ("%s%s%s%s%s%s%s%s[%u] %s%s%s:%s%s %s%s",
                cy1, where.c_str(), cy0,
                bo1, fgw, program_alias().c_str(), fgc0, bo0, ThisThread::thread_pid(),
                fbo1, fgc1, what,
                fgc0, fbo0,
                msg.c_str(), emsg.c_str());
      if (f & DO_ABORT)
        printerr ("Aborting...\n");
    }
  if (f & DO_SYSLOG)
    {
      do_once { openlog (NULL, LOG_PID, LOG_USER); } // force pid logging
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
      btmsg = string_format ("%sBacktrace at 0x%08x (stackframe at 0x%08x):\n", where.c_str(),
                             addr, size_t (__builtin_frame_address (0)) /*size_t (&addr)*/);
      for (size_t i = 0; i < syms.size(); i++)
        btmsg += string_format ("  %s\n", syms[i].c_str());
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
          out = string_format ("[%s] %s[%u]: program started at: %s\n",
                               timestamp_format (delta).c_str(), program_alias().c_str(), ThisThread::thread_pid(),
                               timestamp_format (start).c_str());
          conftest_logfd = fd;
        }
      out += string_format ("[%s] %s[%u]:%s%s%s",
                            timestamp_format (delta).c_str(), program_alias().c_str(), ThisThread::thread_pid(),
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

// == envkey_ functions ==
static int
cstring_option_sense (const char *option_string, const char *option, char *value, const int offset = 0)
{
  const char *haystack = option_string + offset;
  const char *p = strstr (haystack, option);
  if (p)                                // found possible match
    {
      const int l = strlen (option);
      if (p == haystack || (p > haystack && (p[-1] == ':' || p[-1] == ';')))
        {                               // start matches (word boundary)
          const char *d1 = strchr (p + l, ':'), *d2 = strchr (p + l, ';'), *d = MAX (d1, d2);
          d = d ? d : p + l + strlen (p + l);
          bool match = true;
          if (p[l] == '=')              // found value
            {
              const char *v = p + l + 1;
              strncpy (value, v, d - v);
              value[d - v] = 0;
            }
          else if (p[l] == 0 || p[l] == ':' || p[l] == ';')
            {                           // option present
              strcpy (value, "1");
            }
          else
            match = false;              // no match
          if (match)
            {
              const int pos = d - option_string;
              if (d[0])
                {
                  const int next = cstring_option_sense (option_string, option, value, pos);
                  if (next >= 0)        // found overriding match
                    return next;
                }
              return pos;               // this match is last match
            }
        }                               // unmatched, keep searching
      return cstring_option_sense (option_string, option, value, p + l - option_string);
    }
  return -1;                            // not present in haystack
}

static bool
fast_envkey_check (const char *option_string, const char *key, bool include_all)
{
  const int l = max (size_t (64), strlen (option_string) + 1);
  char kvalue[l];
  strcpy (kvalue, "0");
  const int keypos = !key ? -1 : cstring_option_sense (option_string, key, kvalue);
  char avalue[l];
  strcpy (avalue, "0");
  const int allpos = !include_all ? -1 : cstring_option_sense (option_string, "all", avalue);
  if (keypos > allpos)
    return cstring_to_bool (kvalue, false);
  else if (allpos > keypos)
    return cstring_to_bool (avalue, false);
  else
    return false;       // neither key nor "all" found
}

/** Check whether a feature is enabled, use envkey_flipper_check() instead.
 * Variant of envkey_flipper_check() that ignores 'all' as a feature toggle.
 */
bool
envkey_feature_check (const char *env_var, const char *key, bool vdefault, volatile bool *cachep, bool include_all)
{
  if (env_var)          // require explicit activation
    {
      const char *val = getenv (env_var);
      if (!val || val[0] == 0)
        {
          if (cachep)
            *cachep = 0;
        }
      else if (key)
        return fast_envkey_check (val, key, include_all);
    }
  return vdefault;
}

/** Check whether a flipper (feature toggle) is enabled.
 * This function first checks the environment variable @a env_var for @a key, if it is present or if
 * 'all' is present, the function returns @a true, otherwise @a vdefault (usually @a false).
 * The @a cachep argument may point to a caching variable which is reset to 0 if @a env_var is
 * empty (i.e. no features can be enabled), so the caching variable can be used to prevent
 * unneccessary future envkey_flipper_check() calls.
 */
bool
envkey_flipper_check (const char *env_var, const char *key, bool vdefault, volatile bool *cachep)
{
  return envkey_feature_check (env_var, key, vdefault, cachep, true);
}

/** Check whether to print debugging message.
 * This function first checks the environment variable @a env_var for @a key, if the key is present,
 * 'all' is present or if @a env_var is NULL, the debugging message will be printed.
 * A NULL @a key checks for general debugging, it's equivalent to passing "debug" as @a key.
 * The @a cachep argument may point to a caching variable which is reset to 0 if @a env_var is
 * empty (so no debugging is enabled), so the caching variable can be used to prevent unneccessary
 * future debugging calls, e.g. to envkey_debug_message().
 */
bool
envkey_debug_check (const char *env_var, const char *key, volatile bool *cachep)
{
  if (!env_var)
    return true;        // unconditional debugging
  return envkey_flipper_check (env_var, key ? key : "debug", false, cachep);
}

/** Conditionally print debugging message.
 * This function first checks whether debugging is enabled via envkey_debug_check() and returns if not.
 * The arguments @a file_path and @a line are used to denote the debugging message source location,
 * @a format and @a va_args are formatting the message analogously to vprintf().
 */
void
envkey_debug_message (const char *env_var, const char *key, const char *file, const int line,
                      const String &message, volatile bool *cachep)
{
  if (!envkey_debug_check (env_var, key, cachep))
    return;
  String prefix = file ? file : "";
  if (!prefix.empty() && line >= 0)
    prefix += ":" + string_from_int (line);
  debug_handler ('D', prefix, message, key);
}

// == debug_* functions ==
void
debug_message (char kind, const char *file, int line, const String &message)
{
  String prefix = file ? file : "";
  if (!prefix.empty() && line >= 0)
    prefix += ":" + string_from_int (line);
  if (kind == 'F' || kind == 'C' || kind == 'G')
    debug_handler (kind, prefix, message);
}

void
debug_fmessage (const char *file, int line, const String &message)
{
  debug_message ('F', file, line, message);
  ::abort();
}

void
debug_assert (const char *file, const int line, const char *message)
{
  debug_message ('C', file, line, String ("assertion failed: ") + (message ? message : "?"));
}

void
debug_fassert (const char *file, const int line, const char *message)
{
  debug_fmessage (file, line, String ("assertion failed: ") + (message ? message : "?"));
}

struct DebugMap {
  Mutex                   mutex; // constexpr - so save to use from any static ctor/dtor
  String                  envvar;
  std::map<String,String> map;
  DebugMap() {}
  RAPICORN_CLASS_NON_COPYABLE (DebugMap);
};
static DurableInstance<DebugMap> dbg; // an undeletable DebugMap that makes dbg available across all other static ctor/dtor calls

/// Parse environment variable @a name for debug configuration.
void
debug_envvar (const String &name)
{
  ScopedLock<Mutex> locker (dbg->mutex);
  dbg->envvar = name;
}

/// Set debug configuration override.
void
debug_config_add (const String &option)
{
  ScopedLock<Mutex> locker (dbg->mutex);
  String key, value;
  const size_t eq = option.find ('=');
  if (eq != std::string::npos)
    {
      key = option.substr (0, eq);
      value = option.substr (eq + 1);
    }
  else
    {
      key = option;
      value = "1";
    }
  if (!key.empty())
    dbg->map[key] = value;
}

/// Unset debug configuration override.
void
debug_config_del (const String &key)
{
  ScopedLock<Mutex> locker (dbg->mutex);
  dbg->map.erase (key);
}

/// Query debug configuration for option @a key, defaulting to @a default_value.
String
debug_config_get (const String &key, const String &default_value)
{
  ScopedLock<Mutex> locker (dbg->mutex);
  auto pair = dbg->map.find (key);
  if (pair != dbg->map.end())
    return pair->second;
  auto envstring = [] (const char *name) {
    const char *c = name ? getenv (name) : NULL;
    return c ? c : "";
  };
  const String options[3] = {
    envstring (dbg->envvar.c_str()),
    envstring ("RAPICORN_DEBUG"),
    string_format ("fatal-syslog=1:devel=%d", ENABLE_DEVEL_MODE), // debug config defaults
  };
  for (size_t i = 0; i < ARRAY_SIZE (options); i++)
    {
      const int l = max (size_t (64), options[i].size());
      char value[l];
      strcpy (value, "0");
      const int keypos = cstring_option_sense (options[i].c_str(), key.c_str(), value);
      if (keypos >= 0)
        return value;
    }
  return default_value;
}

/// Query debug configuration for option @a key, defaulting to @a default_value, return boolean.
bool
debug_config_bool (const String &key, bool default_value)
{
  return string_to_bool (debug_config_get (key, string_from_int (default_value)));
}

static volatile int debug_development_features = UNCHECKED;      // cached, checked only once after startup

/// Check if debugging features for development versions should be enabled, see also #$RAPICORN_DEBUG.
bool
debug_devel_check ()
{
  if (atomic_load (&debug_development_features) == UNCHECKED)
    atomic_store (&debug_development_features,
                  int (RAPICORN_DEBUG_OPTION ("devel", "Enable debugging features for development versions.")));
  return atomic_load (&debug_development_features);
}

bool volatile _rapicorn_debug_check_cache = true; // initially enable debugging

/** Issue debugging messages according to configuration.
 * Checks the environment variable #$RAPICORN_DEBUG for @a key like rapicorn_debug_check(),
 * and issues a debugging message with source location @a file_path and @a line.
 * The message @a format uses printf-like syntax.
 */
void
rapicorn_debug (const char *key, const char *file, const int line, const String &msg)
{
  envkey_debug_message ("RAPICORN_DEBUG", key, file, line, msg, &_rapicorn_debug_check_cache);
}

/** Check if debugging is enabled for @a key.
 * This function checks if #$RAPICORN_DEBUG contains @a key or "all" and returns true
 * if debugging is enabled for the given key.
 */
bool rapicorn_debug_check   (const char *key);

#ifndef DOXYGEN
bool
FlipperOption::flipper_check (const char *key, bool vdefault)
{
  return envkey_flipper_check ("RAPICORN_FLIPPER", key, vdefault);
}
#endif // !DOXYGEN

/** @def RAPICORN_DEBUG_OPTION(key, blurb)
 * Create a Rapicorn::DebugOption object that can be used to query, cache and document debugging options.
 * When converted to bool, it is checked if #$RAPICORN_DEBUG enables or disables the debugging option @a key.
 * Enabling can be achieved by just adding "key" to the colon separated list in the environment variable.
 * Disabling can be achieved by leaving it out. Alternatively, "key=1" and "key=0" are also recognized
 * to enable and disable the option respectively. Settings listed later can be used as overrides.
 * Typical use: @code
 * static auto dbg_test_feature = RAPICORN_DEBUG_OPTION ("test-feature", "Switch use of the test-feature during debugging on or off.");
 * if (dbg_test_feature)
 *   enable_test_feature();
 * @endcode
 */

/** @def RAPICORN_FLIPPER(key, blurb)
 * Create a Rapicorn::FlipperOption object that can be used to query, cache and document feature toggles.
 * When converted to bool, it is checked if #$RAPICORN_FLIPPER enables or disables the flipper @a key.
 * Enabling can be achieved by just adding "key" to the colon separated list in the environment variable.
 * Disabling can be achieved by leaving it out. Alternatively, "key=1" and "key=0" are also recognized
 * to enable and disable the option respectively. Settings listed later can be used as overrides.
 * Related: #$RAPICORN_DEBUG.
 */

/** @def RAPICORN_FATAL(format,...)
 * Abort the program with a fatal error message.
 * Issues an error message and call abort() to abort the program, see also #$RAPICORN_DEBUG.
 * The error message @a format uses printf-like syntax.
 */

/** @def RAPICORN_ASSERT(cond)
 * Issue an error and abort the program if expression @a cond evaluates to false.
 * Before aborting, a bracktrace is printed to aid debugging of the failing assertion.
 */

/** @def RAPICORN_ASSERT_RETURN(condition [, rvalue])
 * Issue an assertion warning and return if @a condition is false.
 * Issue an error and return @a rvalue if expression @a condition evaluates to false. Returns void if @a rvalue was not specified.
 * This is normally used as function entry condition.
 */

/** @def RAPICORN_ASSERT_UNREACHED()
 * Assertion used to label unreachable code.
 * Issues an error message, and aborts the program if it is reached at runtime.
 * This is normally used to label code conditions intended to be unreachable.
 */

/** @def RAPICORN_CRITICAL(format,...)
 * Issues a critical message, see also #$RAPICORN_DEBUG=fatal-warnings.
 * The error message @a format uses printf-like syntax.
 */

/** @def RAPICORN_CRITICAL_UNLESS(cond)
 * Issue a critical warning if @a condition is false, i.e. this macro behaves
 * like RAPICORN_CRITICAL() if expression @a cond evaluates to false.
 * This is normally used as a non-fatal assertion.
 */

/** @def RAPICORN_STARTUP_ASSERT(condition)
 * Abort during program startup if @a condition is false.
 * This macro expands to executing an assertion via a static constructor during program
 * startup (before main()). In contrast to static_assert(), this macro can execute arbitrary
 * code, e.g. to assert floating point number properties.
 */

/** @def RAPICORN_DIAG(format,...)
 * Issues a diagnostic message, e.g. indicaintg I/O errors or invalid user input.
 * Diagnostic message are enabled by default for dvelopment versions and can be enabled
 * otherwise with #$RAPICORN_DEBUG=devel.
 * The message @a format uses printf-like syntax.
 */

/**
 * @def RAPICORN_KEY_DEBUG(key, format,...)
 * Issues a debugging message if #$RAPICORN_DEBUG contains the literal @a "key" or "all".
 * The message @a format uses printf-like syntax.
 */

// == AnsiColors ==
namespace AnsiColors {

/// Return ASCII code for the specified color.
const char*
color_code (Colors acolor)
{
  switch (acolor)
    {
    default: ;
    case NONE:             return "";
    case RESET:            return "\033[0m";
    case BOLD:             return "\033[1m";
    case BOLD_OFF:         return "\033[22m";
    case ITALICS:          return "\033[3m";
    case ITALICS_OFF:      return "\033[23m";
    case UNDERLINE:        return "\033[4m";
    case UNDERLINE_OFF:    return "\033[24m";
    case INVERSE:          return "\033[7m";
    case INVERSE_OFF:      return "\033[27m";
    case STRIKETHROUGH:    return "\033[9m";
    case STRIKETHROUGH_OFF:return "\033[29m";
    case FG_BLACK:         return "\033[30m";
    case FG_RED:           return "\033[31m";
    case FG_GREEN:         return "\033[32m";
    case FG_YELLOW:        return "\033[33m";
    case FG_BLUE:          return "\033[34m";
    case FG_MAGENTA:       return "\033[35m";
    case FG_CYAN:          return "\033[36m";
    case FG_WHITE:         return "\033[37m";
    case FG_DEFAULT:       return "\033[39m";
    case BG_BLACK:         return "\033[40m";
    case BG_RED:           return "\033[41m";
    case BG_GREEN:         return "\033[42m";
    case BG_YELLOW:        return "\033[43m";
    case BG_BLUE:          return "\033[44m";
    case BG_MAGENTA:       return "\033[45m";
    case BG_CYAN:          return "\033[46m";
    case BG_WHITE:         return "\033[47m";
    case BG_DEFAULT:       return "\033[49m";
    }
}

struct EnvKey {
  String var, key;
  EnvKey() : var (""), key ("") {}
};

static volatile int      colorize_stdout = UNCHECKED;   // cache stdout colorization check
static Exclusive<EnvKey> env_key;

/// Configure the environment variable that always/never/automatically allows colorization.
void
color_envkey (const String &env_var, const String &key)
{
  EnvKey ekey;
  ekey.var = env_var;
  ekey.key = key;
  env_key = ekey; // Atomic access
  atomic_store (&colorize_stdout, UNCHECKED);
}

/// Check whether the tty @a fd should use colorization.
bool
colorize_tty (int fd)
{
  EnvKey ekey = env_key;
  const char *ev = getenv (ekey.var.c_str());
  if (ev)
    {
      if (ekey.key.empty() == false)
        {
          const int l = max (size_t (64), strlen (ev) + 1);
          char value[l];
          strcpy (value, "auto");
          cstring_option_sense (ev, ekey.key.c_str(), value);
          if (strncasecmp (value, "always", 6) == 0)
            return true;
          else if (strncasecmp (value, "never", 5) == 0)
            return false;
          else if (strncasecmp (value, "auto", 4) != 0)
            return string_to_bool (value, 0);
        }
      else if (strncasecmp (ev, "always", 6) == 0)
        return true;
      else if (strncasecmp (ev, "never", 5) == 0)
        return false;
      else if (strncasecmp (ev, "auto", 4) != 0)
        return string_to_bool (ev, 0);
    }
  // found 'auto', sense arbitrary fd
  if (fd >= 3)
    return isatty (fd);
  // sense stdin/stdout/stderr
  if (isatty (1) && isatty (2))
    {
      const char *term = getenv ("TERM");
      if (term && strcmp (term, "dumb") != 0)
        return true;
    }
  return false;
}

/// Return ASCII code for the specified color if stdout & stderr should be colorized.
const char*
color (Colors acolor)
{
  if (atomic_load (&colorize_stdout) == UNCHECKED)
    atomic_store (&colorize_stdout, int (colorize_tty()));
  if (!atomic_load (&colorize_stdout))
    return "";
  return color_code (acolor);
}

} // AnsiColors

} // Rapicorn
