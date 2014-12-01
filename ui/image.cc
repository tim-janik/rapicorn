// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#include "image.hh"
#include "stock.hh"
#include "factory.hh"

namespace Rapicorn {

// == ImageImpl ==
static const uint8* get_broken16_pixdata (void);

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
ImageImpl::source (const String &image_url)
{
  source_ = image_url;
  image_backend_ = load_source (source_, element_);
  if (!image_backend_)
    {
      auto pixmap = Pixmap();
      pixmap.load_pixstream (get_broken16_pixdata());
      assert_return (pixmap.width() > 0 && pixmap.height() > 0);
      image_backend_ = load_pixmap (pixmap);
    }
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
    {
      auto pixmap = Pixmap();
      pixmap.load_pixstream (get_broken16_pixdata());
      assert_return (pixmap.width() > 0 && pixmap.height() > 0);
      image_backend_ = load_pixmap (pixmap);
    }
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
    {
      auto pixmap = Pixmap();
      pixmap.load_pixstream (get_broken16_pixdata());
      assert_return (pixmap.width() > 0 && pixmap.height() > 0);
      image_backend_ = load_pixmap (pixmap);
    }
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
