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
#include "root.hh"

namespace Rapicorn {

ButtonArea::ButtonArea() :
  sig_check_activate (*this),
  sig_activate (*this)
{}

const PropertyList&
ButtonArea::list_properties()
{
  static Property *properties[] = {
    MakeProperty (ButtonArea, on_click,   _("On CLick"),   _("Command on button1 click"), "", "rw"),
    MakeProperty (ButtonArea, on_click2,  _("On CLick2"),  _("Command on button2 click"), "", "rw"),
    MakeProperty (ButtonArea, on_click3,  _("On CLick3"),  _("Command on button3 click"), "", "rw"),
    MakeProperty (ButtonArea, click_type, _("CLick Type"), _("Click event generation type"), CLICK_ON_RELEASE, "rw"),
  };
  static const PropertyList property_list (properties, Container::list_properties());
  return property_list;
}

class ButtonAreaImpl : public virtual ButtonArea, public virtual EventHandler, public virtual SingleContainerImpl {
  uint m_button, m_repeater;
  ClickType m_click_type;
  String m_on_click[3];
public:
  ButtonAreaImpl() :
    m_button (0), m_repeater (0),
    m_click_type (CLICK_ON_RELEASE)
  {}
  void
  set_model (Activatable &activatable)
  {
    // FIXME
  }
  virtual String    on_click   () const                 { return m_on_click[0]; }
  virtual void      on_click   (const String &command)  { m_on_click[0] = string_strip (command); }
  virtual String    on_click2  () const                 { return m_on_click[1]; }
  virtual void      on_click2  (const String &command)  { m_on_click[1] = string_strip (command); }
  virtual String    on_click3  () const                 { return m_on_click[2]; }
  virtual void      on_click3  (const String &command)  { m_on_click[2] = string_strip (command); }
  virtual ClickType click_type () const                 { return m_click_type; }
  virtual void      click_type (ClickType click_type_v) { reset(); m_click_type = click_type_v; }
  bool
  activate_command()
  {
    if (m_button >= 1 && m_button <= 3 && m_on_click[m_button - 1] != "")
      {
        exec_command (m_on_click[m_button - 1], std::nothrow);
        return TRUE;
      }
    else
      return FALSE;
  }
  void
  activate_click (EventType etype)
  {
    bool need_repeat = etype == BUTTON_PRESS && (m_click_type == CLICK_KEY_REPEAT || m_click_type == CLICK_SLOW_REPEAT || m_click_type == CLICK_FAST_REPEAT);
    bool need_click = need_repeat;
    need_click |= etype == BUTTON_PRESS && m_click_type == CLICK_ON_PRESS;
    need_click |= etype == BUTTON_RELEASE && m_click_type == CLICK_ON_RELEASE;
    bool can_exec = need_click && activate_command();
    need_repeat &= can_exec;
    if (need_repeat && !m_repeater)
      {
        if (m_click_type == CLICK_FAST_REPEAT)
          m_repeater = exec_fast_repeater (slot (*this, &ButtonAreaImpl::activate_command));
        else if (m_click_type == CLICK_SLOW_REPEAT)
          m_repeater = exec_slow_repeater (slot (*this, &ButtonAreaImpl::activate_command));
        else if (m_click_type == CLICK_KEY_REPEAT)
          m_repeater = exec_key_repeater (slot (*this, &ButtonAreaImpl::activate_command));
      }
    else if (!need_repeat && m_repeater)
      {
        remove_exec (m_repeater);
        m_repeater = 0;
      }
  }
  virtual void
  reset (ResetMode mode = RESET_ALL)
  {
    remove_exec (m_repeater);
    m_repeater = 0;
    m_button = 0;
  }
  virtual bool
  handle_event (const Event &event)
  {
    ButtonArea &view = *this;
    bool handled = false, proper_release = false;
    switch (event.type)
      {
        const EventButton *bevent;
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
        bevent = dynamic_cast<const EventButton*> (&event);
        if (!m_button and bevent->button >= 1 and bevent->button <= 3 and
            m_on_click[bevent->button - 1] != "")
          {
            bool inbutton = view.prelight();
            m_button = bevent->button;
            view.impressed (true);
            view.root()->add_grab (view);
            activate_click (inbutton ? BUTTON_PRESS : BUTTON_CANCELED);
            handled = true;
          }
        break;
      case BUTTON_RELEASE:
      case BUTTON_2RELEASE:
      case BUTTON_3RELEASE:
        proper_release = true;
      case BUTTON_CANCELED:
        bevent = dynamic_cast<const EventButton*> (&event);
        if (m_button == bevent->button)
          {
            bool inbutton = view.prelight();
            view.root()->remove_grab (view);
            activate_click (inbutton && proper_release ? BUTTON_RELEASE : BUTTON_CANCELED);
            view.impressed (false);
            m_button = 0;
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
};
static const ItemFactory<ButtonAreaImpl> button_area_factory ("Rapicorn::ButtonArea");

} // Rapicorn
