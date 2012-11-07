// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html

// C++ interface code insertions for client stubs

includes:
#include <ui/utilities.hh>
namespace Rapicorn {
using Aida::Any;
class ItemImpl; // FIXME
class WindowImpl;
}

IGNORE:
struct DUMMY { // dummy class for auto indentation

class_scope:StringSeq:
  explicit StringSeqStruct () {}
  /*ctor*/ StringSeqStruct (const std::vector<std::string> &strv) : Sequence (strv) {}

class_scope:Pixbuf:
  /// Construct Pixbuf at given width and height.
  explicit        PixbufStruct (uint w, uint h) : row_length (0) { pixels.resize (w, h); }
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
  inline RequisitionStruct (double w, double h) : width (w), height (h) {}

class_scope:Item:
  /// Carry out Item::query_selector_unique() and Target::downcast() in one step, for type safe access to a descendant.
  template<class Target> Target component (const std::string &selector) { return Target::downcast (query_selector_unique (selector)); }

class_scope:Application:
  static int                      run            ();
  static void                     quit           (int quit_code = 0);
  static void                     shutdown       ();
  static int                      run_and_exit   () RAPICORN_NORETURN;
  static ApplicationHandle        the            ();
  static Aida::ClientConnection   ipc_connection ();
protected:
  static MainLoop*                main_loop     ();

IGNORE: // close last _scope
}; // close dummy class scope

global_scope:
namespace Rapicorn {

ApplicationHandle init_app                (const String       &app_ident,
                                           int                *argcp,
                                           char              **argv,
                                           const StringVector &args = StringVector());
ApplicationHandle init_test_app           (const String       &app_ident,
                                           int                *argcp,
                                           char              **argv,
                                           const StringVector &args = StringVector());
void              exit                    (int status) RAPICORN_NORETURN;

} // Rapicorn
#define RAPICORN_PIXBUF_TYPE    Rapicorn::PixbufStruct
#include <ui/pixmap.hh>
