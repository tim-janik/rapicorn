/* This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 */
#include "svg.hh"
#include "strings.hh"
#include <librsvg/rsvg.h>
#include "main.hh"
#include <regex>

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

String
BBox::to_string () const
{
  return string_format ("%.7g,%.7g,%.7g,%.7g", x, y, width, height);
}

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

// == FileImpl ==
struct FileImpl : public File {
  RsvgHandle           *handle_;
  String                name_;
  StringVector          good_ids_;
  std::set<String>      id_candidates_;
  explicit              FileImpl        (RsvgHandle *hh, const String &name) : handle_ (hh), name_ (name) {}
  /*dtor*/             ~FileImpl        () { if (handle_) g_object_unref (handle_); }
  virtual String        name            () const override { return name_; }
  virtual ElementP      lookup          (const String &elementid) override;
  virtual StringVector  list            () override;
};

// == File ==
FileP
File::load (const String &svgfilename)
{
  Blob blob = Blob::load (svgfilename);
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
  auto fp = std::make_shared<FileImpl> (handle, svg_blob.name());
  // workaround for https://bugzilla.gnome.org/show_bug.cgi?id=764610 - missing rsvg_handle_list()
  {
    static const std::regex id_pattern ("\\bid=\"([^\"]+)\""); // find id="..." occourances
    const String &input = svg_blob.string(); // l-value to pass into sregex_iterator
    const std::sregex_iterator ids_end;
    auto ids_begin = std::sregex_iterator (input.begin(), input.end(), id_pattern);
    for (auto it = ids_begin; it != ids_end; ++it)
      {
        const std::smatch &match = *it;
        const std::string &m = match[1];
        fp->id_candidates_.insert (m);
      }
  }
  errno = 0;
  return fp;
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

/**
 * @fn File::list
 * List all element IDs in an SVG file that start with @a prefix.
 */
StringVector
FileImpl::list()
{
  if (good_ids_.empty() && !id_candidates_.empty())
    {
      for (const String &candidate : id_candidates_)
        if (rsvg_handle_has_sub (handle_, ("#" + candidate).c_str()))
          good_ids_.push_back (candidate);
      id_candidates_.clear();
    }
  return good_ids_;
}

// == Span ==
/** Distribute @a amount across the @a length fields of all @a spans.
 * Increase (or decrease in case of a negative @a amount) the @a lentgh
 * of all @a resizable @a spans to distribute @a amount most equally.
 * Spans are considered resizable if the @a resizable field is >=
 * @a resizable_level.
 * Precedence will be given to spans in the middle of the list.
 * If @a amount is negative, each span will only be shrunk up to its
 * existing length, the rest is evenly distributed among the remaining
 * shrnkable spans. If no spans are left to be resized,
 * the remaining @a amount is be returned.
 * @return Reminder of @a amount that could not be distributed.
 */
ssize_t
Span::distribute (const size_t n_spans, Span *spans, ssize_t amount, size_t resizable_level)
{
  if (n_spans == 0 || amount == 0)
    return amount;
  assert (spans);
  // distribute amount
  if (amount > 0)
    {
      // sort expandable spans by mid-point distance with right-side bias for odd amounts
      size_t n = 0;
      Span *rspans[n_spans];
      for (size_t j = 0; j < n_spans; j++) // helper index, i does outwards indexing
        {
          const size_t i = outwards_index (j, n_spans, true);
          if (spans[i].resizable >= resizable_level) // considered resizable
            rspans[n++] = &spans[i];
        }
      if (!n)
        return amount;
      // expand in equal shares
      const size_t delta = amount / n;
      if (delta)
        for (size_t i = 0; i < n; i++)
          rspans[i]->length += delta;
      amount -= delta * n;
      // spread remainings
      for (size_t i = 0; i < n && amount; i++, amount--)
        rspans[i]->length += 1;
    }
  else /* amount < 0 */
    {
      // sort shrinkable spans by mid-point distance with right-side bias for odd amounts
      size_t n = 0;
      Span *rspans[n_spans];
      for (size_t j = 0; j < n_spans; j++) // helper index, i does outwards indexing
        {
          const size_t i = outwards_index (j, n_spans, true);
          if (spans[i].resizable >= resizable_level &&  // considered resizable
              spans[i].length > 0)                      // and shrinkable
            rspans[n++] = &spans[i];
        }
      // shrink in equal shares
      while (n && amount)
        {
          // shrink spans within length limits
          const size_t delta = -amount / n;
          if (delta == 0)
            break;                              // delta < 1 per span
          size_t m = 0;
          for (size_t i = 0; i < n; i++)
            {
              const size_t adjustable = std::min (rspans[i]->length, delta);
              rspans[i]->length -= adjustable;
              amount += adjustable;
              if (rspans[i]->length)
                rspans[m++] = rspans[i];        // retain shrinkable spans
            }
          n = m;                                // discard non-shrinkable spans
        }
      // spread remainings
      for (size_t i = 0; i < n && amount; i++, amount++)
        rspans[i]->length -= 1;
    }
  return amount;
}

String
Span::to_string() const
{
  return string_format ("%u,%u", length, resizable);
}

String
Span::to_string (size_t n, const Span *spans)
{
  String s;
  for (size_t i = 0; i < n; i++)
    {
      if (!s.empty())
        s += ",";
      s += "{" + spans[i].to_string() + "}";
    }
  return s;
}

/* Element::stretch renders an SVG element into a surface and then interpolates
 * some parts to match the target image size. Prerendering is scaled up (per
 * dimension) to avoid interpolation in case the target dimension has no fixed
 * (non-interpolated) parts.
 * TODO1: support adaptions to different DPI by applying a factor to the bbox dimensions and ?scale_factor.
 * TODO2: test quality improvements by scaling up the prerendering so that interpolation only ever shrinks.
 */
///< Render a stretched image by resizing horizontal/vertical parts (spans).
cairo_surface_t*
Element::stretch (const size_t image_width, const size_t image_height,
                  size_t n_hspans, const Span *svg_hspans,
                  size_t n_vspans, const Span *svg_vspans,
                  cairo_filter_t filter)
{
  assert_return (image_width > 0 && image_height > 0, NULL);
  assert_return (n_hspans >= 1 && n_vspans >= 1, NULL);
  // setup SVG element sizes
  const BBox bb = bbox();
  double hscale_factor = 1, vscale_factor = 1;
  size_t svg_width = bb.width + 0.5, svg_height = bb.height + 0.5;
  // copy and distribute spans to match image size
  Span image_hspans[n_hspans], image_vspans[n_vspans];
  bool hscalable = image_width != svg_width;
  ssize_t span_sum = 0;
  for (size_t i = 0; i < n_hspans; i++)
    {
      image_hspans[i] = svg_hspans[i];
      span_sum += svg_hspans[i].length;
      if (svg_hspans[i].length && svg_hspans[i].resizable == 0)
        hscalable = false;
    }
  assert_return (span_sum == bb.width, NULL);
  if (hscalable)
    {
      hscale_factor = image_width / double (svg_width);
      svg_width = image_width;
      n_hspans = 1;
      image_hspans[0].length = svg_width;
      image_hspans[0].resizable = 1;
    }
  else
    {
      ssize_t dremain = Span::distribute (n_hspans, image_hspans, image_width - svg_width, 1);
      if (dremain < 0)
        dremain = Span::distribute (n_hspans, image_hspans, dremain, 0); // shrink *any* segment
      critical_unless (dremain == 0);
    }
  bool vscalable = image_height != svg_height;
  span_sum = 0;
  for (size_t i = 0; i < n_vspans; i++)
    {
      image_vspans[i] = svg_vspans[i];
      span_sum += svg_vspans[i].length;
      if (svg_vspans[i].length && svg_vspans[i].resizable == 0)
        vscalable = false;
    }
  assert_return (span_sum == bb.height, NULL);
  if (vscalable)
    {
      vscale_factor = image_height / double (svg_height);
      svg_height = image_height;
      n_vspans = 1;
      image_vspans[0].length = svg_height;
      image_vspans[0].resizable = 1;
    }
  else
    {
      ssize_t dremain = Span::distribute (n_vspans, image_vspans, image_height - svg_height, 1);
      if (dremain < 0)
        dremain = Span::distribute (n_vspans, image_vspans, dremain, 0); // shrink *any* segment
      critical_unless (dremain == 0);
    }
  // render (scaled) SVG image into svg_surface at non-interpolated size
  cairo_surface_t *svg_surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, svg_width, svg_height);
  assert_return (CAIRO_STATUS_SUCCESS == cairo_surface_status (svg_surface), NULL);
  const bool svg_rendered = render (svg_surface, Svg::RenderSize::ZOOM, hscale_factor, vscale_factor);
  critical_unless (svg_rendered == true);
  if (hscalable && vscalable)
    return svg_surface;
  // stretch svg_surface to image size by interpolating the resizable spans
  cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, image_width, image_height);
  assert_return (CAIRO_STATUS_SUCCESS == cairo_surface_status (surface), NULL);
  cairo_t *cr = cairo_create (surface);
  critical_unless (CAIRO_STATUS_SUCCESS == cairo_status (cr));
  ssize_t svg_voffset = 0, vspan_offset = 0;
  for (size_t v = 0; v < n_vspans; v++)
    {
      if (image_vspans[v].length <= 0)
        {
          svg_voffset += svg_vspans[v].length;
          continue;
        }
      ssize_t svg_hoffset = 0, hspan_offset = 0;
      for (size_t h = 0; h < n_hspans; h++)
        {
          if (image_hspans[h].length <= 0)
            {
              svg_hoffset += svg_hspans[h].length;
              continue;
            }
          cairo_set_source_surface (cr, svg_surface, 0, 0); // (ix,iy) are set in the matrix below
          cairo_matrix_t matrix;
          cairo_matrix_init_translate (&matrix, svg_hoffset, svg_voffset); // adjust image origin
          const double xscale = hscalable ? 1.0 : svg_hspans[h].length / double (image_hspans[h].length);
          const double yscale = vscalable ? 1.0 : svg_vspans[v].length / double (image_vspans[v].length);
          if (xscale != 1.0 || yscale != 1.0)
            {
              cairo_matrix_scale (&matrix, xscale, yscale);
              cairo_pattern_set_filter (cairo_get_source (cr), filter);
            }
          cairo_matrix_translate (&matrix, - hspan_offset, - vspan_offset); // shift image fragment into position
          cairo_pattern_set_matrix (cairo_get_source (cr), &matrix);
          cairo_rectangle (cr, hspan_offset, vspan_offset, image_hspans[h].length, image_vspans[v].length);
          cairo_clip (cr);
          cairo_paint (cr);
          cairo_reset_clip (cr);
          hspan_offset += image_hspans[h].length;
          svg_hoffset += svg_hspans[h].length;
        }
      vspan_offset += image_vspans[v].length;
      svg_voffset += svg_vspans[v].length;
    }
  // cleanup
  cairo_destroy (cr);
  cairo_surface_destroy (svg_surface);
  return surface;
}

} // Svg
} // Rapicorn
