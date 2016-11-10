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
  invalidate_content();
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
  invalidate_content();
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
  invalidate_content();
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
  invalidate_content();
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
  invalidate_content();
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
  invalidate_content();
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
  invalidate_content();
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
  invalidate_content();
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
  invalidate_content();
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
  invalidate_content();
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
  invalidate_content();
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
  invalidate_content();
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
  Color light = light_clinch().shade (shade_alpha), dark = dark_clinch().shade (shade_alpha);
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
AmbienceImpl::render (RenderContext &rcontext)
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
  cairo_t *cr = cairo_context (rcontext);
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
      const bool onoff = onoffx != 0;
      if (onoff != tight_focus_)
        {
          tight_focus_ = onoff;
          invalidate_size();
          invalidate_content();
        }
    }
  return tight_focus_;
}

bool
FrameImpl::is_tight_focus() const
{
  return tight_focus_ && ((normal_frame_ == DrawFrame::FOCUS || normal_frame_ == DrawFrame::NONE) &&
                          (active_frame_ == DrawFrame::FOCUS || active_frame_ == DrawFrame::NONE));
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
      invalidate_content();
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
      invalidate_content();
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
  if (overlap_child_ != ovc)
    {
      overlap_child_ = ovc;
      invalidate_size();
      invalidate_content();
      changed ("overlap_child");
    }
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
FrameImpl::size_allocate (Allocation area)
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
      layout_child_allocation (child, carea);
    }
}

void
FrameImpl::render (RenderContext &rcontext)
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
          outer_upper_left = dark_clinch();
          inner_upper_left = dark_color();
          inner_lower_right = light_color();
          outer_lower_right = light_clinch();
          break;
        case DrawFrame::OUT:
          outer_upper_left = light_color();
          inner_upper_left = light_clinch();
          inner_lower_right = dark_clinch();
          outer_lower_right = dark_color();
          break;
        case DrawFrame::ETCHED_IN:
          outer_upper_left = dark_color();
          inner_upper_left = light_clinch();
          inner_lower_right = dark_color();
          outer_lower_right = light_clinch();
          break;
        case DrawFrame::ETCHED_OUT:
          outer_upper_left = light_clinch();
          inner_upper_left = dark_color();
          inner_lower_right = light_clinch();
          outer_lower_right = dark_color();
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
      cairo_t *cr = cairo_context (rcontext);
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
    invalidate_content();
}

