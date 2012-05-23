/* Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html */
#include "svg.hh"
#include "../strings.hh"
#include "../blobres.hh"
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

/// @namespace Rapicorn::Svg The Rapicorn::Svg namespace provides functions for handling and rendering of SVG files and elements.
namespace Svg {

Allocation::Allocation () :
  x (-1), y (-1), width (0), height (0)
{}

Allocation::Allocation (double _x, double _y, double w, double h) :
  x (_x), y (_y), width (w), height (h)
{}

struct Tweaker {
  double m_cx, m_cy, m_lx, m_rx, m_by, m_ty;
  double m_xoffset, m_xscale, m_yoffset, m_yscale;
  explicit      Tweaker         (double cx, double cy, double lx, double rx, double by, double ty,
                                 double xoffset, double xscale, double yoffset, double yscale) :
    m_cx (cx), m_cy (cy), m_lx (lx), m_rx (rx), m_by (by), m_ty (ty),
    m_xoffset (xoffset), m_xscale (xscale), m_yoffset (yoffset), m_yscale (yscale) {}
  bool          point_tweak     (double vx, double vy, double *px, double *py);
  static void   thread_set      (Tweaker *tweaker);
};

bool
Tweaker::point_tweak (double vx, double vy, double *px, double *py)
{
  bool mod = 0;
#if 0
  const double xdelta = *px - m_xoffset;
  *px = m_xoffset + xdelta * m_xscale;
  const double ydelta = *py - m_yoffset;
  *py = m_yoffset + ydelta * m_yscale;
  mod = true;
#else // nonlinear shifting away from center
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
#endif
  return mod;
}

struct ElementImpl : public Element {
  String         m_id;
  int            m_x, m_y, m_width, m_height, m_rw, m_rh; // relative to bottom_left
  double         m_em, m_ex;
  RsvgHandle    *m_handle;
  explicit              ElementImpl     () : m_x (0), m_y (0), m_width (0), m_height (0), m_em (0), m_ex (0), m_handle (NULL) {}
  virtual              ~ElementImpl     ();
  virtual Info          info            ();
  virtual Allocation    allocation      () { return Allocation (m_x, m_y, m_width, m_height); }
  virtual Allocation    allocation      (Allocation &_containee);
  virtual Allocation    containee       ();
  virtual Allocation    containee       (Allocation &_resized);
  virtual bool          render          (cairo_surface_t *surface, const Allocation &area);
};

const ElementP
Element::none ()
{
  return ElementP();
}

ElementImpl::~ElementImpl ()
{
  m_id = "";
  RsvgHandle *ohandle = m_handle;
  m_handle = NULL;
  if (ohandle)
    g_object_unref (ohandle);
}

Info
ElementImpl::info ()
{
  Info i;
  i.id = m_id;
  i.em = m_em;
  i.ex = m_ex;
  return i;
}

Allocation
ElementImpl::allocation (Allocation &containee)
{
  if (containee.width <= 0 || containee.height <= 0)
    return allocation();
  // FIXME: resize for _containee width/height
  Allocation a = Allocation (m_x, m_y, containee.width + 4, containee.height + 4);
  return a;
}

Allocation
ElementImpl::containee ()
{
  if (m_width <= 4 || m_height <= 4)
    return Allocation();
  // FIXME: calculate _containee size
  Allocation a = Allocation (m_x + 2, m_y + 2, m_width - 4, m_height - 4);
  return a;
}

Allocation
ElementImpl::containee (Allocation &resized)
{
  if (resized.width == m_width && resized.height == m_height)
    return containee();
  // FIXME: calculate _containee size when resized
  return containee();
}

bool
ElementImpl::render (cairo_surface_t *surface, const Allocation &area)
{
  assert_return (surface != NULL, false);
  cairo_t *cr = cairo_create (surface);
  cairo_translate (cr, -m_x, -m_y); // shift sub into top_left
  cairo_translate (cr, area.x, area.y);    // translate by requested offset
  const char *cid = m_id.empty() ? NULL : m_id.c_str();
  const double rx = (area.width - m_width) / 2.0;
  const double lx = (area.width - m_width) - rx;
  const double ty = (area.height - m_height) / 2.0;
  const double by = (area.height - m_height) - ty;
  Tweaker tw (m_x + m_width / 2.0, m_y + m_height / 2.0, lx, rx, ty, by,
              m_x, area.width / m_width, m_y, area.height / m_height);
  if (svg_tweak_debugging)
    printerr ("TWEAK: mid = %g %g ; shiftx = %g %g ; shifty = %g %g ; (dim = %d,%d,%dx%d)\n",
              m_x + m_width / 2.0, m_y + m_height / 2.0, lx, rx, ty, by,
              m_x, m_y, m_width, m_height);
  svg_tweak_debugging = debug_enabled();
  tw.thread_set (&tw);
  bool rendered = rsvg_handle_render_cairo_sub (m_handle, cr, cid);
  tw.thread_set (NULL);
  cairo_destroy (cr);
  return rendered;
}

struct FileImpl : public File {
  RsvgHandle           *m_handle;
  int                   m_errno;
  explicit              FileImpl        (RsvgHandle *hh) : m_handle (hh), m_errno (0) {}
  /*dtor*/             ~FileImpl        () { if (m_handle) g_object_unref (m_handle); }
  virtual int           error           () { return m_errno; }
  virtual void          dump_tree       ();
  virtual ElementP      lookup          (const String &elementid);
};

static vector<String>      library_search_dirs;

// Add a directory to search for SVG files with relative pathnames.
void
File::add_search_dir (const String &absdir)
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

FileP
File::load (const String &svgfilename)
{
  RsvgHandle *handle = NULL;
  bool success = false;
  errno = 0;
  FILE *file = fopen (find_library_file (svgfilename).c_str(), "rb");
  int errsaved = errno;
  if (file)
    {
      handle = rsvg_handle_new();
      g_object_ref_sink (handle);
      uint8 buffer[8192];
      ssize_t len;
      while ((len = fread (buffer, 1, sizeof (buffer), file)) > 0)
        if (!(success = rsvg_handle_write (handle, buffer, len, NULL)))
          break;
      if (!success)
        errsaved = errsaved ? errsaved : errno;
      fclose (file);
      success = success && rsvg_handle_close (handle, NULL);
      if (!success)
        {
          g_object_unref (handle);
          handle = NULL;
          errsaved = errsaved ? errsaved : EINVAL;
        }
    }
  else
    errsaved = errsaved ? errsaved : EIO;
  FileImpl *fp = new FileImpl (handle);
  fp->m_errno = errsaved;
  return FileP (fp);
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
FileImpl::dump_tree ()
{
  if (m_handle)
    {
      RsvgHandlePrivate *p = m_handle->priv;
      RsvgNode *node = p->treebase;
      printerr ("\n%s:\n", p->title ? p->title->str : "???");
      dump_node (node, 0, INT64_MAX);
    }
}

/**
 * @fn File::lookup
 * Lookup @a elementid in the SVG file and return a non-NULL element on success.
 */
ElementP
FileImpl::lookup (const String &elementid)
{
  if (m_handle)
    {
      RsvgHandle *handle = m_handle;
      RsvgDimensionData rd = { 0, 0, 0, 0 };
      RsvgDimensionData dd = { 0, 0, 0, 0 };
      RsvgPositionData dp = { 0, 0 };
      const char *cid = elementid.empty() ? NULL : elementid.c_str();
      rsvg_handle_get_dimensions (handle, &rd);         // FIXME: cache results
      if (rd.width > 0 && rd.height > 0 &&
          rsvg_handle_get_dimensions_sub (handle, &dd, cid) && dd.width > 0 && dd.height > 0 &&
          rsvg_handle_get_position_sub (handle, &dp, cid))
        {
          ElementImpl *ei = new ElementImpl();
          ei->m_handle = handle;
          g_object_ref (ei->m_handle);
          ei->m_x = dp.x;
          ei->m_y = dp.y;
          ei->m_width = dd.width;
          ei->m_height = dd.height;
          ei->m_rw = rd.width;
          ei->m_rh = rd.height;
          ei->m_em = dd.em;
          ei->m_ex = dd.ex;
          ei->m_id = elementid;
          return ElementP (ei);
        }
    }
  return Element::none();
}

static void
init_svg_lib (const StringVector &args)
{
  g_type_init(); // NOP on subsequent invocations
  File::add_search_dir (RAPICORN_SVGDIR);
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

} // extern "C"
