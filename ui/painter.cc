// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "painter.hh"
#include "blitfuncs.hh"
#include "../rcore/svg.hh"
#include <algorithm>

#define SVGDEBUG(...)   RAPICORN_KEY_DEBUG ("SVG", __VA_ARGS__)

#define CHECK_CAIRO_STATUS(status)      do {    \
  cairo_status_t ___s = (status);               \
  if (___s != CAIRO_STATUS_SUCCESS)             \
    SVGDEBUG ("%s: %s", cairo_status_to_string (___s), #status);        \
  } while (0)

namespace Rapicorn {

// == CPainter ==
CPainter::CPainter (cairo_t *_context) :
  cr (_context)
{}

CPainter::~CPainter ()
{}

void
CPainter::draw_border (int x, int y, int width, int height, Color border, const vector<double> &dashes, double dash_offset)
{
  const double line_width = 1.0, l2 = line_width / 2.;
  cairo_set_line_width (cr, line_width);
  if (width && height)
    {
      Rect r (x + l2, y + l2, width - 1, height - 1);
      cairo_set_source_rgba (cr, border.red1(), border.green1(), border.blue1(), border.alpha1());
      cairo_rectangle (cr, r.x, r.y, r.width, r.height);
      cairo_set_dash (cr, dashes.data(), dashes.size(), dash_offset);
      cairo_stroke (cr);
    }
}

void
CPainter::draw_shadow (int x, int y, int width, int height,
                       Color outer_upper_left, Color inner_upper_left,
                       Color inner_lower_right, Color outer_lower_right)
{
  cairo_set_line_width (cr, 1.0);
  // outer upper left triangle
  if (width >= 1 && height >= 1)
    {
      cairo_set_source_rgba (cr, outer_upper_left.red1(), outer_upper_left.green1(),
                             outer_upper_left.blue1(), outer_upper_left.alpha1());
      cairo_move_to (cr, x + .5, y + height);
      cairo_rel_line_to (cr, 0, -(height - .5));
      cairo_rel_line_to (cr, +width - .5, 0);
      cairo_stroke (cr);
    }
  // outer lower right triangle
  if (width >= 2 && height >= 2)
    {
      cairo_set_source_rgba (cr, outer_lower_right.red1(), outer_lower_right.green1(),
                             outer_lower_right.blue1(), outer_lower_right.alpha1());
      cairo_move_to (cr, x + 1, y + height - .5);
      cairo_rel_line_to (cr, +width - 1.5, 0);
      cairo_rel_line_to (cr, 0, -(height - 1.5));
      cairo_stroke (cr);
    }
  // inner upper left triangle
  if (width >= 3 && height >= 3)
    {
      cairo_set_source_rgba (cr, inner_upper_left.red1(), inner_upper_left.green1(),
                             inner_upper_left.blue1(), inner_upper_left.alpha1());
      cairo_move_to (cr, x + 1.5, y + (height - 1));
      cairo_rel_line_to (cr, 0, -(height - 2.5));
      cairo_rel_line_to (cr, +width - 2.5, 0);
      cairo_stroke (cr);
    }
  // inner lower right triangle
  if (width >= 4 && height >= 4)
    {
      cairo_set_source_rgba (cr, inner_lower_right.red1(), inner_lower_right.green1(),
                             inner_lower_right.blue1(), inner_lower_right.alpha1());
      cairo_move_to (cr, x + 2, y + height - 1.5);
      cairo_rel_line_to (cr, +width - 3.5, 0);
      cairo_rel_line_to (cr, 0, -(height - 3.5));
      cairo_stroke (cr);
    }
}

void
CPainter::draw_filled_rect (int x, int y, int width, int height, Color fill)
{
  cairo_set_source_rgba (cr, fill.red1(), fill.green1(), fill.blue1(), fill.alpha1());
  cairo_rectangle (cr, x, y, width, height);
  cairo_fill (cr);
}

void
CPainter::draw_shaded_rect (int xc0, int yc0, Color color0, int xc1, int yc1, Color color1)
{
  cairo_pattern_t *gradient = cairo_pattern_create_linear (xc0, yc0, xc1, yc1);
  cairo_pattern_add_color_stop_rgba (gradient, 0,
                                     color0.red1(), color0.green1(),
                                     color0.blue1(), color0.alpha1());
  cairo_pattern_add_color_stop_rgba (gradient, 1,
                                     color1.red1(), color1.green1(),
                                     color1.blue1(), color1.alpha1());
  cairo_set_source (cr, gradient);
  cairo_pattern_destroy (gradient);
  cairo_rectangle (cr, MIN (xc0, xc1), MIN (yc0, yc1),
                   MAX (xc0, xc1) - MIN (xc0, xc1) + 1,
                   MAX (yc0, yc1) - MIN (yc0, yc1) + 1);
  cairo_fill (cr);
}

void
CPainter::draw_center_shade_rect (int xc0, int yc0, Color color0, int xc1, int yc1, Color color1)
{
  draw_shaded_rect (xc0, yc0, color0, xc1, yc1, color1);
}

void
CPainter::draw_dir_arrow (double x, double y, double width, double height, Color fill, Direction dir)
{
  double xhalf = width / 2., yhalf = height / 2.;
  switch (dir)
    {
    case Direction::RIGHT:
      cairo_move_to (cr, x + width, y + yhalf);
      cairo_rel_line_to (cr, -width, +yhalf);
      cairo_rel_line_to (cr, 0, -height);
      cairo_close_path (cr);
      break;
    case Direction::DOWN:
      cairo_move_to (cr, x, y);
      cairo_rel_line_to (cr, +width, 0);
      cairo_rel_line_to (cr, -xhalf, height);
      cairo_close_path (cr);
      break;
    case Direction::LEFT:
      cairo_move_to (cr, x, y + yhalf);
      cairo_rel_line_to (cr, +width, -yhalf);
      cairo_rel_line_to (cr, 0, +height);
      cairo_close_path (cr);
      break;
    case Direction::UP:
      cairo_move_to (cr, x + width, y + height);
      cairo_rel_line_to (cr, -width, 0);
      cairo_rel_line_to (cr, +xhalf, -height);
      cairo_close_path (cr);
      break;
    default: ;
    }
  cairo_set_source_rgba (cr, fill.red1(), fill.green1(), fill.blue1(), fill.alpha1());
  cairo_fill (cr);
}

// == Cairo Utilities ==
cairo_surface_t*
cairo_surface_from_pixmap (Pixmap pixmap)
{
  const int stride = pixmap.width() * 4;
  uint32 *data = pixmap.row (0);
  cairo_surface_t *isurface =
    cairo_image_surface_create_for_data ((unsigned char*) data,
                                         CAIRO_FORMAT_ARGB32,
                                         pixmap.width(),
                                         pixmap.height(),
                                         stride);
  assert_return (CAIRO_STATUS_SUCCESS == cairo_surface_status (isurface), NULL);
  return isurface;
}

// == ImageBackend ==
struct ImagePainter::ImageBackend : public std::enable_shared_from_this<ImageBackend> {
  virtual             ~ImageBackend     () {}
  virtual Requisition  image_size       () = 0;
  virtual Rect         fill_area        () = 0;
  virtual void         draw_image       (cairo_t *cairo_context, const Rect &render_rect, const Rect &image_rect) = 0;
  virtual StringVector list             (const String &prefix) = 0;
};

// == SvgImageBackend ==
struct SvgImageBackend : public virtual ImagePainter::ImageBackend {
  Svg::FileP      svgf_;
  Svg::ElementP   svge_;
  const Rect      fill_;
  const Svg::Span hscale_spans_[3], vscale_spans_[3];
public:
  SvgImageBackend (Svg::FileP svgf, Svg::ElementP svge, const Svg::Span (&hscale_spans)[3],
              const Svg::Span (&vscale_spans)[3], const Rect &fill_rect) :
    svgf_ (svgf), svge_ (svge), fill_ (fill_rect),
    hscale_spans_ { hscale_spans[0], hscale_spans[1], hscale_spans[2] },
    vscale_spans_ { vscale_spans[0], vscale_spans[1], vscale_spans[2] }
  {
    SVGDEBUG ("SvgImage: id=%s hspans={ l=%u r=%u, l=%u r=%u, l=%u r=%u } vspans={ l=%u r=%u, l=%u r=%u, l=%u r=%u } fill=%s",
              svge_->info().id,
              hscale_spans_[0].length, hscale_spans_[0].resizable,
              hscale_spans_[1].length, hscale_spans_[1].resizable,
              hscale_spans_[2].length, hscale_spans_[2].resizable,
              vscale_spans_[0].length, vscale_spans_[0].resizable,
              vscale_spans_[1].length, vscale_spans_[1].resizable,
              vscale_spans_[2].length, vscale_spans_[2].resizable,
              fill_.string());
    const Svg::BBox bb = svge_->bbox();
    size_t hsum = 0;
    for (size_t i = 0; i < ARRAY_SIZE (hscale_spans_); i++)
      hsum += hscale_spans_[i].length;
    critical_unless (hsum == bb.width);
    size_t vsum = 0;
    for (size_t i = 0; i < ARRAY_SIZE (vscale_spans_); i++)
      vsum += vscale_spans_[i].length;
    critical_unless (vsum == bb.height);
    critical_unless (fill_.x >= 0 && fill_.x < bb.width);
    critical_unless (fill_.x + fill_.width <= bb.width);
    critical_unless (fill_.y >= 0 && fill_.y < bb.height);
    critical_unless (fill_.y + fill_.height <= bb.height);
  }
  virtual StringVector
  list (const String &prefix)
  {
    return svgf_->list (prefix);
  }
  virtual Requisition
  image_size ()
  {
    const auto bbox = svge_->bbox();
    return Requisition (bbox.width, bbox.height);
  }
  virtual Rect
  fill_area ()
  {
    return fill_;
  }
  virtual void
  draw_image (cairo_t *cairo_context, const Rect &render_rect, const Rect &image_rect)
  {
    // stretch and render SVG image // FIXME: this should be cached
    const size_t w = image_rect.width + 0.5, h = image_rect.height + 0.5;
    cairo_surface_t *img = svge_->stretch (w, h, ARRAY_SIZE (hscale_spans_), hscale_spans_, ARRAY_SIZE (vscale_spans_), vscale_spans_);
    CHECK_CAIRO_STATUS (cairo_surface_status (img));
    // render context rectangle
    Rect rect = image_rect;
    rect.intersect (render_rect);
    return_unless (rect.width > 0 && rect.height > 0);
    cairo_save (cairo_context); cairo_t *cr = cairo_context;
    cairo_set_source_surface (cr, img, 0, 0); // (ix,iy) are set in the matrix below
    cairo_matrix_t matrix;
    cairo_matrix_init_translate (&matrix, -image_rect.x, -image_rect.y); // adjust image origin
    cairo_pattern_set_matrix (cairo_get_source (cr), &matrix);
    cairo_rectangle (cr, rect.x, rect.y, rect.width, rect.height);
    cairo_clip (cr);
    cairo_paint (cr);
    cairo_surface_destroy (img);
    cairo_restore (cairo_context); cr = NULL;
  }
};

// == PixmapImageBackend ==
struct PixmapImageBackend : public virtual ImagePainter::ImageBackend {
  Pixmap pixmap_;
public:
  PixmapImageBackend (const Pixmap &pixmap) :
    pixmap_ (pixmap)
  {}
  virtual StringVector
  list (const String &prefix)
  {
    return StringVector();
  }
  virtual Requisition
  image_size ()
  {
    return Requisition (pixmap_.width(), pixmap_.height());
  }
  virtual Rect
  fill_area ()
  {
    return Rect (0, 0, pixmap_.width(), pixmap_.height());
  }
  virtual void
  draw_image (cairo_t *cairo_context, const Rect &render_rect, const Rect &image_rect)
  {
    Rect rect = image_rect;
    rect.intersect (render_rect);
    return_unless (rect.width > 0 && rect.height > 0);
    cairo_surface_t *isurface = cairo_surface_from_pixmap (pixmap_);
    cairo_save (cairo_context); cairo_t *cr = cairo_context;
    cairo_set_source_surface (cr, isurface, 0, 0); // (ix,iy) are set in the matrix below
    cairo_matrix_t matrix;
    const double xscale = pixmap_.width() / image_rect.width, yscale = pixmap_.height() / image_rect.height;
    cairo_matrix_init_scale (&matrix, xscale, yscale);
    cairo_matrix_translate (&matrix, -image_rect.x, -image_rect.y); // adjust image origin
    cairo_pattern_set_matrix (cairo_get_source (cr), &matrix);
    if (xscale != 1.0 || yscale != 1.0)
      cairo_pattern_set_filter (cairo_get_source (cr), CAIRO_FILTER_BILINEAR);
    cairo_paint (cr);
    cairo_surface_destroy (isurface);
    cairo_restore (cairo_context); cr = NULL;
  }
};

// == ImagePainter ==
ImagePainter::ImagePainter()
{}

ImagePainter::ImagePainter (Pixmap pixmap)
{
  ImageBackendP image_backend;
  if (pixmap.width() && pixmap.height())
    image_backend_ = std::make_shared<PixmapImageBackend> (pixmap);
}

ImagePainter::ImagePainter (const String &resource_identifier)
{
  const ssize_t hashpos = resource_identifier.find ('#');
  const String fragment = hashpos < 0 ? "" : resource_identifier.substr (hashpos);
  const String resource = hashpos < 0 ? resource_identifier : resource_identifier.substr (0, hashpos);
  Blob blob;
  bool do_svg_file_io = false;
  if (!string_startswith (resource, "@res"))
    do_svg_file_io = RAPICORN_FLIPPER ("svg-file-io", "Rapicorn::ImagePainter: allow loading of SVG files from local file system.");
  if (do_svg_file_io)
    blob = Blob::load (resource);
  else
    blob = Res (resource); // call this even if !startswith("@res") to preserve debug message about missing resource
  if (string_endswith (blob.name(), ".svg"))
    {
      auto svgf = Svg::File::load (blob);
      SVGDEBUG ("loading: %s: %s", resource, strerror (errno));
      if (svgf)
        svg_file_setup (svgf, fragment);
    }
  else if (blob)
    {
      auto pixmap = Pixmap (blob);
      if (pixmap.width() && pixmap.height())
        image_backend_ = std::make_shared<PixmapImageBackend> (pixmap);
    }
}

ImagePainter::ImagePainter (Svg::FileP svgfile, const String &fragment)
{
  svg_file_setup (svgfile, fragment);
}

void
ImagePainter::svg_file_setup (Svg::FileP svgfile, const String &fragment)
{
  auto svge = svgfile ? svgfile->lookup (fragment) : Svg::Element::none();
  SVGDEBUG (" lookup: %s%s: %s", svgfile->name(), fragment, svge ? svge->bbox().to_string() : "failed");
  const Svg::BBox ibox = svge ? svge->bbox() : Svg::BBox();
  if (svge && ibox.width > 0 && ibox.height > 0)
    {
      Rect fill { 0, 0, ibox.width, ibox.height };
      Svg::Span hscale_spans[3] = { { 0, 0 }, { 0, 0 }, { 0, 0 } };
      hscale_spans[1].length = ibox.width;
      hscale_spans[1].resizable = 1;
      Svg::Span vscale_spans[3] = { { 0, 0 }, { 0, 0 }, { 0, 0 } };
      vscale_spans[1].length = ibox.height;
      vscale_spans[1].resizable = 1;
      // match stretchable SVG element IDs, syntax: "#" "widgetname" [ ".9" [ ".hvfillscale" ] ] [ ":" "state" [ "+" "state" ]... ]
      const size_t colon = std::min (fragment.find (':'), fragment.size());
      const String fragment_base = fragment.substr (0, colon); // contains everything before ":"
      const String fragment_state = fragment.substr (colon);   // contains ":" and everything after
      if (string_endswith (fragment_base, ".9"))
        {
          const double ix1 = ibox.x, ix2 = ibox.x + ibox.width, iy1 = ibox.y, iy2 = ibox.y + ibox.height;
          Svg::ElementP auxe;
          auxe = svgfile->lookup (fragment_base + ".hscale" + fragment_state);
          if (auxe)
            {
              const Svg::BBox bbox = auxe->bbox();
              SVGDEBUG ("    aux: %s%s: %s", svgfile->name(), auxe->info().id, auxe->bbox().to_string());
              const double bx1 = CLAMP (bbox.x, ix1, ix2), bx2 = CLAMP (bbox.x + bbox.width, ix1, ix2);
              if (bx1 < bx2)
                {
                  hscale_spans[0].resizable = 0, hscale_spans[0].length = bx1 - ix1;
                  hscale_spans[1].resizable = 1, hscale_spans[1].length = bx2 - bx1;
                  hscale_spans[2].resizable = 0, hscale_spans[2].length = ix2 - bx2;
                }
            }
          auxe = svgfile->lookup (fragment_base + ".vscale" + fragment_state);
          if (auxe)
            {
              const Svg::BBox bbox = auxe->bbox();
              SVGDEBUG ("    aux: %s%s: %s", svgfile->name(), auxe->info().id, auxe->bbox().to_string());
              const double by1 = CLAMP (bbox.y, iy1, iy2), by2 = CLAMP (bbox.y + bbox.height, iy1, iy2);
              if (by1 < by2)
                {
                  vscale_spans[0].resizable = 0, vscale_spans[0].length = by1 - iy1;
                  vscale_spans[1].resizable = 1, vscale_spans[1].length = by2 - by1;
                  vscale_spans[2].resizable = 0, vscale_spans[2].length = iy2 - by2;
                }
            }
          auxe = svgfile->lookup (fragment_base + ".hfill" + fragment_state);
          if (auxe)
            {
              const Svg::BBox bbox = auxe->bbox();
              SVGDEBUG ("    aux: %s%s: %s", svgfile->name(), auxe->info().id, auxe->bbox().to_string());
              const double bx1 = CLAMP (bbox.x, ix1, ix2), bx2 = CLAMP (bbox.x + bbox.width, ix1, ix2);
              if (bx1 < bx2)
                {
                  fill.x = bx1 - ix1;
                  fill.width = bx2 - bx1;
                }
            }
          auxe = svgfile->lookup (fragment_base + ".vfill" + fragment_state);
          if (auxe)
            {
              const Svg::BBox bbox = auxe->bbox();
              SVGDEBUG ("    aux: %s%s: %s", svgfile->name(), auxe->info().id, auxe->bbox().to_string());
              const double by1 = CLAMP (bbox.y, iy1, iy2), by2 = CLAMP (bbox.y + bbox.height, iy1, iy2);
              if (by1 < by2)
                {
                  fill.y = by1 - iy1;
                  fill.height = by2 - by1;
                }
            }
        }
      image_backend_ = std::make_shared<SvgImageBackend> (svgfile, svge, hscale_spans, vscale_spans, fill);
    }
}

ImagePainter::~ImagePainter ()
{}

Requisition
ImagePainter::image_size ()
{
  return image_backend_ ? image_backend_->image_size() : Requisition();
}

StringVector
ImagePainter::list (const String &prefix)
{
  return image_backend_ ? image_backend_->list (prefix) : StringVector();
}

Rect
ImagePainter::fill_area ()
{
  return image_backend_ ? image_backend_->fill_area() : Rect();
}

/// Render image into cairo context transformed into @a image_rect, clipped by @a render_rect.
void
ImagePainter::draw_image (cairo_t *cairo_context, const Rect &render_rect, const Rect &image_rect)
{
  if (image_backend_)
    image_backend_->draw_image (cairo_context, render_rect, image_rect);
}

ImagePainter&
ImagePainter::operator= (const ImagePainter &ip)
{
  image_backend_ = ip.image_backend_;
  return *this;
}

ImagePainter::operator bool () const
{
  return image_backend_ != NULL;
}

} // Rapicorn
