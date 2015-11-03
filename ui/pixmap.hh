// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_PIXMAP_HH__
#define __RAPICORN_PIXMAP_HH__

#include <rcore/resources.hh>

namespace Rapicorn {

/** Pixmap (PixmapT) is a Pixbuf wrapper template which provides various pixel operations.
 * A Pixmap really is defined as PixmapT<Pixbuf>, a template class around Pixbuf which
 * provides automatic memory management, pixel operations and IO functions.
 * This class stores ARGB pixels of size @a width * @a height. The pixels are stored as unsigned
 * 32-bit values in native endian format with premultiplied alpha (compatible with libcairo).
 * The @a comment attribute is preserved during saving and loading by some file formats, such as PNG.
 */
template<class Pixbuf>
class PixmapT {
  std::shared_ptr<Pixbuf> pixbuf_;
public:
  explicit      PixmapT         ();                     ///< Construct Pixmap with 0x0 pixesl.
  explicit      PixmapT         (uint w, uint h);       ///< Construct Pixmap at given width and height.
  explicit      PixmapT         (const Pixbuf &source); ///< Copy-construct Pixmap from a Pixbuf structure.
  explicit      PixmapT         (Blob &png_blob);       ///< Construct Pixmap from a PNG resource blob.
  explicit      PixmapT         (const String &res_png); ///< Construct Pixmap from a PNG resource blob.
  PixmapT&      operator=       (const Pixbuf &source); ///< Re-initialize the Pixmap from a Pixbuf structure.
  int           width           () const { return pixbuf_->width(); }  ///< Get the width of the Pixmap.
  int           height          () const { return pixbuf_->height(); } ///< Get the height of the Pixmap.
  void          resize          (uint w, uint h);   ///< Reset width and height and resize pixel sequence.
  bool          try_resize      (uint w, uint h);   ///< Resize unless width and height are too big.
  const uint32* row             (uint y) const { return pixbuf_->row (y); } ///< Access row read-only.
  uint32*       row             (uint y) { return pixbuf_->row (y); } ///< Access row as endian dependant ARGB integers.
  uint32&       pixel           (uint x, uint y) { return pixbuf_->row (y)[x]; }       ///< Retrieve an ARGB pixel value reference.
  uint32        pixel           (uint x, uint y) const { return pixbuf_->row (y)[x]; } ///< Retrieve an ARGB pixel value.
  bool          load_png        (const String &filename, bool tryrepair = false); ///< Load from PNG file, assigns errno on failure.
  bool          load_png        (size_t nbytes, const char *bytes, bool tryrepair = false); ///< Load PNG data, sets errno.
  bool          save_png        (const String &filename); ///< Save to PNG, assigns errno on failure.
  bool          load_pixstream  (const uint8 *pixstream); ///< Decode and load from pixel stream, assigns errno on failure.
  void          set_attribute   (const String &name, const String &value); ///< Set string attribute, e.g. "comment".
  String        get_attribute   (const String &name) const;                ///< Get string attribute, e.g. "comment".
  void          copy            (const Pixbuf &source, uint sx, uint sy,
                                 int swidth, int sheight, uint tx, uint ty); ///< Copy a Pixbuf area into this pximap.
  bool          compare         (const Pixbuf &source, uint sx, uint sy, int swidth, int sheight,
                                 uint tx, uint ty, double *averrp = NULL, double *maxerrp = NULL, double *nerrp = NULL,
                                 double *npixp = NULL) const; ///< Compare area and calculate difference metrics.
  operator const Pixbuf& () const { return *pixbuf_; } ///< Allow automatic conversion of a Pixmap into a Pixbuf.
};

#if __RAPICORN_IDL_ALIASES__ != 0
typedef PixmapT<Rapicorn::Pixbuf> Pixmap; ///< Pixmap is a template implemented for ClnT_Pixbuf and SrvT_Pixbuf
#endif

} // Rapicorn

#endif /* __RAPICORN_PIXMAP_HH__ */
