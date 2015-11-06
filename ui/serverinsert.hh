// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0

// C++ interface code insertions for server stubs

includes:
#include <ui/utilities.hh>
namespace Rapicorn {
class WidgetImpl;
class WindowImpl;
}

IGNORE:
struct DUMMY { // dummy class for auto indentation

class_scope:StringSeq:
  explicit SrvT_StringSeq () {}
  /*ctor*/ SrvT_StringSeq (const std::vector<std::string> &strv) : Sequence (strv) {}

class_scope:Pixbuf:
  /// Construct Pixbuf at given width and height.
  explicit        SrvT_Pixbuf  (uint w, uint h) : row_length (0) { pixels.resize (w, h); }
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
  explicit SrvT_UpdateSpan (int _start, int _length) : start (_start), length (_length) {}

class_scope:UpdateRequest:
  explicit SrvT_UpdateRequest (UpdateKind _kind, const SrvT_UpdateSpan &rs, const SrvT_UpdateSpan &cs = SrvT_UpdateSpan()) :
    kind (_kind), rowspan (rs), colspan (cs) {}

class_scope:Requisition:
  inline SrvT_Requisition (double w, double h) : width (w), height (h) {}

class_scope:Widget:
  WidgetImpl&       impl ();
  const WidgetImpl& impl () const;

class_scope:Window:
  WindowImpl&       impl ();
  const WindowImpl& impl () const;

IGNORE: // close last _scope
}; // close dummy class scope

global_scope:
#include <ui/pixmap.hh>
