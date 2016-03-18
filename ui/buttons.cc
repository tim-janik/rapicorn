// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "buttons.hh"
#include "container.hh"
#include "painter.hh"
#include "factory.hh"
#include "window.hh"
#include <unistd.h>

namespace Rapicorn {

ButtonAreaImpl::ButtonAreaImpl() :
  button_ (0), repeater_ (0), unpress_ (0),
  click_type_ (Click::ON_RELEASE)
{}

void
ButtonAreaImpl::construct ()
{
  set_flag (NEEDS_FOCUS_INDICATOR); // prerequisite for focusable
  set_flag (ALLOW_FOCUS);
  SingleContainerImpl::construct();
}

Click
ButtonAreaImpl::click_type () const
{
  return click_type_;
}

void
ButtonAreaImpl::click_type (Click click_type)
{
  reset();
  click_type_ = click_type;
  changed ("click_type");
}

String
ButtonAreaImpl::on_click () const
{
  return on_click_[0];
}

void
ButtonAreaImpl::on_click (const String &command)
{
  on_click_[0] = string_strip (command);
  changed ("on_click");
}

String
ButtonAreaImpl::on_click2 () const
{
  return on_click_[1];
}

void
ButtonAreaImpl::on_click2 (const String &command)
{
  on_click_[1] = string_strip (command);
  changed ("on_click2");
}

String
ButtonAreaImpl::on_click3 () const
{
  return on_click_[2];
}

void
ButtonAreaImpl::on_click3 (const String &command)
{
  on_click_[2] = string_strip (command);
  changed ("on_click3");
}

void
ButtonAreaImpl::dump_private_data (TestStream &tstream)
{
  SingleContainerImpl::dump_private_data (tstream);
  tstream.dump_intern ("button_", button_);
  tstream.dump_intern ("repeater_", repeater_);
}

bool
ButtonAreaImpl::activate_widget ()
{
  ButtonAreaImpl &view = *this;
  WindowImpl *window = get_window();
  EventLoop *loop = window ? window->get_loop() : NULL;
  bool handled = false;
  if (loop)
    {
      view.active (true);
      window->draw_child (view);
      handled = activate_button_command (1);
      if (!unpress_)
        {
          auto unpress_handler = [this, &view] () {
            view.active (false);
            remove_exec (unpress_);
            unpress_ = 0;
            return false;
          };
          unpress_ = loop->exec_timer (unpress_handler, 50, 0);
        }
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
  bool need_repeat = etype == BUTTON_PRESS && (click_type_ == Click::KEY_REPEAT || click_type_ == Click::SLOW_REPEAT || click_type_ == Click::FAST_REPEAT);
  bool need_click = need_repeat;
  need_click |= etype == BUTTON_PRESS && click_type_ == Click::ON_PRESS;
  need_click |= etype == BUTTON_RELEASE && click_type_ == Click::ON_RELEASE;
  bool can_exec = need_click && activate_button_command (button);
  need_repeat &= can_exec;
  if (need_repeat && !repeater_)
    {
      if (click_type_ == Click::FAST_REPEAT)
        repeater_ = exec_fast_repeater (Aida::slot (*this, &ButtonAreaImpl::activate_command));
      else if (click_type_ == Click::SLOW_REPEAT)
        repeater_ = exec_slow_repeater (Aida::slot (*this, &ButtonAreaImpl::activate_command));
      else if (click_type_ == Click::KEY_REPEAT)
        repeater_ = exec_key_repeater (Aida::slot (*this, &ButtonAreaImpl::activate_command));
    }
  else if (!need_repeat && repeater_)
    {
      remove_exec (repeater_);
      repeater_ = 0;
    }
}

void
ButtonAreaImpl::reset (ResetMode mode)
{
  ButtonAreaImpl &view = *this;
  view.active (false);
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
      view.active (button_ != 0);
      view.hover (true);
      break;
    case MOUSE_LEAVE:
      view.hover (false);
      view.active (button_ != 0);
      break;
    case BUTTON_PRESS:
    case BUTTON_2PRESS:
    case BUTTON_3PRESS:
      bevent = dynamic_cast<const EventButton*> (&event);
      if (!button_ and bevent->button >= 1 and bevent->button <= 3 and
          on_click_[bevent->button - 1] != "")
        {
          bool inbutton = view.hover();
          button_ = bevent->button;
          view.active (true);
          if (inbutton)
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
          bool inbutton = view.hover();
          view.get_window()->remove_grab (*this);
          button_ = 0;
          // activation may recurse here
          activate_click (bevent->button, inbutton && proper_release ? BUTTON_RELEASE : BUTTON_CANCELED);
          view.active (false);
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

static const WidgetFactory<ButtonAreaImpl> button_area_factory ("Rapicorn::ButtonArea");

} // Rapicorn
