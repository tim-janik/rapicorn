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
#include "root.hh"

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
  double pi = adj->page_increment();
  double si = adj->step_increment();
  if (flipped())
    {
      pi = -pi;
      si = -si;
    }
  switch (adj ? distance : 0)
    {
    case +2:
      adj->value (adj->value() + pi);
      return true;
    case +1:
      adj->value (adj->value() + si);
      return true;
    case -1:
      adj->value (adj->value() - si);
      return true;
    case -2:
      adj->value (adj->value() - pi);
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

const PropertyList&
SliderArea::list_properties()
{
  static Property *properties[] = {
    MakeProperty (SliderArea, flipped, _("Flipped"), _("Invert (flip) display of the adjustment value"), false, "rw"),
  };
  static const PropertyList property_list (properties, Container::list_properties());
  return property_list;
}

class SliderAreaImpl : public virtual SliderArea, public virtual TableImpl {
  Adjustment *m_adjustment;
  bool m_flip;
  void
  unset_adjustment()
  {
    m_adjustment->sig_value_changed -= slot (sig_slider_changed);
    m_adjustment->sig_range_changed -= slot (sig_slider_changed);
    m_adjustment->unref();
    m_adjustment = NULL;
  }
protected:
  virtual bool
  flipped () const
  {
    return m_flip;
  }
  virtual void
  flipped (bool flip)
  {
    if (m_flip != flip)
      {
        m_flip = flip;
        changed();
      }
  }
  virtual ~SliderAreaImpl()
  {
    unset_adjustment();
  }
  virtual const PropertyList&
  list_properties()
  {
    static Property *properties[] = {};
    static const PropertyList property_list (properties,
                                             SliderArea::list_properties(),
                                             TableImpl::list_properties());
    return property_list;
  }
public:
  SliderAreaImpl() :
    m_adjustment (NULL),
    m_flip (false)
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
    changed();
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
  bool
  flipped()
  {
    SliderArea *slider_area = parent_interface<SliderArea*>();
    return slider_area ? slider_area->flipped() : false;
  }
public:
  SliderTroughImpl()
  {}
  ~SliderTroughImpl()
  {}
protected:
  virtual void
  hierarchy_changed (Item *old_toplevel)
  {
    SliderArea *slider_area = parent_interface<SliderArea*>();
    if (slider_area)
      slider_area->sig_slider_changed -= slot (*this, &SliderTroughImpl::reallocate_child);
    this->SingleContainerImpl::hierarchy_changed (old_toplevel);
    if (anchored())
      {
        if (!slider_area)
          throw Exception ("SliderTrough without SliderArea ancestor: ", name());
        slider_area->sig_slider_changed += slot (*this, &SliderTroughImpl::reallocate_child);
      }
  }
  Adjustment*
  adjustment () const
  {
    SliderArea *slider_area = parent_interface<SliderArea*>();
    return slider_area ? slider_area->adjustment() : NULL;
  }
  double
  value()
  {
    Adjustment &adj = *adjustment();
    return flipped() ? adj.flipped_value() : adj.value();
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
      case SCROLL_RIGHT:
        exec_command ("increment");
        break;
      case SCROLL_DOWN:
      case SCROLL_LEFT:
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
  bool        m_vertical_skid;
  bool
  flipped()
  {
    SliderTroughImpl &trough = parent_interface<SliderTroughImpl>();
    return trough.flipped();
  }
  bool
  vertical_skid () const
  {
    return m_vertical_skid;
  }
  void
  vertical_skid (bool vs)
  {
    if (m_vertical_skid != vs)
      {
        m_vertical_skid = vs;
        changed();
      }
  }
public:
  SliderSkidImpl() :
    m_button (0),
    m_coffset (0),
    m_vertical_skid (false)
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
  }
  double
  value()
  {
    SliderTroughImpl &trough = parent_interface<SliderTroughImpl>();
    Adjustment &adj = *trough.adjustment();
    return flipped() ? adj.flipped_value() : adj.value();
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
    SliderTroughImpl &trough = parent_interface<SliderTroughImpl>();
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
            double ep = vertical_skid() ? event.y : event.x;
            double cp = vertical_skid() ? ep - allocation().y : ep - allocation().x;
            double clength = vertical_skid() ? allocation().height : allocation().width;
            if (cp >= 0 && cp < clength)
              m_coffset = cp / clength;
            else
              {
                m_coffset = 0.5;
                /* confine offset to not slip the skip off trough boundaries */
                cp = ep - clength * m_coffset;
                const Allocation &ta = trough.allocation();
                double start_slip = (vertical_skid() ? ta.y : ta.x) - cp;
                double tlength = vertical_skid() ? ta.y + ta.height : ta.x + ta.width;
                double end_slip = cp + clength - tlength;
                /* adjust skid position */
                cp += MAX (0, start_slip);
                cp -= MAX (0, end_slip);
                /* recalculate offset */
                m_coffset = (ep - cp) / clength;
              }
          }
        break;
      case MOUSE_MOVE:
        if (m_button)
          {
            double ep = vertical_skid() ? event.y : event.x;
            const Allocation &ta = trough.allocation();
            double tp = vertical_skid() ? ta.y : ta.x;
            double pos = ep - tp;
            int tlength = vertical_skid() ? ta.height : ta.width;
            double clength = vertical_skid() ? allocation().height : allocation().width;
            tlength -= clength;
            pos -= m_coffset * clength;
            pos /= tlength;
            pos = CLAMP (pos, 0, 1);
            if (flipped())
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
private:
  virtual const PropertyList&
  list_properties()
  {
    static Property *properties[] = {
      MakeProperty (SliderSkidImpl, vertical_skid, _("Vertical Skid"), _("Adjust behaviour to vertical skid movement"), false, "rw"),
    };
    static const PropertyList property_list (properties, SingleContainerImpl::list_properties());
    return property_list;
  }
};
static const ItemFactory<SliderSkidImpl> slider_skid_factory ("Rapicorn::SliderSkid");

} // Rapicorn
