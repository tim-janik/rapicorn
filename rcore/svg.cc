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

IBox::IBox () :
  x (-1), y (-1), width (0), height (0)
{}

IBox::IBox (int _x, int _y, int w, int h) :
  x (_x), y (_y), width (w), height (h)
{}

String
IBox::to_string () const
{
  return string_format ("%+d%+d%+dx%d", x, y, width, height);
}

struct ElementImpl : public Element {
  String        id_;
  RsvgHandle   *handle_;
  IBox          bbox_, ink_; // bounding box and ink box
  double        em_, ex_;
  bool          is_measured_;
  explicit      ElementImpl     () : handle_ (NULL), em_ (0), ex_ (0), is_measured_ (false) {}
  virtual      ~ElementImpl     ();
  virtual Info  info            ();
  virtual IBox  bbox            ()                      { return bbox_; }
  virtual IBox  ibox            ()                      { return ink_; }
  virtual bool  render          (cairo_surface_t *surface, RenderSize rsize, double xscale, double yscale);
  void          measure_ink    ();
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
                       -ink_.x + (ink_.width * xscale - ink_.width) / 2.0,
                       -ink_.y + (ink_.height * yscale - ink_.height) / 2.0);
      rendered = rsvg_handle_render_cairo_sub (handle_, cr, cid);
      break;
    case RenderSize::ZOOM:
      cairo_scale (cr, xscale, yscale); // scale as requested
      cairo_translate (cr, -ink_.x, -ink_.y); // shift sub into top_left of surface
      rendered = rsvg_handle_render_cairo_sub (handle_, cr, cid);
      break;
    }
  cairo_destroy (cr);
  return rendered;
}

static inline uint32
cairo_surface_scan_col (cairo_surface_t *surface, int x)
{
  assert_return (surface != NULL, 0x00000000);
  assert_return (cairo_image_surface_get_format (surface) == CAIRO_FORMAT_ARGB32, 0x00000000);
  const int width = cairo_image_surface_get_width (surface);
  double xoff = 0, yoff = 0;
  cairo_surface_get_device_offset (surface, &xoff, &yoff);
  x += xoff;
  assert_return (0 <= x && x < width, 0x00000000);
  const int height = cairo_image_surface_get_height (surface);
  const int stride = cairo_image_surface_get_stride (surface);
  const uint32 *const pixels = (const uint32*) cairo_image_surface_get_data (surface);
  for (int y = 0; y < height; y++)
    {
      const uint32 argb = pixels[stride / 4 * y + x];
      if (argb & 0xff000000)
        return argb;
    }
  return 0x00000000;
}

static inline uint32
cairo_surface_scan_row (cairo_surface_t *surface, int y)
{
  assert_return (surface != NULL, 0x00000000);
  assert_return (cairo_image_surface_get_format (surface) == CAIRO_FORMAT_ARGB32, 0x00000000);
  const int height = cairo_image_surface_get_height (surface);
  double xoff = 0, yoff = 0;
  cairo_surface_get_device_offset (surface, &xoff, &yoff);
  y += yoff;
  assert_return (0 <= y && y < height, 0x00000000);
  const int width = cairo_image_surface_get_width (surface);
  const int stride = cairo_image_surface_get_stride (surface);
  const uint32 *const pixels = (const uint32*) cairo_image_surface_get_data (surface);
  for (int x = 0; x < width; x++)
    {
      const uint32 argb = pixels[stride / 4 * y + x];
      if (argb & 0xff000000)
        return argb;
    }
  return 0x00000000;
}

