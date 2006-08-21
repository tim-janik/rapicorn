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
#include "root.hh"

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
  return round (hadjustment().value());
}

double
ScrollAreaImpl::yoffset () const
{
  return round (vadjustment().value());
}

void
ScrollAreaImpl::scroll_to (double x,
                           double y)
{
  hadjustment().freeze();
  vadjustment().freeze();
  m_hadjustment->value (round (x));
  m_vadjustment->value (round (y));
  m_hadjustment->thaw();
  m_vadjustment->thaw();
}

static const ItemFactory<ScrollAreaImpl> scroll_area_factory ("Rapicorn::ScrollArea");

/* --- ScrollPortImpl --- */
class ScrollPortImpl : public virtual SingleContainerImpl {
  Adjustment *m_hadjustment, *m_vadjustment;
  double last_xoffset, last_yoffset;
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
    const double small_step = 10;
    if (m_hadjustment)
      {
        m_hadjustment->freeze();
        m_hadjustment->lower (0);
        m_hadjustment->upper (area.width);
        double d, w = allocation().width, l = min (w, 1);
        m_hadjustment->page (w);
        m_hadjustment->step_increment (min (small_step, round (m_hadjustment->page() / 10)));
        m_hadjustment->page_increment (max (small_step, round (m_hadjustment->page() / 3)));
        w = MIN (m_hadjustment->page(), area.width);
        d = m_hadjustment->step_increment ();
        m_hadjustment->step_increment (CLAMP (d, l, w));
        d = m_hadjustment->page_increment ();
        m_hadjustment->page_increment (CLAMP (d, l, w));
      }
    if (m_vadjustment)
      {
        m_vadjustment->freeze();
        m_vadjustment->lower (0);
        m_vadjustment->upper (area.height);
        double d, h = allocation().height, l = min (h, 1);
        m_vadjustment->page (h);
        m_vadjustment->step_increment (min (small_step, round (m_vadjustment->page() / 10)));
        m_vadjustment->page_increment (max (small_step, round (m_vadjustment->page() / 3)));
        h = MIN (m_vadjustment->page(), area.height);
        d = m_vadjustment->step_increment ();
        m_vadjustment->step_increment (CLAMP (d, l, h));
        d = m_vadjustment->page_increment ();
        m_vadjustment->page_increment (CLAMP (d, l, h));
      }
    last_xoffset = m_hadjustment ? round (m_hadjustment->value()) : 0.0;
    last_yoffset = m_vadjustment ? round (m_vadjustment->value()) : 0.0;
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
    double xoffset = m_hadjustment ? round (m_hadjustment->value()) : 0.0;
    double yoffset = m_vadjustment ? round (m_vadjustment->value()) : 0.0;
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
        Plane &plane = display.create_plane (background());
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
    double xoffset = m_hadjustment ? round (m_hadjustment->value()) : 0.0;
    double yoffset = m_vadjustment ? round (m_vadjustment->value()) : 0.0;
    bool need_expose_handling = false;
    if (has_visible_child())
      {
        Item &child = get_child();
        if (child.drawable())
          {
            if (xoffset != last_xoffset || yoffset != last_yoffset)
              {
                double xdelta = xoffset - last_xoffset;
                double ydelta = yoffset - last_yoffset;
                Allocation a = allocation();
                copy_area (Rect (Point (a.x, a.y), a.width, a.height),
                           Point (a.x - xdelta, a.y - ydelta));
                if (xdelta > 0)
                  expose (Allocation (a.x + a.width + max (-a.width, -xdelta), a.y, min (a.width, xdelta), a.height));
                else if (xdelta < 0)
                  expose (Allocation (a.x, a.y, min (a.width, -xdelta), a.height));
                if (ydelta > 0)
                  expose (Allocation (a.x, a.y + a.height + max (-a.height, -ydelta), a.width, min (a.height, ydelta)));
                else if (ydelta < 0)
                  expose (Allocation (a.x, a.y, a.width, min (a.height, -ydelta)));
                need_expose_handling = true;
              }
          }
      }
    last_xoffset = xoffset;
    last_yoffset = yoffset;
    if (need_expose_handling)
      {
        Root *ritem = root();
        if (ritem)
          ritem->draw_now();
      }
  }
public:
  ScrollPortImpl() :
    m_hadjustment (NULL), m_vadjustment (NULL),
    last_xoffset (0), last_yoffset(0)
  {}
};
static const ItemFactory<ScrollPortImpl> scroll_port_factory ("Rapicorn::ScrollPort");


} // Rapicorn
