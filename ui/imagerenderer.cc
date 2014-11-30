// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#include "imagerenderer.hh"
#include "factory.hh"
#include "../rcore/rsvg/svg.hh"

#define SVGDEBUG(...)   RAPICORN_KEY_DEBUG ("SVG", __VA_ARGS__)

#define CHECK_CAIRO_STATUS(status)      do { \
  cairo_status_t ___s = (status);            \
  if (___s != CAIRO_STATUS_SUCCESS)          \
    SVGDEBUG ("%s: %s", cairo_status_to_string (___s), #status);   \
  } while (0)

namespace Rapicorn {

static cairo_surface_t*
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

typedef ImageRendererImpl::ImageBackend ImageBackend;

struct ImageRendererImpl::ImageBackend {
  virtual            ~ImageBackend    () {}
  virtual Requisition image_size      () = 0;
  virtual Allocation  fill_area       () = 0;
  virtual void        render_image    (const std::function<cairo_t* (const Rect&)> &mkcontext,
                                       const Rect &render_rect, const Rect &image_rect) = 0;
};

struct PixBackend : public virtual ImageBackend {
  Pixmap pixmap_;
public:
  PixBackend (const Pixmap &pixmap) :
    pixmap_ (pixmap)
  {}
  virtual Requisition
  image_size ()
  {
    return Requisition (pixmap_.width(), pixmap_.height());
  }
  virtual Allocation
  fill_area ()
  {
    return Allocation (0, 0, pixmap_.width(), pixmap_.height());
  }
  virtual void
  render_image (const std::function<cairo_t* (const Rect&)> &mkcontext, const Rect &render_rect, const Rect &image_rect)
  {
    Rect rect = image_rect;
    rect.intersect (render_rect);
    return_unless (rect.width > 0 && rect.height > 0);
    cairo_surface_t *isurface = cairo_surface_from_pixmap (pixmap_);
    cairo_t *cr = mkcontext (rect);
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
  }
};

struct SvgBackend : public virtual ImageBackend {
  Svg::FileP      svgf_;
  Svg::ElementP   svge_;
  const Rect      fill_;
  const Svg::Span hscale_spans_[3], vscale_spans_[3];
public:
  SvgBackend (Svg::FileP svgf, Svg::ElementP svge, const Svg::Span (&hscale_spans)[3],
              const Svg::Span (&vscale_spans)[3], const Rect &fill_rect) :
    svgf_ (svgf), svge_ (svge), fill_ (fill_rect), hscale_spans_ (hscale_spans), vscale_spans_ (vscale_spans)
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
  virtual Requisition
  image_size ()
  {
    const auto bbox = svge_->bbox();
    return Requisition (bbox.width, bbox.height);
  }
  virtual Allocation
  fill_area ()
  {
    return fill_;
  }
  virtual void
  render_image (const std::function<cairo_t* (const Rect&)> &mkcontext, const Rect &render_rect, const Rect &image_rect)
  {
    // stretch and render SVG image // FIXME: this should be cached
    const size_t w = image_rect.width + 0.5, h = image_rect.height + 0.5;
    cairo_surface_t *img = svge_->stretch (w, h, ARRAY_SIZE (hscale_spans_), hscale_spans_, ARRAY_SIZE (vscale_spans_), vscale_spans_);
    CHECK_CAIRO_STATUS (cairo_surface_status (img));
    // render context rectangle
    Rect rect = image_rect;
    rect.intersect (render_rect);
    return_unless (rect.width > 0 && rect.height > 0);
    cairo_t *cr = mkcontext (rect);
    cairo_set_source_surface (cr, img, 0, 0); // (ix,iy) are set in the matrix below
    cairo_matrix_t matrix;
    cairo_matrix_init_translate (&matrix, -image_rect.x, -image_rect.y); // adjust image origin
    cairo_pattern_set_matrix (cairo_get_source (cr), &matrix);
    cairo_rectangle (cr, rect.x, rect.y, rect.width, rect.height);
    cairo_clip (cr);
    cairo_paint (cr);
    cairo_surface_destroy (img);
  }
};

// == ImageRendererImpl ==
ImageRendererImpl::ImageRendererImpl()
{}

ImageRendererImpl::~ImageRendererImpl()
{}

ImageRendererImpl::ImageBackendP
ImageRendererImpl::load_source (const String &resource, const String &element_id)
{
  ImageBackendP image_backend;
  Blob blob = Res (resource);
  if (string_endswith (blob.name(), ".svg"))
    {
      auto svgf = Svg::File::load (blob);
      SVGDEBUG ("loading: %s: %s", resource, strerror (errno));
      auto svge = svgf ? svgf->lookup (element_id) : Svg::Element::none();
      SVGDEBUG (" lookup: %s%s: %s", resource, element_id, svge ? svge->bbox().to_string() : "failed");
      if (svge)
        {
          const Svg::BBox ibox = svge->bbox();
          Rect fill { 0, 0, ibox.width, ibox.height };
          Svg::Span hscale_spans[3] = { { 0, 0 }, { 0, 0 }, { 0, 0 } };
          hscale_spans[1].length = ibox.width;
          hscale_spans[1].resizable = 1;
          Svg::Span vscale_spans[3] = { { 0, 0 }, { 0, 0 }, { 0, 0 } };
          vscale_spans[1].length = ibox.height;
          vscale_spans[1].resizable = 1;
          if (string_endswith (element_id, ".9"))
            {
              const double ix1 = ibox.x, ix2 = ibox.x + ibox.width, iy1 = ibox.y, iy2 = ibox.y + ibox.height;
              Svg::ElementP auxe;
              auxe = svgf->lookup (element_id + ".hscale");
              if (auxe)
                {
                  const Svg::BBox bbox = auxe->bbox();
                  SVGDEBUG ("    aux: %s%s: %s", resource, auxe->info().id, auxe->bbox().to_string());
                  const double bx1 = CLAMP (bbox.x, ix1, ix2), bx2 = CLAMP (bbox.x + bbox.width, ix1, ix2);
                  if (bx1 < bx2)
                    {
                      hscale_spans[0].resizable = 0, hscale_spans[0].length = bx1 - ix1;
                      hscale_spans[1].resizable = 1, hscale_spans[1].length = bx2 - bx1;
                      hscale_spans[2].resizable = 0, hscale_spans[2].length = ix2 - bx2;
                    }
                }
              auxe = svgf->lookup (element_id + ".vscale");
              if (auxe)
                {
                  const Svg::BBox bbox = auxe->bbox();
                  SVGDEBUG ("    aux: %s%s: %s", resource, auxe->info().id, auxe->bbox().to_string());
                  const double by1 = CLAMP (bbox.y, iy1, iy2), by2 = CLAMP (bbox.y + bbox.height, iy1, iy2);
                  if (by1 < by2)
                    {
                      vscale_spans[0].resizable = 0, vscale_spans[0].length = by1 - iy1;
                      vscale_spans[1].resizable = 1, vscale_spans[1].length = by2 - by1;
                      vscale_spans[2].resizable = 0, vscale_spans[2].length = iy2 - by2;
                    }
                }
              auxe = svgf->lookup (element_id + ".hfill");
              if (auxe)
                {
                  const Svg::BBox bbox = auxe->bbox();
                  SVGDEBUG ("    aux: %s%s: %s", resource, auxe->info().id, auxe->bbox().to_string());
                  const double bx1 = CLAMP (bbox.x, ix1, ix2), bx2 = CLAMP (bbox.x + bbox.width, ix1, ix2);
                  if (bx1 < bx2)
                    {
                      fill.x = bx1 - ix1;
                      fill.width = bx2 - bx1;
                    }
                }
              auxe = svgf->lookup (element_id + ".vfill");
              if (auxe)
                {
                  const Svg::BBox bbox = auxe->bbox();
                  SVGDEBUG ("    aux: %s%s: %s", resource, auxe->info().id, auxe->bbox().to_string());
                  const double by1 = CLAMP (bbox.y, iy1, iy2), by2 = CLAMP (bbox.y + bbox.height, iy1, iy2);
                  if (by1 < by2)
                    {
                      fill.y = by1 - iy1;
                      fill.height = by2 - by1;
                    }
                }
            }
          image_backend = std::make_shared<SvgBackend> (svgf, svge, hscale_spans, vscale_spans, fill);
        }
    }
  else
    {
      auto pixmap = Pixmap (blob);
      if (pixmap.width() && pixmap.height())
        image_backend = std::make_shared<PixBackend> (pixmap);
    }
  return image_backend;
}

ImageRendererImpl::ImageBackendP
ImageRendererImpl::load_pixmap (Pixmap pixmap)
{
  ImageBackendP image_backend;
  if (pixmap.width() && pixmap.height())
    image_backend = std::make_shared<PixBackend> (pixmap);
  return image_backend;
}

Requisition
ImageRendererImpl::get_image_size (const ImageBackendP &image_backend)
{
  return image_backend ? image_backend->image_size() : Requisition();
}

Allocation
ImageRendererImpl::get_fill_area (const ImageBackendP &image_backend)
{
  return image_backend ? image_backend->fill_area() : Allocation();
}

void
ImageRendererImpl::paint_image (const ImageBackendP &image_backend, RenderContext &rcontext, const Rect &rect)
{
  return_unless (image_backend != NULL);
  // figure image size
  const Rect &area = allocation();
  const Requisition ims = image_backend->image_size();
  Rect view; // image position relative to allocation
  switch (2)
    {
    case 1: // center image
      view.width = MIN (ims.width, area.width);
      view.height = MIN (ims.height, area.height);
      view.x = area.x + iround (0.5 * (area.width - view.width));
      view.y = area.y + iround (0.5 * (area.height - view.height));
      break;
    case 2: // stretch image
      view.x = area.x;
      view.y = area.y;
      view.width = area.width;
      view.height = area.height;
      break;
    }
  // render image into cairo_context
  const auto mkcontext = [&] (const Rect &crect) { return cairo_context (rcontext, crect); };
  image_backend->render_image (mkcontext, rect, view);
}

} // Rapicorn
