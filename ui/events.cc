// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "events.hh"

namespace Rapicorn {

Event::Event (EventType           etype,
              const EventContext &econtext) :
  type (etype), time (econtext.time),
  synthesized (econtext.synthesized),
  modifiers (ModifierState (econtext.modifiers & MOD_MASK)),
  key_state (ModifierState (modifiers & MOD_KEY_MASK)),
  x (econtext.x), y (econtext.y)
{}

Event::~Event()
{}

class EventImpl : public Event {
public:
  explicit EventImpl (EventType           etype,
                      const EventContext &econtext) :
    Event (etype, econtext)
  {
    assert ((etype >= MOUSE_ENTER && etype <= MOUSE_LEAVE) ||
            (etype >= FOCUS_IN    && etype <= FOCUS_OUT) ||
            (etype >= SCROLL_UP   && etype <= SCROLL_RIGHT) ||
            etype == CANCEL_EVENTS ||
            etype == WIN_DELETE);
  }
};

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

Event*
create_event_transformed (const Event  &source_event,
                          const Affine &affine)
{
  EventContext dcontext (source_event);
  Point p = affine.point (Point (dcontext.x, dcontext.y));
  dcontext.x = p.x;
  dcontext.y = p.y;
  switch (source_event.type)
    {
    case MOUSE_ENTER:
    case MOUSE_MOVE:
    case MOUSE_LEAVE:           return create_event_mouse (source_event.type, dcontext);
    case BUTTON_PRESS:
    case BUTTON_2PRESS:
    case BUTTON_3PRESS:
    case BUTTON_CANCELED:
    case BUTTON_RELEASE:
    case BUTTON_2RELEASE:
    case BUTTON_3RELEASE:       return create_event_button (source_event.type, dcontext,
                                                            dynamic_cast<const EventButton*> (&source_event)->button);
    case FOCUS_IN:
    case FOCUS_OUT:             return create_event_focus (source_event.type, dcontext);
    case KEY_PRESS:
    case KEY_CANCELED:
    case KEY_RELEASE:
      {
        const EventKey *key_event = dynamic_cast<const EventKey*> (&source_event);
        return create_event_key (source_event.type, dcontext, key_event->key, key_event->key_name.c_str());
      }
    case SCROLL_UP:
    case SCROLL_DOWN:
    case SCROLL_LEFT:
    case SCROLL_RIGHT:          return create_event_scroll (source_event.type, dcontext);
    case CANCEL_EVENTS:         return create_event_cancellation (dcontext);
    case WIN_SIZE:
      {
        const EventWinSize *source = dynamic_cast<const EventWinSize*> (&source_event);
        return create_event_win_size (dcontext, affine.hexpansion() * source->width, affine.vexpansion() * source->height, source->intermediate);
      }
    case WIN_DRAW:
      {
        const EventWinDraw *source = dynamic_cast<const EventWinDraw*> (&source_event);
        std::vector<Rect> rects;
        for (uint i = 0; i < source->rectangles.size(); i++)
          rects.push_back (Rect (affine.point (source->rectangles[i].lower_left()), affine.point (source->rectangles[i].upper_right()))); // FIXME: transform 4 points
        return create_event_win_draw (dcontext, source->draw_stamp, rects);
      }
    case WIN_DELETE:            return create_event_win_delete (dcontext);
    case EVENT_NONE:
    case EVENT_LAST:
    default:                    fatal ("uncopyable event type: %s", string_from_event_type (source_event.type));
    }
}

Event*
create_event_cancellation (const EventContext &econtext)
{
  Event *event = new EventImpl (CANCEL_EVENTS, econtext);
  return event;
}

EventMouse*
create_event_mouse (EventType           type,
                    const EventContext &econtext)
{
  assert (type >= MOUSE_ENTER && type <= MOUSE_LEAVE);
  EventMouse *event = new EventImpl (type, econtext);
  return event;
}

EventButton::~EventButton()
{}

EventButton::EventButton (EventType           etype,
                          const EventContext &econtext,
                          uint                btn) :
  Event (etype, econtext),
  button (btn)
{}

EventButton*
create_event_button (EventType           type,
                     const EventContext &econtext,
                     uint                button)
{
  struct EventButtonImpl : public EventButton {
    EventButtonImpl (EventType           etype,
                     const EventContext &econtext,
                     uint                btn) :
      EventButton (etype, econtext, btn)
    {}
  };
  assert (type >= BUTTON_PRESS && type <= BUTTON_3RELEASE);
  assert (button >= 1 && button <= 16);
  return new EventButtonImpl (type, econtext, button);
}

EventScroll*
create_event_scroll (EventType           type,
                     const EventContext &econtext)
{
  assert (type == SCROLL_UP || type == SCROLL_RIGHT || type == SCROLL_DOWN || type == SCROLL_LEFT);
  EventMouse *event = new EventImpl (type, econtext);
  return event;
}

EventFocus*
create_event_focus (EventType           type,
                    const EventContext &econtext)
{
  assert (type == FOCUS_IN || type == FOCUS_OUT);
  EventMouse *event = new EventImpl (type, econtext);
  return event;
}

EventKey::~EventKey()
{}

EventKey::EventKey (EventType           etype,
                    const EventContext &econtext,
                    uint32              _key,
                    const String       &_key_name) :
  Event (etype, econtext),
  key (_key), key_name (_key_name)
{}

EventKey*
create_event_key (EventType           type,
                  const EventContext &econtext,
                  uint32              key,
                  const char         *name)
{
  struct EventKeyImpl : public EventKey {
    EventKeyImpl (EventType           etype,
                  const EventContext &econtext,
                  uint32              _key,
                  const String       &_key_name) :
      EventKey (etype, econtext, _key, _key_name)
    {}
  };
  assert (type == KEY_PRESS || type == KEY_RELEASE || type == KEY_CANCELED);
  EventKey *kevent = new EventKeyImpl (type, econtext, key, name);
  Event &test = *kevent;
  assert (dynamic_cast<const EventKey*> (&test) != NULL);
  return kevent;
}

EventWinSize::~EventWinSize()
{}

EventWinSize::EventWinSize (EventType           etype,
                            const EventContext &econtext,
                            double              _width,
                            double              _height,
                            bool                _intermediate) :
  Event (etype, econtext),
  intermediate (_intermediate),
  width (_width), height (_height)
{}

EventWinSize*
create_event_win_size (const EventContext &econtext,
                       double              width,
                       double              height,
                       bool                intermediate)
{
  struct EventWinSizeImpl : public EventWinSize {
    EventWinSizeImpl (EventType           etype,
                      const EventContext &econtext,
                      double              _width,
                      double              _height,
                      bool                _intermediate) :
      EventWinSize (etype, econtext, _width, _height, _intermediate)
    {}
  };
  EventWinSize *wevent = new EventWinSizeImpl (WIN_SIZE, econtext, width, height, intermediate);
  return wevent;
}

EventWinDraw::~EventWinDraw()
{}

EventWinDraw::EventWinDraw (EventType                etype,
                            const EventContext      &econtext,
                            uint                     _draw_stamp,
                            const std::vector<Rect> &_rects) :
  Event (etype, econtext),
  draw_stamp (_draw_stamp),
  rectangles (_rects)
{
  for (uint i = 0; i < rectangles.size(); i++)
    bbox.rect_union (rectangles[i]);
}

EventWinDraw*
create_event_win_draw (const EventContext      &econtext,
                       uint                     draw_stamp,
                       const std::vector<Rect> &rects)
{
  struct EventWinDrawImpl : public EventWinDraw {
    EventWinDrawImpl (EventType                etype,
                      const EventContext      &econtext,
                      uint                     _draw_stamp,
                      const std::vector<Rect> &_rects) :
      EventWinDraw (etype, econtext, _draw_stamp, _rects)
    {}
  };
  EventWinDraw *wevent = new EventWinDrawImpl (WIN_DRAW, econtext, draw_stamp, rects);
  return wevent;
}

EventWinDelete*
create_event_win_delete (const EventContext &econtext)
{
  EventMouse *event = new EventImpl (WIN_DELETE, econtext);
  return event;
}

static const unsigned short modifier_table[] = {
  KEY_Shift_L,          KEY_Shift_R,            KEY_Shift_Lock,         KEY_Caps_Lock,  KEY_ISO_Lock,
  KEY_Control_L,        KEY_Control_R,
  KEY_Alt_L,            KEY_Alt_R,              KEY_Meta_L,             KEY_Meta_R,
  KEY_Super_L,          KEY_Super_R,            KEY_Hyper_L,            KEY_Hyper_R,
  KEY_Num_Lock,         KEY_Scroll_Lock,
  KEY_Mode_switch,      KEY_Multi_key,
  KEY_ISO_Level3_Shift,
};

bool
key_value_is_modifier (uint32 keysym)
{
  for (uint i = 0; i < ARRAY_SIZE (modifier_table); i++)
    if (keysym == modifier_table[i])
      return true;
  return false;
}

static const unsigned short non_accelerator_table[] = {
  KEY_Sys_Req,
  KEY_Tab,
  KEY_ISO_Left_Tab,             KEY_KP_Tab,
  KEY_ISO_First_Group,          KEY_ISO_Last_Group,
  KEY_ISO_Next_Group,           KEY_ISO_Prev_Group,
  KEY_First_Virtual_Screen,     KEY_Last_Virtual_Screen,
  KEY_Prev_Virtual_Screen,      KEY_Next_Virtual_Screen,
  KEY_Terminate_Server,         KEY_AudibleBell_Enable,
};

bool
key_value_is_accelerator (uint32 keysym)
{
  if (key_value_is_modifier (keysym))
    return false;
  for (uint i = 0; i < ARRAY_SIZE (non_accelerator_table); i++)
    if (keysym == non_accelerator_table[i])
      return false;
  return true;
}

FocusDirType
key_value_to_focus_dir (uint32 keysym)
{
  switch (keysym)
    {
    case KEY_Tab: case KEY_KP_Tab:      return FOCUS_NEXT;
    case KEY_ISO_Left_Tab:              return FOCUS_PREV;
    case KEY_Right:                     return FOCUS_RIGHT;
    case KEY_Up:                        return FOCUS_UP;
    case KEY_Left:                      return FOCUS_LEFT;
    case KEY_Down:                      return FOCUS_DOWN;
    default:                            return FocusDirType (0);
    }
}

bool
key_value_is_focus_dir (uint32 keysym)
{
  return key_value_to_focus_dir (keysym) != 0;
}

} // Rapicorn

/* implement key_value_to_unichar() */
#include "key2utf8.cc"
