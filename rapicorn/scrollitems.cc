/* Rapicorn
 * Copyright (C) 2006 Tim Janik
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
#include "scrollitemsimpl.hh"

namespace Rapicorn {
using namespace std;

/* --- ScrollArea --- */
ScrollArea::ScrollArea()
{}

/* --- ScrollAreaImpl --- */
ScrollAreaImpl::ScrollAreaImpl() :
  m_hadjustment (NULL), m_vadjustment (NULL)
{}

Adjustment&
ScrollAreaImpl::hadjustment () const
{
  if (!m_hadjustment)
    m_hadjustment = Adjustment::create (0, 0, 1, 0.01, 0.2);
  return *m_hadjustment;
}

Adjustment&
ScrollAreaImpl::vadjustment () const
{
  if (!m_vadjustment)
    m_vadjustment = Adjustment::create (0, 0, 1, 0.01, 0.2);
  return *m_vadjustment;
}

Adjustment*
ScrollAreaImpl::get_adjustment (AdjustmentSourceType adj_source,
                                const String        &name)
{
  switch (adj_source)
    {
    case ADJUSTMENT_SOURCE_ANCESTOR_HORIZONTAL:
      return &hadjustment();
    case ADJUSTMENT_SOURCE_ANCESTOR_VERTICAL:
      return &vadjustment();
    default:
      return NULL;
    }
}

double
ScrollAreaImpl::xoffset () const
{
  return hadjustment().value();
}

double
ScrollAreaImpl::yoffset () const
{
  return vadjustment().value();
}

void
ScrollAreaImpl::scroll_to (double x,
                           double y)
{
  hadjustment().value (x);
  vadjustment().value (y);
}

static const ItemFactory<ScrollAreaImpl> scroll_area_factory ("Rapicorn::ScrollArea");

/* --- ScrollPortImpl --- */
class ScrollPortImpl : public virtual SingleContainerImpl {
  Adjustment *m_hadjustment, *m_vadjustment;
  virtual void
  hierarchy_changed (Item *old_toplevel)
  {
    if (m_hadjustment)
      m_hadjustment->sig_value_changed -= slot (*this, &ScrollPortImpl::adjustment_changed);
    m_hadjustment = NULL;
    if (m_vadjustment)
      m_vadjustment->sig_value_changed -= slot (*this, &ScrollPortImpl::adjustment_changed);
    m_vadjustment = NULL;
    this->SingleContainerImpl::hierarchy_changed (old_toplevel);
    if (anchored())
      {
        AdjustmentSource *m_adjustment_source = parent_interface<AdjustmentSource*>();
        if (m_adjustment_source)
          {
            m_hadjustment = m_adjustment_source->get_adjustment (ADJUSTMENT_SOURCE_ANCESTOR_HORIZONTAL);
            if (m_hadjustment)
              m_hadjustment->sig_value_changed += slot (*this, &ScrollPortImpl::adjustment_changed);
            m_vadjustment = m_adjustment_source->get_adjustment (ADJUSTMENT_SOURCE_ANCESTOR_VERTICAL);
            if (m_vadjustment)
              m_vadjustment->sig_value_changed += slot (*this, &ScrollPortImpl::adjustment_changed);
          }
      }
  }
  virtual void
  size_request (Requisition &requisition)
  {
    bool chspread = false, cvspread = false;
    if (has_children())
      {
        Item &child = get_child();
        if (child.visible())
          {
            requisition = child.size_request ();
            chspread = child.hspread();
            cvspread = child.vspread();
          }
      }
    if (0) /* ScrollPortImpl does NOT propagate h/v-spreading */
      {
        set_flag (HSPREAD_CONTAINER, chspread);
        set_flag (VSPREAD_CONTAINER, cvspread);
      }
  }
  virtual void
  size_allocate (Allocation area)
  {
    if (!has_visible_child())
      {
        allocation (area);
        return;
      }
    Item &child = get_child();
    Requisition rq = child.size_request();
    if (rq.width < area.width)
      {
        area.x += (area.width - rq.width) / 2;
        area.width = rq.width;
      }
    if (rq.height < area.height)
      {
        area.y += (area.height - rq.height) / 2;
        area.height = rq.height;
      }
    allocation (area);
    area.x = 0; /* 0-offset */
    area.y = 0; /* 0-offset */
    area.width = rq.width;
    area.height = rq.height;
    child.set_allocation (area);
    area = child.allocation();
    if (m_hadjustment)
      {
        m_hadjustment->freeze();
        m_hadjustment->lower (0);
        m_hadjustment->upper (area.width);
        double d, w = allocation().width;
        w = MIN (w, area.width);
        d = m_hadjustment->step_increment ();
        m_hadjustment->step_increment (MIN (d, w));
        d = m_hadjustment->page_increment ();
        m_hadjustment->page_increment (MIN (d, w));
        m_hadjustment->page (w);
      }
    if (m_vadjustment)
      {
        m_vadjustment->freeze();
        m_vadjustment->lower (0);
        m_vadjustment->upper (area.height);
        double d, h = allocation().height;
        h = MIN (h, area.height);
        d = m_vadjustment->step_increment ();
        m_vadjustment->step_increment (MIN (d, h));
        d = m_vadjustment->page_increment ();
        m_vadjustment->page_increment (MIN (d, h));
        m_vadjustment->page (h);
      }
    if (m_hadjustment)
      m_hadjustment->thaw();
    if (m_vadjustment)
      m_vadjustment->thaw();
  }
  virtual void
  render (Display &display)
  {
    if (!has_visible_child())
      return;
    double xoffset = m_hadjustment ? m_hadjustment->value() : 0.0;
    double yoffset = m_vadjustment ? m_vadjustment->value() : 0.0;
    const Allocation area = allocation();
    Item &child = get_child();
    const Allocation carea = child.allocation();
    Display scroll_display;
    scroll_display.push_clip_rect (0, 0, carea.width, carea.height);
    scroll_display.push_clip_rect (xoffset, yoffset, area.width, area.height);
    Rect pr = display.current_rect();
    scroll_display.push_clip_rect (pr.ll.x - area.x + xoffset, pr.ll.y - area.y + yoffset, pr.width(), pr.height());
    if (!scroll_display.empty())
      {
        child.render (scroll_display);
        Plane &plane = display.create_plane ();
        const Rect &r = plane.rect();
        int real_x = plane.xstart();
        int real_y = plane.ystart();
        Plane::warp_plane_iknowwhatimdoing (plane,
                                            xoffset + real_x - area.x,
                                            yoffset + real_y - area.y);
        scroll_display.render_combined (plane);
        Plane::warp_plane_iknowwhatimdoing (plane, real_x, real_y);
      }
    scroll_display.pop_clip_rect();
    scroll_display.pop_clip_rect();
    scroll_display.pop_clip_rect();
  }
  void
  adjustment_changed()
  {
    if (has_visible_child())
      {
        Item &child = get_child();
        if (child.drawable())
          g_printerr ("ScrollPortImpl: expose %f %f\n", m_hadjustment->value(), m_vadjustment->value());
        if (child.drawable())
          expose();
      }
  }
public:
  ScrollPortImpl() :
    m_hadjustment (NULL), m_vadjustment (NULL)
  {}
};
static const ItemFactory<ScrollPortImpl> scroll_port_factory ("Rapicorn::ScrollPort");


} // Rapicorn
