// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "main.hh"
#include "../configure.h"
#include "inout.hh"
#include "strings.hh"
#include "thread.hh"
#include "testutils.hh"
#include <unistd.h>
#include <string.h>
#include <algorithm>

#include <glib.h>

namespace Rapicorn {

String  rapicorn_version ()     { return RAPICORN_VERSION; }
String  rapicorn_buildid ()     { return RapicornInternal::buildid(); }

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
  *boolp = false;
  if (s.size() > length && s[length] == '=' && strncmp (&s[0], arg, length) == 0)
    {
      *boolp = string_to_bool (s.substr (length + 1));
      return true;
    }
  else if (s == arg)
    {
      *boolp = true;
      return true;
    }
  return false;
}

// === initialization ===
static const char   *program_argv0_ = NULL;
static const String *program_name_ = NULL;
static const String *application_name_ = NULL;

struct VInitSettings : InitSettings {
  bool&   autonomous()  { return autonomous_; }
  uint64& test_codes()  { return test_codes_; }
  VInitSettings() { autonomous_ = false; test_codes_ = 0; }
};
static VInitSettings vinit_settings;
const InitSettings  &InitSettings::is = vinit_settings;
static const char *internal_init_args_ = NULL;

static void
parse_settings_and_args (VInitSettings &vsettings, int *argcp, char **argv, const StringVector &args)
{
  static_assert (sizeof (NULL) == sizeof (void*), "NULL must be defined to __null in C++ on 64bit");

  // setup program and application name
  if (argcp && *argcp && argv && argv[0] && argv[0][0] != 0 && !program_argv0_)
    program_argv0_init (argv[0]);

  bool b, testing_mode = false;
  uint64 tco = 0;
  // apply init settings
  const char *const internal_args = internal_init_args_ ? internal_init_args_ : "";
  internal_init_args_ = NULL;
  for (StringVector::const_iterator it = args.begin(); it != args.end(); it++)
    if      (parse_bool_option (*it, "autonomous", &b))
      vsettings.autonomous() = b;
    else if (parse_bool_option (*it, "testing", &b))
      testing_mode = b;
    else if (parse_bool_option (*it, "test-verbose", &b) && b)
      tco |= Test::MODE_VERBOSE;
    else if (parse_bool_option (*it, "test-slow", &b) && b)
      tco |= Test::MODE_SLOW;
  if (strstr (internal_args, ":autonomous:"))
    vsettings.autonomous() = true;
  if (strstr (internal_args, ":testing:"))
    testing_mode = true;
  if (strstr (internal_args, ":test-verbose:"))
    tco |= Test::MODE_VERBOSE;
  if (strstr (internal_args, ":test-slow:"))
    tco |= Test::MODE_SLOW;
  if (testing_mode)
    vsettings.test_codes() |= Test::MODE_TESTING | tco;
  // parse command line args
  bool fatal_warnings = strstr (internal_args, ":fatal-warnings:") != NULL;
  const size_t argc = *argcp;
  for (size_t i = 1; i < argc; i++)
    {
      if (arg_parse_option (*argcp, argv, &i, "--fatal-warnings") ||
          arg_parse_option (*argcp, argv, &i, "--g-fatal-warnings")) // legacy option support
        fatal_warnings = true;
      else if (testing_mode && arg_parse_option (*argcp, argv, &i, "--test-verbose"))
        vsettings.test_codes() |= Test::MODE_VERBOSE;
      else if (testing_mode && arg_parse_option (*argcp, argv, &i, "--test-slow"))
        vsettings.test_codes() |= Test::MODE_SLOW;
    }
  if (fatal_warnings)
    {
      debug_config_add ("fatal-warnings");
      const uint fatal_mask = g_log_set_always_fatal (GLogLevelFlags (G_LOG_FATAL_MASK));
      g_log_set_always_fatal (GLogLevelFlags (fatal_mask | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL));
    }
  // collapse NULL arguments
  arg_parse_collapse (argcp, argv);
  // incorporate test flags from RAPICORN_TEST
  if (testing_mode)
    {
      auto test_flipper_check = [] (const char *key) { return envkey_feature_check ("RAPICORN_TEST", key, false, NULL, false); };
      vsettings.test_codes() |= test_flipper_check ("test-verbose") ? Test::MODE_VERBOSE : 0;
      vsettings.test_codes() |= test_flipper_check ("test-slow") ? Test::MODE_SLOW : 0;
    }
}