void
FocusFrameImpl::set_focus_child (WidgetImpl *widget)
{
  FrameImpl::set_focus_child (widget);
  invalidate_content();
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
      while (container && !container->test_any (NEEDS_FOCUS_INDICATOR))
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
LayerPainterImpl::size_allocate (Allocation area)
{
  for (auto childp : *this)
    {
      WidgetImpl &child = *childp;
      if (!child.visible())
        continue;
      layout_child_allocation (child, area);
    }
}

static const WidgetFactory<LayerPainterImpl> layer_painter_factory ("Rapicorn::LayerPainter");

// == ElementPainter ==
ElementPainterImpl::ElementPainterImpl() :
  child_area_ { 0, 0, -1, -1 },
  svg_source_ ("auto")
{}

ElementPainterImpl::~ElementPainterImpl()
{}

void
ElementPainterImpl::svg_source (const String &resource)
{
  String val = string_strip (resource);
  return_unless (val != svg_source_);
  svg_source_ = val;
  cached_svg_.reset();
  invalidate_all();
  changed ("svg_source");
}

Svg::FileP
ElementPainterImpl::svg_file() const
{
  if (!cached_svg_)
    {
      if (svg_source_ != "auto")
        {
          cached_svg_ = style()->load_svg (svg_source_);
          if (!cached_svg_)
            user_warning (user_source(), "failed to lookup SVG: %s", svg_source_);
        }
      if (!cached_svg_)
        cached_svg_ = style()->svg_file();
    }
  return cached_svg_;
}

const ElementPainterImpl::PaintEntry&
ElementPainterImpl::peek_entry (uint64 state_bit) const
{
  for (const auto &p : paint_elements_)
    if (p.state_bit == state_bit)
      return p;
  static const PaintEntry dummy { "", 0 };
  return dummy;
}

ElementPainterImpl::PaintEntry&
ElementPainterImpl::fetch_entry (uint64 state_bit)
{
  for (auto &p : paint_elements_)
    if (p.state_bit == state_bit)
      return p;
  const PaintEntry new_entry { "", state_bit };
  paint_elements_.push_back (new_entry);
  return paint_elements_.back();
}

void
ElementPainterImpl::assign_entry (uint64 state_bit, const String &element, const String &property)
{
  assert_return ((state_bit & (state_bit - 1)) == 0); // have at most one bit set
  assert_return (property.empty() == false);
  PaintEntry &pe = fetch_entry (state_bit);
  const String stripped_element = string_strip (element);
  if (pe.element != stripped_element)
    {
      pe.element = stripped_element;
      if (state_bit == FILLING_PAINTER)
        invalidate_content();
      else if (state_bit == SIZING_PAINTER)
        invalidate_size();
      else // WidgetState changes
        invalidate_all();
      changed (property);
    }
}

WidgetState
ElementPainterImpl::render_state () const
{
  return state();
}

String
ElementPainterImpl::size_fragment () const
{
  const PaintEntry *pe = NULL;
  for (auto &e : paint_elements_)
    if (e.element.empty())
      continue;
    else if (e.state_bit == SIZING_PAINTER)             // give SIZING precedence
      return StyleIface::pick_fragment (e.element, WidgetState::NORMAL, svg_file()->list());
    else if (!pe ||                                     // first, match any
             (e.state_bit != FILLING_PAINTER &&
              e.state_bit < pe->state_bit))             // minimize match requirements
      pe = &e;
  String fragment;
  if (pe)
    {
      fragment = StyleIface::pick_fragment (pe->element, WidgetState::NORMAL, svg_file()->list());
      if (fragment.empty() && pe->state_bit)    // if needed, allow state fragment with duplicate "+state" flag
        fragment = StyleIface::pick_fragment (pe->element, WidgetState (pe->state_bit), svg_file()->list());
    }
  return fragment;
}

String
ElementPainterImpl::filling_fragment (WidgetState widget_state) const
{
  for (auto &e : paint_elements_)
    if (e.element.empty())
      continue;
    else if (e.state_bit == FILLING_PAINTER)
      return StyleIface::pick_fragment (e.element, widget_state, svg_file()->list());
  return "";
}

String
ElementPainterImpl::state_fragment (WidgetState widget_state) const
{
  const uint64 state = (uint64) widget_state;
  const PaintEntry *pe = NULL;
  for (auto &e : paint_elements_)
    if (e.element.empty())
      continue;
    else if ((!pe && e.state_bit == 0) ||               // fallback to NORMAL if present
             (!pe && (e.state_bit & state)) ||          // find first state match
             (pe &&
              (e.state_bit & state) > pe->state_bit))   // improve match
      pe = &e;
  String fragment;
  if (pe)
    {
      fragment = StyleIface::pick_fragment (pe->element, WidgetState (state & ~pe->state_bit), svg_file()->list());
      if (fragment.empty() && (state & pe->state_bit))  // if needed, allow state fragment with duplicate "+state" flag
        fragment = StyleIface::pick_fragment (pe->element, widget_state, svg_file()->list());
    }
  return fragment;
}

ImagePainter
ElementPainterImpl::load_painter (const String &fragment) const
{
  ImagePainter image_painter;
  if (!fragment.empty())
    {
      image_painter = ImagePainter (svg_file(), fragment);
      if (!image_painter)
        user_warning (user_source(), "failed to lookup SVG element: %s %s", svg_source_, fragment);
    }
  return image_painter;
}

ImagePainter
ElementPainterImpl::size_painter () const
{
  return load_painter (size_fragment());
}

ImagePainter
ElementPainterImpl::state_painter () const
{
  return load_painter (state_fragment (render_state()));
}

ImagePainter
ElementPainterImpl::filling_painter () const
{
  return load_painter (filling_fragment (render_state()));
}

void
ElementPainterImpl::size_request (Requisition &requisition)
{
  bool chspread = false, cvspread = false;
  ImagePainter painter = size_painter();
  const Requisition image_size = painter.image_size ();
  const IRect fill = painter.fill_area();
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
  set_flag (HSPREAD_CONTAINER, chspread);
  set_flag (VSPREAD_CONTAINER, cvspread);
}

void
ElementPainterImpl::size_allocate (Allocation area)
{
  ImagePainter painter = size_painter();
  const Requisition image_size = painter.image_size ();
  const IRect fill = painter.fill_area();
  assert_return (fill.x + fill.width <= image_size.width);
  assert_return (fill.y + fill.height <= image_size.height);
  if (fill.width > 0 && fill.height > 0)
    {
      Svg::Span spans[3] = { { 0, 0 }, { 0, 1 }, { 0, 0 } };
      // horizontal distribution & allocation
      spans[0].length = fill.x;
      spans[1].length = fill.width;
      spans[2].length = image_size.width - fill.x - fill.width;
      ssize_t dremain = Svg::Span::distribute (ARRAY_SIZE (spans), spans, area.width - image_size.width, 1);
      if (dremain < 0)
        Svg::Span::distribute (ARRAY_SIZE (spans), spans, dremain, 0); // shrink *all* segments
      child_area_.x = area.x + spans[0].length;
      child_area_.width = spans[1].length;
      // vertical distribution & allocation
      spans[0].length = fill.y;
      spans[1].length = fill.height;
      spans[2].length = image_size.height - fill.y - fill.height;
      dremain = Svg::Span::distribute (ARRAY_SIZE (spans), spans, area.height - image_size.height, 1);
      if (dremain < 0)
        Svg::Span::distribute (ARRAY_SIZE (spans), spans, dremain, 0); // shrink *all* segments
      child_area_.y = area.y + spans[0].length;
      child_area_.height = spans[1].length;
    }
  else
    child_area_ = area;
  if (has_visible_child())
    {
      WidgetImpl &child = get_child();
      layout_child_allocation (child, child_area_);
    }
}

void
ElementPainterImpl::do_changed (const String &name)
{
  SingleContainerImpl::do_changed (name);
  if (name == "state")
    {
      // reset state_painter()
      // reset filling_painter()
      invalidate_content();
    }
}

void
ElementPainterImpl::render (RenderContext &rcontext)
{
  state_painter().draw_image (cairo_context (rcontext), allocation(), allocation());
  filling_painter().draw_image (cairo_context (rcontext), child_area_, child_area_);
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
    invalidate_content();
}

void
FocusPainterImpl::set_focus_child (WidgetImpl *widget)
{
  ElementPainterImpl::set_focus_child (widget);
  invalidate_content();
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
      while (container && !container->test_any (NEEDS_FOCUS_INDICATOR))
        container = container->parent();
      focus_container_ = container;
      container_has_focus_ = focus_container_ && focus_container_->has_focus();
      if (focus_container_)
        focus_container_->register_focus_indicator (*this);
    }
}

