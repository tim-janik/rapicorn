/* Rapicorn
 * Copyright (C) 2005 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "paintcontainers.hh"
#include "containerimpl.hh"
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
  insensitive_lighting (LIGHTING_NONE);
  prelight_lighting (LIGHTING_NONE);
  impressed_lighting (LIGHTING_NONE);
  normal_lighting (sh);
}

class AmbienceImpl : public virtual SingleContainerImpl, public virtual Ambience {
  String m_insensitive_background, m_prelight_background, m_impressed_background, m_normal_background;
  LightingType m_insensitive_lighting, m_prelight_lighting, m_impressed_lighting, m_normal_lighting;
  LightingType m_insensitive_shade, m_prelight_shade, m_impressed_shade, m_normal_shade;
public:
  explicit AmbienceImpl() :
    m_insensitive_background ("none"),
    m_prelight_background ("none"),
    m_impressed_background ("none"),
    m_normal_background ("none"),
    m_insensitive_lighting (LIGHTING_CENTER),
    m_prelight_lighting (LIGHTING_UPPER_LEFT),
    m_impressed_lighting (LIGHTING_LOWER_RIGHT),
    m_normal_lighting (LIGHTING_UPPER_LEFT),
    m_insensitive_shade (LIGHTING_CENTER),
    m_prelight_shade (LIGHTING_UPPER_LEFT),
    m_impressed_shade (LIGHTING_LOWER_RIGHT),
    m_normal_shade (LIGHTING_UPPER_LEFT)
  {}
  ~AmbienceImpl()
  {}
  virtual void          insensitive_background  (const String &color) { m_insensitive_background = color; expose(); }
  virtual String        insensitive_background  () const              { return m_insensitive_background; }
  virtual void          prelight_background     (const String &color) { m_prelight_background = color; expose(); }
  virtual String        prelight_background     () const              { return m_prelight_background; }
  virtual void          impressed_background    (const String &color) { m_impressed_background = color; expose(); }
  virtual String        impressed_background    () const              { return m_impressed_background; }
  virtual void          normal_background       (const String &color) { m_normal_background = color; expose(); }
  virtual String        normal_background       () const              { return m_normal_background; }
  virtual void          insensitive_lighting    (LightingType sh)     { m_insensitive_lighting = sh; expose(); }
  virtual LightingType  insensitive_lighting    () const              { return m_insensitive_lighting; }
  virtual void          prelight_lighting       (LightingType sh)     { m_prelight_lighting = sh; expose(); }
  virtual LightingType  prelight_lighting       () const              { return m_prelight_lighting; }
  virtual void          impressed_lighting      (LightingType sh)     { m_impressed_lighting = sh; expose(); }
  virtual LightingType  impressed_lighting      () const              { return m_impressed_lighting; }
  virtual void          normal_lighting         (LightingType sh)     { m_normal_lighting = sh; expose(); }
  virtual LightingType  normal_lighting         () const              { return m_normal_lighting; }
  virtual void          insensitive_shade       (LightingType sh)     { m_insensitive_shade = sh; expose(); }
  virtual LightingType  insensitive_shade       () const              { return m_insensitive_shade; }
  virtual void          prelight_shade          (LightingType sh)     { m_prelight_shade = sh; expose(); }
  virtual LightingType  prelight_shade          () const              { return m_prelight_shade; }
  virtual void          impressed_shade         (LightingType sh)     { m_impressed_shade = sh; expose(); }
  virtual LightingType  impressed_shade         () const              { return m_impressed_shade; }
  virtual void          normal_shade            (LightingType sh)     { m_normal_shade = sh; expose(); }
  virtual LightingType  normal_shade            () const              { return m_normal_shade; }
private:
  Color
  resolve_color (const String &color)
  {
    EnumTypeColorType ect;
    if (color[0] == '#')
      {
        uint32 argb = string_to_int (&color[1], 16);
        /* invert alpha */
        Color c (argb);
        c.alpha (0xff - c.alpha());
        return c;
        printf ("color: %s => 0x%08x (mag: 0x%08x)\n", &color[1], argb,
                style()->color (state(), ColorType (ect.find_first("magenta")->value)).argb());
        return argb;
      }
    const EnumClass::Value *value = ect.find_first (color);
    if (value)
      return style()->color (state(), ColorType (value->value));
    else
      return style()->color (state(), color, COLOR_BACKGROUND);
  }
  void
  render_shade (Plane        &plane,
                Affine        affine,
                int           x,
                int           y,
                int           width,
                int           height,
                LightingType     st)
  {
    int shade_alpha = 0x3b;
    Color light = light_glint().shade (shade_alpha), dark = dark_glint().shade (shade_alpha);
    LightingType dark_flag = st & LIGHTING_DARK_FLAG;
    Painter painter (plane);
    if (dark_flag)
      swap (light, dark);
    switch (st & ~LIGHTING_DARK_FLAG)
      {
      case LIGHTING_UPPER_LEFT:
        painter.draw_shaded_rect (x, y + height - 1, light, x + width - 1, y, dark);
        break;
      case LIGHTING_UPPER_RIGHT:
        painter.draw_shaded_rect (x + width - 1, y + height - 1, light, x, y, dark);
        break;
      case LIGHTING_LOWER_LEFT:
        painter.draw_shaded_rect (x, y, light, x + width - 1, y + height - 1, dark);
        break;
      case LIGHTING_LOWER_RIGHT:
        painter.draw_shaded_rect (x + width - 1, y, light, x, y + height - 1, dark);
        break;
      case LIGHTING_CENTER:
        render_shade (plane, affine, x, y, width / 2, height / 2, LIGHTING_UPPER_RIGHT | dark_flag);
        render_shade (plane, affine, x, y + height / 2, width / 2, height / 2, LIGHTING_LOWER_RIGHT | dark_flag);
        render_shade (plane, affine, x + width / 2, y + height / 2, width / 2, height / 2, LIGHTING_LOWER_LEFT | dark_flag);
        render_shade (plane, affine, x + width / 2, y, width / 2, height / 2, LIGHTING_UPPER_LEFT | dark_flag);
        break;
      case LIGHTING_DIFFUSE:
        painter.draw_shaded_rect (x, y, light, x + width - 1, y + height - 1, light);
        break;
      case LIGHTING_NONE:
        break;
      }
  }
