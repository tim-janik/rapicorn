/* Rapicorn
 * Copyright (C) 2005-2006 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <sys/time.h>
#include <cxxabi.h>
#include <signal.h>
#include <glib.h>
#include <vector>
#include <algorithm>
#include <cstring>
#include <iconv.h>
#include <syslog.h>
#include "rapicornutils.hh"
#include "rapicornutf8.hh"
#include "rapicornthread.hh"
#include "rapicornmsg.hh"
#include "rapicorncpu.hh"
#include "types.hh"

#ifndef _
#define _(s)    s
#endif

#if !__GNUC_PREREQ (3, 4) || (__GNUC__ == 3 && __GNUC_MINOR__ == 4 && __GNUC_PATCHLEVEL__ < 6)
#error This GNU C++ compiler version is known to be broken - please consult ui/README
#endif

namespace Rapicorn {

static void             process_handle_and_seed_init ();


static Msg::CustomType debug_browser ("browser", Msg::DEBUG);

static const InitSettings *rapicorn_init_settings = NULL;

InitSettings
init_settings ()
{
  return *rapicorn_init_settings;
}

/* --- InitHooks --- */
static void    (*run_init_hooks) () = NULL;
static InitHook *init_hooks = NULL;

InitHook::InitHook (InitHookFunc _func,
                    int          _priority) :
  next (NULL), priority (_priority), hook (_func)
{
  RAPICORN_ASSERT (rapicorn_init_settings == NULL);
  /* the above assertion guarantees single-threadedness */
  next = init_hooks;
  init_hooks = this;
  run_init_hooks = invoke_hooks;
}

void
InitHook::invoke_hooks (void)
{
  std::vector<InitHook*> hv;
  struct Sub {
    static int
    init_hook_cmp (const InitHook *const &v1,
                   const InitHook *const &v2)
    {
      return v1->priority < v2->priority ? -1 : v1->priority > v2->priority;
    }
  };
  for (InitHook *ihook = init_hooks; ihook; ihook = ihook->next)
    hv.push_back (ihook);
  stable_sort (hv.begin(), hv.end(), Sub::init_hook_cmp);
  for (std::vector<InitHook*>::iterator it = hv.begin(); it != hv.end(); it++)
    (*it)->hook();
}

/* --- initialization --- */
static InitSettings global_init_settings = {
  false,        /* stand_alone */
  false,        /* perf_test */
};

static void
rapicorn_parse_settings_and_args (InitValue *value,
                                  int       *argc_p,
                                  char     **argv)
{
  bool parse_test_args = false;
  /* apply settings */
  if (value)
    while (value->value_name)
      {
        if (strcmp (value->value_name, "stand-alone") == 0)
          global_init_settings.stand_alone = init_value_bool (value);
        else if (strcmp (value->value_name, "test-quick") == 0)
          global_init_settings.test_quick = init_value_bool (value);
        else if (strcmp (value->value_name, "test-slow") == 0)
          global_init_settings.test_slow = init_value_bool (value);
        else if (strcmp (value->value_name, "test-perf") == 0)
          global_init_settings.test_perf = init_value_bool (value);
        else if (strcmp (value->value_name, "test-verbose") == 0)
          global_init_settings.test_verbose = init_value_bool (value);
        else if (strcmp (value->value_name, "rapicorn-test-parse-args") == 0)
          parse_test_args = init_value_bool (value);
        value++;
      }
  /* parse args */
  uint argc = *argc_p;
  for (uint i = 1; i < argc; i++)
    {
      if (strcmp (argv[i], "--g-fatal-warnings") == 0)
        {
          uint fatal_mask = g_log_set_always_fatal (GLogLevelFlags (G_LOG_FATAL_MASK));
          fatal_mask |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;
          g_log_set_always_fatal (GLogLevelFlags (fatal_mask));
          argv[i] = NULL;
        }
      else if (parse_test_args && strcmp ("--test-quick", argv[i]) == 0)
        {
          global_init_settings.test_quick = true;
          argv[i] = NULL;
        }
      else if (parse_test_args && strcmp ("--test-slow", argv[i]) == 0)
        {
          global_init_settings.test_slow = true;
          argv[i] = NULL;
        }
      else if (parse_test_args && strcmp ("--test-perf", argv[i]) == 0)
        {
          global_init_settings.test_perf = true;
          argv[i] = NULL;
        }
      else if (parse_test_args && strcmp ("--test-verbose", argv[i]) == 0)
        {
          global_init_settings.test_verbose = true;
          argv[i] = NULL;
        }
      else if (parse_test_args && strcmp ("--verbose", argv[i]) == 0)
        {
          global_init_settings.test_verbose = true;
          /* interpret --verbose for GLib compat but don't delete the argument
           * since regular non-test programs may need this. could be fixed by
           * having a separate test program argument parser.
           */
        }
    }
  /* fallback handling for tests */
  if (parse_test_args && !global_init_settings.test_quick && !global_init_settings.test_slow && !global_init_settings.test_perf)
    global_init_settings.test_quick = true;
  /* collapse args */
  uint e = 1;
  for (uint i = 1; i < argc; i++)
    if (argv[i])
      {
        argv[e++] = argv[i];
        if (i >= e)
          argv[i] = NULL;
      }
  *argc_p = e;
}

static struct _InternalConstructorTest_lrcc0 { int v; _InternalConstructorTest_lrcc0() : v (0x12affe17) {} } _internalconstructortest;

static String program_argv0 = "";
static String program_cwd = "";

String
process_name ()
{
  return program_argv0;
}

String
process_cwd ()
{
  return program_cwd;
}

/**
 * @param argcp         location of the 'argc' argument to main()
 * @param argvp         location of the 'argv' argument to main()
 * @param app_name      Application name as needed g_set_application_name()
 * @param ivalues       program specific initialization values
 *
 * Initialize rapicorn's rcore components like random number
 * generators, CPU detection or the thread system.
 * Pre-parses the arguments passed in to main() for rapicorn
 * specific arguments.
 * Also initializes the GLib program name, application name and thread system
 * if those are still uninitialized.
 */
void
rapicorn_init_core (int        *argcp,
                    char      **argvp,
                    const char *app_name,
                    InitValue   ivalues[])
{
  /* mandatory initial initialization */
  if (!g_threads_got_initialized)
    g_thread_init (NULL);

  /* update program/application name upon repeated initilization */
  if (argcp && *argcp && argvp && argvp[0] && argvp[0][0] != 0 && program_argv0.size() == 0)
    program_argv0 = argvp[0];
  if (program_cwd.empty())
    program_cwd = Path::cwd();
  char *prg_name = argcp && *argcp ? g_path_get_basename (argvp[0]) : NULL;
  if (rapicorn_init_settings != NULL)
    {
      if (prg_name && !g_get_prgname ())
        g_set_prgname (prg_name);
      g_free (prg_name);
      if (app_name && !g_get_application_name())
        g_set_application_name (app_name);
      return;   /* simply ignore repeated initializations */
    }

  Logging::setup();

  /* normal initialization */
  rapicorn_init_settings = &global_init_settings;
  if (prg_name)
    g_set_prgname (prg_name);
  g_free (prg_name);
  if (app_name && (!g_get_application_name() || g_get_application_name() == g_get_prgname()))
    g_set_application_name (app_name);

  /* verify constructur runs to catch link errors */
  if (_internalconstructortest.v != 0x12affe17)
    error ("librapicorncore: link error: C++ constructors have not been executed");

  rapicorn_parse_settings_and_args (ivalues, argcp, argvp);

  /* initialize random numbers */
  process_handle_and_seed_init();

  /* initialize sub systems */
  _rapicorn_init_cpuinfo();
  _rapicorn_init_threads();
  _rapicorn_init_types();
  if (run_init_hooks)
    run_init_hooks();
}

bool
init_value_bool (InitValue *value)
{
  if (value->value_string)
    switch (value->value_string[0])
      {
      case 0:   // FIXME: use string_to_bool()
      case '0': case 'f': case 'F':
      case 'n': case 'N':               /* false assigments */
        return FALSE;
      default:
        return TRUE;
      }
  else
    return ABS (value->value_num) >= 0.5;
}

double
init_value_double (InitValue *value)
{
  if (value->value_string && value->value_string[0])
    return g_ascii_strtod (value->value_string, NULL);
  return value->value_num;
}

int64
init_value_int (InitValue *value)
{
  if (value->value_string && value->value_string[0])
    return strtoll (value->value_string, NULL, 0);
  return int64 (value->value_num + 0.5);
}

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
RAPICORN_STATIC_ASSERT (FLT_MIN      <= 1E-37);
RAPICORN_STATIC_ASSERT (FLT_MAX      >= 1E+37);
RAPICORN_STATIC_ASSERT (FLT_EPSILON  <= 1E-5);
RAPICORN_STATIC_ASSERT (DBL_MIN      <= 1E-37);
RAPICORN_STATIC_ASSERT (DBL_MAX      >= 1E+37);
RAPICORN_STATIC_ASSERT (DBL_EPSILON  <= 1E-9);
RAPICORN_STATIC_ASSERT (LDBL_MIN     <= 1E-37);
RAPICORN_STATIC_ASSERT (LDBL_MAX     >= 1E+37);
RAPICORN_STATIC_ASSERT (LDBL_EPSILON <= 1E-9);

/* === printing, errors, warnings, assertions, debugging === */
#define LSTRING_OFFSET  16
static inline bool
inword (char c)
{
  return isalnum (c) || strchr ("+-_/*$!%()", c);
}

static ssize_t
lstring_find_word (const String &s,
                   const String &w,
                   char          postfix = 0)
{
  size_t start = 0, p;
  ssize_t last = -1;
  while (p = s.find (w, start),
         p != s.npos)
    {
      start = p + 1;
      if (p > 0 && inword (s[p - 1]))
        continue; // no preceding word boundary
      if (!postfix && p + w.size() < s.size() && inword (s[p + w.size()]))
        continue; // no subsequent word boundary
      if (postfix && (p + w.size() >= s.size() || s[p + w.size()] != postfix))
        continue; // missing required postfix
      last = MAX (last, ssize_t (p)); // match
    }
  return last < 0 ? 0 : last + LSTRING_OFFSET;
}

bool   Logging::cdebug = true;
bool   Logging::cany = true;
bool   Logging::cdiag = true;
bool   Logging::cdevel = true;
bool   Logging::cverbose = false;
bool   Logging::cwfatal = false;
bool   Logging::cstderr = true;
bool   Logging::csyslog = false;
bool   Logging::cnfsyslog = false;
bool   Logging::ctestpid0 = false;
String Logging::logfile = "";
String Logging::config = "debug";
String Logging::ovrconfig = "";

String
Logging::override_config ()
{
  return ovrconfig;
}

void
Logging::override_config (const String &cfg)
{
  String str = cfg;
  std::transform (str.begin(), str.end(), str.begin(), ::tolower);
  ovrconfig = str;
  setup();
}