WidgetState
FocusPainterImpl::render_state () const
{
  bool in_focus = has_focus();
  in_focus = in_focus || get_focus_child() != NULL;
  in_focus = in_focus || (focus_container_ && focus_container_->has_focus());
  in_focus = in_focus || (focus_container_ && focus_container_->get_focus_child() != NULL);
  WidgetState mystate = ElementPainterImpl::render_state();
  if (in_focus)
    mystate = WidgetState (uint64 (mystate) | WidgetState::FOCUSED);
  return mystate;
}

static const WidgetFactory<FocusPainterImpl> focus_painter_factory ("Rapicorn::FocusPainter");

// == ShapePainterImpl ==
ShapePainterImpl::ShapePainterImpl ()
{}

ShapePainterImpl::~ShapePainterImpl ()
{}

void
ShapePainterImpl::shape_style_normal (const String &kind)
{
  String val = string_strip (kind);
  return_unless (val != shape_style_normal_);
  shape_style_normal_ = val;
  invalidate_size();
  invalidate_content();
  changed ("shape_style_normal");
}

WidgetState
ShapePainterImpl::render_state () const
{
  return state();
}

void
ShapePainterImpl::size_request (Requisition &requisition)
{
  bool chspread = false, cvspread = false;
  set_flag (HSPREAD_CONTAINER, chspread);
  set_flag (VSPREAD_CONTAINER, cvspread);
  const int thickness = 1;
  requisition.width = 2 * thickness + 2 * thickness;
  requisition.height = 2 * thickness + 2 * thickness;
  if (has_visible_child())
    {
      const Requisition child_requisition = size_request_child (get_child(), &chspread, &cvspread);
      requisition.width += child_requisition.width;
      requisition.height += child_requisition.height;
    }
  else
    { // dummy size
      requisition.width += 4 * thickness;
      requisition.height += 4 * thickness;
    }
}

