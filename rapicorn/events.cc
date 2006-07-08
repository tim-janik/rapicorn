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
    case CANCEL_EVENTS:         return "CancelEvents";
    case WIN_SIZE:              return "WinSize";
    case WIN_DRAW:              return "WinDraw";
    case WIN_DELETE:            return "WinDelete";
    case EVENT_NONE:
    case EVENT_LAST:
      break;
    }
  return "<unknown>";
}

EventContext::EventContext () :
  time (0), synthesized (true),
  modifiers (ModifierState (0)),
  x (-1), y (-1)
{}

EventContext::EventContext (const Event &event) :
  time (event.time), synthesized (event.synthesized),
  modifiers (event.modifiers),
  x (event.x), y (event.y)
{}

EventContext&
EventContext::operator= (const Event &event)
{
  time = event.time;
  synthesized = event.synthesized;
  modifiers = event.modifiers;
  x = event.x;
  y = event.y;
  return *this;
}

Event::Event () :
  type (EVENT_NONE), time (0), synthesized (true),
  modifiers (ModifierState (0)), key_state (ModifierState (0)),
  x (-1), y (-1)
{}

Event::~Event()
{
}

class EventFactory {
public:
  template<class EventKind> static EventKind*
  new_event (EventType           type,
             const EventContext &econtext)
  {
    EventKind *event = new EventKind();
    event->type = type;
    event->time = econtext.time;
    event->synthesized = econtext.synthesized;
    event->modifiers = ModifierState (econtext.modifiers & MOD_MASK);
    event->key_state = ModifierState (event->modifiers & MOD_KEY_MASK);
    event->x = econtext.x;
    event->y = econtext.y;
    return event;
  }
};

Event*
create_event_cancellation (const EventContext &econtext)
{
  Event *event = EventFactory::new_event<Event> (CANCEL_EVENTS, econtext);
  return event;
}

EventMouse*
create_event_mouse (EventType           type,
                    const EventContext &econtext)
{
  assert (type >= MOUSE_ENTER && type <= MOUSE_LEAVE);
  EventMouse *event = EventFactory::new_event<EventMouse> (type, econtext);
  return event;
}

EventButton*
create_event_button (EventType           type,
                     const EventContext &econtext,
                     uint                button)
{
  assert (type >= BUTTON_PRESS && type <= BUTTON_3RELEASE);
  assert (button >= 1 && button <= 16);
  EventButton *event = EventFactory::new_event<EventButton> (type, econtext);
  event->button = button;
  return event;
}

EventScroll*
create_event_scroll (EventType           type,
                     const EventContext &econtext)
{
  assert (type == SCROLL_UP || type == SCROLL_RIGHT || type == SCROLL_DOWN || type == SCROLL_LEFT);
  EventScroll *event = EventFactory::new_event<EventScroll> (type, econtext);
  return event;
}

EventFocus*
create_event_focus (EventType           type,
                    const EventContext &econtext)
{
  assert (type == FOCUS_IN || type == FOCUS_OUT);
  EventFocus *event = EventFactory::new_event<EventFocus> (type, econtext);
  return event;
}

EventKey*
create_event_key (EventType           type,
                  const EventContext &econtext,
                  uint32              key,
                  const char         *name)
{
  assert (type == KEY_PRESS || type == KEY_RELEASE || type == KEY_CANCELED);
  EventKey *event = EventFactory::new_event<EventKey> (type, econtext);
  event->key = key;
  event->key_name = name;
  return event;
}

EventWinSize*
create_event_win_size (const EventContext &econtext,
                       uint                draw_stamp,
                       double              width,
                       double              height)
{
  EventWinSize *event = EventFactory::new_event<EventWinSize> (WIN_SIZE, econtext);
  event->draw_stamp = draw_stamp;
  event->width = width;
  event->height = height;
  return event;
}

EventWinDraw*
create_event_win_draw (const EventContext &econtext,
                       uint                     draw_stamp,
                       const std::vector<Rect> &rects)
{
  EventWinDraw *event = EventFactory::new_event<EventWinDraw> (WIN_DRAW, econtext);
  event->draw_stamp = draw_stamp;
  event->rectangles = rects;
  for (uint i = 0; i < rects.size(); i++)
    event->bbox.rect_union(rects[i]);
  return event;
}

EventWinDelete*
create_event_win_delete (const EventContext &econtext)
{
  EventWinDelete *event = EventFactory::new_event<EventWinDelete> (WIN_DELETE, econtext);
  return event;
}

} // Rapicorn