/// File name of the current process as set in argv[0] at startup.
String
program_argv0 ()
{
  // assert program_argv0_init() has been called earlier
  const char *fallback = "";
#ifdef  _GNU_SOURCE
  fallback = program_invocation_name;
#endif
  assert_return (program_argv0_ != NULL, fallback);
  return program_argv0_;
}

// Early initialization of program_argv0() from argv0 as passed into main().
void
program_argv0_init (const char *argv0)
{
  assert_return (argv0 != NULL);
  if (program_argv0_)
    {
      assert_return (strcmp (program_argv0_, argv0) == 0); // there's only *one* argv[0]
      return;
    }
  const char *libc_argv0 = NULL;
#ifdef  _GNU_SOURCE
  libc_argv0 = program_invocation_name; // from glibc
  assert_return (strcmp (program_invocation_name, argv0) == 0); // there's only *one* argv[0]
#endif
  program_argv0_ = libc_argv0 ? libc_argv0 : strdup (argv0);
}

/// Formal name of the program, used to retrieve resources and store session data.
String
program_name ()
{
  if (program_name_)
    return *program_name_;
  if (program_argv0_)
    return program_argv0_;
#ifdef  _GNU_SOURCE
  if (program_invocation_name) // from glibc
    return program_invocation_name;
#endif
  return "/proc/self/exe";
}

/** Provide short name for the current process.
 * The short name is usually the last part of argv[0]. See also GNU Libc program_invocation_short_name.
 */
String
program_alias ()
{
  if (program_name_)
    {
      const char *slash = strchr (program_name_->c_str(), '/');
      return slash ? slash + 1 : *program_name_;
    }
  return program_invocation_short_name; // _GNU_SOURCE?
}

struct CwdString {
  String dir_;
  CwdString() : dir_ (Path::cwd()) {}
};
static DurableInstance<CwdString> program_cwd_;         // a DurableInstance Class is create on-demand
static Init program_cwd_init ([]() { program_cwd(); }); // force program_cwd_ initialization during static ctors

/// The current working directory during startup.
String
program_cwd ()
{
  return program_cwd_->dir_;
}

/// Application name suitable for user interface display.
String
application_name ()
{
  if (application_name_)
    return *application_name_;
  return program_alias();
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

void
init_core (int *argcp, char **argv, const StringVector &args)
{
  static int initialized = 0;
  assert_return (initialized++ == 0);

  // ensure logging is fully initialized
  const char *env_rapicorn = getenv ("RAPICORN");
  RAPICORN_STARTUP_DEBUG ("$RAPICORN=%s", env_rapicorn ? env_rapicorn : "");

  // setup init settings
  parse_settings_and_args (vinit_settings, argcp, argv, args);

  // initialize sub systems
  struct InitHookCaller : public InitHook {
    static void  invoke (const String &kind, int *argcp, char **argv, const StringVector &args)
    { invoke_hooks (kind, argcp, argv, args); }
  };
  InitHookCaller::invoke ("core/", argcp, argv, args);
  InitHookCaller::invoke ("threading/", argcp, argv, args);
}

} // Rapicorn

namespace RapicornInternal {

bool
application_setup (const String &application_name, const String &program_name)
{
  assert_return (application_name.empty() == false, false);
  if (application_name_)
    return false;
  if (!program_argv0_)
    program_argv0_init (program_invocation_name);       // need _GNU_SOURCE?
  application_name_ = new String (application_name);
  if (!program_name_ && !program_name.empty())
    program_name_ = new String (program_name);
  return true;
}

void
inject_init_args (const char *const internal_args)
{
  internal_init_args_ = internal_args;
}

} // RapicornInternal