void
ShapePainterImpl::size_allocate (Allocation area)
{
  const int thickness = 1; // FIXME: query viewport for thickness
  fill_area_ = area;
  fill_area_.x += MIN (area.width / 2, 2 * thickness);
  fill_area_.width -= MIN (area.width, 2 * thickness + 2 * thickness);
  fill_area_.y += MIN (area.height / 2, 2 * thickness);
  fill_area_.height -= MIN (area.height, 2 * thickness + 2 * thickness);
  if (has_visible_child())
    {
      WidgetImpl &child = get_child();
      layout_child_allocation (child, fill_area_);
    }
}

static void
draw_gradient_fill (cairo_t *cr, cairo_pattern_t *pat, Color start_color, Color end_color, bool hard_gradient)
{
  cairo_pattern_add_color_stop_rgba (pat, .0, start_color.red1(), start_color.green1(), start_color.blue1(), start_color.alpha1());
  if (hard_gradient)
    {
      cairo_pattern_add_color_stop_rgba (pat, 0.5, start_color.red1(), start_color.green1(), start_color.blue1(), start_color.alpha1());
      cairo_pattern_add_color_stop_rgba (pat, 0.50001, end_color.red1(), end_color.green1(), end_color.blue1(), end_color.alpha1());
    }
  cairo_pattern_add_color_stop_rgba (pat, 1., end_color.red1(), end_color.green1(), end_color.blue1(), end_color.alpha1());
  cairo_set_source (cr, pat);
  cairo_fill (cr);
}

static bool
in_list (int needle, const int *list, size_t length)
{
  for (size_t j = 0; j < length; j++)
    if (needle == list[j])
      return true;
  return false;
}

static void
draw_star (cairo_t *cr, double x, double y, double width, double height, Color start_color, Color end_color, bool retained)
{
  const double points[] = {
    0.500000, 1.000000, // top peak
    0.654508, 0.670820,
    1.000000, 0.618034, // right peak
    0.750000, 0.361803,
    0.809017, 0.000000, // bottom right
    0.500000, 0.170820,
    0.190983, 0.000000, // bottom left
    0.250000, 0.361803,
    0.000000, 0.618034, // left peak
    0.345492, 0.670820, // gradient start
  };
  const double *gradient_start = points + 18;
  const double gradient_end[2] = { 0.5, 0.5 };
  const int skip_peaks[] = { 1, 2, 3, 7, 8, 9 }; // peak points to skip for retained mode
  for (size_t i = 0; i < ARRAY_SIZE (points); i+= 2)
    {
      if (retained && in_list (i / 2, skip_peaks, ARRAY_SIZE (skip_peaks)))
        continue;
      const double px = x + points[i] * width, py = y + (1.0 - points[i + 1]) * height;
      cairo_line_to (cr, px, py);
    }
  cairo_pattern_t *pat = cairo_pattern_create_linear (x + width * gradient_start[0], y + height * (1 - gradient_start[1]),
                                                      x + width * gradient_end[0], y + height * (1 - gradient_end[1]));
  draw_gradient_fill (cr, pat, start_color, end_color, false);
  cairo_pattern_destroy (pat);
}

static void
draw_retained (cairo_t *cr, double x, double y, double width, double height, Color start_color, Color end_color)
{
  const double th = MIN (1, MIN (width, height) * 0.5); // th = thickness
  const double midx = x + width * 0.5, topy = y, bottomy = y + height;
  cairo_move_to (cr, midx - th, topy);
  cairo_line_to (cr, midx + th, topy);
  cairo_line_to (cr, midx + th, bottomy);
  cairo_line_to (cr, midx - th, bottomy);
  cairo_close_path (cr);
  cairo_pattern_t *pat = cairo_pattern_create_linear (x + width * 0.5, y, x + width * 0.5, y + height);
  draw_gradient_fill (cr, pat, start_color, end_color, false);
  cairo_pattern_destroy (pat);
}

static void
draw_circle (cairo_t *cr, double x, double y, double width, double height, Color start_color, Color end_color, bool retained)
{
  if (retained)
    return draw_retained (cr, x, y, width, height, start_color, end_color);
  const double r = MIN (width, height) * 0.5;
  cairo_arc (cr, x + width / 2, y + height / 2, r, 0, 2 * M_PI);
  const double q = 0.7, s = 1 - q;
  cairo_pattern_t *pat = cairo_pattern_create_linear (x + width * s, y + height * s, x + width * q, y + height * q);
  draw_gradient_fill (cr, pat, start_color, end_color, false);
  cairo_pattern_destroy (pat);
}

