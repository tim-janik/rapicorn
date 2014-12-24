// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#include "image.hh"
#include "stock.hh"
#include "factory.hh"

namespace Rapicorn {

// == ImageImpl ==
void
ImageImpl::pixbuf (const Pixbuf &pixbuf)
{
  image_painter_ = ImagePainter (Pixmap (pixbuf));
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
  image_painter_ = ImagePainter (icon);
  if (!image_painter_)
    critical ("missing stock: broken-image");
  invalidate();
}

void
ImageImpl::source (const String &image_url)
{
  source_ = image_url;
  image_painter_ = ImagePainter (source_);
  if (!image_painter_)
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
  image_painter_ = ImagePainter (stock_icon);
  if (!image_painter_)
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
ImageImpl::render (RenderContext &rcontext, const Rect &rect)
{
  image_painter_.draw_image (cairo_context (rcontext, rect), rect, allocation());
}

static const WidgetFactory<ImageImpl> image_factory ("Rapicorn_Factory:Image");

// == StatePainterImpl ==
void
StatePainterImpl::source (const String &resource)
{
  return_unless (source_ != resource);
  source_ = resource;
  source_painter_ = ImagePainter();
  state_painter_ = ImagePainter();
  state_image_ = "";
  invalidate();
  changed ("source");
}

String
StatePainterImpl::current_source ()
{
  StateType s = ancestry_impressed() ? STATE_IMPRESSED : state(); // FIXME: priority for insensitive?
  const String current = [&]() {
    switch (s)
      {
      case STATE_NORMAL:          return normal_image_.empty()      ? source_ : normal_image_;
      case STATE_INSENSITIVE:     return insensitive_image_.empty() ? source_ : insensitive_image_;
      case STATE_PRELIGHT:        return prelight_image_.empty()    ? source_ : prelight_image_;
      case STATE_IMPRESSED:       return impressed_image_.empty()   ? source_ : impressed_image_;
      case STATE_FOCUS:           return focus_image_.empty()       ? source_ : focus_image_;
      case STATE_DEFAULT:         return default_image_.empty()     ? source_ : default_image_;
      default:                    return source_;
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
  if (!source_painter_)
    source_painter_ = ImagePainter (source_);
  requisition = source_painter_.image_size();
}

void
StatePainterImpl::size_allocate (Allocation area, bool changed)
{} // default: allocation_ = area;

void
StatePainterImpl::do_changed (const String &name)
{
  WidgetImpl::do_changed (name);
  if (name == "state" && state_image_ != current_source())
    invalidate (INVALID_CONTENT);
}

void
StatePainterImpl::render (RenderContext &rcontext, const Rect &rect)
{
  const String current = current_source();
  if (!state_painter_ || state_image_ != current)
    {
      state_painter_ = ImagePainter (current);
      state_image_ = state_painter_ ? current : "";
    }
  state_painter_.draw_image (cairo_context (rcontext, rect), rect, allocation());
}

static const WidgetFactory<StatePainterImpl> state_painter_factory ("Rapicorn_Factory:StatePainter");

} // Rapicorn