void
Logging::setup ()
{
  const char *s = getenv ("RAPICORN");
  String str = s ? s : "";
  std::transform (str.begin(), str.end(), str.begin(), ::tolower);
  config = str;
  str = config + ":" + ovrconfig;
  const ssize_t wall = lstring_find_word (str, "log-all");
  const ssize_t wvrb = lstring_find_word (str, "verbose");
  const ssize_t wfat = lstring_find_word (str, "fatal-warnings");
  const ssize_t wnft = lstring_find_word (str, "no-fatal-warnings");
  const ssize_t wbrf = lstring_find_word (str, "brief");
  const ssize_t wdbg = lstring_find_word (str, "debug"); // debug any
  const ssize_t wndg = lstring_find_word (str, "no-debug");
  const ssize_t wdev = lstring_find_word (str, "devel");
  const ssize_t wndv = lstring_find_word (str, "no-devel");
  const ssize_t wstb = lstring_find_word (str, "stable");
  const ssize_t wdag = lstring_find_word (str, "diag");
  const ssize_t wdng = lstring_find_word (str, "no-diag");
  const ssize_t wyse = lstring_find_word (str, "stderr");
  const ssize_t wnse = lstring_find_word (str, "no-stderr");
  const ssize_t wsys = lstring_find_word (str, "syslog");
  const ssize_t wnsy = lstring_find_word (str, "no-syslog");
  const ssize_t whlp = lstring_find_word (str, "help");
  const ssize_t wnfs = lstring_find_word (str, "no-fatal-syslog");
  const ssize_t wtp0 = lstring_find_word (str, "testpid0");
  // due to LSTRING_OFFSET, we can use bool(default)
  const ssize_t devel1 = MAX (bool (RAPICORN_DEVEL_VERSION), MAX (wall, wdev));
  const ssize_t devel0 = MAX (wstb, wndv);
  cdevel = devel1 > devel0;
  cdiag = MAX (bool (cdevel), MAX (wall, wdag)) > wdng;
  cverbose = MAX (bool (wall), wvrb) > wbrf;
  cwfatal = wfat > wnft;
  cany = MAX (bool (wall), wdbg) > wndg;
  cdebug = cany || lstring_find_word (str, "debug", '-');
  cstderr = MAX (bool (1), wyse) > wnse;
  csyslog = wsys > wnsy;
  cnfsyslog = bool (wnfs);
  ctestpid0 = bool (wtp0);
  if (whlp)
    {
      String keys = "  log-all:verbose:brief:debug:no-debug:devel:stable\n"
                    "  help:diag:no-diag:stderr:no-stderr:syslog:no-syslog\n"
                    "  fatal-warnings:no-fatal-warnings\n"
                    "  debug-*:no-debug-*: (where * is a custom debug prefix)";
      vector<String> dk = Logging::debug_keys();
      for (uint i = 0; i < dk.size(); i++)
        {
          keys += i % 6 ? ":" : "\n  ";
          keys += dk[i];
        }
      printerr ("Rapicorn::Logging: configuration help, available keys for $RAPICORN:\n%s\n",
                keys.c_str());
    }
  if (logfile.empty())
    {
      const char *start = str.c_str(), *p = start, *last = NULL;
      while ((p = strstr (p, "logfile=")) && *p)
        {
          if (p == start || strchr (":;,", p[-1]))
            last = p;
          p++;
        }
      if (last && last[8])
        {
          start = last + 8;
          p = start;
          while (*p && !strchr (":;,", *p))
            p++;
          logfile = String (start, p - start);
        }
    }
  /* Logging configuration implemented in here:
   * Message  Syslog Stable Devel Abort Option
   * fatal:     [*]   [*]   [*]   [*]     -
   * error:     [D]   [*]   [*]   [D]     -
   * warning:   [ ]   [*]   [*]   [ ]     -
   * diag:      [ ]   [ ]   [*]   [ ]   diag/devel/log-all
   * debug:     [ ]   [ ]   [ ]   [ ]   log-all/debug/debug-*
   */
}

void
Logging::abort()
{
  printerr ("aborting...\n");
  ::abort();
}

void
Logging::dmessage (const char *file, int line, const char *func, const char *domain,
                   const Logging &detail, const char *format, ...)
{
  va_list args;
  va_start (args, format);
  Logging::vmessage (file, line, func, domain, detail, format, args);
  va_end (args);
}

