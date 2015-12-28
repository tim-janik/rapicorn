// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "paintcontainers.hh"
#include "container.hh"
#include "painter.hh"
#include "factory.hh"
#include "../rcore/rsvg/svg.hh" // Svg::Span

namespace Rapicorn {

// == AmbienceImpl ==
AmbienceImpl::AmbienceImpl() :
  normal_background_ ("none"),
  hover_background_ ("none"),
  active_background_ ("none"),
  insensitive_background_ ("none"),
  normal_lighting_ (LIGHTING_UPPER_LEFT),
  hover_lighting_ (LIGHTING_UPPER_LEFT),
  active_lighting_ (LIGHTING_LOWER_RIGHT),
  insensitive_lighting_ (LIGHTING_CENTER),
  normal_shade_ (LIGHTING_UPPER_LEFT),
  hover_shade_ (LIGHTING_UPPER_LEFT),
  active_shade_ (LIGHTING_LOWER_RIGHT),
  insensitive_shade_ (LIGHTING_CENTER)
{}

AmbienceImpl::~AmbienceImpl()
{}

void
AmbienceImpl::background (const String &color)
{
  insensitive_background (color);
  hover_background (color);
  active_background (color);
  normal_background (color);
}

void
AmbienceImpl::lighting (LightingType sh)
{
  insensitive_lighting (sh);
  hover_lighting (sh);
  active_lighting (sh);
  normal_lighting (sh);
}

void
AmbienceImpl::shade (LightingType sh)
{
  insensitive_shade (LIGHTING_NONE);
  hover_shade (LIGHTING_NONE);
  active_shade (LIGHTING_NONE);
  normal_shade (sh);
}

String
AmbienceImpl::background () const
{
  RAPICORN_ASSERT_UNREACHED();
}

LightingType
AmbienceImpl::lighting () const
{
  RAPICORN_ASSERT_UNREACHED();
}

LightingType
AmbienceImpl::shade () const
{
  RAPICORN_ASSERT_UNREACHED();
}

void
AmbienceImpl::insensitive_background (const String &color)
{
  insensitive_background_ = color;
  expose();
  changed ("insensitive_background");
}

String
AmbienceImpl::insensitive_background () const
{
  return insensitive_background_;
}

void
AmbienceImpl::hover_background (const String &color)
{
  hover_background_ = color;
  expose();
  changed ("hover_background");
}

String
AmbienceImpl::hover_background () const
{
  return hover_background_;
}

void
AmbienceImpl::active_background (const String &color)
{
  active_background_ = color;
  expose();
  changed ("active_background");
}

String
AmbienceImpl::active_background () const
{
  return active_background_;
}

void
AmbienceImpl::normal_background (const String &color)
{
  normal_background_ = color;
  expose();
  changed ("normal_background");
}

String
AmbienceImpl::normal_background () const
{
  return normal_background_;
}

void
AmbienceImpl::insensitive_lighting (LightingType sh)
{
  insensitive_lighting_ = sh;
  expose();
  changed ("insensitive_lighting");
}

LightingType
AmbienceImpl::insensitive_lighting () const
{
  return insensitive_lighting_;
}

void
AmbienceImpl::hover_lighting (LightingType sh)
{
  hover_lighting_ = sh;
  expose();
  changed ("hover_lighting");
}

LightingType
AmbienceImpl::hover_lighting () const
{
  return hover_lighting_;
}

void
AmbienceImpl::active_lighting (LightingType sh)
{
  active_lighting_ = sh;
  expose();
  changed ("active_lighting");
}

LightingType
AmbienceImpl::active_lighting () const
{
  return active_lighting_;
}

void
AmbienceImpl::normal_lighting (LightingType sh)
{
  normal_lighting_ = sh;
  expose();
  changed ("normal_lighting");
}

LightingType
AmbienceImpl::normal_lighting () const
{
  return normal_lighting_;
}

void
AmbienceImpl::insensitive_shade (LightingType sh)
{
  insensitive_shade_ = sh;
  expose();
  changed ("insensitive_shade");
}

LightingType
AmbienceImpl::insensitive_shade () const
{
  return insensitive_shade_;
}

void
AmbienceImpl::hover_shade (LightingType sh)
{
  hover_shade_ = sh;
  expose();
  changed ("hover_shade");
}

LightingType
AmbienceImpl::hover_shade () const
{
  return hover_shade_;
}

void
AmbienceImpl::active_shade (LightingType sh)
{
  active_shade_ = sh;
  expose();
  changed ("active_shade");
}

LightingType
AmbienceImpl::active_shade () const
{
  return active_shade_;
}

void
AmbienceImpl::normal_shade (LightingType sh)
{
  normal_shade_ = sh;
  expose();
  changed ("normal_shade");
}

LightingType
AmbienceImpl::normal_shade () const
{
  return normal_shade_;
}

void
AmbienceImpl::render_shade (cairo_t *cairo, int x, int y, int width, int height, LightingType st)
{
  int shade_alpha = 0x3b;
  Color light = light_glint().shade (shade_alpha), dark = dark_glint().shade (shade_alpha);
  LightingType dark_flag = st & LIGHTING_DARK_FLAG;
  CPainter painter (cairo);
  if (dark_flag)
    swap (light, dark);
  switch (st & ~LIGHTING_DARK_FLAG)
    {
    case LIGHTING_UPPER_LEFT:
      painter.draw_center_shade_rect (x, y, light, x + width - 1, y + height - 1, dark);
      break;
    case LIGHTING_UPPER_RIGHT:
      painter.draw_center_shade_rect (x + width - 1, y, light, x, y + height - 1, dark);
      break;
    case LIGHTING_LOWER_LEFT:
      painter.draw_center_shade_rect (x, y + height - 1, light, x + width - 1, y, dark);
      break;
    case LIGHTING_LOWER_RIGHT:
      painter.draw_center_shade_rect (x + width - 1, y + height - 1, light, x, y, dark);
      break;
    case LIGHTING_CENTER:
      render_shade (cairo, x, y, width / 2, height / 2, LIGHTING_UPPER_RIGHT | dark_flag);
      render_shade (cairo, x, y + height / 2, width / 2, height / 2, LIGHTING_LOWER_RIGHT | dark_flag);
      render_shade (cairo, x + width / 2, y + height / 2, width / 2, height / 2, LIGHTING_LOWER_LEFT | dark_flag);
      render_shade (cairo, x + width / 2, y, width / 2, height / 2, LIGHTING_UPPER_LEFT | dark_flag);
      break;
    case LIGHTING_DIFFUSE:
      painter.draw_shaded_rect (x, y, light, x + width - 1, y + height - 1, light);
      break;
    case LIGHTING_NONE:
      break;
    }
}

void
AmbienceImpl::render (RenderContext &rcontext, const Rect &rect)
{
  IRect ia = allocation();
  const int x = ia.x, y = ia.y, width = ia.width, height = ia.height;
  const bool aactive = ancestry_active(), ahover = ancestry_hover();
  /* render background */
  String background_color;
  if (aactive)
    background_color = active_background();
  else if (insensitive())
    background_color = insensitive_background();
  else if (ahover)
    background_color = hover_background();
  else
    background_color = normal_background();
  Color background = heritage()->resolve_color (background_color, STATE_NORMAL, COLOR_BACKGROUND);
  cairo_t *cr = cairo_context (rcontext, rect);
  CPainter painter (cr);
  if (background)
    painter.draw_filled_rect (x, y, width, height, background);
  /* render lighting (mutually exclusive) */
  if (aactive && active_lighting())
    render_shade (cr, x, y, width, height, active_lighting());
  else if (insensitive() && insensitive_lighting())
    render_shade (cr, x, y, width, height, insensitive_lighting());
  else if (ahover && hover_lighting())
    render_shade (cr, x, y, width, height, hover_lighting());
  else if (normal_lighting() && !aactive && !insensitive() && !ahover)
    render_shade (cr, x, y, width, height, normal_lighting());
  /* render shade (combinatoric) */
  if (aactive && active_shade())
    render_shade (cr, x, y, width, height, active_shade());
  if (insensitive() && insensitive_shade())
    render_shade (cr, x, y, width, height, insensitive_shade());
  if (ahover && hover_shade())
    render_shade (cr, x, y, width, height, hover_shade());
  if (!aactive && !insensitive() && !ahover && normal_shade())
    render_shade (cr, x, y, width, height, normal_shade());
}

static const WidgetFactory<AmbienceImpl> ambience_factory ("Rapicorn::Ambience");

// == FrameImpl ==
FrameImpl::FrameImpl() :
  normal_frame_ (FRAME_ETCHED_IN),
  active_frame_ (FRAME_ETCHED_IN),
  overlap_child_ (false), tight_focus_ (false)
{}

FrameImpl::~FrameImpl()
{}

// hook for FocusFrameImpl
bool
FrameImpl::tap_tight_focus (int onoffx)
{
  if (onoffx >= 0)
    {
      tight_focus_ = onoffx != 0;
      invalidate();
    }
  return tight_focus_;
}

bool
FrameImpl::is_tight_focus() const
{
  return tight_focus_ && ((normal_frame_ == FRAME_FOCUS || normal_frame_ == FRAME_NONE) &&
                          (active_frame_ == FRAME_FOCUS || active_frame_ == FRAME_NONE));
}

void
FrameImpl::do_changed (const String &name)
{
  SingleContainerImpl::do_changed (name);
  if (overlap_child())
    expose();
  else
    expose_enclosure();
}

FrameType
FrameImpl::normal_frame () const
{
  return normal_frame_;
}

void
FrameImpl::normal_frame (FrameType ft)
{
  if (normal_frame_ != ft)
    {
      normal_frame_ = ft;
      expose_enclosure();
      changed ("normal_frame");
    }
}

FrameType
FrameImpl::active_frame () const
{
  return active_frame_;
}

void
FrameImpl::active_frame (FrameType ft)
{
  if (active_frame_ != ft)
    {
      active_frame_ = ft;
      expose_enclosure();
      changed ("active_frame");
    }
}

FrameType
FrameImpl::frame_type () const
{
  // this is a write-only propery
  RAPICORN_ASSERT_UNREACHED();
}

void
FrameImpl::frame_type (FrameType ft)
{
  normal_frame (ft);
  active_frame (ft);
}

bool
FrameImpl::overlap_child () const
{
  return overlap_child_;
}

void
FrameImpl::overlap_child (bool ovc)
{
  overlap_child_ = ovc;
  invalidate();
  changed ("overlap_child");
}

FrameType
FrameImpl::current_frame ()
{
  return ancestry_active() ? active_frame() : normal_frame();
}

void
FrameImpl::size_request (Requisition &requisition)
{
  SingleContainerImpl::size_request (requisition);
  const int thickness = is_tight_focus() ? 1 : 2;
  if (overlap_child_)
    {
      requisition.width = MAX (requisition.width, 2 * thickness);
      requisition.height = MAX (requisition.height, 2 * thickness);
    }
  else
    {
      requisition.width += 2 * thickness;
      requisition.height += 2 * thickness;
    }
}

void
FrameImpl::size_allocate (Allocation area, bool changed)
{
  if (has_visible_child())
    {
      Allocation carea = area;
      if (!overlap_child_)
        {
          const int thickness = is_tight_focus() ? 1 : 2;
          carea.x += thickness;
          carea.y += thickness;
          carea.width -= 2 * thickness;
          carea.height -= 2 * thickness;
        }
      WidgetImpl &child = get_child();
      Allocation child_area = layout_child (child, carea);
      child.set_allocation (child_area);
    }
}

void
FrameImpl::render (RenderContext &rcontext, const Rect &rect)
{
  IRect ia = allocation();
  int x = ia.x, y = ia.y, width = ia.width, height = ia.height;
  const int thickness = is_tight_focus() ? 1 : 2;
  if (width >= thickness && height >= thickness)
    {
      Color border1, border2;
      Color outer_upper_left;
      Color inner_upper_left;
      Color inner_lower_right;
      Color outer_lower_right;
      switch (current_frame())
        {
        case FRAME_IN:
          outer_upper_left = dark_glint();
          inner_upper_left = dark_shadow();
          inner_lower_right = light_shadow();
          outer_lower_right = light_glint();
          break;
        case FRAME_OUT:
          outer_upper_left = light_shadow();
          inner_upper_left = light_glint();
          inner_lower_right = dark_glint();
          outer_lower_right = dark_shadow();
          break;
        case FRAME_ETCHED_IN:
          outer_upper_left = dark_shadow();
          inner_upper_left = light_glint();
          inner_lower_right = dark_shadow();
          outer_lower_right = light_glint();
          break;
        case FRAME_ETCHED_OUT:
          outer_upper_left = light_glint();
          inner_upper_left = dark_shadow();
          inner_lower_right = light_glint();
          outer_lower_right = dark_shadow();
          break;
        case FRAME_FOCUS:
          if (is_tight_focus())
            border1 = focus_color();
          else
            border2 = focus_color();
          break;
        case FRAME_ALERT_FOCUS:
          border1 = 0xff000000;
          border2 = 0xffff0000;
          break;
        case FRAME_NONE:       /* no space to draw frame */
        case FRAME_BACKGROUND: /* space available, but frame is invisible */
        default: ;
        }
      vector<double> dashes;
      dashes.push_back (3);
      dashes.push_back (2);
      cairo_t *cr = cairo_context (rcontext, rect);
      CPainter painter (cr);
      if (outer_upper_left || inner_upper_left || inner_lower_right || outer_lower_right)
        painter.draw_shadow (x, y, width, height, outer_upper_left, inner_upper_left, inner_lower_right, outer_lower_right);
      if (border1)
        painter.draw_border (x, y, width, height, border1, dashes);
      if (border2)
        painter.draw_border (x + 1, y + 1, width - 2, height - 2, border2, dashes);
    }
}

static const WidgetFactory<FrameImpl> frame_factory ("Rapicorn::Frame");

// == FocusFrameImpl ==
FocusFrameImpl::FocusFrameImpl() :
  focus_container_ (NULL), container_has_focus_ (false), focus_frame_ (FRAME_FOCUS)
{}

FocusFrameImpl::~FocusFrameImpl ()
{}

void
FocusFrameImpl::focusable_container_change (ContainerImpl &focus_container)
{
  assert_return (&focus_container == focus_container_);
  const bool had_focus = container_has_focus_;
  container_has_focus_ = focus_container.has_focus();
  if (had_focus != container_has_focus_)
    {
      if (overlap_child())
        expose();
      else
        expose_enclosure();
    }
}

void
FocusFrameImpl::set_focus_child (WidgetImpl *widget)
{
  FrameImpl::set_focus_child (widget);
  expose_enclosure();
}

void
FocusFrameImpl::hierarchy_changed (WidgetImpl *old_toplevel)
{
  if (focus_container_)
    focus_container_->unregister_focus_indicator (*this);
  focus_container_ = NULL;
  container_has_focus_ = false;
  this->FrameImpl::hierarchy_changed (old_toplevel);
  if (anchored())
    {
      ContainerImpl *container = parent();
      while (container && !container->test_all_flags (NEEDS_FOCUS_INDICATOR))
        container = container->parent();
      focus_container_ = container;
      container_has_focus_ = focus_container_ && focus_container_->has_focus();
      if (focus_container_)
        focus_container_->register_focus_indicator (*this);
    }
}

void
FocusFrameImpl::focus_frame (FrameType ft)
{
  focus_frame_ = ft;
  changed ("focus_frame");
}

FrameType
FocusFrameImpl::focus_frame () const
{
  return focus_frame_;
}

FrameType
FocusFrameImpl::current_frame ()
{
  bool in_focus = has_focus();
  in_focus |= get_focus_child() != NULL;
  in_focus |= focus_container_ && focus_container_->has_focus();
  in_focus |= focus_container_ && focus_container_->get_focus_child() != NULL;
  if (in_focus)
    return focus_frame();
  return ancestry_active() ? active_frame() : normal_frame();
}

bool
FocusFrameImpl::tight_focus () const
{
  return const_cast<FocusFrameImpl*> (this)->tap_tight_focus (-1);
}

void
FocusFrameImpl::tight_focus (bool tf)
{
  if (tf != tight_focus())
    {
      const_cast<FocusFrameImpl*> (this)->tap_tight_focus (tf);
      changed ("tight_focus");
    }
}

static const WidgetFactory<FocusFrameImpl> focus_frame_factory ("Rapicorn::FocusFrame");


// == LayerPainterImpl ==
LayerPainterImpl::LayerPainterImpl()
{}

LayerPainterImpl::~LayerPainterImpl()
{}

void
LayerPainterImpl::size_request (Requisition &requisition)
{
  bool chspread = false, cvspread = false;
  Requisition combo;
  for (auto childp : *this)
    {
      WidgetImpl &child = *childp;
      /* size request all children */
      // Requisition rq = child.requisition();
      if (!child.visible())
        continue;
      chspread |= child.hspread();
      cvspread |= child.vspread();
      const Requisition crq = child.requisition();
      combo.width = MAX (combo.width, crq.width);
      combo.height = MAX (combo.height, crq.height);
    }
  requisition = combo;
  set_flag (HSPREAD_CONTAINER, chspread);
  set_flag (VSPREAD_CONTAINER, cvspread);
}

void
LayerPainterImpl::size_allocate (Allocation area, bool changed)
{
  for (auto childp : *this)
    {
      WidgetImpl &child = *childp;
      if (!child.visible())
        continue;
      Allocation carea = layout_child (child, area);
      child.set_allocation (carea);
    }
}

static const WidgetFactory<LayerPainterImpl> layer_painter_factory ("Rapicorn::LayerPainter");

// == ElementPainter ==
ElementPainterImpl::ElementPainterImpl() :
  cached_painter_ ("")
{}

ElementPainterImpl::~ElementPainterImpl()
{}

void
ElementPainterImpl::svg_source (const String &resource)
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
ElementPainterImpl::svg_element (const String &fragment)
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

static const uint64 BROKEN = 0x80000000;

static uint64
single_state_score (const String &state_string)
{
  switch (fnv1a_consthash64 (state_string.c_str()))
    {
    case fnv1a_consthash64 ("normal"):          return STATE_NORMAL;
    case fnv1a_consthash64 ("hover"):           return STATE_HOVER;
    case fnv1a_consthash64 ("panel"):           return STATE_PANEL;
    case fnv1a_consthash64 ("acceleratable"):   return STATE_ACCELERATABLE;
    case fnv1a_consthash64 ("default"):         return STATE_DEFAULT;
    case fnv1a_consthash64 ("selected"):        return STATE_SELECTED;
    case fnv1a_consthash64 ("focused"):         return STATE_FOCUSED;
    case fnv1a_consthash64 ("insensitive"):     return STATE_INSENSITIVE;
    case fnv1a_consthash64 ("active"):          return STATE_ACTIVE;
    case fnv1a_consthash64 ("retained"):        return STATE_RETAINED;
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
ElementPainterImpl::state_element (StateType state)
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

StateType
ElementPainterImpl::element_state () const
{
  StateType mystate = state();
  if (ancestry_active())
    mystate |= STATE_ACTIVE;
  return mystate;
}

String
ElementPainterImpl::current_element ()
{
  return state_element (element_state());
}

void
ElementPainterImpl::size_request (Requisition &requisition)
{
  bool chspread = false, cvspread = false;
  if (has_visible_child())
    requisition = size_request_child (get_child(), &chspread, &cvspread);
  set_flag (HSPREAD_CONTAINER, chspread);
  set_flag (VSPREAD_CONTAINER, cvspread);
  if (!size_painter_)
    size_painter_ = ImagePainter (state_element (STATE_NORMAL));
  const Requisition image_size = size_painter_.image_size ();
  const Rect fill = size_painter_.fill_area();
  assert_return (fill.x + fill.width <= image_size.width);
  assert_return (fill.y + fill.height <= image_size.height);
  requisition.width += image_size.width - fill.width;
  requisition.height += image_size.height - fill.height;
}

void
ElementPainterImpl::size_allocate (Allocation area, bool changed)
{
  if (has_visible_child())
    {
      WidgetImpl &child = get_child();
      Allocation child_area;
      if (size_painter_)
        {
          const Requisition image_size = size_painter_.image_size ();
          Rect fill = size_painter_.fill_area();
          assert_return (fill.x + fill.width <= image_size.width);
          assert_return (fill.y + fill.height <= image_size.height);
          Svg::Span spans[3] = { { 0, 0 }, { 0, 1 }, { 0, 0 } };
          // horizontal distribution & allocation
          spans[0].length = fill.x;
          spans[1].length = fill.width;
          spans[2].length = image_size.width - fill.x - fill.width;
          ssize_t dremain = Svg::Span::distribute (ARRAY_SIZE (spans), spans, area.width - image_size.width, 1);
          if (dremain < 0)
            Svg::Span::distribute (ARRAY_SIZE (spans), spans, dremain, 0); // shrink *any* segment
          child_area.x = area.x + spans[0].length;
          child_area.width = spans[1].length;
          // vertical distribution & allocation
          spans[0].length = fill.y;
          spans[1].length = fill.height;
          spans[2].length = image_size.height - fill.y - fill.height;
          dremain = Svg::Span::distribute (ARRAY_SIZE (spans), spans, area.height - image_size.height, 1);
          if (dremain < 0)
            Svg::Span::distribute (ARRAY_SIZE (spans), spans, dremain, 0); // shrink *any* segment
          child_area.y = area.y + spans[0].length;
          child_area.height = spans[1].length;
        }
      else
        child_area = area;
      child_area = layout_child (child, child_area);
      child.set_allocation (child_area);
    }
}

void
ElementPainterImpl::do_changed (const String &name)
{
  WidgetImpl::do_changed (name);
  if (name == "state" && (cached_painter_ != current_element()))
    invalidate (INVALID_CONTENT);
}

void
ElementPainterImpl::render (RenderContext &rcontext, const Rect &rect)
{
  const String painter_src = current_element();
  if (!state_painter_ || cached_painter_ != painter_src)
    {
      state_painter_ = ImagePainter (painter_src);
      cached_painter_ = painter_src;
    }
  state_painter_.draw_image (cairo_context (rcontext, rect), rect, allocation());
}

static const WidgetFactory<ElementPainterImpl> element_painter_factory ("Rapicorn::ElementPainter");

// == FocusPainterImpl ==
FocusPainterImpl::FocusPainterImpl() :
  focus_container_ (NULL), container_has_focus_ (false)
{}

FocusPainterImpl::~FocusPainterImpl ()
{}

void
FocusPainterImpl::focusable_container_change (ContainerImpl &focus_container)
{
  assert_return (&focus_container == focus_container_);
  const bool had_focus = container_has_focus_;
  container_has_focus_ = focus_container.has_focus();
  if (had_focus != container_has_focus_)
    expose();
}

void
FocusPainterImpl::set_focus_child (WidgetImpl *widget)
{
  ElementPainterImpl::set_focus_child (widget);
  expose();
}

void
FocusPainterImpl::hierarchy_changed (WidgetImpl *old_toplevel)
{
  if (focus_container_)
    focus_container_->unregister_focus_indicator (*this);
  focus_container_ = NULL;
  container_has_focus_ = false;
  this->ElementPainterImpl::hierarchy_changed (old_toplevel);
  if (anchored())
    {
      ContainerImpl *container = parent();
      while (container && !container->test_all_flags (NEEDS_FOCUS_INDICATOR))
        container = container->parent();
      focus_container_ = container;
      container_has_focus_ = focus_container_ && focus_container_->has_focus();
      if (focus_container_)
        focus_container_->register_focus_indicator (*this);
    }
}

StateType
FocusPainterImpl::element_state () const
{
  bool in_focus = has_focus();
  in_focus = in_focus || get_focus_child() != NULL;
  in_focus = in_focus || (focus_container_ && focus_container_->has_focus());
  in_focus = in_focus || (focus_container_ && focus_container_->get_focus_child() != NULL);
  StateType mystate = ElementPainterImpl::element_state();
  if (in_focus)
    mystate |= STATE_FOCUSED;
  return mystate;
}

static const WidgetFactory<FocusPainterImpl> focus_painter_factory ("Rapicorn::FocusPainter");

} // Rapicorn
