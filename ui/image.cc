// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "image.hh"
#include "stock.hh"
#include "factory.hh"

namespace Rapicorn {

// == ImageImpl ==
void
ImageImpl::pixbuf (const Pixbuf &pixbuf)
{
  image_painter_ = ImagePainter (Pixmap (pixbuf));
  invalidate_size();
  invalidate_content();
}

Pixbuf
ImageImpl::pixbuf() const
{
  return Pixbuf(); // FIXME
}

void
ImageImpl::broken_image()
{
  String icon = Stock ("broken-image").icon();
  image_painter_ = ImagePainter (icon);
  if (!image_painter_)
    critical ("missing stock: broken-image");
  invalidate_size();
  invalidate_content();
}

void
ImageImpl::source (const String &image_url)
{
  source_ = image_url;
  image_painter_ = ImagePainter (source_);
  if (!image_painter_)
    broken_image();
  invalidate_size();
  invalidate_content();
}

String
ImageImpl::source() const
{
  return source_;
}

void
ImageImpl::stock (const String &stock_id)
{
  return_unless (stock_id_ != stock_id);
  stock_id_ = stock_id;
  String stock_icon = Stock (stock_id_).icon();
  image_painter_ = ImagePainter (stock_icon);
  if (!image_painter_)
    broken_image();
  invalidate_size();
  invalidate_content();
}

String
ImageImpl::stock() const
{
  return stock_id_;
}

void
ImageImpl::size_request (Requisition &requisition)
{
  const Requisition irq = image_painter_.image_size();
  requisition.width += irq.width;
  requisition.height += irq.height;
}

void
ImageImpl::size_allocate (Allocation area, bool changed)
{
  // nothing special...
}

void
ImageImpl::render (RenderContext &rcontext, const IRect &rect)
{
  image_painter_.draw_image (cairo_context (rcontext, rect), rect, allocation());
}

static const WidgetFactory<ImageImpl> image_factory ("Rapicorn::Image");


} // Rapicorn