void
Logging::vmessage (const char *file, int line, const char *func, const char *domain,
                   const Logging &detail, const char *format, va_list vargs)
{
  const int saved_errno = errno;
  const size_t a = size_t (&detail);
  const bool debugging = a >= 256;
  bool curtly = false;
  char kind = 0, perrno = 0;
  int syl = LOG_DEBUG;
  String key;
  if (debugging)
    {
      perrno = detail.m_flags & PERRNO;
      curtly = detail.m_flags & CURTLY;
      key = String ("debug-") + detail.m_detail;
      String lookup = key;
      std::transform (lookup.begin(), lookup.end(), lookup.begin(), ::tolower);
      const ssize_t wyes = lstring_find_word (config, lookup);
      const ssize_t w_no = lstring_find_word (config, "no-" + lookup);
      if (MAX (bool (cany), wyes) <= w_no)
        return;
    }
  else
    {
      perrno = bool (a & 0x80);
      kind = a & 0x7f;
      switch (kind)
        {
        case 'f': syl = LOG_ERR;     key = "FATAL";    break; // LOG_CRIT uses system-level routing
        case 'e': syl = LOG_ERR;     key = "ERROR";    break;
        case 'w': syl = LOG_WARNING; key = "WARNING";  break;
        case 'd': syl = LOG_NOTICE;  key = "";         break;
        default: break;
        }
    }
  String l = file ? string_printf ("%s:%u:", file, line) : "";
  String d = domain && !file ? string_printf ("%s:", domain) : "";
  String f = func ? string_printf ("%s():", func) : "";
  String u = string_vprintf (format, vargs);
  String e = perrno ? String (": ") + strerror (saved_errno) : "";
  bool fatal_abort = kind == 'f' ||
                     (kind == 'e' && cdevel) ||
                     (kind == 'e' && cwfatal) ||
                     (kind == 'w' && cwfatal);
  const char *wfatal = kind == 'w' && cwfatal ? "(fatal warnings) " : "";
  // stderr
  if (cstderr || fatal_abort)
    {
      String n = process_name();
      if (n.empty()) n = file ? file : "/";
      if (!cverbose)
        { // simplistic basename implementation
          const char *ptr = strrchr (n.c_str(), RAPICORN_DIR_SEPARATOR);
          if (ptr && *ptr && ptr[1])
            n = n.substr (ptr - n.c_str() + 1);
        }
      String p = string_printf ("%s[%u]:", n.c_str(), ctestpid0 ? 0 : Thread::Self::pid());
      String k = !key.empty() ? string_printf ("%s:", key.c_str()) : "";
      String sk = key.empty() ? "" : " " + k;
      String m = kind && strchr ("feau", kind) ? "\n" : ""; // newline before aborting
      if (curtly)
        m += k + " " + u + e;
      else if (cverbose)
        m += p + l + d + f + sk + " " + u + e;
      else if (debugging)
        m += l + d + sk + " " + u + e;
      else if (file || func) // assertion or check failed
        m += l + d + f + sk + " " + u + e;
      else
        m += p + sk + " " + u + e;
      if (m[m.size() - 1] != '\n') m += "\n"; // newline termination
      m += wfatal;
      printerr ("%s", m.c_str());
    }
  // logfile
  if (!logfile.empty())
    {
      static int logfd = 0;
      if (once_enter (&logfd))
        {
          int fd;
          do
            fd = open (Path::abspath (logfile).c_str(), O_WRONLY | O_CREAT | O_APPEND | O_NOCTTY, 0666);
          while (logfd < 0 && errno == EINTR);
          if (fd == 0) // invalid initialization value
            {
              fd = dup (fd);
              close (0);
            }
          once_leave (&logfd, fd);
        }
      String n = process_name();
      String p = string_printf ("%s[%u]:", n.c_str(), Thread::Self::pid());
      if (logfd > 0)
        {
          String k = !key.empty() ? string_printf (" %s:", key.c_str()) : "";
          String m = p + l + d + f + k + " " + u + e;
          if (m[m.size() - 1] != '\n') m += "\n"; // newline termination
          if (fatal_abort)
            m += String (wfatal) + "aborting...\n";
          int err;
          do
            err = write (logfd, m.data(), m.size());
          while (err < 0 && (errno == EINTR || errno == EAGAIN));
        }
    }
  // syslog
  if (csyslog || (fatal_abort && !cnfsyslog))
    {
      static bool opened = false;
      if (once_enter (&opened))
        {
          openlog (NULL, LOG_PID, LOG_USER);
          once_leave (&opened, true);
        }
      bool verbose = cverbose || fatal_abort;
      String k = !key.empty() ? string_printf ("%s:", key.c_str()) : "";
      String c;
      if (verbose && file)
        c = string_printf ("(%s:%u) ", file, line);
      else if (verbose && func)
        c = string_printf ("(%s) ", func);
      else if (verbose && domain)
        c = string_printf ("(%s) ", domain);
      if (!key.empty() && !c.empty())
        c = " " + c;
      syslog (syl, "%s%s%s%s", k.c_str(), c.c_str(), u.c_str(), e.c_str());
    }
  // aborting
  if (fatal_abort)
    Logging::abort();
}

void
Logging::add ()
{
  if (detail)
    ; // register keys?
}

vector<String>
Logging::debug_keys()
{
  return vector<String>(); // not currently registering
}

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
process_handle_and_seed_init ()
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

/* --- string utils --- */
String
string_multiply (const String &s,
                 uint64        count)
{
  if (count == 1)
    return s;
  else if (count & 1)
    {
      String tmp = string_multiply (s, count - 1);
      tmp += s;
      return tmp;
    }
  else if (count)
    {
      String tmp = string_multiply (s, count / 2);
      tmp += tmp;
      return tmp;
    }
  else
    return "";
}

String
string_tolower (const String &str)
{
  String s (str);
  for (uint i = 0; i < s.size(); i++)
    s[i] = Unichar::tolower (s[i]);
  return s;
}

String
string_toupper (const String &str)
{
  String s (str);
  for (uint i = 0; i < s.size(); i++)
    s[i] = Unichar::toupper (s[i]);
  return s;
}

String
string_totitle (const String &str)
{
  String s (str);
  for (uint i = 0; i < s.size(); i++)
    s[i] = Unichar::totitle (s[i]);
  return s;
}

String
string_printf (const char *format,
               ...)
{
  String str;
  va_list args;
  va_start (args, format);
  str = string_vprintf (format, args);
  va_end (args);
  return str;
}

String
string_vprintf (const char *format,
                va_list     vargs)
{
  char *str = NULL;
  if (vasprintf (&str, format, vargs) >= 0 && str)
    {
      String s = str;
      free (str);
      return s;
    }
  else
    return format;
}

static StringVector
string_whitesplit (const String &string)
{
  static const char whitespaces[] = " \t\n\r\f\v";
  StringVector sv;
  uint i, l = 0;
  for (i = 0; i < string.size(); i++)
    if (strchr (whitespaces, string[i]))
      {
        if (i > l)
          sv.push_back (string.substr (l, i - l));
        l = i + 1;
      }
  if (i > l)
    sv.push_back (string.substr (l, i - l));
  return sv;
}

StringVector
string_split (const String       &string,
              const String       &splitter)
{
  if (splitter == "")
    return string_whitesplit (string);
  StringVector sv;
  uint i, l = 0, k = splitter.size();
  for (i = 0; i < string.size(); i++)
    if (string.substr (i, k) == splitter)
      {
        if (i >= l)
          sv.push_back (string.substr (l, i - l));
        l = i + k;
      }
  if (i >= l)
    sv.push_back (string.substr (l, i - l));
  return sv;
}

