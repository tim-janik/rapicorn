// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_PIXMAP_HH__
#define __RAPICORN_PIXMAP_HH__

#include <rcore/utilities.hh>

namespace Rapicorn {

/// A Pixmap (i.e. PixmapT<Pixbuf>) conveniently wraps a Pixbuf and provides various pixel operations.
template<class Pixbuf>
class PixmapT {
  std::shared_ptr<Pixbuf> m_pixbuf;
public:
  explicit      PixmapT         ();                     ///< Construct Pixmap with 0x0 pixesl.
  explicit      PixmapT         (uint w, uint h);       ///< Construct Pixmap at given width and height.
  explicit      PixmapT         (const Pixbuf &source); ///< Copy-construct Pixmap from a Pixbuf structure.
  PixmapT&      operator=       (const Pixbuf &source); ///< Re-initialize the Pixmap from a Pixbuf structure.
  int           width           () const { return m_pixbuf->width(); }  ///< Get the width of the Pixmap.
  int           height          () const { return m_pixbuf->height(); } ///< Get the height of the Pixmap.
  void          resize          (uint w, uint h);   ///< Reset width and height and resize pixel sequence.
  bool          try_resize      (uint w, uint h);   ///< Resize unless width and height are too big.
  const uint32* row             (uint y) const { return m_pixbuf->row (y); } ///< Access row read-only.
  uint32*       row             (uint y) { return m_pixbuf->row (y); } ///< Access row as endian dependant ARGB integers.
  uint32&       pixel           (uint x, uint y) { return m_pixbuf->row (y)[x]; }       ///< Retrieve an ARGB pixel value reference.
  uint32        pixel           (uint x, uint y) const { return m_pixbuf->row (y)[x]; } ///< Retrieve an ARGB pixel value.
  bool          load_png        (const String &filename, bool tryrepair = false); ///< Load from PNG, assigns errno on failure.
  bool          save_png        (const String &filename); ///< Save to PNG, assigns errno on failure.
  bool          load_pixstream  (const uint8 *pixstream); ///< Decode and load from pixel stream, assigns errno on failure.
  void          set_attribute   (const String &name, const String &value); ///< Set string attribute, e.g. "comment".
  String        get_attribute   (const String &name) const;                ///< Get string attribute, e.g. "comment".
  void          copy            (const Pixbuf &source, uint sx, uint sy,
                                 int swidth, int sheight, uint tx, uint ty); ///< Copy a Pixbuf area into this pximap.
  bool          compare         (const Pixbuf &source, uint sx, uint sy, int swidth, int sheight,
                                 uint tx, uint ty, double *averrp = NULL, double *maxerrp = NULL, double *nerrp = NULL,
                                 double *npixp = NULL) const; ///< Compare area and calculate difference metrics.
  operator const Pixbuf& () const { return *m_pixbuf; } ///< Allow automatic conversion of a Pixmap into a Pixbuf.
};

// RAPICORN_PIXBUF_TYPE is defined in <rcore/clientapi.hh> and <rcore/serverapi.hh>
typedef PixmapT<RAPICORN_PIXBUF_TYPE> Pixmap; ///< Pixmap is a convenience alias for PixmapT<Pixbuf>.

} // Rapicorn

#endif /* __RAPICORN_PIXMAP_HH__ */
