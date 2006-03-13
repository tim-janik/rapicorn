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
#include "slider.hh"
#include "tableimpl.hh"
#include "painter.hh"
#include "factory.hh"

namespace Rapicorn {

SliderArea::SliderArea() :
  sig_slider_changed (*this, &SliderArea::slider_changed)
{}

void
SliderArea::slider_changed()
{}

bool
SliderArea::move (int distance)
{
  Adjustment *adj = adjustment();
  switch (adj ? distance : 0)
    {
    case +2:
      adj->value (adj->value() + adj->page_increment());
      return true;
    case +1:
      adj->value (adj->value() + adj->step_increment());
      return true;
    case -1:
      adj->value (adj->value() - adj->step_increment());
      return true;
    case -2:
      adj->value (adj->value() - adj->page_increment());
      return true;
    default:
      return false;
    }
}

const CommandList&
SliderArea::list_commands ()
{
  static Command *commands[] = {
    MakeNamedCommand (SliderArea, "increment", _("Increment slider"), move, +1),
    MakeNamedCommand (SliderArea, "decrement", _("Decrement slider"), move, -1),
    MakeNamedCommand (SliderArea, "page-increment", _("Large slider increment"), move, +2),
    MakeNamedCommand (SliderArea, "page-decrement", _("Large slider decrement"), move, -2),
  };
  static const CommandList command_list (commands, Container::list_commands());
  return command_list;
}

class SliderAreaImpl : public virtual SliderArea, public virtual TableImpl {
  Adjustment *m_adjustment;
  void
  unset_adjustment()
  {
    m_adjustment->sig_value_changed -= slot (sig_slider_changed);
    m_adjustment->sig_range_changed -= slot (sig_slider_changed);
    m_adjustment->unref();
    m_adjustment = NULL;
  }
protected:
  virtual ~SliderAreaImpl()
  {
    unset_adjustment();
  }
public:
  SliderAreaImpl() :
    m_adjustment (NULL)
  {
    Adjustment *adj = Adjustment::create (0, 0, 1, 0.01, 0.2);
    adjustment (*adj);
    adj->unref();
  }
  virtual void
  adjustment (Adjustment &adjustment)
  {
    adjustment.ref();
    if (m_adjustment)
      unset_adjustment();
    m_adjustment = &adjustment;
    m_adjustment->sig_value_changed += slot (sig_slider_changed);
    m_adjustment->sig_range_changed += slot (sig_slider_changed);
  }
  virtual Adjustment*
  adjustment () const
  {
    return m_adjustment;
  }
  virtual void
  control (const String &command_name,
           const String &arg)
  {
  }
};
static const ItemFactory<SliderAreaImpl> slider_area_factory ("Rapicorn::SliderArea");

class SliderSkidImpl;

class SliderTroughImpl : public virtual EventHandler, public virtual SingleContainerImpl {
  SliderArea *m_slider_area;
public:
  SliderTroughImpl() :
    m_slider_area (NULL)
  {}
  ~SliderTroughImpl()
  {}
protected:
  virtual void
  hierarchy_changed (Item *old_toplevel)
  {
    if (m_slider_area)
      {
        m_slider_area->sig_slider_changed -= slot (*this, &SliderTroughImpl::reallocate_child);
        m_slider_area = NULL;
      }
    this->SingleContainerImpl::hierarchy_changed (old_toplevel);
    if (anchored())
      {
        Item *item = parent();
        while (item)
          {
            m_slider_area = dynamic_cast<SliderArea*> (item);
            if (m_slider_area)
              break;
            item = item->parent();
          }
        if (!m_slider_area)
          throw Exception ("SliderTrough without SliderArea ancestor: ", name());
        m_slider_area->sig_slider_changed += slot (*this, &SliderTroughImpl::reallocate_child);
      }
  }
  Adjustment*
  adjustment () const
  {
    if (!anchored())
      throw Exception ("SliderTrough used outside of anchored hierarchy: ", name());
    return m_slider_area->adjustment();
  }
  double
  value()
  {
    Adjustment &adj = *adjustment();
    bool horizontal = true;
    return horizontal ? adj.flipped_value() : adj.value();
  }
  virtual void
  size_allocate (Allocation area)
  {
    allocation (area);
    reallocate_child();
  }
  void
  reallocate_child ()
  {
    Allocation area = allocation();
    if (!has_visible_child())
      return;
    Item &child = get_child();
    Requisition rq = child.size_request();
    /* expand/scale child */
    if (area.width > rq.width && !child.hexpand())
      {
        area.x += iround (value() * (area.width - rq.width));
        area.width = iround (rq.width);
      }
    if (area.height > rq.height && !child.vexpand())
      {
        area.y += iround (value() * (area.height - rq.height));
        area.height = iround (rq.height);
      }
    child.set_allocation (area);
  }
  friend class SliderSkidImpl;
  virtual void
  reset (ResetMode mode = RESET_ALL)
  {}
  virtual bool
  handle_event (const Event &event)
  {
    bool handled = false;
    switch (event.type)
      {
      case BUTTON_PRESS:
      case BUTTON_2PRESS:
      case BUTTON_3PRESS:
      case BUTTON_RELEASE:
      case BUTTON_2RELEASE:
      case BUTTON_3RELEASE:
      case BUTTON_CANCELED:
        /* forward button events to allow slider warps */
        if (has_visible_child())
          {
            Event cevent = event;
            handled = get_child().process_event (cevent);
          }
        break;
      case SCROLL_UP:
      case SCROLL_LEFT:
        exec_command ("increment");
        break;
      case SCROLL_DOWN:
      case SCROLL_RIGHT:
        exec_command ("decrement");
        break;
      default: break;
      }
    return handled;
  }
};
static const ItemFactory<SliderTroughImpl> slider_trough_factory ("Rapicorn::SliderTrough");

class SliderSkidImpl : public virtual EventHandler, public virtual SingleContainerImpl {
  uint        m_button;
  double      m_coffset;
public:
  SliderSkidImpl() :
    m_button (0),
    m_coffset (0)
  {}
  ~SliderSkidImpl()
  {}
protected:
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
    set_flag (HSPREAD_CONTAINER, chspread);
    set_flag (VSPREAD_CONTAINER, cvspread);
    requisition.width = MAX (requisition.width, 20); // FIXME: hardcoded minimum
  }
  double
  value()
  {
    SliderTroughImpl &trough = parent()->interface<SliderTroughImpl>(); // FIXME: need Item.parent_interface<>();
    Adjustment &adj = *trough.adjustment();
    bool horizontal = true;
    return horizontal ? adj.flipped_value() : adj.value();
  }
  virtual void
  reset (ResetMode mode = RESET_ALL)
  {
    m_button = 0;
    m_coffset = 0;
  }
  virtual bool
  handle_event (const Event &event)
  {
    bool handled = false, proper_release = false;
    SliderTroughImpl &trough = parent()->interface<SliderTroughImpl>(); // FIXME: need Item.parent_interface<>();
    Adjustment &adj = *trough.adjustment();
    switch (event.type)
      {
      case MOUSE_ENTER:
        this->prelight (true);
        break;
      case MOUSE_LEAVE:
        this->prelight (false);
        break;
      case BUTTON_PRESS:
      case BUTTON_2PRESS:
      case BUTTON_3PRESS:
        if (!m_button and (event.button == 1 or event.button == 2))
          {
            m_button = event.button;
            root()->add_grab (this, true);
            handled = true;
            m_coffset = 0;
            double cx = event.x - allocation().x;
            double cwidth = allocation().width;
            if (cx >= 0 && cx < cwidth)
              m_coffset = cx / cwidth;
            else
              {
                m_coffset = 0.5;
                /* confine offset to not slip the skip off trough boundaries */
                cx = event.x - cwidth * m_coffset;
                double start_slip = trough.allocation().x - cx;
                double end_slip = cx + cwidth - (trough.allocation().x + trough.allocation().width);
                /* adjust skid position */
                cx += MAX (0, start_slip);
                cx -= MAX (0, end_slip);
                /* recalculate offset */
                m_coffset = (event.x - cx) / cwidth;
              }
          }
        break;
      case MOUSE_MOVE:
        if (m_button)
          {
            double pos = event.x - trough.allocation().x;
            int width = trough.allocation().width;
            int cwidth = allocation().width;
            width -= cwidth;
            pos -= m_coffset * cwidth;
            pos /= width;
            pos = CLAMP (pos, 0, 1);
            if (1 /* horizontal */)
              adj.flipped_value (pos);
            else
              adj.value (pos);
          }
        break;
      case BUTTON_RELEASE:
      case BUTTON_2RELEASE:
      case BUTTON_3RELEASE:
        proper_release = true;
      case BUTTON_CANCELED:
        if (m_button == event.button)
          {
            root()->remove_grab (this);
            m_button = 0;
            m_coffset = 0;
            handled = true;
          }
        break;
      default: break;
      }
    return handled;
  }
};
static const ItemFactory<SliderSkidImpl> slider_skid_factory ("Rapicorn::SliderSkid");

} // Rapicorn