String
string_join (const String       &junctor,
             const StringVector &strvec)
{
  String s;
  if (strvec.size())
    s = strvec[0];
  for (uint i = 1; i < strvec.size(); i++)
    s += junctor + strvec[i];
  return s;
}

bool
string_to_bool (const String &string)
{
  static const char *spaces = " \t\n\r";
  const char *p = string.c_str();
  /* skip spaces */
  while (*p && strchr (spaces, *p))
    p++;
  /* ignore signs */
  if (p[0] == '-' || p[0] == '+')
    {
      p++;
      /* skip spaces */
      while (*p && strchr (spaces, *p))
        p++;
    }
  /* handle numbers */
  if (p[0] >= '0' && p[0] <= '9')
    return 0 != string_to_uint (p);
  /* handle special words */
  if (strncasecmp (p, "ON", 2) == 0)
    return 1;
  if (strncasecmp (p, "OFF", 3) == 0)
    return 0;
  /* handle non-numbers */
  return !(p[0] == 0 ||
           p[0] == 'f' || p[0] == 'F' ||
           p[0] == 'n' || p[0] == 'N');
}

String
string_from_bool (bool value)
{
  return String (value ? "1" : "0");
}

uint64
string_to_uint (const String &string,
                uint          base)
{
  const char *p = string.c_str();
  while (*p == ' ' || *p == '\n' || *p == '\t' || *p == '\r')
    p++;
  bool hex = p[0] == '0' && (p[1] == 'X' || p[1] == 'x');
  return strtoull (hex ? p + 2 : p, NULL, hex ? 16 : base);
}

String
string_from_uint (uint64 value)
{
  return string_printf ("%llu", value);
}

bool
string_has_int (const String &string)
{
  const char *p = string.c_str();
  while (*p == ' ' || *p == '\n' || *p == '\t' || *p == '\r')
    p++;
  return p[0] >= '0' && p[0] <= '9';
}

int64
string_to_int (const String &string,
               uint          base)
{
  const char *p = string.c_str();
  while (*p == ' ' || *p == '\n' || *p == '\t' || *p == '\r')
    p++;
  bool hex = p[0] == '0' && (p[1] == 'X' || p[1] == 'x');
  return strtoll (hex ? p + 2 : p, NULL, hex ? 16 : base);
}

String
string_from_int (int64 value)
{
  return string_printf ("%lld", value);
}

double
string_to_double (const String &string)
{
  return g_ascii_strtod (string.c_str(), NULL);
}

double
string_to_double (const char  *dblstring,
                  const char **endptr)
{
  return g_ascii_strtod (dblstring, (char**) endptr);
}

String
string_from_float (float value)
{
  char numbuf[G_ASCII_DTOSTR_BUF_SIZE + 1] = { 0, };
  g_ascii_formatd (numbuf, G_ASCII_DTOSTR_BUF_SIZE, "%.7g", value);
  return String (numbuf);
}

String
string_from_double (double value)
{
  char numbuf[G_ASCII_DTOSTR_BUF_SIZE + 1] = { 0, };
  g_ascii_formatd (numbuf, G_ASCII_DTOSTR_BUF_SIZE, "%.17g", value);
  return String (numbuf);
}

vector<double>
string_to_vector (const String &string)
{
  vector<double> dvec;
  const char *spaces = " \t\n";
  const char *obrace = "{([";
  const char *delims = ";";
  const char *cbrace = "])}";
  const char *number = "+-0123456789eE.,";
  const char *s = string.c_str();
  /* skip spaces */
  while (*s && strchr (spaces, *s))
    s++;
  /* skip opening brace */
  if (*s && strchr (obrace, *s))
    s++;
  const char *d = s;
  while (*d && !strchr (cbrace, *d))
    {
      while (*d && strchr (spaces, *d))         /* skip spaces */
        d++;
      s = d;                                    /* start of number */
      if (!*d || (!strchr (number, *d) &&       /* ... if any */
                  !strchr (delims, *d)))
        break;
      while (*d && strchr (number, *d))         /* pass across number */
        d++;
      dvec.push_back (string_to_double (String (s, d - s)));
      while (*d && strchr (spaces, *d))         /* skip spaces */
        d++;
      if (*d && strchr (delims, *d))
        d++;                                    /* eat delimiter */
    }
  // printf ("vector: %d: %s\n", dvec.size(), string_from_vector (dvec).c_str());
  return dvec;
}

String
string_from_vector (const vector<double> &dvec,
                    const String         &delim)
{
  String s;
  for (uint i = 0; i < dvec.size(); i++)
    {
      if (i > 0)
        s += delim;
      s += string_from_double (dvec[i]);
    }
  return s;
}

String
string_from_errno (int errno_val)
{
  if (errno_val < 0)
    errno_val = -errno_val;     // fixup library return values
  char buffer[1024] = { 0, };
  if (strerror_r (errno_val, buffer, sizeof (buffer)) < 0 || !buffer[0])
    {
      /* strerror_r() may be broken on GNU systems, especially if _GNU_SOURCE is defined, so fall back to strerror() */
      return strerror (errno_val);
    }
  return buffer;
}

bool
string_is_uuid (const String &uuid_string) /* check uuid formatting */
{
  int i, l = uuid_string.size();
  if (l != 36)
    return false;
  // 00000000-0000-0000-0000-000000000000
  for (i = 0; i < l; i++)
    if (i == 8 || i == 13 || i == 18 || i == 23)
      {
        if (uuid_string[i] != '-')
          return false;
        continue;
      }
    else if ((uuid_string[i] >= '0' && uuid_string[i] <= '9') ||
             (uuid_string[i] >= 'a' && uuid_string[i] <= 'f') ||
             (uuid_string[i] >= 'A' && uuid_string[i] <= 'F'))
      continue;
    else
      return false;
  return true;
}

int
string_cmp_uuid (const String &uuid_string1,
                 const String &uuid_string2) /* -1=smaller, 0=equal, +1=greater (assuming valid uuid strings) */
{
  return strcasecmp (uuid_string1.c_str(), uuid_string2.c_str()); /* good enough for numeric equality and defines stable order */
}

String
string_from_pretty_function_name (const char *gnuc_pretty_function)
{
  /* finding the function name is non-trivial in the presence of function pointer
   * return types. the following code assumes the function name preceedes the
   * first opening parenthesis not followed by a star.
   */
  const char *op = strchr (gnuc_pretty_function, '(');
  while (op && op[1] == '*')
    op = strchr (op + 1, '(');
  if (!op)
    return gnuc_pretty_function;
  // *op == '(' && op[1] != '*'
  const char *last = op - 1;
  while (last >= gnuc_pretty_function && strchr (" \t\n", *last))
    last--;
  if (last < gnuc_pretty_function)
    return gnuc_pretty_function;
  const char *first = last;
  while (first >= gnuc_pretty_function && strchr ("0123456789_ABCDEFGHIJKLMNOPQRSTUVWXYZ:abcdefghijklmnopqrstuvwxyz$", *first))
    first--;
  String result = String (first + 1, last - first);
  return result;
}

String
string_to_cescape (const String &str)
{
  String buffer;
  for (String::const_iterator it = str.begin(); it != str.end(); it++)
    {
      uint8 d = *it;
      if (d < 32 || d > 126 || d == '?')
        buffer += string_printf ("\\%03o", d);
      else if (d == '\\')
        buffer += "\\\\";
      else if (d == '"')
        buffer += "\\\"";
      else
        buffer += d;
    }
  return buffer;
}

String
string_to_cquote (const String &str)
{
  return String() + "\"" + string_to_cescape (str) + "\"";
}

String
string_from_cquote (const String &input)
{
  uint i = 0;
  if (i < input.size() && (input[i] == '"' || input[i] == '\''))
    {
      const char qchar = input[i];
      i++;
      String out;
      bool be = false;
      while (i < input.size() && (input[i] != qchar || be))
        {
          if (!be && input[i] == '\\')
            be = true;
          else
            {
              if (be)
                switch (input[i])
                  {
                    uint k, oc;
                  case '0': case '1': case '2': case '3':
                  case '4': case '5': case '6': case '7':
                    k = MIN (input.size(), i + 3);
                    oc = input[i++] - '0';
                    while (i < k && input[i] >= '0' && input[i] <= '7')
                      oc = oc * 8 + input[i++] - '0';
                    out += char (oc);
                    i--;
                    break;
                  case 'n':     out += '\n';            break;
                  case 'r':     out += '\r';            break;
                  case 't':     out += '\t';            break;
                  case 'b':     out += '\b';            break;
                  case 'f':     out += '\f';            break;
                  case 'v':     out += '\v';            break;
                  default:      out += input[i];        break;
                  }
              else
                out += input[i];
              be = false;
            }
          i++;
        }
      if (i < input.size() && input[i] == qchar)
        {
          i++;
          if (i < input.size())
            return input; // extraneous characters after string quotes
          return out;
        }
      else
        return input; // unclosed string quotes
    }
  else if (i == input.size())
    return input; // empty string arg: ""
  else
    return input; // missing string quotes
}

static const char *whitespaces = " \t\v\f\n\r";

String
string_lstrip (const String &input)
{
  uint64 i = 0;
  while (i < input.size() && strchr (whitespaces, input[i]))
    i++;
  return i ? input.substr (i) : input;
}

String
string_rstrip (const String &input)
{
  uint64 i = input.size();
  while (i > 0 && strchr (whitespaces, input[i - 1]))
    i--;
  return i < input.size() ? input.substr (0, i) : input;
}

String
string_strip (const String &input)
{
  uint64 a = 0;
  while (a < input.size() && strchr (whitespaces, input[a]))
    a++;
  uint64 b = input.size();
  while (b > 0 && strchr (whitespaces, input[b - 1]))
    b--;
  if (a == 0 && b == input.size())
    return input;
  else if (b == 0)
    return "";
  else
    return input.substr (a, b - a);
}

String
string_substitute_char (const String &input,
                        const char    match,
                        const char    subst)
{
  String output = input;
  if (match != subst)
    for (String::size_type i = 0; i < output.size(); i++)
      if (output.data()[i] == match)
        output[i] = subst; // unshares string
  return output;
}

/* --- string options --- */
static const char*
string_option_find_value (const String   &option_string,
                          const String   &option)
{
  const char *p, *match = NULL;
  int l = strlen (option.c_str());
  return_val_if_fail (l > 0, NULL);
  if (option_string == "")
    return NULL;        /* option not found */

  /* try first match */
  p = strstr (option_string.c_str(), option.c_str());
  if (p &&
      (p == option_string.c_str() || p[-1] == ':') &&
      (p[l] == ':' || p[l] == 0 || p[l] == '=' ||
       ((p[l] == '-' || p[l] == '+') && (p[l + 1] == ':' || p[l + 1] == 0))))
    match = p;
  /* allow later matches to override */
  while (p)
    {
      p = strstr (p + l, option.c_str());
      if (p &&
          p[-1] == ':' &&
          (p[l] == ':' || p[l] == 0 || p[l] == '=' ||
           ((p[l] == '-' || p[l] == '+') && (p[l + 1] == ':' || p[l + 1] == 0))))
        match = p;
    }
  return match ? match + l : NULL;
}

String
string_option_get (const String   &option_string,
                   const String   &option)
{
  const char *value = string_option_find_value (option_string, option);

  if (!value)
    return NULL;                        /* option not present */
  else switch (value[0])
      {
        const char *s;
      case ':':   return "1";           /* option was present, no modifier */
      case 0:     return "1";           /* option was present, no modifier */
      case '+':   return "1";           /* option was present, enable modifier */
      case '-':   return NULL;          /* option was present, disable modifier */
      case '=':                         /* option present with assignment */
        value++;
        s = strchr (value, ':');
        return s ? String (value, s - value) : value;
      default:    return NULL;            /* anything else, undefined */
      }
}

bool
string_option_check (const String   &option_string,
                     const String   &option)
{
  const char *value = string_option_find_value (option_string, option);

  if (!value)
    return false;                       /* option not present */
  else switch (value[0])
    {
      const char *s;
    case ':':   return true;            /* option was present, no modifier */
    case 0:     return true;            /* option was present, no modifier */
    case '+':   return true;            /* option was present, enable modifier */
    case '-':   return false;           /* option was present, disable modifier */
    case '=':                           /* option present with assignment */
      value++;
      s = strchr (value, ':');
      if (!s || s == value)             /* empty assignment */
        return false;
      else switch (s[0])
        {
        case '0': case 'f': case 'F':
        case 'n': case 'N':             /* false assigments */
          return false;
        default: return true;           /* anything else holds true */
        }
    default:    return false;           /* anything else, undefined */
    }
}


/* --- charset conversions --- */
static bool
unalias_encoding (String &name)
{
  /* list of common aliases for MIME encodings */
  static const char *encoding_aliases[] = {
    /* alias            MIME (GNU CANONICAL) */
    "UTF8",             "UTF-8",
    /* ascii */
    "646",              "ASCII",
    "ISO_646.IRV:1983", "ASCII",
    "CP20127",          "ASCII",
    /* iso8859 aliases */
    "LATIN1",           "ISO-8859-1",
    "LATIN2",           "ISO-8859-2",
    "LATIN3",           "ISO-8859-3",
    "LATIN4",           "ISO-8859-4",
    "LATIN5",           "ISO-8859-9",
    "LATIN6",           "ISO-8859-10",
    "LATIN7",           "ISO-8859-13",
    "LATIN8",           "ISO-8859-14",
    "LATIN9",           "ISO-8859-15",
    "LATIN10",          "ISO-8859-16",
    "ISO8859-1",        "ISO-8859-1",
    "ISO8859-2",        "ISO-8859-2",
    "ISO8859-3",        "ISO-8859-3",
    "ISO8859-4",        "ISO-8859-4",
    "ISO8859-5",        "ISO-8859-5",
    "ISO8859-6",        "ISO-8859-6",
    "ISO8859-7",        "ISO-8859-7",
    "ISO8859-8",        "ISO-8859-8",
    "ISO8859-9",        "ISO-8859-9",
    "ISO8859-13",       "ISO-8859-13",
    "ISO8859-15",       "ISO-8859-15",
    "CP28591",          "ISO-8859-1",
    "CP28592",          "ISO-8859-2",
    "CP28593",          "ISO-8859-3",
    "CP28594",          "ISO-8859-4",
    "CP28595",          "ISO-8859-5",
    "CP28596",          "ISO-8859-6",
    "CP28597",          "ISO-8859-7",
    "CP28598",          "ISO-8859-8",
    "CP28599",          "ISO-8859-9",
    "CP28603",          "ISO-8859-13",
    "CP28605",          "ISO-8859-15",
    /* EUC aliases */
    "eucCN",            "GB2312",
    "IBM-eucCN",        "GB2312",
    "dechanzi",         "GB2312",
    "eucJP",            "EUC-JP",
    "IBM-eucJP",        "EUC-JP",
    "sdeckanji",        "EUC-JP",
    "eucKR",            "EUC-KR",
    "IBM-eucKR",        "EUC-KR",
    "deckorean",        "EUC-KR",
    "eucTW",            "EUC-TW",
    "IBM-eucTW",        "EUC-TW",
    "CNS11643",         "EUC-TW",
    "CP20866",          "KOI8-R",
    /* misc */
    "PCK",              "SHIFT_JIS",
    "SJIS",             "SHIFT_JIS",
  };
  /* find a MIME encoding from alias list */
  for (uint i = 0; i < sizeof (encoding_aliases) / sizeof (encoding_aliases[0]); i += 2)
    if (strcasecmp (encoding_aliases[i], name.c_str()) == 0)
      {
        name = encoding_aliases[i + 1];
        return true;
      }
  /* last resort, try upper-casing the encoding name */
  String upper = name;
  for (uint i = 0; i < upper.size(); i++)
    if (upper[i] >= 'a' && upper[i] <= 'z')
      upper[i] += 'A' - 'a';
  if (upper != name)
    {
      name = upper;
      return true;
    }
  /* alias not found */
  return false;
}

static iconv_t
aliased_iconv_open (const String &tocode,
                    const String &fromcode)
{
  const iconv_t icNONE = (iconv_t) -1;
  iconv_t cd = iconv_open (tocode.c_str(), fromcode.c_str());
  if (cd != icNONE)
    return cd;
  /* lookup destination encoding from alias and retry */
  String to_encoding = tocode;
  if (unalias_encoding (to_encoding))
    {
      cd = iconv_open (to_encoding.c_str(), fromcode.c_str());
      if (cd != icNONE)
        return cd;
      /* lookup source and destination encoding from alias and retry */
      String from_encoding = fromcode;
      if (unalias_encoding (from_encoding))
        {
          cd = iconv_open (to_encoding.c_str(), from_encoding.c_str());
          if (cd != icNONE)
            return cd;
        }
    }
  /* lookup source encoding from alias and retry */
  String from_encoding = fromcode;
  if (unalias_encoding (from_encoding))
    {
      cd = iconv_open (tocode.c_str(), from_encoding.c_str());
      if (cd != icNONE)
        return cd;
    }
  return icNONE; /* encoding not found */
}

