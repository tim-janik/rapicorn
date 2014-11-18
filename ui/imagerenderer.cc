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
public:
  SvgBackend (Svg::FileP svgf, Svg::ElementP svge) :
    svgf_ (svgf), svge_ (svge)
  {}
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

bool
ImageRendererImpl::load_source (const String &resource, const String &element_id)
{
  image_backend_ = NULL;
  Blob blob = Blob::load (resource);
  if (string_endswith (blob.name(), ".svg"))
    {
      auto svgf = Svg::File::load (blob);
      SVGDEBUG ("loading: %s: %s", resource, strerror (errno));
      auto svge = svgf ? svgf->lookup (element_id) : Svg::Element::none();
      SVGDEBUG ("lookup: %s%s: %s", resource, element_id, svge ? "OK" : "failed");
      if (svge)
        image_backend_ = std::make_shared<SvgBackend> (svgf, svge);
    }
  else
    {
      auto pixmap = Pixmap (blob);
      if (pixmap.width() && pixmap.height())
        image_backend_ = std::make_shared<PixBackend> (pixmap);
    }
  return image_backend_ != NULL;
}

void
ImageRendererImpl::load_pixmap (Pixmap pixmap)
{
  if (pixmap.width() && pixmap.height())
    image_backend_ = std::make_shared<PixBackend> (pixmap);
  else
    image_backend_ = NULL;
}

void
ImageRendererImpl::get_image_size (Requisition &image_size)
{
  image_size = image_backend_ ? image_backend_->image_size() : Requisition();
}

void
ImageRendererImpl::get_fill_area (Allocation &fill_area)
{
  Requisition image_size;
  get_image_size (image_size);
  fill_area.x = 0;
  fill_area.y = 0;
  fill_area.width = image_size.width;
  fill_area.height = image_size.height;
}

void
ImageRendererImpl::paint_image (RenderContext &rcontext, const Rect &rect)
{
  return_unless (image_backend_ != NULL);
  // figure image size
  const Rect &area = allocation();
  const Requisition ims = image_backend_->image_size();
  Rect view; // image position relative to allocation
  view.width = MIN (ims.width, area.width);
  view.height = MIN (ims.height, area.height);
  // position image
  view.x = area.x + iround (0.5 * (area.width - view.width));
  view.y = area.y + iround (0.5 * (area.height - view.height));
  // render image into cairo_context
  const auto mkcontext = [&] (const Rect &crect) { return cairo_context (rcontext, crect); };
  image_backend_->render_image (mkcontext, rect, view);
}

} // Rapicorn
