// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0

// C++ interface code insertions for server stubs

includes:
#include <ui/utilities.hh>
#include <ui/clientapi.hh>
namespace Rapicorn {
class WidgetImpl;
class WindowImpl;
}

IGNORE:
struct DUMMY { // dummy class for auto indentation

class_scope:Widget:
  WidgetImpl&       impl ();
  const WidgetImpl& impl () const;

class_scope:Window:
  WindowImpl&       impl ();
  const WindowImpl& impl () const;

class_scope:Application:
  static bool            xurl_add   (const String &model_path, ListModelIface &model);
  static bool            xurl_sub   (ListModelIface &model);
  static ListModelIface* xurl_find  (const String &model_path);
  static String          xurl_path  (const ListModelIface &model);

IGNORE: // close last _scope
}; // close dummy class scope

global_scope:
// #define RAPICORN_PIXBUF_TYPE    Rapicorn::Pixbuf
#include <ui/pixmap.hh>