public:
  void
  render (Plane        &plane,
          Affine        affine)
  {
    int x = allocation().x, y = allocation().y, width = allocation().width, height = allocation().height;
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
    Color background = resolve_color (background_color);
    Painter painter (plane);
    if (background)
      painter.draw_filled_rect (x, y, width, height, background);
    /* render lighting (mutually exclusive) */
    if (bimpressed && impressed_lighting())
      {
        Plane shade = Plane::create_from_size (plane);
        render_shade (shade, affine, x, y, width, height, impressed_lighting());
        plane.combine (shade, COMBINE_OVER);
      }
    else if (insensitive() && insensitive_lighting())
      {
        Plane shade = Plane::create_from_size (plane);
        render_shade (shade, affine, x, y, width, height, insensitive_lighting());
        plane.combine (shade, COMBINE_OVER);
      }
    else if (bprelight && prelight_lighting())
      {
        Plane shade = Plane::create_from_size (plane);
        render_shade (shade, affine, x, y, width, height, prelight_lighting());
        plane.combine (shade, COMBINE_OVER);
      }
    else if (normal_lighting() && !bimpressed && !insensitive() && !bprelight)
      {
        Plane shade = Plane::create_from_size (plane);
        render_shade (shade, affine, x, y, width, height, normal_lighting());
        plane.combine (shade, COMBINE_OVER);
      }
    /* render shade (combinatoric) */
    if (bimpressed && impressed_shade())
      {
        Plane shade = Plane::create_from_size (plane);
        render_shade (shade, affine, x, y, width, height, impressed_shade());
        plane.combine (shade, COMBINE_OVER);
      }
    if (insensitive() && insensitive_shade())
      {
        Plane shade = Plane::create_from_size (plane);
        render_shade (shade, affine, x, y, width, height, insensitive_shade());
        plane.combine (shade, COMBINE_OVER);
      }
    if (bprelight && prelight_shade())
      {
        Plane shade = Plane::create_from_size (plane);
        render_shade (shade, affine, x, y, width, height, prelight_shade());
        plane.combine (shade, COMBINE_OVER);
      }
    if (!bimpressed && !insensitive() && !bprelight && normal_shade())
      {
        Plane shade = Plane::create_from_size (plane);
        render_shade (shade, affine, x, y, width, height, normal_shade());
        plane.combine (shade, COMBINE_OVER);
      }
    SingleContainerImpl::render (plane, affine);
  }
