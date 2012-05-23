/* Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html */
#ifndef __RAPICORN_SVG_HH__
#define __RAPICORN_SVG_HH__

#include <rcore/objects.hh>
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

/// Information structure for SVG element sizes.
struct Allocation {
  double x, y, width, height;
  Allocation ();
  Allocation (double, double, double, double);
};

typedef std::shared_ptr<Element> ElementP;      ///< Smart pointer to an Svg::Element.
/// An SVG Element can be queried for size, rendered to pixmaps and scaled in various ways.
class Element {
public:
  virtual Info          info            () = 0;                         ///< Provides information on size units.
  virtual Allocation    allocation      () = 0;                         ///< Provides the size of an element.
  virtual Allocation    allocation      (Allocation &_containee) = 0;   ///< Provides the size of an element for a given containee size.
  virtual Allocation    containee       () = 0;                         ///< Provides the size of the containee if supported.
  virtual Allocation    containee       (Allocation &_resized) = 0;     ///< Provides the containee size for a given element size.
  virtual bool          render          (cairo_surface_t *surface,
                                         const Allocation &area) = 0;   ///< Renders an SVG element into a cairo_surface_t.
  // empty and none types
  static const ElementP none            (); ///< Returns null ElementP, which yields false in boolean tests.
protected: // Impl details
  ~Element() {}                         ///< Internal destructor, Svg::ElementP automatically manages the Element class lifetime.
};

typedef std::shared_ptr<File> FileP;            ///< Smart pointer to an Svg::File.
/// The File class represents a successfully loaded SVG file.
class File {
public:
  virtual void          dump_tree       () = 0;
  virtual ElementP      lookup          (const String &elementid) = 0;  ///< Lookup an SVG element from an SVG File.
  static  void          add_search_dir  (const String &absdir);         ///< Adds a directory for relative Svg::File::load() calls.
  static  FileP         load            (const String &svgfilename);    ///< Load an SVG file, returns non-null on success and sets errno.
protected: // Impl details
  ~File() {}                            ///< Internal destructor, Svg::FileP automatically manages the File class lifetime.
};

} // Svg
} // Rapicorn

#endif /* __RAPICORN_SVG_HH__ */
