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
#ifndef __RAPICORN_EVENT_HH__
#define __RAPICORN_EVENT_HH__

#include <rapicorn/primitives.hh>

namespace Rapicorn {

enum ModifierState {
  MOD_0         = 0,
  MOD_SHIFT     = 1 << 0,
  MOD_CAPS_LOCK = 1 << 1,
  MOD_CONTROL   = 1 << 2,
  MOD_ALT       = 1 << 3,
  MOD_MOD1      = MOD_ALT,
  MOD_MOD2      = 1 << 4,
  MOD_MOD3      = 1 << 5,
  MOD_MOD4      = 1 << 6,
  MOD_MOD5      = 1 << 7,
  MOD_BUTTON1   = 1 << 8,
  MOD_BUTTON2   = 1 << 9,
  MOD_BUTTON3   = 1 << 10,
  MOD_KEY_MASK  = (MOD_SHIFT | MOD_CONTROL | MOD_ALT),
  MOD_MASK      = 0x07ff,
};

enum KeyValue {
#include <rapicorn/keycodes.hh>
};

typedef enum {
  EVENT_NONE,
  MOUSE_ENTER,
  MOUSE_MOVE,
  MOUSE_LEAVE,
  BUTTON_PRESS,
  BUTTON_2PRESS,
  BUTTON_3PRESS,
  BUTTON_CANCELED,
  BUTTON_RELEASE,
  BUTTON_2RELEASE,
  BUTTON_3RELEASE,
  FOCUS_IN,
  FOCUS_OUT,
  KEY_PRESS,
  KEY_CANCELED,
  KEY_RELEASE,
  SCROLL_UP,          /* button4 */
  SCROLL_DOWN,        /* button5 */
  SCROLL_LEFT,        /* button6 */
  SCROLL_RIGHT,       /* button7 */
  CANCEL_EVENTS,
  WIN_SIZE,
  WIN_DRAW,
  WIN_DELETE,
  EVENT_LAST
} EventType;
String string_from_event_type (EventType etype);

class Event {
  friend class EventFactory;
  BIRNET_PRIVATE_CLASS_COPY (Event);
protected:
  explicit      Event();
public:
  virtual       ~Event();
  EventType     type;
  uint32        time;
  bool          synthesized;
  ModifierState modifiers;
  ModifierState key_state; /* modifiers & MOD_KEY_MASK */
  double        x, y;
};
typedef Event EventMouse;
class EventButton : public Event {
  friend class EventFactory;
  EventButton() {}
  BIRNET_PRIVATE_CLASS_COPY (EventButton);
public:
  /* button press/release */
  uint          button; /* 1, 2, 3 */
};
typedef Event EventScroll;
typedef Event EventFocus;
class EventKey : public Event {
  friend class EventFactory;
  EventKey() {}
  BIRNET_PRIVATE_CLASS_COPY (EventKey);
public:
  /* key press/release */
  uint32        key;    /* of type KeyValue */
  String        key_name;
};
struct EventWinSize : public Event {
  friend class EventFactory;
  EventWinSize() {}
  BIRNET_PRIVATE_CLASS_COPY (EventWinSize);
public:
  uint   draw_stamp;
  double width, height;
};
struct EventWinDraw : public Event {
  friend class EventFactory;
  EventWinDraw() {}
  BIRNET_PRIVATE_CLASS_COPY (EventWinDraw);
public:
  uint              draw_stamp;
  Rect              bbox; /* bounding box */
  std::vector<Rect> rectangles;
};
typedef Event EventWinDelete;
struct EventContext {
  uint32        time;
  bool          synthesized;
  ModifierState modifiers;
  double        x, y;
  explicit      EventContext ();
  explicit      EventContext (const Event&);
  EventContext& operator=    (const Event&);
};

Event*          create_event_cancellation (const EventContext &econtext);
EventMouse*     create_event_mouse        (EventType           type,
                                           const EventContext &econtext);
EventButton*    create_event_button       (EventType           type,
                                           const EventContext &econtext,
                                           uint                button);
EventScroll*    create_event_scroll       (EventType           type,
                                           const EventContext &econtext);
EventFocus*     create_event_focus        (EventType           type,
                                           const EventContext &econtext);
EventKey*       create_event_key          (EventType           type,
                                           const EventContext &econtext,
                                           uint32              key,
                                           const char         *name);
EventWinSize*   create_event_win_size     (const EventContext &econtext,
                                           uint                draw_stamp,
                                           double              width,
                                           double              height);
EventWinDraw*   create_event_win_draw     (const EventContext &econtext,
                                           uint                draw_stamp,
                                           const std::vector<Rect> &rects);
EventWinDelete* create_event_win_delete   (const EventContext &econtext);

} // Rapicorn

#endif  /* __RAPICORN_EVENT_HH__ */