protected:
  virtual const PropertyList&
  list_properties()
  {
    static Property *properties[] = {
      MakeProperty (Ambience, insensitive_background, _("Insensitive Background"), _("The kind of background painted when insensitive"), "none", "rw"),
      MakeProperty (Ambience, prelight_background, _("Prelight Background"), _("The kind of background painted when prelight"), "none", "rw"),
      MakeProperty (Ambience, impressed_background, _("Impressed Background"), _("The kind of background painted when impressed"), "none", "rw"),
      MakeProperty (Ambience, normal_background, _("Normal Background"), _("The kind of background painted when normal"), "none", "rw"),
      MakeProperty (Ambience, insensitive_lighting, _("Insensitive Lighting"), _("The kind of lighting painted when insensitive"), LIGHTING_NONE, "rw"),
      MakeProperty (Ambience, prelight_lighting, _("Prelight Lighting"), _("The kind of lighting painted when prelight"), LIGHTING_NONE, "rw"),
      MakeProperty (Ambience, impressed_lighting, _("Impressed Lighting"), _("The kind of lighting painted when impressed"), LIGHTING_NONE, "rw"),
      MakeProperty (Ambience, normal_lighting, _("Normal Lighting"), _("The kind of lighting painted when normal"), LIGHTING_NONE, "rw"),
      MakeProperty (Ambience, insensitive_shade, _("Insensitive Shade"), _("The kind of shade painted when insensitive"), LIGHTING_NONE, "rw"),
      MakeProperty (Ambience, prelight_shade, _("Prelight Shade"), _("The kind of shade painted when prelight"), LIGHTING_NONE, "rw"),
      MakeProperty (Ambience, impressed_shade, _("Impressed Shade"), _("The kind of shade painted when impressed"), LIGHTING_NONE, "rw"),
      MakeProperty (Ambience, normal_shade, _("Normal Shade"), _("The kind of shade painted when normal"), LIGHTING_NONE, "rw"),
      MakeProperty (Ambience, background, _("Background"), _("The kind of background painted for all modes"), "none", "rw"),
      MakeProperty (Ambience, lighting, _("Lighting"), _("The kind of lighting painted for all modes"), LIGHTING_NONE, "rw"),
      MakeProperty (Ambience, shade, _("Shade"), _("The kind of shade painted for all modes"), LIGHTING_NONE, "rw"),
    };
    static const PropertyList property_list (properties, SingleContainerImpl::list_properties());
    return property_list;
  }
};
static const ItemFactory<AmbienceImpl> ambience_factory ("Rapicorn::Ambience");

void
Frame::frame_type (FrameType ft)
{
  normal_frame (ft);
  impressed_frame (ft);
}

