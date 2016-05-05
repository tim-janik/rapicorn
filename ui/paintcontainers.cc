// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "paintcontainers.hh"
#include "container.hh"
#include "painter.hh"
#include "factory.hh"
#include "../rcore/svg.hh" // Svg::Span

namespace Rapicorn {

// == AmbienceImpl ==
AmbienceImpl::AmbienceImpl() :
  normal_background_ ("none"),
  hover_background_ ("none"),
  active_background_ ("none"),
  insensitive_background_ ("none"),
  normal_lighting_ (Lighting::UPPER_LEFT),
  hover_lighting_ (Lighting::UPPER_LEFT),
  active_lighting_ (Lighting::LOWER_RIGHT),
  insensitive_lighting_ (Lighting::CENTER),
  normal_shade_ (Lighting::UPPER_LEFT),
  hover_shade_ (Lighting::UPPER_LEFT),
  active_shade_ (Lighting::LOWER_RIGHT),
  insensitive_shade_ (Lighting::CENTER)
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
AmbienceImpl::lighting (Lighting sh)
{
  insensitive_lighting (sh);
  hover_lighting (sh);
  active_lighting (sh);
  normal_lighting (sh);
}

void
AmbienceImpl::shade (Lighting sh)
{
  insensitive_shade (Lighting::NONE);
  hover_shade (Lighting::NONE);
  active_shade (Lighting::NONE);
  normal_shade (sh);
}

String
AmbienceImpl::background () const
{
  RAPICORN_ASSERT_UNREACHED();
}

Lighting
AmbienceImpl::lighting () const
{
  RAPICORN_ASSERT_UNREACHED();
}

Lighting
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
AmbienceImpl::insensitive_lighting (Lighting sh)
{
  insensitive_lighting_ = sh;
  expose();
  changed ("insensitive_lighting");
}

Lighting
AmbienceImpl::insensitive_lighting () const
{
  return insensitive_lighting_;
}

void
AmbienceImpl::hover_lighting (Lighting sh)
{
  hover_lighting_ = sh;
  expose();
  changed ("hover_lighting");
}

Lighting
AmbienceImpl::hover_lighting () const
{
  return hover_lighting_;
}

void
AmbienceImpl::active_lighting (Lighting sh)
{
  active_lighting_ = sh;
  expose();
  changed ("active_lighting");
}

Lighting
AmbienceImpl::active_lighting () const
{
  return active_lighting_;
}

void
AmbienceImpl::normal_lighting (Lighting sh)
{
  normal_lighting_ = sh;
  expose();
  changed ("normal_lighting");
}

Lighting
AmbienceImpl::normal_lighting () const
{
  return normal_lighting_;
}

void
AmbienceImpl::insensitive_shade (Lighting sh)
{
  insensitive_shade_ = sh;
  expose();
  changed ("insensitive_shade");
}

Lighting
AmbienceImpl::insensitive_shade () const
{
  return insensitive_shade_;
}

void
AmbienceImpl::hover_shade (Lighting sh)
{
  hover_shade_ = sh;
  expose();
  changed ("hover_shade");
}

Lighting
AmbienceImpl::hover_shade () const
{
  return hover_shade_;
}

void
AmbienceImpl::active_shade (Lighting sh)
{
  active_shade_ = sh;
  expose();
  changed ("active_shade");
}

Lighting
AmbienceImpl::active_shade () const
{
  return active_shade_;
}

void
AmbienceImpl::normal_shade (Lighting sh)
{
  normal_shade_ = sh;
  expose();
  changed ("normal_shade");
}

Lighting
AmbienceImpl::normal_shade () const
{
  return normal_shade_;
}

void
AmbienceImpl::render_shade (cairo_t *cairo, int x, int y, int width, int height, Lighting st)
{
  int shade_alpha = 0x3b;
  Color light = light_glint().shade (shade_alpha), dark = dark_glint().shade (shade_alpha);
  uint64 dark_flag = uint64 (st) & Lighting::DARK_FLAG;
  CPainter painter (cairo);
  if (dark_flag != Lighting::NONE)
    swap (light, dark);
  const Lighting lighting = Lighting (st & ~uint64 (Lighting::DARK_FLAG));
  switch (lighting)
    {
    case Lighting::UPPER_LEFT:
      painter.draw_center_shade_rect (x, y, light, x + width - 1, y + height - 1, dark);
      break;
    case Lighting::UPPER_RIGHT:
      painter.draw_center_shade_rect (x + width - 1, y, light, x, y + height - 1, dark);
      break;
    case Lighting::LOWER_LEFT:
      painter.draw_center_shade_rect (x, y + height - 1, light, x + width - 1, y, dark);
      break;
    case Lighting::LOWER_RIGHT:
      painter.draw_center_shade_rect (x + width - 1, y + height - 1, light, x, y, dark);
      break;
    case Lighting::CENTER:
      render_shade (cairo, x, y, width / 2, height / 2, Lighting (dark_flag | Lighting::UPPER_RIGHT));
      render_shade (cairo, x, y + height / 2, width / 2, height / 2, Lighting (dark_flag | Lighting::LOWER_RIGHT));
      render_shade (cairo, x + width / 2, y + height / 2, width / 2, height / 2, Lighting (dark_flag | Lighting::LOWER_LEFT));
      render_shade (cairo, x + width / 2, y, width / 2, height / 2, Lighting (dark_flag | Lighting::UPPER_LEFT));
      break;
    case Lighting::DIFFUSE:
      painter.draw_shaded_rect (x, y, light, x + width - 1, y + height - 1, light);
      break;
    case Lighting::NONE:
    default:
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
  Color background = style()->resolve_color (background_color, WidgetState::NORMAL, StyleColor::BACKGROUND);
  cairo_t *cr = cairo_context (rcontext, rect);
  CPainter painter (cr);
  if (background)
    painter.draw_filled_rect (x, y, width, height, background);
  /* render lighting (mutually exclusive) */
  if (aactive && Lighting::NONE != active_lighting())
    render_shade (cr, x, y, width, height, active_lighting());
  else if (insensitive() && Lighting::NONE != insensitive_lighting())
    render_shade (cr, x, y, width, height, insensitive_lighting());
  else if (ahover && Lighting::NONE != hover_lighting())
    render_shade (cr, x, y, width, height, hover_lighting());
  else if (Lighting::NONE != normal_lighting() && !aactive && !insensitive() && !ahover)
    render_shade (cr, x, y, width, height, normal_lighting());
  /* render shade (combinatoric) */
  if (aactive && Lighting::NONE != active_shade())
    render_shade (cr, x, y, width, height, active_shade());
  if (insensitive() && Lighting::NONE != insensitive_shade())
    render_shade (cr, x, y, width, height, insensitive_shade());
  if (ahover && Lighting::NONE != hover_shade())
    render_shade (cr, x, y, width, height, hover_shade());
  if (!aactive && !insensitive() && !ahover && Lighting::NONE != normal_shade())
    render_shade (cr, x, y, width, height, normal_shade());
}

static const WidgetFactory<AmbienceImpl> ambience_factory ("Rapicorn::Ambience");

// == FrameImpl ==
FrameImpl::FrameImpl() :
  normal_frame_ (DrawFrame::ETCHED_IN),
  active_frame_ (DrawFrame::ETCHED_IN),
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
  return tight_focus_ && ((normal_frame_ == DrawFrame::FOCUS || normal_frame_ == DrawFrame::NONE) &&
                          (active_frame_ == DrawFrame::FOCUS || active_frame_ == DrawFrame::NONE));
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

DrawFrame
FrameImpl::normal_frame () const
{
  return normal_frame_;
}

void
FrameImpl::normal_frame (DrawFrame ft)
{
  if (normal_frame_ != ft)
    {
      normal_frame_ = ft;
      expose_enclosure();
      changed ("normal_frame");
    }
}

DrawFrame
FrameImpl::active_frame () const
{
  return active_frame_;
}

void
FrameImpl::active_frame (DrawFrame ft)
{
  if (active_frame_ != ft)
    {
      active_frame_ = ft;
      expose_enclosure();
      changed ("active_frame");
    }
}

DrawFrame
FrameImpl::frame_type () const
{
  // this is a write-only propery
  RAPICORN_ASSERT_UNREACHED();
}

void
FrameImpl::frame_type (DrawFrame ft)
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

DrawFrame
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
        case DrawFrame::IN:
          outer_upper_left = dark_glint();
          inner_upper_left = dark_shadow();
          inner_lower_right = light_shadow();
          outer_lower_right = light_glint();
          break;
        case DrawFrame::OUT:
          outer_upper_left = light_shadow();
          inner_upper_left = light_glint();
          inner_lower_right = dark_glint();
          outer_lower_right = dark_shadow();
          break;
        case DrawFrame::ETCHED_IN:
          outer_upper_left = dark_shadow();
          inner_upper_left = light_glint();
          inner_lower_right = dark_shadow();
          outer_lower_right = light_glint();
          break;
        case DrawFrame::ETCHED_OUT:
          outer_upper_left = light_glint();
          inner_upper_left = dark_shadow();
          inner_lower_right = light_glint();
          outer_lower_right = dark_shadow();
          break;
        case DrawFrame::FOCUS:
          if (is_tight_focus())
            border1 = focus_color();
          else
            border2 = focus_color();
          break;
        case DrawFrame::ALERT_FOCUS:
          border1 = 0xff000000;
          border2 = 0xffff0000;
          break;
        case DrawFrame::NONE:       /* no space to draw frame */
        case DrawFrame::BACKGROUND: /* space available, but frame is invisible */
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
  focus_container_ (NULL), container_has_focus_ (false), focus_frame_ (DrawFrame::FOCUS)
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
FocusFrameImpl::focus_frame (DrawFrame ft)
{
  focus_frame_ = ft;
  changed ("focus_frame");
}

DrawFrame
FocusFrameImpl::focus_frame () const
{
  return focus_frame_;
}

DrawFrame
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

String
ElementPainterImpl::state_element (WidgetState state)
{
  if (!size_painter_)
    size_painter_ = ImagePainter (svg_source_);
  if (size_painter_ && svg_fragment_.empty())
    return svg_source_;
  else if (size_painter_ && !svg_fragment_.empty())
    {
      // match an SVG element to state, xml ID syntax: <element id="elementname:active+insensitive"/>
      const String element = svg_fragment_.substr (svg_fragment_[0] == '#' ? 1 : 0); // strip initial hash
      const String match = StyleIface::pick_fragment (element, state, size_painter_.list());
      if (!match.empty())
        return svg_source_ + "#" + match;
    }
  if (!svg_source_.empty() && !size_painter_)
    user_warning (user_source(), "failed to lookup SVG: %s", svg_source_);
  else if (!svg_source_.empty() && !svg_fragment_.empty())
    user_warning (user_source(), "failed to lookup SVG element: %s %s", svg_source_, svg_fragment_);
  return "";
}

WidgetState
ElementPainterImpl::element_state () const
{
  WidgetState mystate = state();
  if (ancestry_active())
    mystate = WidgetState (uint64 (mystate) | WidgetState::ACTIVE);
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
  set_flag (HSPREAD_CONTAINER, chspread);
  set_flag (VSPREAD_CONTAINER, cvspread);
  if (!size_painter_)
    size_painter_ = ImagePainter (state_element (WidgetState::NORMAL));
  const Requisition image_size = size_painter_.image_size ();
  const Rect fill = size_painter_.fill_area();
  assert_return (fill.x + fill.width <= image_size.width);
  assert_return (fill.y + fill.height <= image_size.height);
  if (has_visible_child())
    {
      requisition = size_request_child (get_child(), &chspread, &cvspread);
      requisition.width += image_size.width - fill.width;
      requisition.height += image_size.height - fill.height;
    }
  else
    {
      requisition.width = image_size.width;
      requisition.height = image_size.height;
    }
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

WidgetState
FocusPainterImpl::element_state () const
{
  bool in_focus = has_focus();
  in_focus = in_focus || get_focus_child() != NULL;
  in_focus = in_focus || (focus_container_ && focus_container_->has_focus());
  in_focus = in_focus || (focus_container_ && focus_container_->get_focus_child() != NULL);
  WidgetState mystate = ElementPainterImpl::element_state();
  if (in_focus)
    mystate = WidgetState (uint64 (mystate) | WidgetState::FOCUSED);
  return mystate;
}

static const WidgetFactory<FocusPainterImpl> focus_painter_factory ("Rapicorn::FocusPainter");

} // Rapicorn
