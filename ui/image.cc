// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "image.hh"
#include "stock.hh"
#include "painter.hh"
#include "factory.hh"
#include "../rcore/rsvg/svg.hh"
#include <errno.h>

/// @TODO: unify CHECK_CAIRO_STATUS() macro implementations
#define CHECK_CAIRO_STATUS(status)      do {    \
  cairo_status_t ___s = (status);               \
  if (___s != CAIRO_STATUS_SUCCESS)             \
    RAPICORN_CRITICAL ("%s: %s", cairo_status_to_string (___s), #status);   \
  } while (0)

namespace Rapicorn {

static const uint8* get_broken16_pixdata (void);

struct ImageImpl::ImageBackend {
  virtual            ~ImageBackend    () {}
  virtual Requisition image_size      () = 0;
  virtual void        render_image    (const std::function<cairo_t* (const Rect&)> &mkcontext,
                                       const Rect &render_rect, const Rect &image_rect) = 0;
};

struct SvgImage : public virtual ImageImpl::ImageBackend {
  Svg::FileP    svgf_;
  Svg::ElementP svge_;
public:
  SvgImage (Svg::FileP svgf, Svg::ElementP svge) :
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
    uint8 *pixels = new uint8[int (npixels * 4)]; // FIXME: cast?
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

struct PixImage : public virtual ImageImpl::ImageBackend {
  Pixmap        pixmap_;
public:
  PixImage (const Pixmap &pixmap) :
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

void
ImageImpl::pixbuf (const Pixbuf &pixbuf)
{
  image_backend_ = std::make_shared<PixImage> (Pixmap (pixbuf));
  invalidate();
}

Pixbuf
ImageImpl::pixbuf() const
{
  return Pixbuf(); // FIXME
}

void
ImageImpl::load_pixmap()
{
  image_backend_ = NULL;
  Blob blob = Blob::load ("");
  if (!image_url_.empty())
    blob = Blob::load (image_url_);
  if (!blob && !stock_id_.empty())
    blob = Stock::stock_image (stock_id_);
  if (string_endswith (blob.name(), ".svg"))
    {
      auto svgf = Svg::File::load (blob);
      auto svge = svgf ? svgf->lookup ("") : Svg::Element::none();
      if (svge)
        image_backend_ = std::make_shared<SvgImage> (svgf, svge);
    }
  else
    {
      auto pixmap = Pixmap (blob);
      if (pixmap.width() && pixmap.height())
        image_backend_ = std::make_shared<PixImage> (pixmap);
    }
  if (!image_backend_)
    {
      auto pixmap = Pixmap();
      pixmap.load_pixstream (get_broken16_pixdata());
      assert_return (pixmap.width() > 0 && pixmap.height() > 0);
      image_backend_ = std::make_shared<PixImage> (pixmap);
    }
  invalidate();
}

void
ImageImpl::source (const String &image_url)
{
  image_url_ = image_url;
  load_pixmap();
}

String
ImageImpl::source() const
{
  return image_url_;
}

void
ImageImpl::stock (const String &stock_id)
{
  stock_id_ = stock_id;
  load_pixmap();
}

String
ImageImpl::stock() const
{
  return stock_id_;
}

void
ImageImpl::size_request (Requisition &requisition)
{
  return_unless (image_backend_ != NULL);
  const Requisition irq = image_backend_->image_size();
  requisition.width += irq.width;
  requisition.height += irq.height;
}

void
ImageImpl::size_allocate (Allocation area, bool changed)
{
  // nothing special...
}

void
ImageImpl::render (RenderContext &rcontext, const Rect &rect)
{
  return_unless (image_backend_ != NULL);
  // figure image size
  const Rect &area = allocation();
  const Requisition irq = image_backend_->image_size();
  Rect view;
  view.width = MIN (irq.width, area.width);
  view.height = MIN (irq.height, area.height);
  // position image
  const PackInfo &pi = pack_info();
  view.x = area.x + iround (pi.halign * (area.width - view.width));
  view.y = area.y + iround (pi.valign * (area.height - view.height));
  // render image into cairo_context
  const auto mkcontext = [&] (const Rect &crect) { return cairo_context (rcontext, crect); };
  image_backend_->render_image (mkcontext, rect, view);
}

static const WidgetFactory<ImageImpl> image_factory ("Rapicorn::Factory::Image");

static const uint8*
get_broken16_pixdata (void)
{
  /* GdkPixbuf RGBA C-Source image dump 1-byte-run-length-encoded */
#ifdef __SUNPRO_C
#pragma align 4 (broken16_pixdata)
#endif
#ifdef __GNUC__
  static const uint8 broken16_pixdata[] __attribute__ ((__aligned__ (4))) = 
#else
    static const uint8 broken16_pixdata[] = 
#endif
    { ""
      /* Pixbuf magic (0x47646b50) */
      "GdkP"
      /* length: header (24) + pixel_data (485) */
      "\0\0\1\375"
      /* pixdata_type (0x2010002) */
      "\2\1\0\2"
      /* rowstride (64) */
      "\0\0\0@"
      /* width (16) */
      "\0\0\0\20"
      /* height (16) */
      "\0\0\0\20"
      /* pixel_data: */
      "\221\0\0\0\0\203\0\0\0\377\2>\0\10\377\0\0\0\377\202\0\0\0\0\1\253\0"
      "\30\\\203\0\0\0\377\2\0\0\0\177\0\0\0\77\203\0\0\0\0\1\0\0\0\377\202"
      "\377\377\377\377\2\377r\205\377\377Jc\262\202\0\0\0\0\7\377D_\301\377"
      "\201\222\377\377\377\377\377\0\0\0\377\300\300\300\377\0\0\0\177\0\0"
      "\0\77\202\0\0\0\0\1\0\0\0\377\202\377\377\377\377\2\377\"A\377\377Kd"
      "D\202\0\0\0\0\7\377\32;\367\377\317\325\377\377\377\377\377\0\0\0\377"
      "\377\377\377\377\300\300\300\377\0\0\0\177\202\0\0\0\0\4\0\0\0\377\377"
      "\377\377\377\377\324\332\377\377\25""6\371\202\0\0\0\0\2\377Kd=\377\33"
      ";\377\202\377\377\377\377\204\0\0\0\377\202\0\0\0\0\4\0\0\0\377\377\377"
      "\377\377\377\206\227\377\377A\\\307\202\0\0\0\0\2\377.K\224\377k\177"
      "\377\205\377\377\377\377\1\0\0\0\377\202\0\0\0\0\11\0\0\0\377\377\377"
      "\377\377\377\257\272\377\377\5(\363\377\0$8\0\0\0\0\377\0$P\377\5(\363"
      "\377\305\315\370\204\377\377\377\377\1\0\0\0\377\202\0\0\0\0\1\0\0\0"
      "\377\202\377\377\377\377\7\377\273\305\366\377\4'\366\377\0$R\0\0\0\0"
      "\377\0$A\377\2%\364\377\247\264\360\203\377\377\377\377\1\0\0\0\377\202"
      "\0\0\0\0\1\0\0\0\377\203\377\377\377\377\7\377\325\333\376\377\12,\365"
      "\377\0$w\0\0\0\0\377\0$)\377\2%\355\377|\216\350\202\377\377\377\377"
      "\1\0\0\0\377\202\0\0\0\0\1\0\0\0\377\204\377\377\377\377\11\377\366\367"
      "\377\377(F\352\377\1%\251\377\0$\10\377\0$\11\377\3'\310\377F`\350\377"
      "\367\370\377\0\0\0\377\202\0\0\0\0\1\0\0\0\377\204\377\377\377\377\11"
      "\377\365\366\377\3777S\361\377\4'\256\377\0$\6\377\0$\12\377\5(\301\377"
      "F`\354\377\371\371\377\0\0\0\377\202\0\0\0\0\1\0\0\0\377\203\377\377"
      "\377\377\12\377\356\360\377\377+H\355\377\1%\252\377\0$\2\377\0$\21\377"
      "\6)\312\377Jd\357\377\375\375\377\377\377\377\377\0\0\0\377\202\0\0\0"
      "\0\1\0\0\0\377\202\377\377\377\377\10\377\356\360\377\377\35=\360\377"
      "\1%\216\377\0$\1\377\0$\21\377\3'\327\377h}\357\377\376\376\377\202\377"
      "\377\377\377\1\0\0\0\377\202\0\0\0\0\11\0\0\0\377\377\377\377\377\377"
      "\340\344\377\377\15/\365\377\2%z\0\0\0\0\377\0$\37\377\3&\353\377\200"
      "\222\364\204\377\377\377\377\1\0\0\0\377\202\0\0\0\0\7\0\0\0\377&\0\5"
      "\377\0\0\0\377\326\0\36p\0\0\0\0\377\0$&\370\0#\351\207\0\0\0\377\221"
      "\0\0\0\0"
    };
  return broken16_pixdata;
}

} // Rapicorn
