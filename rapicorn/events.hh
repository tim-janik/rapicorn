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
  EVENT_LAST
} EventType;
String string_from_event_type (EventType etype);

struct Event {
  EventType     type; /* of type EventType */
  uint32        time;
  bool          synthesized;
  ModifierState modifiers;
  ModifierState key_state; /* modifiers & MOD_KEY_MASK */
  double        x, y;
  virtual       ~Event();
  /* button press/release */
  uint          button; /* 1, 2, 3 */
  /* key press/release */
  uint32        key;    /* of type KeyValue */
  String        key_name;
};
struct EventMouse  : Event {};
struct EventButton : EventMouse { /* buton */ };
struct EventScroll : EventMouse {};
struct EventFocus  : Event {};
struct EventKey    : Event { /* key, key_name */ };

struct EventContext {
  uint32        time;
  bool          synthesized;
  ModifierState modifiers;
  double        x, y;
  EventContext() : time (0), synthesized (true), modifiers (ModifierState (0)), x (-1), y (-1) {}
};

EventMouse*     create_event_mouse      (EventType           type,
                                         const EventContext &econtext);
EventButton*    create_event_button     (EventType           type,
                                         const EventContext &econtext,
                                         uint                button);
EventScroll*    create_event_scroll     (EventType           type,
                                         const EventContext &econtext);
EventFocus*     create_event_focus      (EventType           type,
                                         const EventContext &econtext);
EventKey*       create_event_key        (EventType           type,
                                         const EventContext &econtext,
                                         uint32              key,
                                         const char         *name);

} // Rapicorn

#endif  /* __RAPICORN_EVENT_HH__ */
