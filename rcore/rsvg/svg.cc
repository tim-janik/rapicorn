/* Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html */
#include "svg.hh"
#include "rsvg.h"
#include "rsvg-cairo.h"
#include "rsvg-private.h"
#include "../main.hh"

#include "svg-tweak.h"

/* A note on coordinate system increments:
 * - Librsvg:  X to right, Y downwards.
 * - Cairo:    X to right, Y downwards.
 * - X11/GTK:  X to right, Y downwards.
 * - Windows:  X to right, Y downwards.
 * - Inkscape: X to right, Y upwards (UI coordinate system).
 * - Cocoa:    X to right, Y upwards.
 * - ACAD:     X to right, Y upwards.
 */

static double
affine_x (const double x, const double y, const double affine[6])
{
  return affine[0] * x + affine[2] * y + affine[4];
}

static double
affine_y (const double x, const double y, const double affine[6])
{
  return affine[1] * x + affine[3] * y + affine[5];
}

namespace Rapicorn {
namespace Svg {

struct Tweaker {
  double m_cx, m_cy, m_lx, m_rx, m_by, m_ty;
  explicit      Tweaker         (double cx, double cy, double lx, double rx, double by, double ty) :
    m_cx (cx), m_cy (cy), m_lx (lx), m_rx (rx), m_by (by), m_ty (ty) {}
  bool          point_tweak     (double vx, double vy, double *px, double *py);
  static void   thread_set      (Tweaker *tweaker);
};

struct ElementImpl : public ReferenceCountable {
  RsvgHandle    *handle;
  int            x, y, width, height, rw, rh; // relative to bottom_left
  double         em, ex;
  String         id;
  explicit      ElementImpl     () : handle (NULL), x (0), y (0), width (0), height (0), em (0), ex (0) {}
  virtual      ~ElementImpl     () { id = ""; if (handle) g_object_unref (handle); }
};

Info::Info () : em (0), ex (0)
{}

Allocation::Allocation () :
  x (-1), y (-1), width (0), height (0)
{}

Allocation::Allocation (double _x, double _y, double w, double h) :
  x (_x), y (_y), width (w), height (h)
{}

const Element&
Element::none ()
{
  static const Element _none;
  return _none;
}

Element&
Element::operator= (const Element &src)
{
  if (this != &src)
    {
      ElementImpl *old = impl;
      this->impl = src.impl;
      if (impl)
        impl->ref();
      if (old)
        old->unref();
    }
  return *this;
}

Element::Element (const Element &src) :
  impl (NULL)
{
  *this = src;
}

Element::Element () :
  impl (NULL)
{}

Element::~Element ()
{
  ElementImpl *old = impl;
  impl = NULL;
  if (old)
    old->unref();
}

Info
Element::info ()
{
  Info i;
  if (impl)
    {
      i.id = impl->id;
      i.em = impl->em;
      i.ex = impl->ex;
    }
  else
    {
      i.id = "";
      i.em = i.ex = 0;
    }
  return i;
}

Allocation
Element::allocation ()
{
  if (impl)
    {
      Allocation a = Allocation (impl->x, impl->y, impl->width, impl->height);
      return a;
    }
  else
    return Allocation();
}

Allocation
Element::allocation (Allocation &_containee)
{
  if (!impl || _containee.width <= 0 || _containee.height <= 0)
    return allocation();
  // FIXME: resize for _containee width/height
  Allocation a = Allocation (impl->x, impl->y, _containee.width + 4, _containee.height + 4);
  return a;
}

Allocation
Element::containee ()
{
  if (!impl || impl->width <= 4 || impl->height <= 4)
    return Allocation();
  // FIXME: calculate _containee size
  Allocation a = Allocation (impl->x + 2, impl->y + 2, impl->width - 4, impl->height - 4);
  return a;
}

Allocation
Element::containee (Allocation &_resized)
{
  if (!impl || (_resized.width == impl->width && _resized.height == impl->height))
    return containee();
  // FIXME: calculate _containee size when resized
  return containee();
}

bool
Element::render (cairo_surface_t  *surface,
                 const Allocation &area)
{
  assert_return (surface != NULL, false);
  if (!impl) return false;
  cairo_t *cr = cairo_create (surface);
  cairo_translate (cr, -impl->x, -impl->y); // shift sub into top_left
  cairo_translate (cr, area.x, area.y);    // translate by requested offset
  const char *cid = impl->id.empty() ? NULL : impl->id.c_str();
  const double rx = (area.width - impl->width) / 2.0;
  const double lx = (area.width - impl->width) - rx;
  const double ty = (area.height - impl->height) / 2.0;
  const double by = (area.height - impl->height) - ty;
  Tweaker tw (impl->x + impl->width / 2.0, impl->y + impl->height / 2.0, lx, rx, ty, by);
  if (svg_tweak_debugging)
    printerr ("TWEAK: mid = %g %g ; shiftx = %g %g ; shifty = %g %g ; (dim = %d,%d,%dx%d)\n",
              impl->x + impl->width / 2.0, impl->y + impl->height / 2.0, lx, rx, ty, by,
              impl->x,impl->y,impl->width,impl->height);
  svg_tweak_debugging = debug_enabled();
  tw.thread_set (&tw);
  bool rendered = rsvg_handle_render_cairo_sub (impl->handle, cr, cid);
  tw.thread_set (NULL);
  cairo_destroy (cr);
  return rendered;
}

bool
Tweaker::point_tweak (double vx, double vy, double *px, double *py)
{
  bool mod = 0;
  const double eps = 0.0001;
  // FIXME: for filters to work, the document page must grow so that it contains the tweaked
  // item without clipping. e.g. an item (5,6,7x8) requires a document size at least: 12x14
  if (vx > m_cx + eps)
    *px += m_lx + m_rx, mod = 1;
  else if (vx >= m_cx - eps)
    *px += m_lx, mod = 1;
  if (vy > m_cy + eps)
    *py += m_ty + m_by, mod = 1;
  else if (vy >= m_cy - eps)
    *py += m_ty, mod = 1;
  return mod;
}

static vector<String>      library_search_dirs;

/**
 * Add a directory to search for SVG library fiels.
 */
void
Library::add_search_dir (const String &absdir)
{
  assert_return (Path::isabs (absdir));
  library_search_dirs.push_back (absdir);
}

static String
find_library_file (const String &filename)
{
  if (Path::isabs (filename))
    return filename;
  for (size_t i = 0; i < library_search_dirs.size(); i++)
    {
      String fpath = Path::join (library_search_dirs[i], filename);
      if (Path::check (fpath, "e"))
        return fpath;
    }
  return Path::abspath (filename); // uses CWD
}

static vector<RsvgHandle*> library_handles;

/**
 * Add @filename to the list of SVG libraries used for looking up UI graphics.
 */
void
Library::add_library (const String &filename)
{
  // loading *could* be deferred here
  FILE *fp = fopen (find_library_file (filename).c_str(), "rb");
  bool success = false;
  if (fp)
    {
      uint8 buffer[8192];
      ssize_t len;
      RsvgHandle *handle = rsvg_handle_new();
      g_object_ref_sink (handle);
      while ((len = fread (buffer, 1, sizeof (buffer), fp)) > 0)
        if (!(success = rsvg_handle_write (handle, buffer, len, NULL)))
          break;
      fclose (fp);
      success = success && rsvg_handle_close (handle, NULL);
      if (success)
        library_handles.push_back (handle);
      else
        g_object_unref (handle);
    }
  DEBUG ("loading uilib: %s", strerror());
}

static void
dump_node (RsvgNode *self, int64 depth, int64 maxdepth)
{
  if (depth > maxdepth)
    return;
  for (int64 i = 0; i < depth; i++)
    printerr (" ");
  printerr ("<%s/>\n", self->type->str);
  for (uint i = 0; i < self->children->len; i++)
    {
      RsvgNode *child = (RsvgNode*) g_ptr_array_index (self->children, i);
      dump_node (child, depth + 1, maxdepth);
    }
}

void
Library::dump_tree (const String &id)
{
  for (size_t i = 0; i < library_handles.size(); i++)
    if (library_handles[i])
      {
        RsvgHandlePrivate *p = library_handles[i]->priv;
        RsvgNode *node = p->treebase;
        printerr ("\n%s:\n", p->title ? p->title->str : "???");
        dump_node (node, 0, INT64_MAX);
      }
}

Element
Library::lookup_element (const String &id)
{
  for (size_t i = 0; i < library_handles.size(); i++)
    {
      RsvgHandle *handle = library_handles[i];
      RsvgDimensionData rd = { 0, 0, 0, 0 };
      RsvgDimensionData dd = { 0, 0, 0, 0 };
      RsvgPositionData dp = { 0, 0 };
      const char *cid = id.empty() ? NULL : id.c_str();
      rsvg_handle_get_dimensions (handle, &rd);         // FIXME: cache results
      if (rd.width > 0 && rd.height > 0 &&
          rsvg_handle_get_dimensions_sub (handle, &dd, cid) && dd.width > 0 && dd.height > 0 &&
          rsvg_handle_get_position_sub (handle, &dp, cid))
        {
          ElementImpl *ei = new ElementImpl();
          ei->handle = handle;
          g_object_ref (ei->handle);
          ei->x = dp.x;
          ei->y = dp.y;
          ei->width = dd.width;
          ei->height = dd.height;
          ei->rw = rd.width;
          ei->rh = rd.height;
          ei->em = dd.em;
          ei->ex = dd.ex;
          ei->id = id;
          struct ElementInternal : public Element {
            ElementInternal (ElementImpl *ei) { impl = ref_sink (ei); }
          };
          return ElementInternal (ei);
        }
    }
  return Element();
}

static void
init_svg_lib (const StringVector &args)
{
  g_type_init(); // NOP on subsequent invocations
  Library::add_search_dir (RAPICORN_SVGDIR);
}
static InitHook _init_svg_lib ("core/35 Init SVG Lib", init_svg_lib);

static __thread Tweaker *thread_tweaker = NULL;

void
Tweaker::thread_set (Tweaker *tweaker)
{
  if (tweaker)
    assert_return (thread_tweaker == NULL);
  thread_tweaker = tweaker;
}

} // Svg
} // Rapicorn

extern "C" {

int svg_tweak_debugging = 0;

int
svg_tweak_point_tweak (double  vx, double  vy,
                       double *px, double *py,
                       const double affine[6],
                       const double iaffine[6])
{
  if (!Rapicorn::Svg::thread_tweaker)
    return false;
  const double sx = affine_x (vx, vy, affine), sy = affine_y (vx, vy, affine);
  double x = affine_x (*px, *py, affine), y = affine_y (*px, *py, affine);
  if (Rapicorn::Svg::thread_tweaker->point_tweak (sx, sy, &x, &y))
    {
      *px = affine_x (x, y, iaffine);
      *py = affine_y (x, y, iaffine);
      return true;
    }
  return false;
}

int
svg_tweak_point_simple (double *px, double *py, const double affine[6], const double iaffine[6])
{
  return svg_tweak_point_tweak (*px, *py, px, py, affine, iaffine);
}

};
