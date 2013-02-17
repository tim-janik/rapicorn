// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "buttons.hh"
#include "container.hh"
#include "painter.hh"
#include "factory.hh"
#include "window.hh"
#include <unistd.h>

namespace Rapicorn {

ButtonAreaImpl::ButtonAreaImpl() :
  button_ (0), repeater_ (0), unpress_ (0),
  click_type_ (CLICK_ON_RELEASE),
  focus_frame_ (NULL)
{}

const PropertyList&
ButtonAreaImpl::_property_list()
{
  static Property *properties[] = {
    MakeProperty (ButtonAreaImpl, click_type, _("CLick Type"), _("Click event generation type"), "rw"),
  };
  static const PropertyList property_list (properties, SingleContainerImpl::_property_list(), ButtonAreaIface::_property_list());
  return property_list;
}

void
ButtonAreaImpl::dump_private_data (TestStream &tstream)
{
  tstream.dump_intern ("button_", button_);
  tstream.dump_intern ("repeater_", repeater_);
}

bool
ButtonAreaImpl::activate_item ()
{
  ButtonAreaImpl &view = *this;
  WindowImpl *window = get_window();
  EventLoop *loop = window ? window->get_loop() : NULL;
  bool handled = false;
  if (loop)
    {
      view.impressed (true);
      window->draw_child (view);
      handled = activate_button_command (1);
      if (!unpress_)
        unpress_ = loop->exec_timer (50, 0, [this, &view] () {
            view.impressed (false);
            remove_exec (unpress_);
            unpress_ = 0;
            return false;
          });
    }
  return handled;
}

bool
ButtonAreaImpl::activate_button_command (int button)
{
  if (button >= 1 && button <= 3 && on_click_[button - 1] != "")
    {
      exec_command (on_click_[button - 1]);
      return true;
    }
  else
    return false;
}

bool
ButtonAreaImpl::activate_command()
{
  return activate_button_command (button_);
}

void
ButtonAreaImpl::activate_click (int       button,
                                EventType etype)
{
  bool need_repeat = etype == BUTTON_PRESS && (click_type_ == CLICK_KEY_REPEAT || click_type_ == CLICK_SLOW_REPEAT || click_type_ == CLICK_FAST_REPEAT);
  bool need_click = need_repeat;
  need_click |= etype == BUTTON_PRESS && click_type_ == CLICK_ON_PRESS;
  need_click |= etype == BUTTON_RELEASE && click_type_ == CLICK_ON_RELEASE;
  bool can_exec = need_click && activate_button_command (button);
  need_repeat &= can_exec;
  if (need_repeat && !repeater_)
    {
      if (click_type_ == CLICK_FAST_REPEAT)
        repeater_ = exec_fast_repeater (Aida::slot (*this, &ButtonAreaImpl::activate_command));
      else if (click_type_ == CLICK_SLOW_REPEAT)
        repeater_ = exec_slow_repeater (Aida::slot (*this, &ButtonAreaImpl::activate_command));
      else if (click_type_ == CLICK_KEY_REPEAT)
        repeater_ = exec_key_repeater (Aida::slot (*this, &ButtonAreaImpl::activate_command));
    }
  else if (!need_repeat && repeater_)
    {
      remove_exec (repeater_);
      repeater_ = 0;
    }
}

bool
ButtonAreaImpl::can_focus () const
{
  return focus_frame_ != NULL;
}

bool
ButtonAreaImpl::register_focus_frame (FocusFrame &frame)
{
  if (!focus_frame_)
    focus_frame_ = &frame;
  return focus_frame_ == &frame;
}

void
ButtonAreaImpl::unregister_focus_frame (FocusFrame &frame)
{
  if (focus_frame_ == &frame)
    focus_frame_ = NULL;
}

void
ButtonAreaImpl::reset (ResetMode mode)
{
  ButtonAreaImpl &view = *this;
  view.impressed (false);
  remove_exec (unpress_);
  unpress_ = 0;
  remove_exec (repeater_);
  repeater_ = 0;
  button_ = 0;
}

bool
ButtonAreaImpl::handle_event (const Event &event)
{
  ButtonAreaImpl &view = *this;
  bool handled = false, proper_release = false;
  switch (event.type)
    {
      const EventButton *bevent;
    case MOUSE_ENTER:
      view.impressed (button_ != 0);
      view.prelight (true);
      break;
    case MOUSE_LEAVE:
      view.prelight (false);
      view.impressed (button_ != 0);
      break;
    case BUTTON_PRESS:
    case BUTTON_2PRESS:
    case BUTTON_3PRESS:
      bevent = dynamic_cast<const EventButton*> (&event);
      if (!button_ and bevent->button >= 1 and bevent->button <= 3 and
          on_click_[bevent->button - 1] != "")
        {
          bool inbutton = view.prelight();
          button_ = bevent->button;
          view.impressed (true);
          if (inbutton && can_focus())
            grab_focus();
          view.get_window()->add_grab (*this);
          activate_click (bevent->button, inbutton ? BUTTON_PRESS : BUTTON_CANCELED);
          handled = true;
        }
      break;
    case BUTTON_RELEASE:
    case BUTTON_2RELEASE:
    case BUTTON_3RELEASE:
      proper_release = true;
    case BUTTON_CANCELED:
      bevent = dynamic_cast<const EventButton*> (&event);
      if (button_ == bevent->button)
        {
          bool inbutton = view.prelight();
          view.get_window()->remove_grab (*this);
          button_ = 0;
          // activation may recurse here
          activate_click (bevent->button, inbutton && proper_release ? BUTTON_RELEASE : BUTTON_CANCELED);
          view.impressed (false);
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

static const ItemFactory<ButtonAreaImpl> button_area_factory ("Rapicorn::Factory::ButtonArea");

} // Rapicorn
