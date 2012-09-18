// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html

// C++ interface code insertions for server stubs

includes:
#include <ui/utilities.hh>
#include <rcore/serverapi.hh>
namespace Rapicorn {
class ItemImpl;
class WindowImpl;
}

IGNORE:
struct DUMMY { // dummy class for auto indentation

class_scope:Requisition:
  inline RequisitionImpl (double w, double h) : width (w), height (h) {}

class_scope:Item:
  ItemImpl&       impl ();
  const ItemImpl& impl () const;

class_scope:Window:
  WindowImpl&       impl ();
  const WindowImpl& impl () const;

class_scope:Application:
  static bool            xurl_add   (const String &model_path, ListModelIface &model);
  static bool            xurl_sub   (ListModelIface &model);
  static ListModelIface* xurl_find  (const String &model_path);
  static String          xurl_path  (const ListModelIface &model);
  /* global mutex */
  struct ApplicationMutex {
    static void lock    () { rapicorn_thread_enter (); }
    static bool trylock () { return rapicorn_thread_try_enter (); }
    static void unlock  () { rapicorn_thread_leave (); }
  };
  static ApplicationMutex mutex;  // singleton

IGNORE: // close last _scope
}; // close dummy class scope

global_scope:
