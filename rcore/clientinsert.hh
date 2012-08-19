// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html

// PLIC insertion file for Client stubs

includes:
#include <rcore/rapicornconfig.h>

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

IGNORE: // close last _scope
}; // close dummy class scope

global_scope:
#define RAPICORN_PIXBUF_TYPE    Rapicorn::PixbufStruct
#include <rcore/pixmap.hh>