class FrameImpl : public virtual SingleContainerImpl, public virtual Frame {
  FrameType m_normal_frame, m_impressed_frame;
public:
  explicit FrameImpl() :
    m_normal_frame (FRAME_ETCHED_IN),
    m_impressed_frame (FRAME_ETCHED_IN)
  {
    set_flag (EXPOSE_ON_CHANGE, true);
  }
  ~FrameImpl()
  {}
  virtual void          normal_frame    (FrameType ft)  { m_normal_frame = ft; invalidate(); }
  virtual FrameType     normal_frame    () const        { return m_normal_frame; }
  virtual void          impressed_frame (FrameType ft)  { m_impressed_frame = ft; invalidate(); }
  virtual FrameType     impressed_frame () const        { return m_impressed_frame; }
  FrameType             current_frame   () const        { return branch_impressed() ? impressed_frame() : normal_frame(); }
protected:
  virtual void
  size_request (Requisition &requisition)
  {
    bool chspread = false, cvspread = false;
    if (has_visible_child())
      {
        Item &child = get_child();
        Requisition cr = child.size_request ();
        requisition.width += cr.width;
        requisition.height += cr.height;
        chspread = child.hspread();
        cvspread = child.vspread();
      }
    set_flag (HSPREAD_CONTAINER, chspread);
    set_flag (VSPREAD_CONTAINER, cvspread);
    if (current_frame() != FRAME_NONE)
      {
        requisition.width += 2 + 2;
        requisition.height += 2 + 2;
      }
  }
  virtual void
  size_allocate (Allocation area)
  {
    allocation (area);
    if (has_children())
      {
        Item &child = get_child();
        if (child.visible())
          {
            if (current_frame() != FRAME_NONE)
              {
                area.x += 2;
                area.y += 2;
                area.width -= 4;
                area.height -= 4;
              }
            child.set_allocation (area);
          }
      }
  }
public:
  void
  render (Plane        &plane,
          Affine        affine)
  {
    int x = allocation().x, y = allocation().y, width = allocation().width, height = allocation().height;
    if (width >= 2 && height >= 2)
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
        vector<int> dashes;
#if 1
        dashes.push_back (3);
        dashes.push_back (2);
#else
        dashes.push_back (5);
        dashes.push_back (4);
        dashes.push_back (4);
        dashes.push_back (4);
        dashes.push_back (3);
        dashes.push_back (4);
        dashes.push_back (2);
        dashes.push_back (4);
        dashes.push_back (1);
        dashes.push_back (4);
#endif
        Painter painter (plane);
        if (outer_upper_left || inner_upper_left || inner_lower_right || outer_lower_right)
          painter.draw_shadow (x, y, width, height, outer_upper_left, inner_upper_left, inner_lower_right, outer_lower_right);
        if (border1)
          painter.draw_border (x, y, width, height, border1, dashes);
        if (border2)
          painter.draw_border (x + 1, y + 1, width - 2, height - 2, border2, dashes);
      }
    SingleContainerImpl::render (plane, affine);
  }
  virtual const PropertyList&
  list_properties()
  {
    static Property *properties[] = {
      MakeProperty (Frame, normal_frame, _("Normal Frame"), _("The kind of frame to draw in normal state"), FRAME_ETCHED_IN, "rw"),
      MakeProperty (Frame, impressed_frame, _("Impresed Frame"), _("The kind of frame to draw in impressed state"), FRAME_ETCHED_IN, "rw"),
      MakeProperty (Frame, frame_type, _("Frame Type"), _("The kind of frame to draw in all states"), FRAME_ETCHED_IN, "w"),
    };
    static const PropertyList property_list (properties, Container::list_properties());
    return property_list;
  }
};
static const ItemFactory<FrameImpl> frame_factory ("Rapicorn::Frame");

ButtonView::ButtonView () :
  sig_clicked (*this, &ButtonView::do_clicked)
{}

bool
ButtonView::pressed()
{
  return false;
}

void
ButtonView::do_clicked()
{}

class ButtonViewImpl : public virtual SingleContainerImpl, public virtual ButtonView {
  uint press_button;
public:
  explicit ButtonViewImpl() :
    press_button (0)
  {}
  ~ButtonViewImpl()
  {}
protected:
  virtual bool
  do_event (const Event &event)
  {
    bool handled = false, proper_release = false;
    switch (event.type)
      {
      case MOUSE_ENTER:
        impressed (press_button != 0 || pressed());
        prelight (true);
        break;
      case MOUSE_LEAVE:
        prelight (false);
        impressed (pressed());
        break;
      case BUTTON_PRESS:
      case BUTTON_2PRESS:
      case BUTTON_3PRESS:
        if (!press_button && event.button == 1)
          {
            press_button = event.button;
            impressed (true);
            root()->add_grab (*this);
            handled = true;
          }
        break;
      case BUTTON_RELEASE:
      case BUTTON_2RELEASE:
      case BUTTON_3RELEASE:
        proper_release = true;
      case BUTTON_CANCELED:
        if (press_button == event.button)
          {
            bool inbutton = prelight();
            root()->remove_grab (*this);
            press_button = 0;
            impressed (pressed());
            if (proper_release && inbutton)
              diag ("button-clicked: impressed=%d (event.button=%d)", impressed(), event.button);
            handled = true;
          }
        break;
      case KEY_PRESS:
      case KEY_RELEASE:
        break;
      case FOCUS_IN:
      case FOCUS_OUT:
        break;
      default: break;
      }
    return handled;
  }
public:
  virtual const PropertyList&
  list_properties()
  {
    static Property *properties[] = {
    };
    static const PropertyList property_list (properties, Container::list_properties());
    return property_list;
  }
};
static const ItemFactory<ButtonViewImpl> button_view_factory ("Rapicorn::ButtonView");

} // Rapicorn
