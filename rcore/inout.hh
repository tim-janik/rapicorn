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
void debug_assert         (const char*, int, const char*);
void debug_fassert        (const char*, int, const char*)      __attribute__ ((__noreturn__));
void debug_fatal          (const char*, int, const char*, ...) __attribute__ ((__format__ (__printf__, 3, 4), __noreturn__));
void debug_critical       (const char*, int, const char*, ...) __attribute__ ((__format__ (__printf__, 3, 4)));
void debug_fixit          (const char*, int, const char*, ...) __attribute__ ((__format__ (__printf__, 3, 4)));

} // Rapicorn

#endif /* __RAPICORN_INOUT_HH__ */
