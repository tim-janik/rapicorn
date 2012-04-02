// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "main.hh"
#include "strings.hh"
#include <string.h>
#include <algorithm>

#include <glib.h>

namespace Rapicorn {

// === initialization hooks ===
static InitHook *init_hooks = NULL;

InitHook::InitHook (const String &fname, InitHookFunc func) :
  next (NULL), hook (func), m_name (fname)
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
          RAPICORN_DEBUG ("InitHook: %s", name.c_str());
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
  bool& autonomous()    { return m_autonomous; }
  uint& test_codes()    { return m_test_codes; }
  VInitSettings() { m_autonomous = false; m_test_codes = 0; }
} static vsettings;
static VInitSettings vinit_settings;
const InitSettings  *InitSettings::sis = &vinit_settings;

static void
parse_settings_and_args (VInitSettings      &vsettings,
                         const StringVector &args,
                         int                *argcp,
                         char              **argv)
{
  bool b = 0, pta = false;
  // apply init settings
  for (StringVector::const_iterator it = args.begin(); it != args.end(); it++)
    if      (parse_bool_option (*it, "autonomous", &b))
      vsettings.autonomous() = b;
    else if (parse_bool_option (*it, "parse-testargs", &b))
      pta = b;
    else if (parse_bool_option (*it, "test-verbose", &b))
      vsettings.test_codes() |= 0x1;
    else if (parse_bool_option (*it, "test-log", &b))
      vsettings.test_codes() |= 0x2;
    else if (parse_bool_option (*it, "test-slow", &b))
      vsettings.test_codes() |= 0x4;
  // parse command line args
  const size_t argc = *argcp;
  for (size_t i = 1; i < argc; i++)
    {
      if (            arg_parse_option (*argcp, argv, &i, "--g-fatal-warnings"))
        {
          const uint fatal_mask = g_log_set_always_fatal (GLogLevelFlags (G_LOG_FATAL_MASK));
          g_log_set_always_fatal (GLogLevelFlags (fatal_mask | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL));
        }
      else if (pta && arg_parse_option (*argcp, argv, &i, "--test-verbose"))
        vsettings.test_codes() |= 0x1;
      else if (pta && arg_parse_option (*argcp, argv, &i, "--test-log"))
        vsettings.test_codes() |= 0x2;
      else if (pta && arg_parse_option (*argcp, argv, &i, "--test-slow"))
        vsettings.test_codes() |= 0x4;
      else if (pta && strcmp ("--verbose", argv[i]) == 0)
        {
          vsettings.test_codes() |= 0x1;
          /* interpret --verbose for GLib compat but don't delete the argument
           * since regular non-test programs may need this. could be fixed by
           * having a separate test program argument parser.
           */
        }
    }
  // collapse NULL arguments
  arg_parse_collapse (argcp, argv);
}

static String program_argv0 = "";
static String program_app_ident = "";
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
 * The program identifier @a app_ident as specified during initialization of Rapicorn.
 */
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

static struct __StaticCTorTest { int v; __StaticCTorTest() : v (0x12affe16) { v++; } } __staticctortest;

/**
 * @param app_ident     Application identifier, used to associate persistent resources
 * @param argcp         location of the 'argc' argument to main()
 * @param argv          location of the 'argv' arguments to main()
 * @param args          program specific initialization values
 *
 * Initialize the Rapicorn toolkit, including threading, CPU detection, loading resource libraries, etc.
 * The arguments passed in @a argcp and @a argv are parsed and any Rapicorn specific arguments
 * are stripped.
 * Supported command line arguments are:
 * - @c --test-verbose - execute test cases verbosely.
 * - @c --test-log - execute logtest test cases.
 * - @c --test-slow - execute slow test cases.
 * - @c --verbose - behaves like @c --test-verbose, this option is recognized but not stripped.
 * .
 * Additional initialization arguments can be passed in @a args, currently supported are:
 * - @c autonomous - For test programs to request a self-contained runtime environment.
 * - @c cpu-affinity - CPU# to bind rapicorn thread to.
 * - @c parse-testargs - Used by init_core_test() internally.
 * - @c test-verbose - acts like --test-verbose.
 * - @c test-log - acts like --test-log.
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
init_core (const String       &app_ident,
           int                *argcp,
           char              **argv,
           const StringVector &args)
{
  assert_return (app_ident.empty() == false);
  // assert global_ctors work
  if (__staticctortest.v != 0x12affe17)
    fatal ("librapicorncore: link error: C++ constructors have not been executed");

  // guard against double initialization
  if (program_app_ident.empty() == false)
    fatal ("librapicorncore: multiple calls to Rapicorn::init_app()");
  program_app_ident = app_ident;

  // mandatory threading initialization
  if (!g_threads_got_initialized)
    g_thread_init (NULL);

  // setup program and application name
  if (program_argv0.empty() && argcp && *argcp && argv && argv[0] && argv[0][0] != 0)
    program_argv0 = argv[0];
  if (program_cwd0.empty())
    program_cwd0 = Path::cwd();
  if (!g_get_prgname() && !program_argv0.empty())
    g_set_prgname (Path::basename (program_argv0).c_str());
  if (!g_get_application_name() || g_get_application_name() == g_get_prgname())
    g_set_application_name (program_app_ident.c_str());

  // ensure logging is fully initialized
  debug_configure ("");
  const char *env_rapicorn = getenv ("RAPICORN");
  RAPICORN_DEBUG ("startup; RAPICORN=%s", env_rapicorn ? env_rapicorn : "");

  // setup init settings
  parse_settings_and_args (vinit_settings, args, argcp, argv);

  // initialize sub systems
  struct InitHookCaller : public InitHook {
    static void  invoke (const String &kind, int *argcp, char **argv, const StringVector &args)
    { invoke_hooks (kind, argcp, argv, args); }
  };
  InitHookCaller::invoke ("core/", argcp, argv, args);
  InitHookCaller::invoke ("threading/", argcp, argv, args);
}

} // Rapicorn
