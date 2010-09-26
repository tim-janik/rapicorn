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
#include "scrollitemsimpl.hh"
#include "root.hh"

namespace Rapicorn {

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
    case ADJUSTMENT_SOURCE_ANCESTRY_HORIZONTAL:
      return &hadjustment();
    case ADJUSTMENT_SOURCE_ANCESTRY_VERTICAL:
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
  m_vadjustment->flipped_value (round (y));
  m_hadjustment->constrain();
  m_vadjustment->constrain();
  m_hadjustment->thaw();
  m_vadjustment->thaw();
}

static const ItemFactory<ScrollAreaImpl> scroll_area_factory ("Rapicorn::Factory::ScrollArea");

/* --- ScrollPortImpl --- */
class ScrollPortImpl : public virtual SingleContainerImpl {
  Adjustment *m_hadjustment, *m_vadjustment;
  double m_last_xoffset, m_last_yoffset;
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
        find_adjustments (ADJUSTMENT_SOURCE_ANCESTRY_HORIZONTAL, &m_hadjustment,
                          ADJUSTMENT_SOURCE_ANCESTRY_VERTICAL, &m_vadjustment);
        if (m_hadjustment)
          m_hadjustment->sig_value_changed += slot (*this, &ScrollPortImpl::adjustment_changed);
        if (m_vadjustment)
          m_vadjustment->sig_value_changed += slot (*this, &ScrollPortImpl::adjustment_changed);
      }
  }
  virtual void
  size_request (Requisition &requisition)
  {
    bool chspread = false, cvspread = false;
    if (has_children())
      {
        Item &child = get_child();
        if (child.allocatable())
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
    if (!has_allocatable_child())
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
    m_last_xoffset = m_hadjustment ? round (m_hadjustment->value()) : 0.0;
    m_last_yoffset = m_vadjustment ? round (m_vadjustment->flipped_value()) : 0.0;
    if (m_hadjustment)
      m_hadjustment->constrain();
    if (m_vadjustment)
      m_vadjustment->constrain();
    if (m_hadjustment)
      m_hadjustment->thaw();
    if (m_vadjustment)
      m_vadjustment->thaw();
  }
  virtual void
  render (Display &display)
  {
    if (!has_drawable_child())
      return;
    double xoffset = m_hadjustment ? round (m_hadjustment->value()) : 0.0;
    double yoffset = m_vadjustment ? round (m_vadjustment->flipped_value()) : 0.0;
    const Allocation area = allocation();
    Item &child = get_child();
    const IRect ica = child.allocation();
    Display scroll_display;
    // constrain scroll_display to child allocation
    scroll_display.push_clip_rect (ica.x, ica.y, ica.width, ica.height);
    // constrain scroll_display to visible area
    scroll_display.push_clip_rect (ifloor (xoffset), ifloor (yoffset), iceil (area.width), iceil (area.height));
    // constrain scroll_display to expose area
    Rect pr = display.current_rect();
    scroll_display.push_clip_rect (ifloor (pr.x - area.x + xoffset), ifloor (pr.y - area.y + yoffset), iceil (pr.width), iceil (pr.height));
    if (!scroll_display.empty())
      {
        // render child
        child.render (scroll_display);
        // paint scroll area background
        cairo_t *cr = display.create_cairo (background());
        // shift by scroll offset
        cairo_translate (cr, area.x -xoffset, area.y -yoffset);
        // combine onto parent
        scroll_display.render_backing (cr);
        cairo_destroy (cr);
      }
    scroll_display.pop_clip_rect();
    scroll_display.pop_clip_rect();
    scroll_display.pop_clip_rect();
  }
  void
  adjustment_changed()
  {
    double xoffset = m_hadjustment ? round (m_hadjustment->value()) : 0.0;
    double yoffset = m_vadjustment ? round (m_vadjustment->flipped_value()) : 0.0;
    double xdelta = xoffset - m_last_xoffset;
    double ydelta = yoffset - m_last_yoffset;
    Root *ritem = get_root();
    if (!drawable() || !ritem)
      {
        m_last_xoffset = xoffset;
        m_last_yoffset = yoffset;
      }
    else if (fabs (xdelta) + fabs (ydelta) >= 0.1)
      {
        /* copy scrolled area */
        Allocation a = allocation();
        copy_area (Rect (Point (a.x, a.y), a.width, a.height),
                   Point (a.x - xdelta, a.y - ydelta));
        /* expose areas uncovered by scrolling */
        if (xdelta > 0)
          expose (Allocation (a.x + a.width + max (-a.width, -xdelta), a.y, min (a.width, xdelta), a.height));
        else if (xdelta < 0)
          expose (Allocation (a.x, a.y, min (a.width, -xdelta), a.height));
        if (ydelta > 0)
          expose (Allocation (a.x, a.y + a.height + max (-a.height, -ydelta), a.width, min (a.height, ydelta)));
        else if (ydelta < 0)
          expose (Allocation (a.x, a.y, a.width, min (a.height, -ydelta)));
        /* update last scroll position */
        m_last_xoffset = xoffset;
        m_last_yoffset = yoffset;
      }
  }
  virtual Affine
  child_affine (const Item &item)
  {
    const Allocation area = allocation();
    double xoffset = m_hadjustment ? round (m_hadjustment->value()) : 0.0;
    double yoffset = m_vadjustment ? round (m_vadjustment->flipped_value()) : 0.0;
    return AffineTranslate (-area.x + xoffset, -area.y + yoffset);
  }
  virtual void
  set_focus_child (Item *item)
  {
    SingleContainerImpl::set_focus_child (item);
    Item *fchild = get_focus_child();
    if (!fchild)
      return;
    Root *rt = get_root();
    if (!rt)
      return;
    Item *const rfitem = rt->get_focus();
    Item *fitem = rfitem;
    if (!fitem)
      return;
    /* list focus items between focus_item and out immediate child */
    std::list<Item*> fitems;
    while (fitem)
      {
        fitems.push_back (fitem);
        if (fitem == fchild)
          break;
        fitem = fitem->parent();
      }
    /* find the first focus descendant that fits the scroll area */
    fitem = rfitem; /* fallback to innermost focus item */
    const Rect area = allocation();
    for (Walker<Item*> w = walker (fitems); w.has_next(); w++)
      {
        Item *item = *w;
        Rect a = item->allocation();
        if (!translate_from (*item, 1, &a))
          {
            fitem = NULL; // no geographic descendant
            break;
          }
        if (a.width <= area.width && a.height <= area.height)
          {
            fitem = item;
            break;
          }
      }
    if (!fitem)
      return;
    /* adjust scroll area to fitem's area */
    Rect farea = fitem->allocation();
    if (!translate_from (*fitem, 1, &farea))
      return;           // not geographic descendant
    if (0)
      printerr ("scroll-focus: area=%s farea=%s child=%p (%s)\n",
                area.string().c_str(), farea.string().c_str(), fitem,
                fitem->allocation().string().c_str());
    /* calc new scroll position, giving precedence to lower left */
    double deltax = 0, deltay = 0;
    if (farea.upper_x() > area.upper_x() + deltax)
      deltax += farea.upper_x() - (area.upper_x() + deltax);
    if (farea.x < area.x + deltax)
      deltax += farea.x - (area.x + deltax);
    if (farea.upper_y() > area.upper_y() + deltay)
      deltay += farea.upper_y() - (area.upper_y() + deltay);
    if (farea.y < area.y + deltay)
      deltay += farea.y - (area.y + deltay);
    /* scroll to new position */
    m_hadjustment->freeze();
    m_vadjustment->freeze();
    if (deltax)
      m_hadjustment->value (m_hadjustment->value() + deltax);
    if (deltay)
      m_vadjustment->flipped_value (m_vadjustment->flipped_value() + deltay);
    m_hadjustment->constrain();
    m_vadjustment->constrain();
    m_hadjustment->thaw();
    m_vadjustment->thaw();
  }
public:
  ScrollPortImpl() :
    m_hadjustment (NULL), m_vadjustment (NULL),
    m_last_xoffset (0), m_last_yoffset(0)
  {}
};
static const ItemFactory<ScrollPortImpl> scroll_port_factory ("Rapicorn::Factory::ScrollPort");


} // Rapicorn
