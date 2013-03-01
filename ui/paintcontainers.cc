// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "paintcontainers.hh"
#include "container.hh"
#include "painter.hh"
#include "factory.hh"

namespace Rapicorn {

void
Ambience::background (const String &color)
{
  insensitive_background (color);
  prelight_background (color);
  impressed_background (color);
  normal_background (color);
}

void
Ambience::lighting (LightingType sh)
{
  insensitive_lighting (sh);
  prelight_lighting (sh);
  impressed_lighting (sh);
  normal_lighting (sh);
}

void
Ambience::shade (LightingType sh)
{
  insensitive_shade (LIGHTING_NONE);
  prelight_shade (LIGHTING_NONE);
  impressed_shade (LIGHTING_NONE);
  normal_shade (sh);
}

const PropertyList&
Ambience::_property_list()
{
  static Property *properties[] = {
    MakeProperty (Ambience, insensitive_background, _("Insensitive Background"), _("The kind of background painted when insensitive"), "rw"),
    MakeProperty (Ambience, prelight_background, _("Prelight Background"), _("The kind of background painted when prelight"), "rw"),
    MakeProperty (Ambience, impressed_background, _("Impressed Background"), _("The kind of background painted when impressed"), "rw"),
    MakeProperty (Ambience, normal_background, _("Normal Background"), _("The kind of background painted when normal"), "rw"),
    MakeProperty (Ambience, insensitive_lighting, _("Insensitive Lighting"), _("The kind of lighting painted when insensitive"), "rw"),
    MakeProperty (Ambience, prelight_lighting, _("Prelight Lighting"), _("The kind of lighting painted when prelight"), "rw"),
    MakeProperty (Ambience, impressed_lighting, _("Impressed Lighting"), _("The kind of lighting painted when impressed"), "rw"),
    MakeProperty (Ambience, normal_lighting, _("Normal Lighting"), _("The kind of lighting painted when normal"), "rw"),
    MakeProperty (Ambience, insensitive_shade, _("Insensitive Shade"), _("The kind of shade painted when insensitive"), "rw"),
    MakeProperty (Ambience, prelight_shade, _("Prelight Shade"), _("The kind of shade painted when prelight"), "rw"),
    MakeProperty (Ambience, impressed_shade, _("Impressed Shade"), _("The kind of shade painted when impressed"), "rw"),
    MakeProperty (Ambience, normal_shade, _("Normal Shade"), _("The kind of shade painted when normal"), "rw"),
    MakeProperty (Ambience, background, _("Background"), _("The kind of background painted for all modes"), "wo"),
    MakeProperty (Ambience, lighting, _("Lighting"), _("The kind of lighting painted for all modes"), "wo"),
    MakeProperty (Ambience, shade, _("Shade"), _("The kind of shade painted for all modes"), "wo"),
  };
  static const PropertyList property_list (properties, ContainerImpl::_property_list());
  return property_list;
}

class AmbienceImpl : public virtual SingleContainerImpl, public virtual Ambience {
  String insensitive_background_, prelight_background_, impressed_background_, normal_background_;
  LightingType insensitive_lighting_, prelight_lighting_, impressed_lighting_, normal_lighting_;
  LightingType insensitive_shade_, prelight_shade_, impressed_shade_, normal_shade_;
public:
  explicit AmbienceImpl() :
    insensitive_background_ ("none"),
    prelight_background_ ("none"),
    impressed_background_ ("none"),
    normal_background_ ("none"),
    insensitive_lighting_ (LIGHTING_CENTER),
    prelight_lighting_ (LIGHTING_UPPER_LEFT),
    impressed_lighting_ (LIGHTING_LOWER_RIGHT),
    normal_lighting_ (LIGHTING_UPPER_LEFT),
    insensitive_shade_ (LIGHTING_CENTER),
    prelight_shade_ (LIGHTING_UPPER_LEFT),
    impressed_shade_ (LIGHTING_LOWER_RIGHT),
    normal_shade_ (LIGHTING_UPPER_LEFT)
  {}
  ~AmbienceImpl()
  {}
  virtual void          insensitive_background  (const String &color) { insensitive_background_ = color; expose(); }
  virtual String        insensitive_background  () const              { return insensitive_background_; }
  virtual void          prelight_background     (const String &color) { prelight_background_ = color; expose(); }
  virtual String        prelight_background     () const              { return prelight_background_; }
  virtual void          impressed_background    (const String &color) { impressed_background_ = color; expose(); }
  virtual String        impressed_background    () const              { return impressed_background_; }
  virtual void          normal_background       (const String &color) { normal_background_ = color; expose(); }
  virtual String        normal_background       () const              { return normal_background_; }
  virtual void          insensitive_lighting    (LightingType sh)     { insensitive_lighting_ = sh; expose(); }
  virtual LightingType  insensitive_lighting    () const              { return insensitive_lighting_; }
  virtual void          prelight_lighting       (LightingType sh)     { prelight_lighting_ = sh; expose(); }
  virtual LightingType  prelight_lighting       () const              { return prelight_lighting_; }
  virtual void          impressed_lighting      (LightingType sh)     { impressed_lighting_ = sh; expose(); }
  virtual LightingType  impressed_lighting      () const              { return impressed_lighting_; }
  virtual void          normal_lighting         (LightingType sh)     { normal_lighting_ = sh; expose(); }
  virtual LightingType  normal_lighting         () const              { return normal_lighting_; }
  virtual void          insensitive_shade       (LightingType sh)     { insensitive_shade_ = sh; expose(); }
  virtual LightingType  insensitive_shade       () const              { return insensitive_shade_; }
  virtual void          prelight_shade          (LightingType sh)     { prelight_shade_ = sh; expose(); }
  virtual LightingType  prelight_shade          () const              { return prelight_shade_; }
  virtual void          impressed_shade         (LightingType sh)     { impressed_shade_ = sh; expose(); }
  virtual LightingType  impressed_shade         () const              { return impressed_shade_; }
  virtual void          normal_shade            (LightingType sh)     { normal_shade_ = sh; expose(); }
  virtual LightingType  normal_shade            () const              { return normal_shade_; }
private:
  void
  render_shade (cairo_t      *cairo,
                int           x,
                int           y,
                int           width,
                int           height,
                LightingType  st)
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
public:
  virtual void
  render (RenderContext &rcontext, const Rect &rect)
  {
    IRect ia = allocation();
    const int x = ia.x, y = ia.y, width = ia.width, height = ia.height;
    bool bimpressed = branch_impressed(), bprelight = branch_prelight();
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
};
static const WidgetFactory<AmbienceImpl> ambience_factory ("Rapicorn::Factory::Ambience");

void
Frame::frame_type (FrameType ft)
{
  normal_frame (ft);
  impressed_frame (ft);
}

const PropertyList&
Frame::_property_list()
{
  static Property *properties[] = {
    MakeProperty (Frame, normal_frame,    _("Normal Frame"),   _("The kind of frame to draw in normal state"), "rw"),
    MakeProperty (Frame, impressed_frame, _("Impresed Frame"), _("The kind of frame to draw in impressed state"), "rw"),
    MakeProperty (Frame, frame_type,      _("Frame Type"),     _("The kind of frame to draw in all states"), "w"),
    MakeProperty (Frame, overlap_child,   _("Overlap Child"),  _("Draw frame in the same position as child"), "rw"),
    MakeProperty (Frame, tight_focus,     _("Tight Focus"),    _("Prevent extra padding around focus frames"), "rw"),
  };
  static const PropertyList property_list (properties, ContainerImpl::_property_list());
  return property_list;
}

class FrameImpl : public virtual SingleContainerImpl, public virtual Frame {
  FrameType normal_frame_, impressed_frame_;
  bool      overlap_child_, tight_focus_;
  bool
  is_tight_focus()
  {
    return tight_focus_ && ((normal_frame_ == FRAME_FOCUS || normal_frame_ == FRAME_NONE) &&
                             (impressed_frame_ == FRAME_FOCUS || impressed_frame_ == FRAME_NONE));
  }
public:
  explicit FrameImpl() :
    normal_frame_ (FRAME_ETCHED_IN),
    impressed_frame_ (FRAME_ETCHED_IN),
    overlap_child_ (false), tight_focus_ (false)
  {}
  ~FrameImpl()
  {}
protected:
  virtual void
  do_changed ()
  {
    SingleContainerImpl::do_changed();
    if (overlap_child())
      expose();
    else
      expose_enclosure();
  }
  virtual void          normal_frame    (FrameType ft)  { normal_frame_ = ft; changed(); }
  virtual FrameType     normal_frame    () const        { return normal_frame_; }
  virtual void          impressed_frame (FrameType ft)  { impressed_frame_ = ft; changed(); }
  virtual FrameType     impressed_frame () const        { return impressed_frame_; }
  virtual bool          overlap_child   () const        { return overlap_child_; }
  virtual void          overlap_child   (bool ovc)      { overlap_child_ = ovc; invalidate(); changed(); }
  virtual bool          tight_focus     () const        { return tight_focus_; }
  virtual void          tight_focus     (bool tf)       { tight_focus_ = tf; invalidate(); changed(); }
  virtual FrameType     current_frame   () const        { return branch_impressed() ? impressed_frame() : normal_frame(); }
  virtual void
  size_request (Requisition &requisition)
  {
    SingleContainerImpl::size_request (requisition);
    int thickness = is_tight_focus() ? 1 : 2;
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
  virtual void
  size_allocate (Allocation area, bool changed)
  {
    if (has_visible_child())
      {
        Allocation carea = area;
        if (!overlap_child_)
          {
            int thickness = is_tight_focus() ? 1 : 2;
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
public:
  virtual void
  render (RenderContext &rcontext, const Rect &rect)
  {
    IRect ia = allocation();
    int x = ia.x, y = ia.y, width = ia.width, height = ia.height;
    int thickness = is_tight_focus() ? 1 : 2;
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
};
static const WidgetFactory<FrameImpl> frame_factory ("Rapicorn::Factory::Frame");

const PropertyList&
FocusFrame::_property_list()
{
  static Property *properties[] = {
    MakeProperty (FocusFrame, focus_frame, _("Focus Frame"), _("The kind of frame to draw in focus state"), "rw"),
  };
  static const PropertyList property_list (properties, Frame::_property_list());
  return property_list;
}

class FocusFrameImpl : public virtual FrameImpl, public virtual FocusFrame {
  FrameType focus_frame_;
  Client   *client_;
  size_t    conid_client_;
  virtual void
  set_focus_child (WidgetImpl *widget)
  {
    FrameImpl::set_focus_child (widget);
    expose_enclosure();
  }
  void
  client_changed()
  {
    if (overlap_child())
      expose();
    else
      expose_enclosure();
  }
  virtual void
  hierarchy_changed (WidgetImpl *old_toplevel)
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
protected:
  virtual void          focus_frame     (FrameType ft)  { focus_frame_ = ft; changed(); }
  virtual FrameType     focus_frame     () const        { return focus_frame_; }
  virtual FrameType
  current_frame () const
  {
    bool in_focus = has_focus();
    in_focus |= get_focus_child() != NULL;
    in_focus |= client_ && client_->has_focus();
    ContainerImpl *cclient_ = dynamic_cast<ContainerImpl*> (client_);
    in_focus |= cclient_ && cclient_->get_focus_child() != NULL;
    if (in_focus)
      return focus_frame();
    return branch_impressed() ? impressed_frame() : normal_frame();
  }
public:
  explicit FocusFrameImpl() :
    focus_frame_ (FRAME_FOCUS),
    client_ (NULL), conid_client_ (0)
  {}
};
static const WidgetFactory<FocusFrameImpl> focus_frame_factory ("Rapicorn::Factory::FocusFrame");

} // Rapicorn
