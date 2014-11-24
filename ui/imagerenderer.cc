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
    cairo_matrix_init_identity (&matrix);
    const double xscale = pixmap_.width() / image_rect.width, yscale = pixmap_.height() / image_rect.height;
    cairo_matrix_scale (&matrix, xscale, yscale);
    cairo_matrix_translate (&matrix, -image_rect.x, -image_rect.y); // adjust image origin
    cairo_pattern_set_matrix (cairo_get_source (cr), &matrix);
    if (xscale != 1.0 || yscale != 1.0)
      cairo_pattern_set_filter (cairo_get_source (cr), CAIRO_FILTER_BILINEAR);
    cairo_paint (cr);
    cairo_surface_destroy (isurface);
  }
};

struct SvgBackend : public virtual ImageBackend {
  Svg::FileP    svgf_;
  Svg::ElementP svge_;
  const Svg::Span hscale_spans_[3], vscale_spans_[3];
public:
  SvgBackend (Svg::FileP svgf, Svg::ElementP svge, const Svg::Span (&hscale_spans)[3], const Svg::Span (&vscale_spans)[3]) :
    svgf_ (svgf), svge_ (svge), hscale_spans_ (hscale_spans), vscale_spans_ (vscale_spans)
  {
    SVGDEBUG ("SvgImage: id=%s hscale_spans={ l=%u r=%u, l=%u r=%u, l=%u r=%u } vscale_spans={ l=%u r=%u, l=%u r=%u, l=%u r=%u }",
              svge_->info().id,
              hscale_spans_[0].length, hscale_spans_[0].resizable,
              hscale_spans_[1].length, hscale_spans_[1].resizable,
              hscale_spans_[2].length, hscale_spans_[2].resizable,
              vscale_spans_[0].length, vscale_spans_[0].resizable,
              vscale_spans_[1].length, vscale_spans_[1].resizable,
              vscale_spans_[2].length, vscale_spans_[2].resizable);
    size_t hsum = 0;
    for (size_t i = 0; i < ARRAY_SIZE (hscale_spans_); i++)
      hsum += hscale_spans_[i].length;
    critical_unless (hsum == svge_->bbox().width);
    size_t vsum = 0;
    for (size_t i = 0; i < ARRAY_SIZE (vscale_spans_); i++)
      vsum += vscale_spans_[i].length;
    critical_unless (vsum == svge_->bbox().height);
  }
  virtual Requisition
  image_size ()
  {
    const auto bbox = svge_->bbox();
    return Requisition (bbox.width, bbox.height);
  }
  virtual void
  render_image (const std::function<cairo_t* (const Rect&)> &mkcontext, const Rect &render_rect, const Rect &image_rect)
  {
    Rect rect = image_rect;
    rect.intersect (render_rect);
    return_unless (rect.width > 0 && rect.height > 0);
    const uint npixels = rect.width * rect.height;
    uint8 *pixels = new uint8[npixels * 4];
    std::fill (pixels, pixels + npixels * 4, 0);
    cairo_surface_t *surface = cairo_image_surface_create_for_data (pixels, CAIRO_FORMAT_ARGB32,
                                                                    rect.width, rect.height, 4 * rect.width);
    CHECK_CAIRO_STATUS (cairo_surface_status (surface));
    cairo_surface_set_device_offset (surface, -(rect.x - image_rect.x), -(rect.y - image_rect.y)); // offset into image
    const auto bbox = svge_->bbox();
    const bool rendered = svge_->render (surface, Svg::RenderSize::ZOOM, image_rect.width / bbox.width, image_rect.height / bbox.height);
    if (rendered)
      { // FIXME: optimize by creating surface from rcontext directly?
        cairo_t *cr = mkcontext (rect);
        cairo_set_source_surface (cr, surface, rect.x, rect.y); // shift into allocation area
        cairo_paint (cr);
      }
    else
      critical ("failed to render SVG file");
    cairo_surface_destroy (surface);
    delete[] pixels;
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
  Blob blob = Blob::load (resource);
  if (string_endswith (blob.name(), ".svg"))
    {
      auto svgf = Svg::File::load (blob);
      SVGDEBUG ("loading: %s: %s", resource, strerror (errno));
      auto svge = svgf ? svgf->lookup (element_id) : Svg::Element::none();
      SVGDEBUG (" lookup: %s%s: %s", resource, element_id, svge ? svge->bbox().to_string() : "failed");
      if (svge)
        {
          const Svg::BBox ibox = svge->bbox();
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
                SVGDEBUG ("    aux: %s%s: %s", resource, auxe->info().id, auxe->bbox().to_string());
              auxe = svgf->lookup (element_id + ".vfill");
              if (auxe)
                SVGDEBUG ("    aux: %s%s: %s", resource, auxe->info().id, auxe->bbox().to_string());
            }
          image_backend = std::make_shared<SvgBackend> (svgf, svge, hscale_spans, vscale_spans);
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

void
ImageRendererImpl::get_image_size (const ImageBackendP &image_backend, Requisition &image_size)
{
  image_size = image_backend ? image_backend->image_size() : Requisition();
}

void
ImageRendererImpl::get_fill_area (const ImageBackendP &image_backend, Allocation &fill_area)
{
  Requisition image_size;
  get_image_size (image_backend, image_size);
  fill_area.x = 0;
  fill_area.y = 0;
  fill_area.width = image_size.width;
  fill_area.height = image_size.height;
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
