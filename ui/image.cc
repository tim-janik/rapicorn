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

static const WidgetFactory<ImageImpl> image_factory ("Rapicorn::Image");

// == StatePainterImpl ==
StatePainterImpl::StatePainterImpl() :
  cached_painter_ ("")
{}

void
StatePainterImpl::svg_source (const String &resource)
{
  return_unless (svg_source_ != resource);
  svg_source_ = resource;
  if (size_painter_)
    size_painter_ = ImagePainter();
  if (state_painter_)
    state_painter_ = ImagePainter();
  cached_painter_ = "";
  invalidate();
  changed ("svg_source");
}

void
StatePainterImpl::svg_element (const String &fragment)
{
  return_unless (svg_fragment_ != fragment);
  svg_fragment_ = fragment;
  if (size_painter_)
    size_painter_ = ImagePainter();
  if (state_painter_)
    state_painter_ = ImagePainter();
  cached_painter_ = "";
  invalidate();
  changed ("svg_source");
}

static constexpr uint64
consthash_fnv64a (const char *string, uint64 hash = 0xcbf29ce484222325)
{
  return string[0] == 0 ? hash : consthash_fnv64a (string + 1, 0x100000001b3 * (hash ^ string[0]));
}

static const uint64 BROKEN = 0x80000000;

static uint64
single_state_score (const String &state_string)
{
  switch (consthash_fnv64a (state_string.c_str()))
    {
    case consthash_fnv64a ("normal"):           return STATE_NORMAL;
    case consthash_fnv64a ("hover"):            return STATE_HOVER;
    case consthash_fnv64a ("panel"):            return STATE_PANEL;
    case consthash_fnv64a ("acceleratable"):    return STATE_ACCELERATABLE;
    case consthash_fnv64a ("default"):          return STATE_DEFAULT;
    case consthash_fnv64a ("selected"):         return STATE_SELECTED;
    case consthash_fnv64a ("focused"):          return STATE_FOCUSED;
    case consthash_fnv64a ("insensitive"):      return STATE_INSENSITIVE;
    case consthash_fnv64a ("active"):           return STATE_ACTIVE;
    case consthash_fnv64a ("retained"):         return STATE_RETAINED;
    default:                                    return BROKEN;
    }
}

static uint64
state_score (const String &state_string)
{
  StringVector sv = string_split (state_string, "+");
  uint64 r = 0;
  for (const String &s : sv)
    r |= single_state_score (s);
  return r >= BROKEN ? 0 : r;
}

String
StatePainterImpl::state_element (StateType state)
{
  if (!size_painter_)
    size_painter_ = ImagePainter (svg_source_);
  return_unless (size_painter_ && svg_fragment_.size() && svg_fragment_[0] == '#', "");
  // match an SVG element to state, ID syntax: <element id="elementname:active+insensitive"/>
  const String element = svg_fragment_.substr (1); // fragment without initial hash symbol
  const size_t colon = element.size();
  String fallback, match;
  size_t score = 0;
  for (auto id : size_painter_.list (element))
    if (id == element)                                  // element without state specification
      fallback = id;
    else if (id.size() > colon + 1 && id[colon] == ':') // element with state
      {
        const size_t s = state_score (id.substr (colon + 1));
        if ((s & state) == s && s > score)
          {
            match = id;
            score = s;
          }
      }
  match = match.empty() ? fallback : match;
  return svg_source_ + "#" + match;
}

String
StatePainterImpl::current_element ()
{
  const StateType mystate = ancestry_active() ? STATE_ACTIVE : state();
  return state_element (mystate);
}

void
StatePainterImpl::size_request (Requisition &requisition)
{
  if (!size_painter_)
    size_painter_ = ImagePainter (state_element (STATE_NORMAL));
  requisition = size_painter_.image_size();
}

void
StatePainterImpl::size_allocate (Allocation area, bool changed)
{} // default: allocation_ = area;

void
StatePainterImpl::do_changed (const String &name)
{
  WidgetImpl::do_changed (name);
  if (name == "state" && (cached_painter_ != current_element()))
    invalidate (INVALID_CONTENT);
}

void
StatePainterImpl::render (RenderContext &rcontext, const Rect &rect)
{
  const String painter_src = current_element();
  if (!state_painter_ || cached_painter_ != painter_src)
    {
      state_painter_ = ImagePainter (painter_src);
      cached_painter_ = painter_src;
    }
  state_painter_.draw_image (cairo_context (rcontext, rect), rect, allocation());
}

static const WidgetFactory<StatePainterImpl> state_painter_factory ("Rapicorn::StatePainter");

} // Rapicorn
