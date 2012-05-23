/* Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html */
#ifndef __RAPICORN_SVG_HH__
#define __RAPICORN_SVG_HH__

#include <rcore/objects.hh>
#include <cairo.h>
#include <math.h>

namespace Rapicorn {
namespace Svg {

struct Info {
  String id; double em, ex;
  Info () : em (0), ex (0) {}
};
struct Allocation {
  double x, y, width, height;
  Allocation ();
  Allocation (double, double, double, double);
};

class Element;
typedef std::shared_ptr<Element> ElementP;
class Element {
public:
  virtual Info          info            () = 0;
  virtual Allocation    allocation      () = 0;
  virtual Allocation    allocation      (Allocation &_containee) = 0;
  virtual Allocation    containee       () = 0;
  virtual Allocation    containee       (Allocation &_resized) = 0;
  virtual bool          render          (cairo_surface_t *surface, const Allocation &area) = 0;
  // empty and none types
  static const ElementP none            (); ///< Returns null ElementP, which yields false in boolean tests.
protected: // Impl details
  ~Element() {}
};

class File;
typedef std::shared_ptr<File> FileP;
struct File {
  virtual void          dump_tree       () = 0;
  virtual ElementP      lookup          (const String &elementid) = 0;
  virtual int           error           () = 0;
  static  void          add_search_dir  (const String &absdir);
  static  FileP         load            (const String &svgfilename);
protected: // Impl details
  ~File() {}
};

} // Svg
} // Rapicorn

#endif /* __RAPICORN_SVG_HH__ */
