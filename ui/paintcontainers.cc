// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "paintcontainers.hh"
#include "container.hh"
#include "painter.hh"
#include "factory.hh"

namespace Rapicorn {

// == AmbienceImpl ==
AmbienceImpl::AmbienceImpl() :
  normal_background_ ("none"),
  prelight_background_ ("none"),
  impressed_background_ ("none"),
  insensitive_background_ ("none"),
  normal_lighting_ (LIGHTING_UPPER_LEFT),
  prelight_lighting_ (LIGHTING_UPPER_LEFT),
  impressed_lighting_ (LIGHTING_LOWER_RIGHT),
  insensitive_lighting_ (LIGHTING_CENTER),
  normal_shade_ (LIGHTING_UPPER_LEFT),
  prelight_shade_ (LIGHTING_UPPER_LEFT),
  impressed_shade_ (LIGHTING_LOWER_RIGHT),
  insensitive_shade_ (LIGHTING_CENTER)
{}

AmbienceImpl::~AmbienceImpl()
{}

void
AmbienceImpl::background (const String &color)
{
  insensitive_background (color);
  prelight_background (color);
  impressed_background (color);
  normal_background (color);
}

void
AmbienceImpl::lighting (LightingType sh)
{
  insensitive_lighting (sh);
  prelight_lighting (sh);
  impressed_lighting (sh);
  normal_lighting (sh);
}

void
AmbienceImpl::shade (LightingType sh)
{
  insensitive_shade (LIGHTING_NONE);
  prelight_shade (LIGHTING_NONE);
  impressed_shade (LIGHTING_NONE);
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
AmbienceImpl::prelight_background (const String &color)
{
  prelight_background_ = color;
  expose();
  changed ("prelight_background");
}

String
AmbienceImpl::prelight_background () const
{
  return prelight_background_;
}

void
AmbienceImpl::impressed_background (const String &color)
{
  impressed_background_ = color;
  expose();
  changed ("impressed_background");
}

String
AmbienceImpl::impressed_background () const
{
  return impressed_background_;
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
AmbienceImpl::prelight_lighting (LightingType sh)
{
  prelight_lighting_ = sh;
  expose();
  changed ("prelight_lighting");
}

LightingType
AmbienceImpl::prelight_lighting () const
{
  return prelight_lighting_;
}

void
AmbienceImpl::impressed_lighting (LightingType sh)
{
  impressed_lighting_ = sh;
  expose();
  changed ("impressed_lighting");
}

LightingType
AmbienceImpl::impressed_lighting () const
{
  return impressed_lighting_;
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
AmbienceImpl::prelight_shade (LightingType sh)
{
  prelight_shade_ = sh;
  expose();
  changed ("prelight_shade");
}

LightingType
AmbienceImpl::prelight_shade () const
{
  return prelight_shade_;
}

void
AmbienceImpl::impressed_shade (LightingType sh)
{
  impressed_shade_ = sh;
  expose();
  changed ("impressed_shade");
}

LightingType
AmbienceImpl::impressed_shade () const
{
  return impressed_shade_;
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
  bool bimpressed = ancestry_impressed(), bprelight = ancestry_prelight();
  /* render background */
  String background_color;
  if (bimpressed)
    background_color = impressed_background();
  else if (insensitive())
    background_color = insensitive_background();
  else if (bprelight)
    background_color = prelight_background();
  else
    background_color = normal_background();
  Color background = heritage()->resolve_color (background_color, STATE_NORMAL, COLOR_BACKGROUND);
  cairo_t *cr = cairo_context (rcontext, rect);
  CPainter painter (cr);
  if (background)
    painter.draw_filled_rect (x, y, width, height, background);
  /* render lighting (mutually exclusive) */
  if (bimpressed && impressed_lighting())
    render_shade (cr, x, y, width, height, impressed_lighting());
  else if (insensitive() && insensitive_lighting())
    render_shade (cr, x, y, width, height, insensitive_lighting());
  else if (bprelight && prelight_lighting())
    render_shade (cr, x, y, width, height, prelight_lighting());
  else if (normal_lighting() && !bimpressed && !insensitive() && !bprelight)
    render_shade (cr, x, y, width, height, normal_lighting());
  /* render shade (combinatoric) */
  if (bimpressed && impressed_shade())
    render_shade (cr, x, y, width, height, impressed_shade());
  if (insensitive() && insensitive_shade())
    render_shade (cr, x, y, width, height, insensitive_shade());
  if (bprelight && prelight_shade())
    render_shade (cr, x, y, width, height, prelight_shade());
  if (!bimpressed && !insensitive() && !bprelight && normal_shade())
    render_shade (cr, x, y, width, height, normal_shade());
}

static const WidgetFactory<AmbienceImpl> ambience_factory ("Rapicorn::Factory::Ambience");

// == FrameImpl ==
FrameImpl::FrameImpl() :
  normal_frame_ (FRAME_ETCHED_IN),
  impressed_frame_ (FRAME_ETCHED_IN),
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
                          (impressed_frame_ == FRAME_FOCUS || impressed_frame_ == FRAME_NONE));
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
FrameImpl::impressed_frame () const
{
  return impressed_frame_;
}

void
FrameImpl::impressed_frame (FrameType ft)
{
  if (impressed_frame_ != ft)
    {
      impressed_frame_ = ft;
      expose_enclosure();
      changed ("impressed_frame");
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
  impressed_frame (ft);
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
  return ancestry_impressed() ? impressed_frame() : normal_frame();
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

static const WidgetFactory<FrameImpl> frame_factory ("Rapicorn::Factory::Frame");

// == FocusFrameImpl ==
FocusFrameImpl::FocusFrameImpl() :
  focus_frame_ (FRAME_FOCUS),
  client_ (NULL), conid_client_ (0)
{}

FocusFrameImpl::~FocusFrameImpl ()
{}

void
FocusFrameImpl::client_changed (const String &name)
{
  if (name == "flags" or name.empty())
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
  if (client_)
    {
      client_->sig_changed() -= conid_client_;
      conid_client_ = 0;
      client_->unregister_focus_frame (*this);
    }
  client_ = NULL;
  this->FrameImpl::hierarchy_changed (old_toplevel);
  if (anchored())
    {
      Client *client = parent_interface<Client*>();
      if (client && client->register_focus_frame (*this))
        client_ = client;
      if (client_)
        conid_client_ = client_->sig_changed() += Aida::slot (*this, &FocusFrameImpl::client_changed);
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
  in_focus |= client_ && client_->has_focus();
  ContainerImpl *cclient_ = container_cast (client_);
  in_focus |= cclient_ && cclient_->get_focus_child() != NULL;
  if (in_focus)
    return focus_frame();
  return ancestry_impressed() ? impressed_frame() : normal_frame();
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

static const WidgetFactory<FocusFrameImpl> focus_frame_factory ("Rapicorn::Factory::FocusFrame");

} // Rapicorn
