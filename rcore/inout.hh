// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_INOUT_HH__
#define __RAPICORN_INOUT_HH__

#include <rcore/cxxaux.hh>
#include <rcore/aida.hh>

namespace Rapicorn {

// == Environment variable key functions ==
bool envkey_flipper_check (const char*, const char*, bool with_all_toggle = true, volatile bool* = NULL);
bool envkey_debug_check   (const char*, const char*, volatile bool* = NULL);
void envkey_debug_message (const char*, const char*, const char*, int, const char*, va_list, volatile bool* = NULL);

// == Debugging Input/Output ==
void   debug_assert      (const char*, int, const char*);
void   debug_fassert     (const char*, int, const char*)      __attribute__ ((__noreturn__));
void   debug_fatal       (const char*, int, const char*, ...) __attribute__ ((__format__ (__printf__, 3, 4), __noreturn__));
void   debug_critical    (const char*, int, const char*, ...) __attribute__ ((__format__ (__printf__, 3, 4)));
void   debug_fixit       (const char*, int, const char*, ...) __attribute__ ((__format__ (__printf__, 3, 4)));
void   debug_envvar      (const String &name);
void   debug_config_add  (const String &option);
void   debug_config_del  (const String &key);
String debug_config_get  (const String &key, const String &default_value = "");
bool   debug_config_bool (const String &key, bool default_value = 0);
bool   debug_devel_check ();

// == Debug Macros ==
#define RAPICORN_DEBUG_OPTION(option, blurb)       Rapicorn::DebugOption (option)

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

// == Implementation Bits ==
/// @cond NOT_4_DOXYGEN
struct DebugOption {
  const char *const key;
  constexpr DebugOption (const char *_key) : key (_key) {}
  operator  bool        () { return debug_config_bool (key); }
};
/// @endcond

} // Rapicorn

#endif /* __RAPICORN_INOUT_HH__ */
