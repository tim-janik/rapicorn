// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0

// C++ interface code insertions for client stubs

includes:
namespace Rapicorn {
class WidgetImpl; // FIXME
class WindowImpl;
}

IGNORE:
struct DUMMY { // dummy class for auto indentation

class_scope:StringSeq:
  explicit ClnT_StringSeq () {}
  /*ctor*/ ClnT_StringSeq (const std::vector<std::string> &strv) : Sequence (strv) {}

class_scope:Pixbuf:
  /// Construct Pixbuf at given width and height.
  explicit        ClnT_Pixbuf  (uint w, uint h) : row_length (0) { pixels.resize (w, h); }
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

class_scope:UpdateSpan:
  explicit ClnT_UpdateSpan (int _start, int _length) : start (_start), length (_length) {}

class_scope:UpdateRequest:
  explicit ClnT_UpdateRequest (UpdateKind _kind, const ClnT_UpdateSpan &rs, const ClnT_UpdateSpan &cs = ClnT_UpdateSpan()) :
    kind (_kind), rowspan (rs), colspan (cs) {}

class_scope:Requisition:
  inline ClnT_Requisition (double w, double h) : width (w), height (h) {}

class_scope:Widget:
  /// Carry out Widget::query_selector_unique() and Target::down_cast() in one step, for type safe access to a descendant.
  template<class Target> Target component (const std::string &selector) { return Target::down_cast (query_selector_unique (selector)); }

class_scope:Application:
  static int                      run            ();
  static void                     quit           (int quit_code = 0);
  static void                     shutdown       ();
  static int                      run_and_exit   () RAPICORN_NORETURN;
  static bool                     iterate        (bool block);
  static ApplicationHandle        the            ();
  static MainLoopP                main_loop      ();

IGNORE: // close last _scope
}; // close dummy class scope

global_scope:
namespace Rapicorn {

bool              init_app_initialized    ();
ApplicationHandle init_app                (const String       &app_ident,
                                           int                *argcp,
                                           char              **argv,
                                           const StringVector &args = StringVector());
ApplicationHandle init_test_app           (const String       &app_ident,
                                           int                *argcp,
                                           char              **argv,
                                           const StringVector &args = StringVector());

} // Rapicorn

#include <ui/pixmap.hh>
