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
#include "events.hh"

namespace Rapicorn {

String
string_from_event_type (EventType etype)
{
  switch (etype)
    {
    case MOUSE_ENTER:           return "MouseEnter";
    case MOUSE_MOVE:            return "MouseMove";
    case MOUSE_LEAVE:           return "MouseLeave";
    case BUTTON_PRESS:          return "ButtonPress";
    case BUTTON_2PRESS:         return "Button2Press";
    case BUTTON_3PRESS:         return "Button3Press";
    case BUTTON_CANCELED:       return "ButtonCanceled";
    case BUTTON_RELEASE:        return "ButtonRelease";
    case BUTTON_2RELEASE:       return "Button2Release";
    case BUTTON_3RELEASE:       return "Button3Release";
    case FOCUS_IN:              return "FocusIn";
    case FOCUS_OUT:             return "FocusOut";
    case KEY_PRESS:             return "KeyPress";
    case KEY_CANCELED:          return "KeyCanceled";
    case KEY_RELEASE:           return "KeyRelease";
    case SCROLL_UP:             return "ScrollUp";
    case SCROLL_DOWN:           return "ScrollDown";
    case SCROLL_LEFT:           return "ScrollLeft";
    case SCROLL_RIGHT:          return "ScrollRight";
    case EVENT_NONE:
    case EVENT_LAST:
      break;
    }
  return "<unknown>";
}

Event::~Event()
{
}

static void
setup_event (Event              *event,
             EventType           type,
             const EventContext &econtext)
{
  event->type = type;
  event->time = econtext.time;
  event->synthesized = econtext.synthesized;
  event->modifiers = ModifierState (econtext.modifiers & MOD_MASK);
  event->key_state = ModifierState (event->modifiers & MOD_KEY_MASK);
  event->x = econtext.x;
  event->y = econtext.y;
  event->button = 0;
  event->key = 0;
  event->key_name = "";
}

EventMouse*
create_event_mouse (EventType           type,
                    const EventContext &econtext)
{
  assert (type >= MOUSE_ENTER && type <= MOUSE_LEAVE);
  EventMouse *event = new EventMouse;
  setup_event (event, type, econtext);
  return event;
}

EventButton*
create_event_button (EventType           type,
                     const EventContext &econtext,
                     uint                button)
{
  assert (type >= BUTTON_PRESS && type <= BUTTON_3RELEASE);
  assert (button >= 1 && button <= 16);
  EventButton *event = new EventButton;
  setup_event (event, type, econtext);
  event->button = button;
  return event;
}

EventScroll*
create_event_scroll (EventType           type,
                     const EventContext &econtext)
{
  assert (type == SCROLL_UP || type == SCROLL_RIGHT || type == SCROLL_DOWN || type == SCROLL_LEFT);
  EventScroll *event = new EventScroll;
  setup_event (event, type, econtext);
  return event;
}

EventFocus*
create_event_focus (EventType           type,
                    const EventContext &econtext)
{
  assert (type == FOCUS_IN || type == FOCUS_OUT);
  EventFocus *event = new EventFocus;
  setup_event (event, type, econtext);
  return event;
}

EventKey*
create_event_key (EventType           type,
                  const EventContext &econtext,
                  uint32              key,
                  const char         *name)
{
  assert (type == KEY_PRESS || type == KEY_RELEASE || type == KEY_CANCELED);
  EventKey *event = new EventKey;
  setup_event (event, type, econtext);
  event->key = key;
  event->key_name = name;
  return event;
}

} // Rapicorn
