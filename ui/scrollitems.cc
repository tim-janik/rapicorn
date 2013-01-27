// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "scrollitemsimpl.hh"
#include "window.hh"
#include "factory.hh"

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
ScrollAreaImpl::scroll_to (double x, double y)
{
  hadjustment().freeze();
  vadjustment().freeze();
  m_hadjustment->value (round (x));
  m_vadjustment->value (round (y));
  m_hadjustment->constrain();
  m_vadjustment->constrain();
  m_hadjustment->thaw();
  m_vadjustment->thaw();
}

static const ItemFactory<ScrollAreaImpl> scroll_area_factory ("Rapicorn::Factory::ScrollArea");

/* --- ScrollPortImpl --- */
class ScrollPortImpl : public virtual ViewportImpl {
  Adjustment *m_hadjustment, *m_vadjustment;
  size_t hadjustment_conid_, vadjustment_conid_;
  virtual void
  hierarchy_changed (ItemImpl *old_toplevel)
  {
    if (m_hadjustment && hadjustment_conid_)
      m_hadjustment->sig_value_changed() -= hadjustment_conid_;
    m_hadjustment = NULL;
    hadjustment_conid_ = 0;
    if (m_vadjustment && vadjustment_conid_)
      m_vadjustment->sig_value_changed() -= vadjustment_conid_;
    m_vadjustment = NULL;
    vadjustment_conid_ = 0;
    this->ViewportImpl::hierarchy_changed (old_toplevel);
    if (anchored())
      {
        find_adjustments (ADJUSTMENT_SOURCE_ANCESTRY_HORIZONTAL, &m_hadjustment,
                          ADJUSTMENT_SOURCE_ANCESTRY_VERTICAL, &m_vadjustment);
        if (m_hadjustment)
          hadjustment_conid_ = m_hadjustment->sig_value_changed() += Aida::slot (*this, &ScrollPortImpl::adjustment_changed);
        if (m_vadjustment)
          vadjustment_conid_ = m_vadjustment->sig_value_changed() += Aida::slot (*this, &ScrollPortImpl::adjustment_changed);
      }
  }
  virtual void
  size_allocate (Allocation area, bool changed)
  {
    if (!has_allocatable_child())
      return;
    ItemImpl &child = get_child();
    Requisition rq = child.requisition();
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
    // FIXME: allocation (area);
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
    if (m_hadjustment)
      m_hadjustment->constrain();
    if (m_vadjustment)
      m_vadjustment->constrain();
    if (m_hadjustment)
      m_hadjustment->thaw();
    if (m_vadjustment)
      m_vadjustment->thaw();
    const int xoffset = m_hadjustment ? iround (m_hadjustment->value()) : 0;
    const int yoffset = m_vadjustment ? iround (m_vadjustment->value()) : 0;
    scroll_offsets (xoffset, yoffset); // syncronize offsets, before adjustment_changed() kicks in
  }
  void
  adjustment_changed()
  {
    const int xoffset = m_hadjustment ? iround (m_hadjustment->value()) : 0;
    const int yoffset = m_vadjustment ? iround (m_vadjustment->value()) : 0;
    scroll_offsets (xoffset, yoffset);
  }
  virtual void
  set_focus_child (ItemImpl *item)
  {
    ViewportImpl::set_focus_child (item);
    ItemImpl *fchild = get_focus_child();
    if (!fchild)
      return;
    WindowImpl *rt = get_window();
    if (!rt)
      return;
    ItemImpl *const rfitem = rt->get_focus();
    ItemImpl *fitem = rfitem;
    if (!fitem)
      return;
    /* list focus items between focus_item and out immediate child */
    std::list<ItemImpl*> fitems;
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
    for (Walker<ItemImpl*> w = walker (fitems); w.has_next(); w++)
      {
        ItemImpl *item = *w;
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
      m_vadjustment->value (m_vadjustment->value() + deltay);
    m_hadjustment->constrain();
    m_vadjustment->constrain();
    m_hadjustment->thaw();
    m_vadjustment->thaw();
  }
public:
  ScrollPortImpl() :
    m_hadjustment (NULL), m_vadjustment (NULL), hadjustment_conid_ (0), vadjustment_conid_ (0)
  {}
};
static const ItemFactory<ScrollPortImpl> scroll_port_factory ("Rapicorn::Factory::ScrollPort");


} // Rapicorn
