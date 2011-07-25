// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html

// PLIC insertion file for Client stubs

includes:
#include <ui/utilities.hh>
namespace Rapicorn {
class ItemImpl; // FIXME
class WindowImpl;
}

IGNORE:
struct DUMMY { // dummy class for auto indentation

class_scope:Requisition:
  inline Requisition_Handle (double w, double h) : width (w), height (h) {}

class_scope:StringList:
  /*Con*/  StringList_Handle () {}
  /*Con*/  StringList_Handle (const std::vector<String> &strv) : Sequence (strv) {}

class_scope:Application:
  static int                      loop_run        ();
  static void                     loop_quit       (int  quit_code = 0);
  static bool                     loop_pending    (bool blocking = false);
  static void                     loop_iteration  (void);
  static int                      loop_and_exit   () RAPICORN_NORETURN;
  static int                      shutdown        (int pass_through = 0);
  static Application_SmartHandle  the             ();
protected:
  // FIXME: static void        pixstream     (const String &pix_name, const uint8 *static_const_pixstream);
  int           m_tc;           // FIXME: uninitialized

IGNORE: // close last _scope
}; // close dummy class scope

global_scope:
namespace Rapicorn {

Application_SmartHandle init_app                (const String       &app_ident,
                                                 int                *argcp,
                                                 char              **argv,
                                                 const StringVector &args = StringVector());
Application_SmartHandle init_test_app           (const String       &app_ident,
                                                 int                *argcp,
                                                 char              **argv,
                                                 const StringVector &args = StringVector());
Plic::Connection*       uithread_connection     (void);
void                    exit                    (int status) RAPICORN_NORETURN;

} // Rapicorn
