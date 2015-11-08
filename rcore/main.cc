// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "main.hh"
#include "../configure.h"
#include "inout.hh"
#include "strings.hh"
#include "thread.hh"
#include "testutils.hh"
#include "configbits.cc"
#include <unistd.h>
#include <string.h>
#include <algorithm>

#include <glib.h>

namespace Rapicorn {

String  rapicorn_version ()     { return RAPICORN_VERSION; }
String  rapicorn_buildid ()     { return RAPICORN_BUILDID; }

// === initialization hooks ===
static InitHook *init_hooks = NULL;

InitHook::InitHook (const String &fname, InitHookFunc func) :
  next (NULL), hook (func), name_ (fname)
{
  next = init_hooks;
  init_hooks = this;
}

static int
init_hook_cmp (const InitHook *const &v1, const InitHook *const &v2)
{
  static const char *levels[] = { "core/", "threading/", "ui/" };
  uint l1 = UINT_MAX, l2 = UINT_MAX;
  for (uint i = 0; i < RAPICORN_ARRAY_SIZE (levels); i++)
    {
      const uint len = strlen (levels[i]);
      if (l1 == UINT_MAX && strncmp (levels[i], v1->name().c_str(), len) == 0)
        {
          l1 = i;
          if (l2 != UINT_MAX)
            break;
        }
      if (l2 == UINT_MAX && strncmp (levels[i], v2->name().c_str(), len) == 0)
        {
          l2 = i;
          if (l1 != UINT_MAX)
            break;
        }
    }
  if (l1 != l2)
    return l1 < l2 ? -1 : l2 > l1;
  return strverscmp (v1->name().c_str(), v2->name().c_str()) < 0;
}

static StringVector *init_hook_main_args = NULL;

StringVector
InitHook::main_args () const
{
  return *init_hook_main_args;
}

void
InitHook::invoke_hooks (const String &kind, int *argcp, char **argv, const StringVector &args)
{
  std::vector<InitHook*> hv;
  for (InitHook *ihook = init_hooks; ihook; ihook = ihook->next)
    hv.push_back (ihook);
  stable_sort (hv.begin(), hv.end(), init_hook_cmp);
  StringVector main_args;
  for (uint i = 1; i < uint (*argcp); i++)
    main_args.push_back (argv[i]);
  init_hook_main_args = &main_args;
  for (std::vector<InitHook*>::iterator it = hv.begin(); it != hv.end(); it++)
    {
      String name = (*it)->name();
      if (name.size() > kind.size() && strncmp (name.data(), kind.data(), kind.size()) == 0)
        {
          RAPICORN_STARTUP_DEBUG ("InitHook: %s", name.c_str());
          (*it)->hook (args);
        }
    }
  init_hook_main_args = NULL;
}

// === arg parsers ===
/**
 * Try to parse argument @a arg at position @a i in @a argv.
 * If successfull, @a i is incremented and the argument
 * is set to NULL.
 * @returns true if successfull.
 */
bool
arg_parse_option (uint         argc,
                  char       **argv,
                  size_t      *i,
                  const char  *arg)
{
  if (strcmp (argv[*i], arg) == 0)
    {
      argv[*i] = NULL;
      return true;
    }
  return false;
}

/**
 * Try to parse argument @a arg at position @a i in @a argv.
 * If successfull, @a i is incremented and the argument and possibly
 * the next option argument are set to NULL.
 * @returns true if successfull and the string option in @a strp.
 */
bool
arg_parse_string_option (uint         argc,
                         char       **argv,
                         size_t      *i,
                         const char  *arg,
                         const char **strp)
{
  const size_t length = strlen (arg);
  if (strncmp (argv[*i], arg, length) == 0)
    {
      const char *equal = argv[*i] + length;
      if (*equal == '=')                // -x=VAL
        *strp = equal + 1;
      else if (*equal)                  // -xVAL
        *strp = equal;
      else if (*i + 1 < argc)           // -x VAL
        {
          argv[(*i)++] = NULL;
          *strp = argv[*i];
        }
      else
        *strp = NULL;
      argv[*i] = NULL;
      if (*strp)
        return true;
    }
  return false;
}

/**
 * Collapse @a argv by eliminating NULL strings.
 * @returns Number of collapsed arguments.
 */
int
arg_parse_collapse (int   *argcp,
                    char **argv)
{
  // collapse args
  const uint argc = *argcp;
  uint e = 1;
  for (uint i = 1; i < argc; i++)
    if (argv[i])
      {
        argv[e++] = argv[i];
        if (i >= e)
          argv[i] = NULL;
      }
  const int collapsed = *argcp - e;
  *argcp = e;
  return collapsed;
}

static bool
parse_bool_option (const String &s, const char *arg, bool *boolp)
{
  const size_t length = strlen (arg);
  if (s.size() > length && s[length] == '=' && strncmp (&s[0], arg, length) == 0)
    {
      *boolp = string_to_bool (s.substr (length + 1));
      return true;
    }
  return false;
}

// === initialization ===
struct VInitSettings : InitSettings {
  bool&   autonomous()  { return autonomous_; }
  uint64& test_codes()  { return test_codes_; }
  VInitSettings() { autonomous_ = false; test_codes_ = 0; }
};
static VInitSettings vinit_settings;
const InitSettings  &InitSettings::is = vinit_settings;

static void
parse_settings_and_args (VInitSettings &vsettings, int *argcp, char **argv, const StringVector &args)
{
  bool b, testing_mode = false;
  uint64 tco = 0;
  // apply init settings
  for (StringVector::const_iterator it = args.begin(); it != args.end(); it++)
    if      (parse_bool_option (*it, "autonomous", &b))
      vsettings.autonomous() = b;
    else if (parse_bool_option (*it, "rapicorn-test-initialization", &b))
      testing_mode = b;
    else if (parse_bool_option (*it, "test-verbose", &b))
      tco |= Test::MODE_VERBOSE;
    else if (parse_bool_option (*it, "test-readout", &b))
      tco |= Test::MODE_READOUT;
    else if (parse_bool_option (*it, "test-slow", &b))
      tco |= Test::MODE_SLOW;
  if (testing_mode)
    vsettings.test_codes() |= Test::MODE_TESTING | tco;
  // parse command line args
  const size_t argc = *argcp;
  for (size_t i = 1; i < argc; i++)
    {
      if (            arg_parse_option (*argcp, argv, &i, "--fatal-warnings") ||
                      arg_parse_option (*argcp, argv, &i, "--g-fatal-warnings")) // legacy option support
        {
          debug_config_add ("fatal-warnings");
          const uint fatal_mask = g_log_set_always_fatal (GLogLevelFlags (G_LOG_FATAL_MASK));
          g_log_set_always_fatal (GLogLevelFlags (fatal_mask | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL));
        }
      else if (testing_mode && arg_parse_option (*argcp, argv, &i, "--test-verbose"))
        vsettings.test_codes() |= Test::MODE_VERBOSE;
      else if (testing_mode && arg_parse_option (*argcp, argv, &i, "--test-readout"))
        vsettings.test_codes() |= Test::MODE_READOUT;
      else if (testing_mode && arg_parse_option (*argcp, argv, &i, "--test-slow"))
        vsettings.test_codes() |= Test::MODE_SLOW;
    }
  // collapse NULL arguments
  arg_parse_collapse (argcp, argv);
  // incorporate test flags from RAPICORN_TEST
  if (testing_mode)
    {
      auto test_flipper_check = [] (const char *key) { return envkey_feature_check ("RAPICORN_TEST", key, false, NULL, false); };
      vsettings.test_codes() |= test_flipper_check ("test-verbose") ? Test::MODE_VERBOSE : 0;
      vsettings.test_codes() |= test_flipper_check ("test-readout") ? Test::MODE_READOUT : 0;
      vsettings.test_codes() |= test_flipper_check ("test-slow") ? Test::MODE_SLOW : 0;
      if (vsettings.test_codes() & Test::MODE_READOUT)
        vsettings.test_codes() |= Test::MODE_VERBOSE;
    }
}

static String program_argv0 = "";
static String program_app_ident = ""; // used to flag init_core() initialization
static String program_cwd0 = "";

/**
 * File name of the current process as set in argv[0] at startup.
 */
String
program_file ()
{
  return program_argv0;
}

/**
 * Provides a short name for the current process, usually the last part of argv[0].
 * See also GNU Libc program_invocation_short_name.
 */
String
program_alias ()
{
#ifdef  _GNU_SOURCE
  return program_invocation_short_name;
#else
  const char *last = strrchr (program_argv0.c_str(), '/');
  return last ? last + 1 : "";
#endif
}

/// The program identifier @a app_ident as specified during initialization of Rapicorn.
String
program_ident ()
{
  return program_app_ident;
}

/**
 * The current working directory during startup.
 */
String
program_cwd ()
{
  return program_cwd0;
}

static Mutex       prng_mutex;
static KeccakPRNG *keccak_prng = NULL;

static inline KeccakPRNG&
initialize_random_generator_LOCKED()
{
  if (RAPICORN_UNLIKELY (!keccak_prng))
    {
      assert (prng_mutex.try_lock() == false);
      static uint64 space[(sizeof (KeccakPRNG) + 7) / 8];
      Entropy e;
      keccak_prng = new (space) KeccakPRNG (e); // uses e as generator & initializes Entropy
    }
  return *keccak_prng;
}

/// Provide a unique 64 bit identifier that is not 0, see also random_int64().
uint64_t
random_nonce ()
{
  ScopedLock<Mutex> locker (prng_mutex);
  KeccakPRNG &rgen = initialize_random_generator_LOCKED();
  uint64_t nonce = rgen();
  while (RAPICORN_UNLIKELY (nonce == 0))
    nonce = rgen();
  return nonce;
}

/** Generate uniformly distributed 64 bit pseudo-random number.
 * This function generates a pseudo-random number using class KeccakPRNG,
 * seeded from class Entropy.
 */
uint64_t
random_int64 ()
{
  ScopedLock<Mutex> locker (prng_mutex);
  KeccakPRNG &rgen = initialize_random_generator_LOCKED();
  return rgen();
}

/** Generate uniformly distributed pseudo-random integer within a range.
 * This function generates a pseudo-random number using class KeccakPRNG,
 * seeded from class Entropy.
 * The generated number will be in the range: @a begin <= number < @a end.
 */
int64_t
random_irange (int64_t begin, int64_t end)
{
  return_unless (begin < end, begin);
  const uint64_t range    = end - begin;
  const uint64_t quotient = 0xffffffffffffffffULL / range;
  const uint64_t bound    = quotient * range;
  ScopedLock<Mutex> locker (prng_mutex);
  KeccakPRNG &rgen = initialize_random_generator_LOCKED();
  uint64_t r = rgen ();
  while (RAPICORN_UNLIKELY (r >= bound))        // repeats with <50% probability
    r = rgen ();
  return begin + r / quotient;
}

/** Generate uniformly distributed pseudo-random floating point number.
 * This function generates a pseudo-random number using class KeccakPRNG,
 * seeded from class Entropy.
 * The generated number will be in the range: 0.0 <= number < 1.0.
 */
double
random_float ()
{
  ScopedLock<Mutex> locker (prng_mutex);
  KeccakPRNG &rgen = initialize_random_generator_LOCKED();
  double r01;
  do
    r01 = rgen() * 5.42101086242752217003726400434970855712890625e-20; // 1.0 / 2^64
  while (RAPICORN_UNLIKELY (r01 >= 1.0));       // retry if arithmetic exceeds boundary
  return r01;
}

/** Generate uniformly distributed pseudo-random floating point number within a range.
 * This function generates a pseudo-random number using class KeccakPRNG,
 * seeded from class Entropy.
 * The generated number will be in the range: @a begin <= number < @a end.
 */
double
random_frange (double begin, double end)
{
  return_unless (begin < end, begin + 0 * end); // catch and propagate NaNs
  ScopedLock<Mutex> locker (prng_mutex);
  KeccakPRNG &rgen = initialize_random_generator_LOCKED();
  const double r01 = rgen() * 5.42101086242752217003726400434970855712890625e-20; // 1.0 / 2^64
  return end * r01 + (1.0 - r01) * begin;
}

ScopedLocale::ScopedLocale (locale_t scope_locale) :
  locale_ (NULL)
{
  if (!scope_locale)
    locale_ = uselocale (LC_GLOBAL_LOCALE);     // use process locale
  else
    locale_ = uselocale (scope_locale);         // use custom locale
  assert (locale_ != NULL);
}

ScopedLocale::~ScopedLocale ()
{
  uselocale (locale_);                          // restore locale
}

#if 0
ScopedLocale::ScopedLocale (const String &locale_name = "")
{
  /* this constructor should:
   * - uselocale (LC_GLOBAL_LOCALE) if locale_name == "",
   * - create newlocale from locale_name, use it and later delete it, but:
   * - freelocale(newlocale()) seems buggy on glibc-2.7 (crashes)
   */
}
#endif

ScopedPosixLocale::ScopedPosixLocale () :
  ScopedLocale (posix_locale())
{}

locale_t
ScopedPosixLocale::posix_locale ()
{
  static locale_t volatile posix_locale_ = NULL;
  if (!posix_locale_)
    {
      locale_t posix_locale = NULL;
      if (!posix_locale)
        posix_locale = newlocale (LC_ALL_MASK, "POSIX.UTF-8", NULL);
      if (!posix_locale)
        posix_locale = newlocale (LC_ALL_MASK, "C.UTF-8", NULL);
      if (!posix_locale)
        posix_locale = newlocale (LC_ALL_MASK, "POSIX", NULL);
      if (!posix_locale)
        posix_locale = newlocale (LC_ALL_MASK, "C", NULL);
      if (!posix_locale)
        posix_locale = newlocale (LC_ALL_MASK, NULL, NULL);
      assert (posix_locale != NULL);
      if (!__sync_bool_compare_and_swap (&posix_locale_, NULL, posix_locale))
        freelocale (posix_locale_);
    }
  return posix_locale_;
}


static struct __StaticCTorTest {
  int v;
  __StaticCTorTest() : v (0x12affe16)
  {
    v++;
    ThreadInfo::self().name ("MainThread");
  }
} __staticctortest;

/// Check and return if init_core() has already been called.
bool
init_core_initialized ()
{
  return program_app_ident.empty() == false;
}

/**
 * @param app_ident     Application identifier, used to associate persistent resources
 * @param argcp         location of the 'argc' argument to main()
 * @param argv          location of the 'argv' arguments to main()
 * @param args          program specific initialization values
 *
 * Initialize the Rapicorn toolkit core, including threading, CPU detection, loading resource libraries, etc.
 * The arguments passed in @a argcp and @a argv are parsed and any Rapicorn specific arguments
 * are stripped.
 * If 'rapicorn-test-initialization=1' is passed in @a args, these command line arguments are supported:
 * - @c --test-verbose - Execute test cases with verbose message generation.
 * - @c --test-readout - Execute only data driven test cases to verify readouts.
 * - @c --test-slow - Execute only test cases excercising slow code paths or loops.
 * .
 * Additional initialization arguments can be passed in @a args, currently supported are:
 * - @c autonomous - Flag indicating a self-contained runtime environment (e.g. for tests) without loading rc-files, etc.
 * - @c cpu-affinity - CPU# to bind rapicorn thread to.
 * - @c rapicorn-test-initialization - Enable testing framework, used by init_core_test(), see also #$RAPICORN_TEST.
 * - @c test-verbose - acts like --test-verbose.
 * - @c test-readout - acts like --test-readout.
 * - @c test-slow - acts like --test-slow.
 * .
 * Additionally, the @c $RAPICORN environment variable affects toolkit behaviour. It supports
 * multiple colon (':') separated options (options can be prfixed with 'no-' to disable):
 * - @c debug - Enables verbose debugging output (default=off).
 * - @c fatal-syslog - Fatal program conditions that lead to aborting are recorded via syslog (default=on).
 * - @c syslog - Critical and warning conditions are recorded via syslog (default=off).
 * - @c fatal-warnings - Critical and warning conditions are treated as fatal conditions (default=off).
 * - @c logfile=FILENAME - Record all messages and conditions into FILENAME.
 * .
 */
void
init_core (const String &app_ident, int *argcp, char **argv, const StringVector &args)
{
  assert_return (app_ident.empty() == false);   // application identifier is hard requirement
  // rudimentary tests
  if (__staticctortest.v != 0x12affe17)                 // check global_ctors work
    {
      errno = ENOTSUP;
      perror ("librapicorncore: runtime error: C++ constructors have not been executed");
      _exit (127);
    }
  static_assert (sizeof (NULL) == sizeof (void*), "NULL must be defined to __null in C++ on 64bit");

  // guard against double initialization, checks if program_app_ident has already been set
  if (init_core_initialized())
    return;
  program_app_ident = app_ident;

  // setup program and application name
  if (program_argv0.empty() && argcp && *argcp && argv && argv[0] && argv[0][0] != 0)
    program_argv0 = argv[0];
  if (program_cwd0.empty())
    program_cwd0 = Path::cwd();
  if (!g_get_prgname() && !program_argv0.empty())
    g_set_prgname (Path::basename (program_argv0).c_str());
  if (!g_get_application_name() || g_get_application_name() == g_get_prgname())
    g_set_application_name (program_app_ident.c_str());
  if (!program_argv0.empty())
    ThreadInfo::self().name (string_format ("%s-MainThread", Path::basename (program_argv0).c_str()));

  // ensure logging is fully initialized
  const char *env_rapicorn = getenv ("RAPICORN");
  RAPICORN_STARTUP_DEBUG ("$RAPICORN=%s", env_rapicorn ? env_rapicorn : "");

  // full locale initialization is needed by X11, etc
  if (!setlocale (LC_ALL,""))
    {
      auto sgetenv = [] (const char *var)  {
        const char *str = getenv (var);
        return str ? str : "";
      };
      String lv = string_format ("LANGUAGE=%s;LC_ALL=%s;LC_MONETARY=%s;LC_MESSAGES=%s;LC_COLLATE=%s;LC_CTYPE=%s;LC_TIME=%s;LANG=%s",
                                 sgetenv ("LANGUAGE"), sgetenv ("LC_ALL"), sgetenv ("LC_MONETARY"), sgetenv ("LC_MESSAGES"),
                                 sgetenv ("LC_COLLATE"), sgetenv ("LC_CTYPE"), sgetenv ("LC_TIME"), sgetenv ("LANG"));
      RAPICORN_STARTUP_DEBUG ("environment: %s", lv.c_str());
      setlocale (LC_ALL, "C");
      RAPICORN_STARTUP_DEBUG ("failed to initialize locale, falling back to \"C\"");
    }

  // setup init settings
  parse_settings_and_args (vinit_settings, argcp, argv, args);

  // initialize sub systems
  struct InitHookCaller : public InitHook {
    static void  invoke (const String &kind, int *argcp, char **argv, const StringVector &args)
    { invoke_hooks (kind, argcp, argv, args); }
  };
  InitHookCaller::invoke ("core/", argcp, argv, args);
  InitHookCaller::invoke ("threading/", argcp, argv, args);

  // initialize testing framework
  if (vinit_settings.test_codes() & Test::MODE_TESTING)
    {
      debug_config_add ("fatal-warnings");
      const uint fatal_mask = g_log_set_always_fatal (GLogLevelFlags (G_LOG_FATAL_MASK));
      g_log_set_always_fatal (GLogLevelFlags (fatal_mask | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL));
      String ci = cpu_info(); // initialize cpu info
      (void) ci; // silence compiler
    }
}

} // Rapicorn
