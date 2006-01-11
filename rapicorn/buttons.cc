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

ButtonArea::ButtonArea() :
  sig_check_activate (*this),
  sig_activate (*this)
{}

const PropertyList&
ButtonArea::list_properties()
{
  static Property *properties[] = {
    MakeProperty (ButtonArea, on_click,  _("On CLick"), _("Command on button1 click"), "", "rw"),
    MakeProperty (ButtonArea, on_click2, _("On CLick2"), _("Command on button2 click"), "", "rw"),
    MakeProperty (ButtonArea, on_click3, _("On CLick3"), _("Command on button3 click"), "", "rw"),
  };
  static const PropertyList property_list (properties, Container::list_properties());
  return property_list;
}

class ButtonAreaImpl : public virtual ButtonArea, public virtual EventHandler, public virtual SingleContainerImpl {
  uint m_button;
  String m_on_click[3];
public:
  ButtonAreaImpl() :
    m_button (0)
  {}
  void
  set_model (Activatable &activatable)
  {
    // FIXME
  }
  virtual String on_click  () const                     { return m_on_click[0]; }
  virtual void   on_click  (const String &command)      { m_on_click[0] = string_strip (command); }
  virtual String on_click2 () const                     { return m_on_click[1]; }
  virtual void   on_click2 (const String &command)      { m_on_click[1] = string_strip (command); }
  virtual String on_click3 () const                     { return m_on_click[2]; }
  virtual void   on_click3 (const String &command)      { m_on_click[2] = string_strip (command); }
  virtual void
  reset (ResetMode mode = RESET_ALL)
  {
    m_button = 0;
  }
  virtual bool
  handle_event (const Event &event)
  {
    ButtonArea &view = *this;
    bool handled = false, proper_release = false;
    switch (event.type)
      {
      case MOUSE_ENTER:
        view.impressed (m_button != 0);
        view.prelight (true);
        break;
      case MOUSE_LEAVE:
        view.prelight (false);
        view.impressed (m_button != 0);
        break;
      case BUTTON_PRESS:
      case BUTTON_2PRESS:
      case BUTTON_3PRESS:
        if (!m_button and event.button >= 1 and event.button <= 3 and
            m_on_click[event.button - 1] != "")
          {
            m_button = event.button;
            view.impressed (true);
            view.root()->add_grab (view);
            handled = true;
          }
        break;
      case BUTTON_RELEASE:
      case BUTTON_2RELEASE:
      case BUTTON_3RELEASE:
        proper_release = true;
      case BUTTON_CANCELED:
        if (m_button == event.button)
          {
            bool inbutton = view.prelight();
            view.root()->remove_grab (view);
            m_button = 0;
            view.impressed (false);
            handled = true;
            if (proper_release and inbutton and
                event.button >= 1 and event.button <= 3 and
                m_on_click[event.button - 1] != "")
              exec_command (m_on_click[event.button - 1], std::nothrow);
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
};
static const ItemFactory<ButtonAreaImpl> button_area_factory ("Rapicorn::ButtonArea");

} // Rapicorn
