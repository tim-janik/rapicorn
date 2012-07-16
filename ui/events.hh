// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_EVENT_HH__
#define __RAPICORN_EVENT_HH__

#include <ui/primitives.hh>

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
#include <ui/keycodes.hh>
};
unichar      key_value_to_unichar     (uint32 keysym);
bool         key_value_is_modifier    (uint32 keysym);
bool         key_value_is_accelerator (uint32 keysym);
FocusDirType key_value_to_focus_dir   (uint32 keysym);
bool         key_value_is_focus_dir   (uint32 keysym);

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
  WIN_DELETE,
  EVENT_LAST
} EventType;
const char* string_from_event_type (EventType etype);

struct EventContext;
class Event : public Deletable, protected NonCopyable {
protected:
  explicit        Event (EventType, const EventContext&);
public:
  virtual        ~Event();
  const EventType type;
  uint32          time;
  bool            synthesized;
  ModifierState   modifiers;
  ModifierState   key_state; /* modifiers & MOD_KEY_MASK */
  double          x, y;
};
typedef Event EventMouse;
class EventButton : public Event {
protected:
  explicit        EventButton (EventType, const EventContext&, uint);
public:
  virtual        ~EventButton();
  /* button press/release */
  uint          button; /* 1, 2, 3 */
};
typedef Event EventScroll;
typedef Event EventFocus;
class EventKey : public Event {
protected:
  explicit        EventKey (EventType, const EventContext&, uint32, const String &);
public:
  virtual        ~EventKey();
  /* key press/release */
  uint32          key;  /* of type KeyValue */
  String          key_name;
};
struct EventWinSize : public Event {
protected:
  explicit        EventWinSize (EventType, const EventContext&, double, double, bool);
public:
  virtual        ~EventWinSize();
  double          width, height;
  bool            intermediate;
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

Event*          create_event_transformed  (const Event        &event,
                                           const Affine       &affine);
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
                                           double              width,
                                           double              height,
                                           bool                intermediate);
EventWinDelete* create_event_win_delete   (const EventContext &econtext);

} // Rapicorn

#endif  /* __RAPICORN_EVENT_HH__ */
