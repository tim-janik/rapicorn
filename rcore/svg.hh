/* This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 */
#ifndef __RAPICORN_SVG_HH__
#define __RAPICORN_SVG_HH__

#include <rcore/resources.hh>
#include <cairo.h>
#include <math.h>

namespace Rapicorn {
namespace Svg {

class Element;
class File;

/// Information struct for SVG element size units.
struct Info {
  String id; double em, ex;
  Info () : em (0), ex (0) {}
};

/// A BBox provides bounding box information for SVG elements.
struct BBox {
  double x, y, width, height;
  explicit BBox      ();
  explicit BBox      (double, double, double, double);
  String   to_string () const;
};

/// A simple POD to represent resizable lengths.
struct Span {
  size_t length;        ///< Length of this Span
  size_t resizable;     ///< Resizing flag (count) for this Span
  static ssize_t distribute (const size_t n_spans, Span *spans, ssize_t amount, size_t resizable_level);
  String         to_string() const;
  static String  to_string (size_t n, const Span *spans);
};

enum class RenderSize {
  STATIC,       ///< Keep the SVG elements native size.
  ZOOM,         ///< Zoom the SVG element according to scale factors.
};

typedef std::shared_ptr<Element> ElementP;      ///< Smart pointer to an Svg::Element.
/// An SVG Element can be queried for size, rendered to pixmaps and scaled in various ways.
class Element {
public:
  virtual Info  info            () = 0;                 ///< Provides information on size units.
  virtual BBox  bbox            () = 0;                 ///< Provides the bounding box of an element.
  virtual BBox  enfolding_bbox  (BBox &inner) = 0;      ///< Provides the size of an element for a given containee size.
  virtual BBox  containee_bbox  () = 0;                 ///< Provides the bounding box of a containee if supported.
  virtual BBox  containee_bbox  (BBox &_resized) = 0;   ///< Provides the containee size for a given element size. FIXME: scaling
  virtual bool  render          (cairo_surface_t *surface, RenderSize rsize = RenderSize::ZOOM, double xscale = 1,
                                 double yscale = 1) = 0;///< Renders a scaled SVG element into a cairo_surface_t.
  cairo_surface_t* stretch      (size_t, size_t, size_t, const Span*,  size_t, const Span*, cairo_filter_t = CAIRO_FILTER_BILINEAR);
  static const ElementP none    ();                     ///< Returns null ElementP, which yields false in boolean tests.
protected: // Impl details
  ~Element() {}                         ///< Internal destructor, Svg::ElementP automatically manages the Element class lifetime.
};

typedef std::shared_ptr<File> FileP;            ///< Smart pointer to an Svg::File.
/// The File class represents a successfully loaded SVG file.
class File {
public:
  virtual ElementP     lookup (const String &elementid) = 0;   ///< Lookup an SVG element from an SVG File.
  virtual StringVector list   () = 0;                          ///< List the element IDs in an SVG File.
  virtual String       name   () const = 0;                    ///< Provide the name of this file.
  static  FileP        load   (const String &svgfilename);     ///< Load an SVG file, returns non-null on success and sets errno.
  static  FileP        load   (Blob svg_blob);                 ///< Load an SVG file from a binary SVG resource blob, sets errno.
protected: // Impl details
  ~File() {}                            ///< Internal destructor, Svg::FileP automatically manages the File class lifetime.
};

} // Svg
} // Rapicorn

#endif /* __RAPICORN_SVG_HH__ */
