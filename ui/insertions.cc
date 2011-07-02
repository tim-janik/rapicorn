// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html

// PLIC insertion file
// Here we provide insertion snippets to be included in struct
// definitions when generating interfaces.hh from interfaces.idl

includes:
#include <ui/utilities.hh>
namespace Rapicorn {
class Window;
}

IGNORE:
struct DUMMY { // dummy class for auto indentation

class_scope:Requisition:
  inline Requisition (double w, double h) : width (w), height (h) {}

class_scope:StringList:
  /*Con*/  StringList () {}
  /*Con*/  StringList (const std::vector<String> &strv) : Sequence (strv) {}

class_scope:Wind0wIface:
  virtual Window&    window        () = 0;
class_scope:ApplicationIface:
  static void        pixstream     (const String &pix_name, const uint8 *static_const_pixstream);
  int                execute_loops ();
  static bool        plor_add      (ItemIface    &item, const String &plor_name);
  static ItemIface*  plor_remove   (const String &plor_name);
  /* global mutex */
  struct ApplicationMutex {
    static void lock    () { rapicorn_thread_enter (); }
    static bool trylock () { return rapicorn_thread_try_enter (); }
    static void unlock  () { rapicorn_thread_leave (); }
  };
  static ApplicationMutex mutex;  // singleton
  /* singleton defs */
protected:
  int                m_tc; // FIXME: uninitialized

IGNORE: // close last _scope
}; // close dummy class scope

// global_scope:
