// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#include "image.hh"
#include "stock.hh"
#include "factory.hh"

namespace Rapicorn {

// == ImageImpl ==
void
ImageImpl::pixbuf (const Pixbuf &pixbuf)
{
  image_backend_ = load_pixmap (Pixmap (pixbuf));
  invalidate();
}

Pixbuf
ImageImpl::pixbuf() const
{
  return Pixbuf(); // FIXME
}

void
ImageImpl::broken_image()
{
  Stock::Icon icon = Stock ("broken-image").icon();
  image_backend_ = load_source (icon.resource, icon.element);
  if (!image_backend_)
    critical ("missing stock: broken-image");
  invalidate();
}

void
ImageImpl::source (const String &image_url)
{
  source_ = image_url;
  image_backend_ = load_source (source_, element_);
  if (!image_backend_)
    broken_image();
  invalidate();
}

String
ImageImpl::source() const
{
  return source_;
}

void
ImageImpl::element (const String &element_id)
{
  // FIXME: setting source() + element() loads+parses SVG files twice
  element_ = element_id;
  image_backend_ = load_source (source_, element_);
  if (!image_backend_)
    broken_image();
  invalidate();
}

String
ImageImpl::element() const
{
  return element_;
}

void
ImageImpl::stock (const String &stock_id)
{
  return_unless (stock_id_ != stock_id);
  stock_id_ = stock_id;
  Stock::Icon stock_icon = Stock (stock_id_).icon();
  image_backend_ = load_source (stock_icon.resource, stock_icon.element);
  if (!image_backend_)
    broken_image();
  invalidate();
}

String
ImageImpl::stock() const
{
  return stock_id_;
}

void
ImageImpl::size_request (Requisition &requisition)
{
  const Requisition irq = get_image_size (image_backend_);
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
  paint_image (image_backend_, rcontext, rect);
}

static const WidgetFactory<ImageImpl> image_factory ("Rapicorn_Factory:Image");

// == StatePainterImpl ==
void
StatePainterImpl::source (const String &resource)
{
  return_unless (source_ != resource);
  source_ = resource;
  element_image_ = NULL;
  state_image_ = NULL;
  state_element_ = "";
  invalidate();
  changed ("source");
}

void
StatePainterImpl::element (const String &e)
{
  return_unless (element_ != e);
  element_ = e;
  element_image_ = NULL;
  state_image_ = NULL;
  state_element_ = "";
  invalidate();
  changed ("element");
}

String
StatePainterImpl::current_element ()
{
  StateType s = ancestry_impressed() ? STATE_IMPRESSED : state(); // FIXME: priority for insensitive?
  switch (s)
    {
    case STATE_NORMAL:          return normal_element_.empty()      ? element_ : normal_element_;
    case STATE_INSENSITIVE:     return insensitive_element_.empty() ? element_ : insensitive_element_;
    case STATE_PRELIGHT:        return prelight_element_.empty()    ? element_ : prelight_element_;
    case STATE_IMPRESSED:       return impressed_element_.empty()   ? element_ : impressed_element_;
    case STATE_FOCUS:           return focus_element_.empty()       ? element_ : focus_element_;
    case STATE_DEFAULT:         return default_element_.empty()     ? element_ : default_element_;
    default:                    return element_;
    }
}

void
StatePainterImpl::update_element (String &member, const String &value, const char *name)
{
  return_unless (member != value);
  const String previous = current_element();
  member = value;
  if (previous != current_element())
    invalidate (INVALID_CONTENT);
  changed (name);
}

void
StatePainterImpl::size_request (Requisition &requisition)
{
  if (!element_image_)
    element_image_ = load_source (source_, element_);
  requisition = get_image_size (element_image_);
}

void
StatePainterImpl::size_allocate (Allocation area, bool changed)
{} // default: allocation_ = area;

void
StatePainterImpl::do_changed (const String &name)
{
  ImageRendererImpl::do_changed (name);
  if (name == "state" && state_element_ != current_element())
    invalidate (INVALID_CONTENT);
}

void
StatePainterImpl::render (RenderContext &rcontext, const Rect &rect)
{
  const String current = current_element();
  if (!state_image_ || state_element_ != current)
    {
      state_image_ = load_source (source_, current);
      state_element_ = state_image_ ? current : "";
    }
  paint_image (state_image_, rcontext, rect);
}

static const WidgetFactory<StatePainterImpl> state_painter_factory ("Rapicorn_Factory:StatePainter");

} // Rapicorn
