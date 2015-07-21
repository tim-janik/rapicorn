// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_INOUT_HH__
#define __RAPICORN_INOUT_HH__

#include <rcore/strings.hh>

// For convenience, redefine assert() to get backtraces, logging, etc.
#if defined (RAPICORN_CONVENIENCE)
#define ASSERT  RAPICORN_ASSERT                 ///< Shorthand for #RAPICORN_ASSERT if RAPICORN_CONVENIENCE is defined.
#  if !defined (assert)                         // dont redefine an existing custom macro
#  include <assert.h>                           // import definition of assert() from libc
#  undef  assert
#  define assert  RAPICORN_ASSERT               ///< Shorthand for #RAPICORN_ASSERT if RAPICORN_CONVENIENCE is defined.
#  endif
#endif // RAPICORN_CONVENIENCE

namespace Rapicorn {

// == Basic I/O ==
template<class... Args> void printout (const char *format, const Args &...args) RAPICORN_PRINTF (1, 0);
template<class... Args> void printerr (const char *format, const Args &...args) RAPICORN_PRINTF (1, 0);

// === User Messages ==
struct UserSource;
template<class... Args> void user_notice  (const UserSource &source, const char *format, const Args &...args) RAPICORN_PRINTF (2, 0);
template<class... Args> void user_warning (const UserSource &source, const char *format, const Args &...args) RAPICORN_PRINTF (2, 0);

struct UserSource       ///< Helper structure to capture the origin of a user message.
{
  String module, filename; int line;
  UserSource (const String &module, const String &filename = "", int line = 0);
};

// == Environment variable key functions ==
bool envkey_feature_check (const char*, const char*, bool vdefault = false, volatile bool* = NULL, bool include_all = true);
bool envkey_flipper_check (const char*, const char*, bool vdefault = false, volatile bool* = NULL);
bool envkey_debug_check   (const char*, const char*, volatile bool* = NULL);
void envkey_debug_message (const char*, const char*, const char*, int, const String&, volatile bool* = NULL);

// == Debugging Input/Output ==
void   debug_envvar      (const String &name);
void   debug_config_add  (const String &option);
void   debug_config_del  (const String &key);
String debug_config_get  (const String &key, const String &default_value = "");
bool   debug_config_bool (const String &key, bool default_value = 0);
bool   debug_devel_check ();

// == Debugging Macros ==
#define RAPICORN_FLIPPER(key, blurb,...)  Rapicorn::FlipperOption (key, bool (__VA_ARGS__))
#define RAPICORN_DEBUG_OPTION(key, blurb) Rapicorn::DebugOption (key)
#define RAPICORN_KEY_DEBUG(key,...)       do { if (RAPICORN_UNLIKELY (Rapicorn::_rapicorn_debug_check_cache)) Rapicorn::rapicorn_debug (key, RAPICORN_PRETTY_FILE, __LINE__, Rapicorn::string_format (__VA_ARGS__)); } while (0)
#define RAPICORN_FATAL(...)               do { Rapicorn::debug_fatal_message (RAPICORN_PRETTY_FILE, __LINE__, Rapicorn::string_format (__VA_ARGS__)); } while (0)
#define RAPICORN_ASSERT(cond)             do { if (RAPICORN_LIKELY (cond)) break; Rapicorn::debug_fatal_assert (RAPICORN_PRETTY_FILE, __LINE__, #cond); } while (0)
#define RAPICORN_ASSERT_RETURN(cond, ...) do { if (RAPICORN_LIKELY (cond)) break; Rapicorn::debug_assert (RAPICORN_PRETTY_FILE, __LINE__, #cond); return __VA_ARGS__; } while (0)
#define RAPICORN_ASSERT_UNREACHED()       do { Rapicorn::debug_fatal_assert (RAPICORN_PRETTY_FILE, __LINE__, "line must not be reached"); } while (0)
#define RAPICORN_CRITICAL(...)            do { Rapicorn::debug_message ('C', RAPICORN_PRETTY_FILE, __LINE__, Rapicorn::string_format (__VA_ARGS__)); } while (0)
#define RAPICORN_CRITICAL_UNLESS(cond)    do { if (RAPICORN_LIKELY (cond)) break; Rapicorn::debug_assert (RAPICORN_PRETTY_FILE, __LINE__, #cond); } while (0)
#define RAPICORN_DIAG(...)                do { Rapicorn::debug_message ('G', RAPICORN_PRETTY_FILE, __LINE__, Rapicorn::string_format (__VA_ARGS__)); } while (0)
#define RAPICORN_STARTUP_ASSERT(expr)     RAPICORN_STARTUP_ASSERT_decl (expr, RAPICORN_CPP_PASTE2 (StartupAssertion, __LINE__))

// == Rapicorn Internals Debugging ==
void rapicorn_debug         (const char *key, const char *file, int line, const String &msg);
bool rapicorn_debug_check   (const char *key = NULL);

// == AnsiColors ==
/// The AnsiColors namespace contains utility functions for colored terminal output
namespace AnsiColors {

/// ANSI color symbols.
enum Colors {
  NONE,
  RESET,                ///< Reset combines BOLD_OFF, ITALICS_OFF, UNDERLINE_OFF, INVERSE_OFF, STRIKETHROUGH_OFF.
  BOLD, BOLD_OFF,
  ITALICS, ITALICS_OFF,
  UNDERLINE, UNDERLINE_OFF,
  INVERSE, INVERSE_OFF,
  STRIKETHROUGH, STRIKETHROUGH_OFF,
  FG_BLACK, FG_RED, FG_GREEN, FG_YELLOW, FG_BLUE, FG_MAGENTA, FG_CYAN, FG_WHITE,
  FG_DEFAULT,
  BG_BLACK, BG_RED, BG_GREEN, BG_YELLOW, BG_BLUE, BG_MAGENTA, BG_CYAN, BG_WHITE,
  BG_DEFAULT,
};

const char*     color_code      (Colors acolor);
const char*     color           (Colors acolor);
bool            colorize_tty    (int fd = 1);
void            color_envkey    (const String &env_var, const String &key = "");

} // AnsiColors

// == Convenience Macros ==
#ifdef RAPICORN_CONVENIENCE
#define assert_unreached RAPICORN_ASSERT_UNREACHED  ///< Shorthand for RAPICORN_ASSERT_UNREACHED() if RAPICORN_CONVENIENCE is defined.
#define assert_return    RAPICORN_ASSERT_RETURN     ///< Shorthand for RAPICORN_ASSERT_RETURN() if RAPICORN_CONVENIENCE is defined.
#define fatal            RAPICORN_FATAL             ///< Shorthand for RAPICORN_FATAL() if RAPICORN_CONVENIENCE is defined.
#define critical_unless  RAPICORN_CRITICAL_UNLESS   ///< Shorthand for RAPICORN_CRITICAL_UNLESS() if RAPICORN_CONVENIENCE is defined.
#define critical         RAPICORN_CRITICAL          ///< Shorthand for RAPICORN_CRITICAL() if RAPICORN_CONVENIENCE is defined.
#define STARTUP_ASSERT   RAPICORN_STARTUP_ASSERT    ///< Shorthand for RAPICORN_STARTUP_ASSERT() if RAPICORN_CONVENIENCE is defined.
#define STATIC_ASSERT    RAPICORN_STATIC_ASSERT     ///< Shorthand for RAPICORN_STATIC_ASSERT() if RAPICORN_CONVENIENCE is defined.
#endif // RAPICORN_CONVENIENCE

// == Implementation (Undocumented) ==
/// @cond NOT_4_DOXYGEN

struct DebugOption {
  const char *const key;
  constexpr DebugOption (const char *_key) : key (_key) {}
  operator  bool        () { return debug_config_bool (key); }
};

struct FlipperOption {
  const char *const key;
  const bool  default_value;
  constexpr FlipperOption (const char *_key, bool vdefault) : key (_key), default_value (vdefault) {}
  operator  bool          () { return flipper_check (key, default_value); }
private:
  static bool flipper_check (const char *key, bool vdefault);
};

#define RAPICORN_STARTUP_ASSERT_decl(e, _N)     namespace { static struct _N { inline _N() { RAPICORN_ASSERT (e); } } _N; }

#ifdef __RAPICORN_BUILD__
#define RAPICORN_STARTUP_DEBUG(...)             RAPICORN_KEY_DEBUG ("StartUp", __VA_ARGS__)
#endif

extern bool volatile _rapicorn_debug_check_cache; ///< Caching flag to inhibit useless rapicorn_debug() calls.

inline bool
rapicorn_debug_check (const char *key)
{
  return (RAPICORN_UNLIKELY (_rapicorn_debug_check_cache) &&
          envkey_debug_check ("RAPICORN_DEBUG", key, &_rapicorn_debug_check_cache));
}

void printout_string     (const String &string);
void printerr_string     (const String &string);
void user_notice_string  (const UserSource &source, const String &string);
void user_warning_string (const UserSource &source, const String &string);
void debug_message       (char kind, const char *file, int line, const String &message);
void debug_fatal_message (const char *file, int line, const String &message) RAPICORN_NORETURN;
void debug_fatal_assert  (const char*, int, const char*) RAPICORN_NORETURN;
void debug_assert        (const char*, int, const char*);

/// @endcond

// == Implementation (Documented) ==

/// Print a message on stdout (and flush stdout) ala printf(), using the POSIX/C locale.
template<class... Args> void
printout (const char *format, const Args &...args)
{
  printout_string (string_format (format, args...));
}

/// Print a message on stderr (and flush stderr) ala printf(), using the POSIX/C locale.
template<class... Args> void
printerr (const char *format, const Args &...args)
{
  printerr_string (string_format (format, args...));
}

/// Issue a notice about user resources, @p format uses printf-like syntax.
template<class... Args> void
user_notice (const UserSource &source, const char *format, const Args &...args)
{
  user_notice_string (source, string_format (format, args...));
}

/// Issue a notice about user resources, @p format uses printf-like syntax.
template<class... Args> void
user_warning (const UserSource &source, const char *format, const Args &...args)
{
  user_warning_string (source, string_format (format, args...));
}

} // Rapicorn

#endif /* __RAPICORN_INOUT_HH__ */
