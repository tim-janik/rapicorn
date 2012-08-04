// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "screenwindow.hh"
#include <list>
#include <algorithm>

#define SDEBUG(...)     RAPICORN_KEY_DEBUG ("ScreenDriver", __VA_ARGS__)

namespace Rapicorn {

// == ScreenWindow ==
ScreenWindow::ScreenWindow () :
  m_accessed (0)
{}

ScreenWindow::~ScreenWindow ()
{
  std::list<Event*> events;
  {
    ScopedLock<Mutex> sl (m_async_mutex);
    events.swap (m_async_event_queue);
  }
  while (!events.empty())
    {
      Event *e = events.front ();
      events.pop_front();
      delete e;
    }
  critical_unless (m_async_event_queue.empty()); // this queue must not be accessed at this point
}

void
ScreenWindow::enqueue_event (Event *event)
{
  critical_unless (event);
  ScopedLock<Mutex> sl (m_async_mutex);
  const bool notify = m_async_event_queue.empty();
  m_async_event_queue.push_back (event);
  if (notify && m_async_wakeup)
    m_async_wakeup();
}

void
ScreenWindow::push_event (Event *event)
{
  critical_unless (event);
  ScopedLock<Mutex> sl (m_async_mutex);
  const bool notify = m_async_event_queue.empty();
  m_async_event_queue.push_front (event);
  if (notify && m_async_wakeup)
    m_async_wakeup();
}

Event*
ScreenWindow::pop_event ()
{
  ScopedLock<Mutex> sl (m_async_mutex);
  if (m_async_event_queue.empty())
    return NULL;
  Event *event = m_async_event_queue.front();
  m_async_event_queue.pop_front();
  return event;
}

bool
ScreenWindow::has_event ()
{
  ScopedLock<Mutex> sl (m_async_mutex);
  return !m_async_event_queue.empty();
}

bool
ScreenWindow::peek_events (const std::function<bool (Event*)> &pred)
{
  ScopedLock<Mutex> sl (m_async_mutex);
  for (auto ep : m_async_event_queue)
    if (pred (ep))
      return true;
  return false;
}

void
ScreenWindow::set_event_wakeup (const std::function<void()> &wakeup)
{
  ScopedLock<Mutex> sl (m_async_mutex);
  m_async_wakeup = wakeup;
}

bool
ScreenWindow::update_state (const State &state)
{
  ScopedLock<Spinlock> sl (m_spin);
  const bool accessed = m_accessed;
  m_accessed = false;
  m_state = state;
  return accessed;
}

ScreenWindow::State
ScreenWindow::get_state ()
{
  ScopedLock<Spinlock> sl (m_spin);
  return m_state;
}

bool
ScreenWindow::viewable ()
{
  ScopedLock<Spinlock> sl (m_spin);
  bool viewable = m_state.visible;
  viewable = viewable && !(m_state.window_flags & (ICONIFY | HIDDEN | SHADED));
  return viewable;
}

void
ScreenWindow::configure (const Config &config)
{
  queue_command (new Command (CMD_CONFIGURE, config));
}

void
ScreenWindow::beep ()
{
  queue_command (new Command (CMD_BEEP));
}

void
ScreenWindow::show ()
{
  queue_command (new Command (CMD_SHOW));
}

void
ScreenWindow::present ()
{
  queue_command (new Command (CMD_PRESENT));
}

void
ScreenWindow::blit_surface (cairo_surface_t *surface, const Rapicorn::Region &region)
{
  queue_command (new Command (CMD_BLIT, surface, region));
}

void
ScreenWindow::start_user_move (uint button, double root_x, double root_y)
{
  queue_command (new Command (CMD_MOVE, button, root_x, root_y));
}

void
ScreenWindow::start_user_resize (uint button, double root_x, double root_y, AnchorType edge)
{
  queue_command (new Command (CMD_RESIZE, button, root_x, root_y));
}

void
ScreenWindow::destroy ()
{
  queue_command (new Command (CMD_DESTROY));
}

static const char*
flag_name (uint64 flag)
{
  switch (ScreenWindow::Flags (flag))
    {
    case ScreenWindow::MODAL:		return "MODAL";
    case ScreenWindow::STICKY:	        return "STICKY";
    case ScreenWindow::VMAXIMIZED:	return "VMAXIMIZED";
    case ScreenWindow::HMAXIMIZED:	return "HMAXIMIZED";
    case ScreenWindow::SHADED:	        return "SHADED";
    case ScreenWindow::SKIP_TASKBAR:	return "SKIP_TASKBAR";
    case ScreenWindow::SKIP_PAGER:	return "SKIP_PAGER";
    case ScreenWindow::HIDDEN:	        return "HIDDEN";
    case ScreenWindow::FULLSCREEN:	return "FULLSCREEN";
    case ScreenWindow::ABOVE_ALL:	return "ABOVE_ALL";
    case ScreenWindow::BELOW_ALL:	return "BELOW_ALL";
    case ScreenWindow::ATTENTION:	return "ATTENTION";
    case ScreenWindow::FOCUS_DECO:	return "FOCUS_DECO";
    case ScreenWindow::DECORATED:	return "DECORATED";
    case ScreenWindow::MINIMIZABLE:	return "MINIMIZABLE";
    case ScreenWindow::MAXIMIZABLE:	return "MAXIMIZABLE";
    case ScreenWindow::DELETABLE:	return "DELETABLE";
    case ScreenWindow::ACCEPT_FOCUS:	return "ACCEPT_FOCUS";
    case ScreenWindow::UNFOCUSED:	return "UNFOCUSED";
    case ScreenWindow::ICONIFY:	        return "ICONIFY";
    case ScreenWindow::_WM_STATE_MASK:
    case ScreenWindow::_DECO_MASK:
      ; // pass
    }
 return "UNKNOWN";
}

String
ScreenWindow::flags_name (uint64 flags, String combo)
{
  const uint64 I = 1;
  String result;
  for (size_t i = 0; i < 64; i++)
    if (flags & (I << i))
      result += (result.empty() ? "" : combo) + flag_name (I << i);
  return result;
}

// == ScreenWindow::Command ==
ScreenWindow::Command::Command (CommandType ctype) :
  type (ctype), config (NULL), setup (NULL)
{
  assert (type == CMD_BEEP || type == CMD_SHOW || type == CMD_PRESENT || type == CMD_DESTROY);
}

ScreenWindow::Command::Command (CommandType ctype, const Config &cfg) :
  type (ctype), config (NULL), setup (NULL)
{
  assert (type == CMD_CONFIGURE);
  config = new Config (cfg);
}

ScreenWindow::Command::Command (CommandType ctype, const ScreenWindow::Setup &cs, const ScreenWindow::Config &cfg) :
  type (ctype), config (NULL), setup (NULL)
{
  assert (type == CMD_CREATE);
  setup = new Setup (cs);
  config = new Config (cfg);
}

ScreenWindow::Command::Command (CommandType ctype, cairo_surface_t *csurface, const Rapicorn::Region &cregion) :
  type (ctype), config (NULL), setup (NULL)
{
  assert (type == CMD_BLIT);
  surface = cairo_surface_reference (csurface);
  region = new Rapicorn::Region (cregion);
}

ScreenWindow::Command::Command (CommandType ctype, int cbutton, int croot_x, int croot_y) :
  type (ctype), config (NULL), setup (NULL)
{
  assert (type == CMD_MOVE || type == CMD_RESIZE);
  cbutton = button;
  croot_x = root_x;
  croot_y = root_y;
}

ScreenWindow::Command::~Command()
{
  switch (type)
    {
    case CMD_CREATE:
      delete setup;
      delete config;
      // rc = NULL;
      break;
    case CMD_CONFIGURE:
      delete config;
      assert (!setup);
      break;
    case CMD_BLIT:
      cairo_surface_destroy (surface);
      delete region;
      break;
    case CMD_MOVE: case CMD_RESIZE:
      button = root_x = root_y = 0;
      break;
    case CMD_BEEP: case CMD_SHOW: case CMD_PRESENT: case CMD_DESTROY:
      assert (!config && !setup);
      break;
    }
}

// == ScreenDriver ==
static Mutex                 screen_driver_mutex;
static ScreenDriver         *screen_driver_chain = NULL;

ScreenDriver::ScreenDriver (const String &name, int priority) :
  m_sibling (NULL), m_name (name), m_priority (priority)
{
  ScopedLock<Mutex> locker (screen_driver_mutex);
  m_sibling = screen_driver_chain;
  screen_driver_chain = this;
}

bool
ScreenDriver::driver_priority_lesser (ScreenDriver *d1, ScreenDriver *d2)
{
  return d1->m_priority < d2->m_priority;
}

ScreenDriver*
ScreenDriver::open_screen_driver (const String &backend_name)
{
  ScopedLock<Mutex> locker (screen_driver_mutex);
  vector<ScreenDriver*> screen_driver_array;
  for (ScreenDriver *it = screen_driver_chain; it; it = it->m_sibling)
    screen_driver_array.push_back (it);
  SDEBUG ("trying to open 1/%zd screen drivers...", screen_driver_array.size());
  sort (screen_driver_array.begin(), screen_driver_array.end(), driver_priority_lesser);
  for (auto it : screen_driver_array)
    {
      const char *r;
      if (it->m_name != backend_name && backend_name != "auto")
        r = "not selected";
      else if (it->open())
        r = NULL;
      else
        r = "failed to open";
      SDEBUG ("screen driver %s: %s", CQUOTE (it->m_name), r ? r : "success");
      if (r == NULL)
        return it;
    }
  return NULL;
}

} // Rapicorn