static void
draw_diamond (cairo_t *cr, double x, double y, double width, double height, Color start_color, Color end_color, bool retained)
{
  if (retained)
    return draw_retained (cr, x, y, width, height, start_color, end_color);
  const double th = MIN (1, MIN (width, height) * 0.5), d = 0.5 * th; // th = thickness
  const double midx = x + width * 0.5, leftx = x, rightx = x + width;
  const double midy = y + height * 0.5, topy = y, bottomy = y + height;
  cairo_move_to (cr, midx - d, topy);           cairo_rel_line_to (cr, th, 0);
  cairo_line_to (cr, rightx, midy - d);         cairo_rel_line_to (cr, 0, th);
  cairo_line_to (cr, midx + d, bottomy);        cairo_rel_line_to (cr, -th, 0);
  cairo_line_to (cr, leftx, midy + d);          cairo_rel_line_to (cr, 0, -th);
  cairo_close_path (cr);
  cairo_pattern_t *pat = cairo_pattern_create_linear (x + width * 0.5, y, x + width * 0.5, y + height);
  draw_gradient_fill (cr, pat, start_color, end_color, true);
  cairo_pattern_destroy (pat);
}

static void
draw_square (cairo_t *cr, double x, double y, double width, double height, Color start_color, Color end_color, bool retained)
{
  if (retained)
    return draw_retained (cr, x, y, width, height, start_color, end_color);
  const double leftx = x, rightx = x + width;
  const double topy = y, bottomy = y + height;
  cairo_move_to (cr, leftx,  topy);
  cairo_line_to (cr, rightx, topy);
  cairo_line_to (cr, rightx, bottomy);
  cairo_line_to (cr, leftx,  bottomy);
  cairo_close_path (cr);
  cairo_pattern_t *pat = cairo_pattern_create_linear (leftx, topy, rightx, bottomy);
  draw_gradient_fill (cr, pat, start_color, end_color, true);
  cairo_pattern_destroy (pat);
}

void
ShapePainterImpl::render (RenderContext &rcontext)
{
  return_unless (fill_area_.width > 0 && fill_area_.height > 0);
  std::function<void (cairo_t*, double, double, double, double, Color, Color, bool)> draw_shape;
  const WidgetState rstate = render_state();
  const bool draw_retained = (rstate & WidgetState::RETAINED) != 0;
  const bool draw_mark = draw_retained || (rstate & WidgetState::TOGGLED) != 0;
  const String shape_style = shape_style_normal_;
  if (shape_style == "circle")
    draw_shape = draw_circle;
  else if (shape_style == "diamond")
    draw_shape = draw_diamond;
  else if (shape_style == "square")
    draw_shape = draw_square;
  else if (shape_style == "star")
    draw_shape = draw_star;
  else
    return;
  const int thickness = 1;
  const Allocation area = allocation();
  cairo_t *cr = cairo_context (rcontext);
  int r = MIN (area.width, area.height) / 2;
  int cx = area.x + area.width * 0.5 + 0.5, cy = area.y + area.height * 0.5 + 0.5;
  Color col1, col2;
  // outer shape
  col1 = style()->state_color (rstate, StyleColor::DARK_CLINCH);
  col2 = style()->state_color (rstate, StyleColor::LIGHT_CLINCH);
  draw_shape (cr, cx - r, cy - r, 2 * r, 2 * r, col1, col2, false);
  // inner shape
  r -= thickness;
  col1 = style()->state_color (rstate, StyleColor::DARK);
  col2 = style()->state_color (rstate, StyleColor::LIGHT);
  draw_shape (cr, cx - r, cy - r, 2 * r, 2 * r, col1, col2, false);
  // panel bg
  if (!draw_mark || r > 2 * thickness) // if space is too tight, give draw_mark precedence
    {
      r -= thickness;
      col1 = style()->state_color (rstate | WidgetState::PANEL, StyleColor::BACKGROUND);
      col2 = style()->state_color (rstate | WidgetState::PANEL, StyleColor::BACKGROUND);
      draw_shape (cr, cx - r, cy - r, 2 * r, 2 * r, col1, col2, false);
    }
  // inner spot
  r -= thickness;
  if (draw_mark)
    {
      col1 = style()->state_color (rstate | WidgetState::PANEL, StyleColor::MARK);
      col2 = style()->state_color (rstate | WidgetState::PANEL, StyleColor::MARK);
      draw_shape (cr, cx - r, cy - r, 2 * r, 2 * r, col1, col2, draw_retained);
    }
}

static const WidgetFactory<ShapePainterImpl> shape_painter_factory ("Rapicorn::ShapePainter");

} // Rapicorn
