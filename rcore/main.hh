// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_MAIN_HH__
#define __RAPICORN_MAIN_HH__

#include <rcore/utilities.hh>

#if !defined __RAPICORN_CORE_HH__ && !defined __RAPICORN_BUILD__
#error Only <rapicorn-core.hh> can be included directly.
#endif

namespace Rapicorn {

// === initialization ===
void    init_core               (const String       &app_ident,
                                 int                *argcp,
                                 char              **argv,
                                 const StringVector &args = StringVector());
struct InitSettings {
  static bool  autonomous()    { return sis->m_autonomous; } ///< self-contained runtime, no rcfiles, boot scripts, etc
  static uint  test_codes()    { return sis->m_test_codes; } // internal test flags
protected:
  static const InitSettings *sis;
  bool m_autonomous;
  uint m_test_codes;
};

bool    arg_parse_option        (uint         argc,
                                 char       **argv,
                                 size_t      *i,
                                 const char  *arg);
bool    arg_parse_string_option (uint         argc,
                                 char       **argv,
                                 size_t      *i,
                                 const char  *arg,
                                 const char **strp);
int     arg_parse_collapse      (int         *argcp,
                                 char       **argv);

// === process info ===
String       program_file       ();
String       program_ident      ();
String       program_cwd        ();

// === initialization hooks ===
class InitHook : protected NonCopyable {
  typedef void (*InitHookFunc) (const StringVector &args);
  InitHook    *next;
  InitHookFunc hook;
  const String m_name;
protected:
  static void  invoke_hooks (const String&, int*, char**, const StringVector&);
public:
  String       name () const { return m_name; }
  StringVector main_args () const;
  explicit     InitHook (const String &fname, InitHookFunc func);
};

} // Rapicorn

#endif /* __RAPICORN_MAIN_HH__ */
