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
  hadjustment_ (NULL), vadjustment_ (NULL)
{}

Adjustment&
ScrollAreaImpl::hadjustment () const
{
  if (!hadjustment_)
    hadjustment_ = Adjustment::create (0, 0, 1, 0.01, 0.2);
  return *hadjustment_;
}

Adjustment&
ScrollAreaImpl::vadjustment () const
{
  if (!vadjustment_)
    vadjustment_ = Adjustment::create (0, 0, 1, 0.01, 0.2);
  return *vadjustment_;
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
  hadjustment_->value (round (x));
  vadjustment_->value (round (y));
  hadjustment_->constrain();
  vadjustment_->constrain();
  hadjustment_->thaw();
  vadjustment_->thaw();
}

static const ItemFactory<ScrollAreaImpl> scroll_area_factory ("Rapicorn::Factory::ScrollArea");

/* --- ScrollPortImpl --- */
class ScrollPortImpl : public virtual ViewportImpl, public virtual EventHandler {
  Adjustment *hadjustment_, *vadjustment_;
  size_t conid_hadjustment_, conid_vadjustment_;
  virtual void
  hierarchy_changed (ItemImpl *old_toplevel)
  {
    if (hadjustment_ && conid_hadjustment_)
      hadjustment_->sig_value_changed() -= conid_hadjustment_;
    hadjustment_ = NULL;
    conid_hadjustment_ = 0;
    if (vadjustment_ && conid_vadjustment_)
      vadjustment_->sig_value_changed() -= conid_vadjustment_;
    vadjustment_ = NULL;
    conid_vadjustment_ = 0;
    this->ViewportImpl::hierarchy_changed (old_toplevel);
    if (anchored())
      {
        find_adjustments (ADJUSTMENT_SOURCE_ANCESTRY_HORIZONTAL, &hadjustment_,
                          ADJUSTMENT_SOURCE_ANCESTRY_VERTICAL, &vadjustment_);
        if (hadjustment_)
          conid_hadjustment_ = hadjustment_->sig_value_changed() += Aida::slot (*this, &ScrollPortImpl::adjustment_changed);
        if (vadjustment_)
          conid_vadjustment_ = vadjustment_->sig_value_changed() += Aida::slot (*this, &ScrollPortImpl::adjustment_changed);
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
    if (hadjustment_)
      {
        hadjustment_->freeze();
        hadjustment_->lower (0);
        hadjustment_->upper (area.width);
        double d, w = allocation().width, l = min (w, 1);
        hadjustment_->page (w);
        hadjustment_->step_increment (min (small_step, round (hadjustment_->page() / 10)));
        hadjustment_->page_increment (max (small_step, round (hadjustment_->page() / 3)));
        w = MIN (hadjustment_->page(), area.width);
        d = hadjustment_->step_increment ();
        hadjustment_->step_increment (CLAMP (d, l, w));
        d = hadjustment_->page_increment ();
        hadjustment_->page_increment (CLAMP (d, l, w));
      }
    if (vadjustment_)
      {
        vadjustment_->freeze();
        vadjustment_->lower (0);
        vadjustment_->upper (area.height);
        double d, h = allocation().height, l = min (h, 1);
        vadjustment_->page (h);
        vadjustment_->step_increment (min (small_step, round (vadjustment_->page() / 10)));
        vadjustment_->page_increment (max (small_step, round (vadjustment_->page() / 3)));
        h = MIN (vadjustment_->page(), area.height);
        d = vadjustment_->step_increment ();
        vadjustment_->step_increment (CLAMP (d, l, h));
        d = vadjustment_->page_increment ();
        vadjustment_->page_increment (CLAMP (d, l, h));
      }
    if (hadjustment_)
      hadjustment_->constrain();
    if (vadjustment_)
      vadjustment_->constrain();
    if (hadjustment_)
      hadjustment_->thaw();
    if (vadjustment_)
      vadjustment_->thaw();
    const int xoffset = hadjustment_ ? iround (hadjustment_->value()) : 0;
    const int yoffset = vadjustment_ ? iround (vadjustment_->value()) : 0;
    scroll_offsets (xoffset, yoffset); // syncronize offsets, before adjustment_changed() kicks in
  }
  void
  adjustment_changed()
  {
    const int xoffset = hadjustment_ ? iround (hadjustment_->value()) : 0;
    const int yoffset = vadjustment_ ? iround (vadjustment_->value()) : 0;
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
    scroll_to_child (*fitem);
  }
  virtual void
  scroll_to_child (ItemImpl &item)
  {
    const Rect area = allocation();
    /* adjust scroll area to item's area */
    Rect farea = item.allocation();
    if (!translate_from (item, 1, &farea))
      return;           // not geographic descendant
    if (0)
      printerr ("scroll-focus: area=%s farea=%s child=%p (%s)\n",
                area.string().c_str(), farea.string().c_str(), &item,
                item.allocation().string().c_str());
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
    hadjustment_->freeze();
    vadjustment_->freeze();
    if (deltax)
      hadjustment_->value (hadjustment_->value() + deltax);
    if (deltay)
      vadjustment_->value (vadjustment_->value() + deltay);
    hadjustment_->constrain();
    vadjustment_->constrain();
    hadjustment_->thaw();
    vadjustment_->thaw();
  }
  bool
  scroll (EventType scroll_dir)
  {
    switch (scroll_dir)
      {
      case SCROLL_UP:           return vadjustment_ ? vadjustment_->move (MOVE_STEP_BACKWARD) : false;
      case SCROLL_LEFT:         return hadjustment_ ? hadjustment_->move (MOVE_STEP_BACKWARD) : false;
      case SCROLL_DOWN:         return vadjustment_ ? vadjustment_->move (MOVE_STEP_FORWARD) : false;
      case SCROLL_RIGHT:        return hadjustment_ ? hadjustment_->move (MOVE_STEP_FORWARD) : false;
      default:                  ;
      }
    return false;
  }
  virtual const CommandList&
  list_commands ()
  {
    static Command *commands[] = {
      MakeNamedCommand (ScrollPortImpl, "scroll-up",    _("Scroll upwards"),    scroll, SCROLL_UP),
      MakeNamedCommand (ScrollPortImpl, "scroll-left",  _("Scroll leftwards"),  scroll, SCROLL_LEFT),
      MakeNamedCommand (ScrollPortImpl, "scroll-down",  _("Scroll downwards"),  scroll, SCROLL_DOWN),
      MakeNamedCommand (ScrollPortImpl, "scroll-right", _("Scroll rightwards"), scroll, SCROLL_RIGHT),
    };
    static const CommandList command_list (commands, ViewportImpl::list_commands());
    return command_list;
  }
  virtual bool
  handle_event (const Event &event)
  {
    bool handled = false;
    switch (event.type)
      {
      case SCROLL_UP:           handled = exec_command ("scroll-up");           break;
      case SCROLL_LEFT:         handled = exec_command ("scroll-left");         break;
      case SCROLL_DOWN:         handled = exec_command ("scroll-down");         break;
      case SCROLL_RIGHT:        handled = exec_command ("scroll-right");        break;
      default:                  break;
      }
    return handled;
  }
  virtual void
  reset (ResetMode mode)
  {}
public:
  ScrollPortImpl() :
    hadjustment_ (NULL), vadjustment_ (NULL), conid_hadjustment_ (0), conid_vadjustment_ (0)
  {}
};
static const ItemFactory<ScrollPortImpl> scroll_port_factory ("Rapicorn::Factory::ScrollPort");


} // Rapicorn
