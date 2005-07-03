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
#include "buttons.hh"
#include "containerimpl.hh"
#include "painter.hh"
#include "factory.hh"

namespace Rapicorn {

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
