// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html

// C++ interface code insertions for server stubs

includes:
#include <ui/utilities.hh>
#include <ui/clientapi.hh>
namespace Rapicorn {
using Aida::Any;
class ItemImpl;
class WindowImpl;
}

IGNORE:
struct DUMMY { // dummy class for auto indentation

class_scope:StringSeq:
  explicit StringSeq () {}
  /*ctor*/ StringSeq (const std::vector<std::string> &strv) : Sequence (strv) {}

class_scope:Pixbuf:
  /// Construct Pixbuf at given width and height.
  explicit        Pixbuf   (uint w, uint h) : row_length (0) { pixels.resize (w, h); }
  /// Reset width and height and resize pixel sequence.
  void            resize       (uint w, uint h) { row_length = w; pixels.resize (row_length * h); }
  /// Access row as endian dependant ARGB integers.
  uint32_t*       row          (uint y)         { return y < uint32_t (height()) ? (uint32_t*) &pixels[row_length * y] : NULL; }
  /// Access row as endian dependant ARGB integers.
  const uint32_t* row          (uint y) const   { return y < uint32_t (height()) ? (uint32_t*) &pixels[row_length * y] : NULL; }
  /// Width of the Pixbuf.
  int             width        () const         { return row_length; }
  /// Height of the Pixbuf.
  int             height       () const         { return row_length ? pixels.size() / row_length : 0; }

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
// #define RAPICORN_PIXBUF_TYPE    Rapicorn::Pixbuf
#include <ui/pixmap.hh>
