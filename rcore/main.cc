// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "main.hh"
#include "config/config.h"
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

static bool
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

  return true;
}

/** Parse Rapicorn initialization arguments and adjust global settings.
 * Note that parse_init_args() only parses arguments the first time it is called.
 * Arguments specific to the Rapicorn toolkit are removed from the @a argc, @a argv
 * array passed in.
 */
bool
parse_init_args (int *argcp, char **argv, const StringVector &args)
{
  static bool initialized = parse_settings_and_args (vinit_settings, argcp, argv, args);
  assert (initialized != false);
  return true;
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
