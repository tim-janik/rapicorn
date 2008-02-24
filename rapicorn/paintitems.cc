/* Rapicorn
 * Copyright (C) 2006 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "paintitems.hh"
#include "itemimpl.hh"
#include "factory.hh"
#include "painter.hh"

namespace Rapicorn {

static DataKey<SizePolicyType> size_policy_key;

const PropertyList&
Arrow::list_properties()
{
  static Property *properties[] = {
    MakeProperty (Arrow, arrow_dir,   _("Arrow Direction"), _("The direction the arrow points to"), "rw"),
    MakeProperty (Arrow, size_policy, _("Size Policy"),     _("Policy which determines coupling of width and height"), "rw"),
  };
  static const PropertyList property_list (properties, Item::list_properties());
  return property_list;
}

class ArrowImpl : public virtual ItemImpl, public virtual Arrow {
  DirType m_dir;
public:
  explicit ArrowImpl() :
    m_dir (DIR_RIGHT)
  {}
  ~ArrowImpl()
  {}
  virtual void    arrow_dir (DirType dir)       { m_dir = dir; expose(); }
  virtual DirType arrow_dir () const            { return m_dir; }
  virtual void
  size_policy (SizePolicyType spol)
  {
    if (!spol)
      delete_data (&size_policy_key);
    else
      set_data (&size_policy_key, spol);
  }
  virtual SizePolicyType
  size_policy () const
  {
    SizePolicyType spol = get_data (&size_policy_key);
    return spol;
  }
protected:
  virtual void
  size_request (Requisition &requisition)
  {
    requisition.width = 3;
    requisition.height = 3;
  }
  virtual void
  size_allocate (Allocation area)
  {
    allocation (area);
    SizePolicyType spol = size_policy();
    if (spol == SIZE_POLICY_WIDTH_FROM_HEIGHT)
      tune_requisition (area.height, -1);
    else if (spol == SIZE_POLICY_HEIGHT_FROM_WIDTH)
      tune_requisition (-1, area.width);
  }
  virtual void
  render (Display &display)
  {
    IRect ia = allocation();
    int x = ia.x, y = ia.y, width = ia.width, height = ia.height;
    if (width >= 2 && height >= 2)
      {
        Color color = 0x80000000;
        Plane &plane = display.create_plane();
        Painter painter (plane);
        painter.draw_dir_arrow (x, y, width, height, 0xff000000, m_dir);
      }
  }
};
static const ItemFactory<ArrowImpl> arrow_factory ("Rapicorn::Factory::Arrow");

void
DotGrid::dot_type (FrameType ft)
{
  normal_dot (ft);
  impressed_dot (ft);
}

const PropertyList&
DotGrid::list_properties()
{
  static Property *properties[] = {
    MakeProperty (DotGrid, normal_dot, _("Normal Dot"), _("The kind of dot-frame to draw in normal state"), "rw"),
    MakeProperty (DotGrid, impressed_dot, _("Impresed Dot"), _("The kind of dot-frame to draw in impressed state"), "rw"),
    MakeProperty (DotGrid, dot_type, _("Dot Type"), _("The kind of dot-frame to draw in all states"), "w"),
    MakeProperty (DotGrid, n_hdots, _("H-Dot #"), _("The number of horizontal dots to be drawn"), 0u, 99999u, 3u, "rw"),
    MakeProperty (DotGrid, n_vdots, _("V-Dot #"), _("The number of vertical dots to be drawn"), 0u, 99999u, 3u, "rw"),
    MakeProperty (DotGrid, right_padding_dots, _("Right Padding Dots"), _("Amount of padding in dots to add at the child's right side"), 0, 65535, 3, "rw"),
    MakeProperty (DotGrid, top_padding_dots, _("Top Padding Dots"), _("Amount of padding in dots to add at the child's top side"), 0, 65535, 3, "rw"),
    MakeProperty (DotGrid, left_padding_dots, _("Left Padding Dots"), _("Amount of padding in dots to add at the child's left side"), 0, 65535, 3, "rw"),
    MakeProperty (DotGrid, bottom_padding_dots, _("Bottom Padding Dots"), _("Amount of padding in dots to add at the child's bottom side"), 0, 65535, 3, "rw"),
  };
  static const PropertyList property_list (properties, Item::list_properties());
  return property_list;
}

class DotGridImpl : public virtual ItemImpl, public virtual DotGrid {
  FrameType m_normal_dot, m_impressed_dot;
  uint      m_n_hdots, m_n_vdots;
  uint16    m_right_padding_dots, m_top_padding_dots, m_left_padding_dots, m_bottom_padding_dots;
public:
  explicit DotGridImpl() :
    m_normal_dot (FRAME_IN),
    m_impressed_dot (FRAME_IN),
    m_n_hdots (1), m_n_vdots (1),
    m_right_padding_dots (0), m_top_padding_dots (0),
    m_left_padding_dots (0), m_bottom_padding_dots (0)
  {}
  ~DotGridImpl()
  {}
  virtual void      impressed_dot (FrameType ft)        { m_impressed_dot = ft; expose(); }
  virtual FrameType impressed_dot () const              { return m_impressed_dot; }
  virtual void      normal_dot    (FrameType ft)        { m_normal_dot = ft; expose(); }
  virtual FrameType normal_dot    () const              { return m_normal_dot; }
  FrameType         current_dot   () const              { return branch_impressed() ? impressed_dot() : normal_dot(); }
  virtual void      n_hdots       (uint   num)          { m_n_hdots = num; expose(); }
  virtual uint      n_hdots       () const              { return m_n_hdots; }
  virtual void      n_vdots       (uint   num)          { m_n_vdots = num; expose(); }
  virtual uint      n_vdots       () const              { return m_n_vdots; }
  virtual uint      right_padding_dots () const         { return m_right_padding_dots; }
  virtual void      right_padding_dots (uint c)         { m_right_padding_dots = c; expose(); }
  virtual uint      top_padding_dots  () const          { return m_top_padding_dots; }
  virtual void      top_padding_dots  (uint c)          { m_top_padding_dots = c; expose(); }
  virtual uint      left_padding_dots  () const         { return m_left_padding_dots; }
  virtual void      left_padding_dots  (uint c)         { m_left_padding_dots = c; expose(); }
  virtual uint      bottom_padding_dots  () const       { return m_bottom_padding_dots; }
  virtual void      bottom_padding_dots  (uint c)       { m_bottom_padding_dots = c; expose(); }
  virtual void
  size_request (Requisition &requisition)
  {
    uint ythick = 1, xthick = 1;
    requisition.width = m_n_hdots * (xthick + xthick) + MAX (m_n_hdots - 1, 0) * xthick;
    requisition.height = m_n_vdots * (ythick + ythick) + MAX (m_n_vdots - 1, 0) * ythick;
    requisition.width += (m_right_padding_dots + m_left_padding_dots) * 3 * xthick;
    requisition.height += (m_top_padding_dots + m_bottom_padding_dots) * 3 * ythick;
  }
  virtual void
  size_allocate (Allocation area)
  {
    allocation (area);
  }
  virtual void
  render (Display &display)
  {
    int ythick = 1, xthick = 1, n_hdots = m_n_hdots, n_vdots = m_n_vdots;
    IRect ia = allocation();
    int x = ia.x, y = ia.y, width = ia.width, height = ia.height;
    int rq_width = m_n_hdots * (xthick + xthick) + MAX (n_hdots - 1, 0) * xthick;
    int rq_height = m_n_vdots * (ythick + ythick) + MAX (n_vdots - 1, 0) * ythick;
    /* split up extra width */
    uint hpadding = m_right_padding_dots + m_left_padding_dots;
    double halign = hpadding ? m_left_padding_dots * 1.0 / hpadding : 0.5;
    if (rq_width < width)
      x += ifloor ((width - rq_width) * halign);
    /* split up extra height */
    uint vpadding = m_top_padding_dots + m_bottom_padding_dots;
    double valign = vpadding ? m_bottom_padding_dots * 1.0 / vpadding : 0.5;
    if (rq_height < height)
      y += ifloor ((height - rq_height) * valign);
    /* draw dots */
    if (width >= 2 * xthick && height >= 2 * ythick && n_hdots && n_vdots)
      {
        /* limit n_hdots */
        if (rq_width > width)
          {
            int w = width - 2 * xthick;         /* dot1 */
            n_hdots = 1 + w / (3 * xthick);
          }
        /* limit n_vdots */
        if (rq_height > height)
          {
            int h = height - 2 * ythick;        /* dot1 */
            n_vdots = 1 + h / (3 * ythick);
          }
        Color color = 0x80000000;
        Plane &plane = display.create_plane();
        Painter rp (plane);
        for (int j = 0; j < n_vdots; j++)
          {
            int xtmp = 0;
            for (int i = 0; i < n_hdots; i++)
              {
                rp.draw_shaded_rect (x + xtmp, y + 2 * ythick - 1, dark_shadow(),
                                     x + xtmp + 2 * xthick - 1, y, light_glint());
                xtmp += 3 * xthick;
              }
            y += 3 * ythick;
          }
      }
  }
};
static const ItemFactory<DotGridImpl> dot_grid_factory ("Rapicorn::Factory::DotGrid");

/* --- DrawableImpl --- */
Drawable::Drawable() :
  sig_draw (*this, &Drawable::draw)
{}

class DrawableImpl : public virtual ItemImpl, public virtual Drawable {
public:
  virtual void
  size_request (Requisition &requisition)
  {
    requisition.width = 320;
    requisition.height = 200;
  }
  virtual void
  size_allocate (Allocation area)
  {
    allocation (area);
  }
  virtual void
  render (Display &display)
  {
    sig_draw.emit (display);
  }
  virtual void
  draw (Display &display)
  {}
};
static const ItemFactory<DrawableImpl> drawable_factory ("Rapicorn::Factory::Drawable");

} // Rapicorn
