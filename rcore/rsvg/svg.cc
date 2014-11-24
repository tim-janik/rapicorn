/* This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0 */
#include "svg.hh"
#include "../strings.hh"
#include "../blobres.hh"
#include "rsvg.h"
#include "rsvg-cairo.h"
#include "rsvg-private.h"
#include "../main.hh"

/* A note on coordinate system increments:
 * - Librsvg:  X to right, Y downwards.
 * - Cairo:    X to right, Y downwards.
 * - X11/GTK:  X to right, Y downwards.
 * - Windows:  X to right, Y downwards.
 * - Inkscape: X to right, Y upwards (UI coordinate system).
 * - Cocoa:    X to right, Y upwards.
 * - ACAD:     X to right, Y upwards.
 */

namespace Rapicorn {

/// @namespace Rapicorn::Svg The Rapicorn::Svg namespace provides functions for handling and rendering of SVG files and elements.
namespace Svg {

BBox::BBox () :
  x (-1), y (-1), width (0), height (0)
{}

BBox::BBox (double _x, double _y, double w, double h) :
  x (_x), y (_y), width (w), height (h)
{}

struct ElementImpl : public Element {
  String        id_;
  int           x_, y_, width_, height_, rw_, rh_; // relative to bottom_left
  double        em_, ex_;
  RsvgHandle   *handle_;
  explicit      ElementImpl     () : x_ (0), y_ (0), width_ (0), height_ (0), em_ (0), ex_ (0), handle_ (NULL) {}
  virtual      ~ElementImpl     ();
  virtual Info  info            ();
  virtual BBox  bbox            ()                      { return BBox (x_, y_, width_, height_); }
  virtual BBox  enfolding_bbox  (BBox &inner);
  virtual BBox  containee_bbox  ();
  virtual BBox  containee_bbox  (BBox &_resized);
  virtual bool  render          (cairo_surface_t *surface, RenderSize rsize, double xscale, double yscale);
};

const ElementP
Element::none ()
{
  return ElementP();
}

ElementImpl::~ElementImpl ()
{
  id_ = "";
  RsvgHandle *ohandle = handle_;
  handle_ = NULL;
  if (ohandle)
    g_object_unref (ohandle);
}

Info
ElementImpl::info ()
{
  Info i;
  i.id = id_;
  i.em = em_;
  i.ex = ex_;
  return i;
}

BBox
ElementImpl::enfolding_bbox (BBox &containee)
{
  if (containee.width <= 0 || containee.height <= 0)
    return bbox();
  // FIXME: resize for _containee width/height
  BBox a = BBox (x_, y_, containee.width + 4, containee.height + 4);
  return a;
}

BBox
ElementImpl::containee_bbox ()
{
  if (width_ <= 4 || height_ <= 4)
    return BBox();
  // FIXME: calculate _containee size
  BBox a = BBox (x_ + 2, y_ + 2, width_ - 4, height_ - 4);
  return a;
}

BBox
ElementImpl::containee_bbox (BBox &resized)
{
  if (resized.width == width_ && resized.height == height_)
    return bbox();
  // FIXME: calculate _containee size when resized
  return bbox();
}

bool
ElementImpl::render (cairo_surface_t *surface, RenderSize rsize, double xscale, double yscale)
{
  assert_return (surface != NULL, false);
  cairo_t *cr = cairo_create (surface);
  const char *cid = id_.empty() ? NULL : id_.c_str();
  bool rendered = false;
  switch (rsize)
    {
    case RenderSize::STATIC:
      cairo_translate (cr, // shift sub into top_left and center extra space
                       -x_ + (width_ * xscale - width_) / 2.0,
                       -y_ + (height_ * yscale - height_) / 2.0);
      rendered = rsvg_handle_render_cairo_sub (handle_, cr, cid);
      break;
    case RenderSize::ZOOM:
      cairo_scale (cr, xscale, yscale); // scale as requested
      cairo_translate (cr, -x_, -y_); // shift sub into top_left of surface
      rendered = rsvg_handle_render_cairo_sub (handle_, cr, cid);
      break;
    }
  cairo_destroy (cr);
  return rendered;
}

struct FileImpl : public File {
  RsvgHandle           *handle_;
  explicit              FileImpl        (RsvgHandle *hh) : handle_ (hh) {}
  /*dtor*/             ~FileImpl        () { if (handle_) g_object_unref (handle_); }
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
  Blob blob = Blob::load ("file:///" + find_library_file (svgfilename));
  const int saved_errno = errno;
  if (!blob)
    {
      FileP fp = FileP();
      errno = saved_errno ? saved_errno : ENOENT;
      return fp;
    }
  return load (blob);
}

FileP
File::load (Blob svg_blob)
{
  RsvgHandle *handle = rsvg_handle_new();
  g_object_ref_sink (handle);
  const bool success = rsvg_handle_write (handle, svg_blob.bytes(), svg_blob.size(), NULL) && rsvg_handle_close (handle, NULL);
  if (!success)
    {
      g_object_unref (handle);
      handle = NULL;
      FileP fp = FileP();
      errno = ENODATA;
      return fp;
    }
  FileP fp (new FileImpl (handle));
  errno = 0;
  return fp;
}

static void
dump_node (RsvgNode *self, int64 depth, int64 maxdepth)
{
  if (depth > maxdepth)
    return;
  for (int64 i = 0; i < depth; i++)
    printerr (" ");
  printerr ("<0x%02x/>\n", self->type);
  for (uint i = 0; i < self->children->len; i++)
    {
      RsvgNode *child = (RsvgNode*) g_ptr_array_index (self->children, i);
      dump_node (child, depth + 1, maxdepth);
    }
}

void
FileImpl::dump_tree ()
{
  if (handle_)
    {
      RsvgHandlePrivate *p = handle_->priv;
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
  if (handle_)
    {
      RsvgHandle *handle = handle_;
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
          ei->handle_ = handle;
          g_object_ref (ei->handle_);
          ei->x_ = dp.x;
          ei->y_ = dp.y;
          ei->width_ = dd.width;
          ei->height_ = dd.height;
          ei->rw_ = rd.width;
          ei->rh_ = rd.height;
          ei->em_ = dd.em;
          ei->ex_ = dd.ex;
          ei->id_ = elementid;
          if (0)
            printerr ("SUB: %s: bbox=%d,%d,%dx%d dim=%dx%d em=%f ex=%f\n",
                      ei->id_.c_str(), ei->x_, ei->y_, ei->width_, ei->height_,
                      ei->rw_, ei->rh_, ei->em_, ei->ex_);
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

} // Svg
} // Rapicorn