bool
text_convert (const String &to_charset,
              String       &output_string,
              const String &from_charset,
              const String &input_string,
              const String &fallback_charset,
              const String &output_mark)
{
  output_string = "";
  const iconv_t icNONE = (iconv_t) -1;
  iconv_t alt_cd = icNONE, cd = aliased_iconv_open (to_charset, from_charset);
  if (cd == icNONE)
    return false;                       /* failed to perform the requested conversion */
  const char *iptr = input_string.c_str();
  size_t ilength = input_string.size();
  char obuffer[1024];                   /* declared outside loop to spare re-initialization */
  String alt_charset = fallback_charset;
  while (ilength)
    {
      /* convert */
      char *optr = obuffer;
      size_t olength = sizeof (obuffer);
      size_t n = iconv (cd, const_cast<char**> (&iptr), &ilength, &optr, &olength);
      /* transfer output */
      output_string.append (obuffer, optr - obuffer);
      /* handle conversion errors */
      if (ilength &&                    /* ignore past end errors */
          n == (size_t) -1)
        {
          if (errno == EINVAL ||        /* unfinished multibyte sequences follows (near end of string) */
              errno == EILSEQ)          /* invalid multibyte sequence follows */
            {
              /* open alternate converter */
              if (alt_cd == icNONE && alt_charset.size())
                {
                  alt_cd = aliased_iconv_open (to_charset, alt_charset);
                  alt_charset = ""; /* don't retry iconv_open() */
                }
              size_t former_ilength = ilength;
              if (alt_cd != icNONE)
                {
                  /* convert from alt_charset */
                  optr = obuffer;
                  olength = sizeof (obuffer);
                  n = iconv (alt_cd, const_cast<char**> (&iptr), &ilength, &optr, &olength);
                  /* transfer output */
                  output_string.append (obuffer, optr - obuffer);
                }
              if (ilength == former_ilength)
                {
                  /* failed alternate conversion, mark invalid character */
                  output_string += output_mark;
                  iptr++;
                  ilength--;
                }
            }
          else  /* all other errors are considered fatal */
            return false;               /* failed to perform the requested conversion */
        }
    }
  iconv_close (cd);
  if (alt_cd != icNONE)
    iconv_close (alt_cd);
  return true;

}

/* --- timestamp handling --- */
uint64 // system clock in usecs
timestamp_now ()
{
  struct timeval now = { 0, 0 };
  gettimeofday (&now, NULL);
  uint64 stamp = now.tv_sec; // promotes to 64bit
  stamp = 1000000 * stamp + now.tv_usec;
  return stamp;
}

/**
 * @namespace Rapicorn::Path
 * The Rapicorn::Path namespace covers function for file path manipulation and evaluation.
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
  String pcwd = process_cwd();
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
    g_error ("%s: hook is being destroyed but not unlinked: %p", G_STRFUNC, this);
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
static DeletableMap * volatile deletable_maps = NULL;

static inline void
auto_init_deletable_maps (void)
{
  if (UNLIKELY (deletable_maps == NULL))
    {
      DeletableMap *dmaps = new DeletableMap[DELETABLE_MAP_HASH];
      if (!Atomic::ptr_cas (&deletable_maps, (DeletableMap*) NULL, dmaps))
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
  if (NULL == Atomic::ptr_get (&deletable_maps))
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
  SpinLock     mutex;
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
  return_if_fail (unique_id >= counterstart && unique_id < counter);
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
static SpinLock           locatable_mutex;
static vector<Locatable*> locatable_objs;
static IdAllocatorImpl    locator_ids (1, 227); // has own mutex

Locatable::Locatable () :
  m_locatable_index (0)
{}

Locatable::~Locatable ()
{
  if (UNLIKELY (m_locatable_index))
    {
      ScopedLock<SpinLock> locker (locatable_mutex);
      return_if_fail (m_locatable_index <= locatable_objs.size());
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
      ScopedLock<SpinLock> locker (locatable_mutex);
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
  ScopedLock<SpinLock> locker (locatable_mutex);
  return_val_if_fail (index < locatable_objs.size(), NULL);
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
    error ("ReferenceCountable object allocated on stack instead of heap: %zu > %zu (%p - %p)",
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
        Msg::display (debug_browser, "show \"%s\": %s: %s", url, args[0], error ? error->message : fallback_error);
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

/* --- string utils --- */
void
memset4 (guint32        *mem,
         guint32         filler,
         guint           length)
{
  RAPICORN_STATIC_ASSERT (sizeof (*mem) == 4);
  RAPICORN_STATIC_ASSERT (sizeof (filler) == 4);
  RAPICORN_STATIC_ASSERT (sizeof (wchar_t) == 4);
  wmemset ((wchar_t*) mem, filler, length);
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
  return_if_fail (plor_name() == "");
  if (plor_add (*this, _plor_name))
    set_data (&plor_name_key, _plor_name);
  else
    warning ("invalid plor name for object (%p): %s", this, _plor_name.c_str());
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
    g_error ("failed to decompress (%p, %u): %s", cdata, cdata_size, err);
  
  text[dlen] = 0;
  return text;          /* success */
}

void
zintern_free (uint8 *dc_data)
{
  g_free (dc_data);
}

} // Rapicorn
