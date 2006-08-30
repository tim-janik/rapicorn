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

const char*
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
    default:                    return "<unknown>";
    }
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

struct EventFactory {
  static void                                   copy_transform_event_base       (Event                  &event,
                                                                                 const Event            &source,
                                                                                 const Affine           &affine);
  template<class EventKind> static Event&       copy_transform_event            (const Event            &source_base,
                                                                                 const Affine           &affine);
  template<class EventKind> static EventKind*   new_event                       (EventType               type,
                                                                                 const EventContext     &econtext);
};

void
EventFactory::copy_transform_event_base (Event        &event,
                                         const Event  &source,
                                         const Affine &affine)
{
  event.type = source.type;
  event.time = source.time;
  event.synthesized = source.synthesized;
  event.modifiers = source.modifiers;
  event.key_state = source.key_state;
  Point p = affine.point (Point (source.x, source.y));
  event.x = p.x;
  event.y = p.y;
}

template<> Event&
EventFactory::copy_transform_event<Event> (const Event  &source_base,
                                           const Affine &affine)
{
  Event &event = *new Event();
  copy_transform_event_base (event, source_base, affine);
  return event;
}

template<> Event&
EventFactory::copy_transform_event<EventButton> (const Event  &source_base,
                                                 const Affine &affine)
{
  const EventButton &source = dynamic_cast<const EventButton&> (source_base);
  EventButton &event = *new EventButton();
  copy_transform_event_base (event, source, affine);
  event.button = source.button;
  return event;
}

template<> Event&
EventFactory::copy_transform_event<EventKey> (const Event  &source_base,
                                              const Affine &affine)
{
  const EventKey &source = dynamic_cast<const EventKey&> (source_base);
  EventKey &event = *new EventKey();
  copy_transform_event_base (event, source, affine);
  event.key = source.key;
  event.key_name = source.key_name;
  return event;
}

template<> Event&
EventFactory::copy_transform_event<EventWinSize> (const Event  &source_base,
                                                  const Affine &affine)
{
  const EventWinSize &source = dynamic_cast<const EventWinSize&> (source_base);
  EventWinSize &event = *new EventWinSize();
  copy_transform_event_base (event, source, affine);
  event.draw_stamp = source.draw_stamp;
  event.width = affine.hexpansion() * source.width;
  event.height = affine.vexpansion() * source.height;
  return event;
}

template<> Event&
EventFactory::copy_transform_event<EventWinDraw> (const Event  &source_base,
                                                  const Affine &affine)
{
  const EventWinDraw &source = dynamic_cast<const EventWinDraw&> (source_base);
  EventWinDraw &event = *new EventWinDraw();
  copy_transform_event_base (event, source, affine);
  event.draw_stamp = source.draw_stamp;
  event.bbox = Rect (affine.point (source.bbox.ll), affine.point (source.bbox.ur));
  for (uint i = 0; i < source.rectangles.size(); i++)
    event.rectangles.push_back (Rect (affine.point (source.rectangles[i].ll), affine.point (source.rectangles[i].ur)));
  return event;
}

Event*
create_event_transformed (const Event        &source_event,
                          const Affine       &affine)
{
  switch (source_event.type)
    {
    case MOUSE_ENTER:
    case MOUSE_MOVE:
    case MOUSE_LEAVE:           return &EventFactory::copy_transform_event<EventMouse> (source_event, affine);
    case BUTTON_PRESS:
    case BUTTON_2PRESS:
    case BUTTON_3PRESS:
    case BUTTON_CANCELED:
    case BUTTON_RELEASE:
    case BUTTON_2RELEASE:
    case BUTTON_3RELEASE:       return &EventFactory::copy_transform_event<EventButton> (source_event, affine);
    case FOCUS_IN:
    case FOCUS_OUT:             return &EventFactory::copy_transform_event<EventFocus> (source_event, affine);
    case KEY_PRESS:
    case KEY_CANCELED:
    case KEY_RELEASE:           return &EventFactory::copy_transform_event<EventKey> (source_event, affine);
    case SCROLL_UP:
    case SCROLL_DOWN:
    case SCROLL_LEFT:
    case SCROLL_RIGHT:          return &EventFactory::copy_transform_event<EventScroll> (source_event, affine);
    case CANCEL_EVENTS:         return &EventFactory::copy_transform_event<Event> (source_event, affine);
    case WIN_SIZE:              return &EventFactory::copy_transform_event<EventWinSize> (source_event, affine);
    case WIN_DRAW:              return &EventFactory::copy_transform_event<EventWinDraw> (source_event, affine);
    case WIN_DELETE:            return &EventFactory::copy_transform_event<EventWinDelete> (source_event, affine);
    case EVENT_NONE:
    case EVENT_LAST:
    default:                    throw Exception ("invalid event type for copy");
    }
}

template<class EventKind> EventKind*
EventFactory::new_event (EventType           type,
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
