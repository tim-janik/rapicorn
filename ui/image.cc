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
  String icon = Stock ("broken-image").icon();
  image_backend_ = load_source (icon);
  if (!image_backend_)
    critical ("missing stock: broken-image");
  invalidate();
}

void
ImageImpl::source (const String &image_url)
{
  source_ = image_url;
  image_backend_ = load_source (source_);
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
ImageImpl::stock (const String &stock_id)
{
  return_unless (stock_id_ != stock_id);
  stock_id_ = stock_id;
  String stock_icon = Stock (stock_id_).icon();
  image_backend_ = load_source (stock_icon);
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
StatePainterImpl::current_source ()
{
  StateType s = ancestry_impressed() ? STATE_IMPRESSED : state(); // FIXME: priority for insensitive?
  const String current = [&]() {
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
  } ();
  if (string_startswith (current, "#"))
    {
      const ssize_t hashpos = source_.find ('#');
      const String resource = hashpos < 0 ? source_ : source_.substr (0, hashpos);
      return resource + current;
    }
  else
    return current;
}

void
StatePainterImpl::update_source (String &member, const String &value, const char *name)
{
  return_unless (member != value);
  const String previous = current_source();
  member = value;
  if (previous != current_source())
    invalidate (INVALID_CONTENT);
  changed (name);
}

void
StatePainterImpl::size_request (Requisition &requisition)
{
  if (!element_image_)
    element_image_ = load_source (source_);
  requisition = get_image_size (element_image_);
}

void
StatePainterImpl::size_allocate (Allocation area, bool changed)
{} // default: allocation_ = area;

void
StatePainterImpl::do_changed (const String &name)
{
  ImageRendererImpl::do_changed (name);
  if (name == "state" && state_element_ != current_source())
    invalidate (INVALID_CONTENT);
}

void
StatePainterImpl::render (RenderContext &rcontext, const Rect &rect)
{
  const String current = current_source();
  if (!state_image_ || state_element_ != current)
    {
      state_image_ = load_source (current);
      state_element_ = state_image_ ? current : "";
    }
  paint_image (state_image_, rcontext, rect);
}

static const WidgetFactory<StatePainterImpl> state_painter_factory ("Rapicorn_Factory:StatePainter");

} // Rapicorn