void
ElementImpl::measure_ink()
{
  return_unless (is_measured_ == false);
  const char *cid = id_.empty() ? NULL : id_.c_str();
  // inkscape uses 90 DPI for unit->pixel mapping and librsvg only renders stable at 90 DPI
  rsvg_handle_set_dpi_x_y (handle_, 90, 90);
  // get librsvg's bbox, unfortunately it's often not wide enough due to rounding errors
  RsvgPositionData dp = { 0, 0 };
  rsvg_handle_get_position_sub (handle_, &dp, cid);
  bbox_.x = dp.x;
  bbox_.y = dp.y;
  RsvgDimensionData dd = { 0, 0, 0, 0 };
  rsvg_handle_get_dimensions_sub (handle_, &dd, cid);
  if (0) // adjust for rounding at the output stage, this'd work if dp/dd used doubles
    {
      bbox_.width = int (dp.x + dd.width + 0.95) - bbox_.x;
      bbox_.height = int (dp.y + dd.height + 0.95) - bbox_.y;
    }
  else
    {
      bbox_.width = dd.width;
      bbox_.height = dd.height;
    }
  em_ = dd.em;
  ex_ = dd.ex;
  // measure element size by peeking at output pixels
#if 1
  const int delta = 3; // fudge around origin
  const int x = dp.x - delta, y = dp.y - delta;
  const int width = MAX (dd.width, 8) * 1.5 + 0.9 + 2 * delta;   // add enough padding to make
  const int height = MAX (dd.height, 8) * 1.5 + 0.9 + 2 * delta; // cut-offs highly unlikely
  cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
  assert_return (cairo_surface_status (surface) == CAIRO_STATUS_SUCCESS);
  cairo_surface_set_device_offset (surface, -x, -y);
  cairo_t *cr = cairo_create (surface);
  assert_return (cairo_status (cr) == CAIRO_STATUS_SUCCESS);
  const bool rendered = rsvg_handle_render_cairo_sub (handle_, cr, cid);
  assert_return (rendered);
  for (ink_.x = x; ink_.x < x + width; ink_.x++)
    if (cairo_surface_scan_col (surface, ink_.x) != 0)
      break;
  for (ink_.y = y; ink_.y < y + height; ink_.y++)
    if (cairo_surface_scan_row (surface, ink_.y) != 0)
      break;
  int b;
  for (b = x + width; b > ink_.x; b--)
    if (cairo_surface_scan_col (surface, b - 1) != 0)
      break;
  ink_.width = b - ink_.x;
  for (b = y + height; b > ink_.y; b--)
    if (cairo_surface_scan_row (surface, b - 1) != 0)
      break;
  ink_.height = b - ink_.y;
  if (false && // debug code
      (ink_.width != dd.width || ink_.height != dd.height))
    {
      static int dcounter = 1;
      String fname = string_format ("xdbg%02x.png", dcounter++);
      printerr ("SVG: id=%s rsvg-bbox=%+d%+d%+dx%d measured-bbox=%s (%s)\n", id_,
                dp.x, dp.y, dd.width, dd.height,
                ink_.to_string(), fname);
      unlink (fname.c_str());
      if (cairo_surface_write_to_png (surface, fname.c_str()) != CAIRO_STATUS_SUCCESS)
        unlink (fname.c_str());
    }
  // grow bbox to cover ink
  if (ink_.width && ink_.height)
    {
      if (ink_.x < bbox_.x)
        {
          bbox_.width += bbox_.x - ink_.x;
          bbox_.x = ink_.x;
        }
      if (ink_.x + ink_.width > bbox_.x + bbox_.width)
        bbox_.width = ink_.x + ink_.width - bbox_.x;
      if (ink_.y < bbox_.y)
        {
          bbox_.height += bbox_.y - ink_.y;
          bbox_.y = ink_.y;
        }
      if (ink_.y + ink_.height > bbox_.y + bbox_.height)
        bbox_.height = ink_.y + ink_.height - bbox_.y;
    }
  else
    ink_ = IBox (bbox_.x, bbox_.y, 0, 0);
  cairo_destroy (cr);
  cairo_surface_destroy (surface);
#endif
  // increase DPI to subsample bbox
#if 0
  const double dpi = 90, subsampling = 3.33; // subsample DPI to reduce librsvg rounding errors
  rsvg_handle_set_dpi_x_y (handle_, dpi * subsampling, dpi * subsampling);
  rsvg_handle_get_dimensions_sub (handle_, &dd, cid);
  rsvg_handle_get_position_sub (handle_, &dp, cid);
  x_ = dp.x / subsampling + 0.5;
  y_ = dp.y / subsampling + 0.5;
  width_ = dd.width / subsampling + 0.5;
  height_ = dd.height / subsampling + 0.5;
  rsvg_handle_set_dpi_x_y (handle_, 90, 90); // restore librsvg standard DPI
#endif
  // done
  is_measured_ = true;
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
  rsvg_handle_set_dpi_x_y (handle, 90, 90); // ensure 90 DPI for librsvg and inkscape content to work correctly
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
      String fragment;
      const char *cid;
      if (elementid.empty() || elementid == "#")
        fragment = "";                  // pick root
      else if (elementid[0] == '#')
        fragment = elementid;           // hash included
      else if (elementid[0] != '#')
        fragment = "#" + elementid;     // add missing hash
      cid = fragment.empty() ? NULL : fragment.c_str();
      if (!cid || rsvg_handle_has_sub (handle, cid))
        {
          ElementImpl *ei = new ElementImpl();
          ei->handle_ = handle;
          g_object_ref (ei->handle_);
          ei->id_ = fragment;
          ei->measure_ink();
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
  const IBox ib = ibox();
  double hscale_factor = 1, vscale_factor = 1;
  size_t svg_width = ib.width, svg_height = ib.height;
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
  assert_return (span_sum == ib.width, NULL);
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
  assert_return (span_sum == ib.height, NULL);
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
