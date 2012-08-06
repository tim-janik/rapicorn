// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "screenwindow.hh"
#include <list>
#include <algorithm>

#define SDEBUG(...)     RAPICORN_KEY_DEBUG ("ScreenDriver", __VA_ARGS__)

namespace Rapicorn {

// == ScreenWindow ==
ScreenWindow::ScreenWindow () :
  m_async_state_accessed (0)
{}

ScreenWindow::~ScreenWindow ()
{
  std::list<Event*> events;
  {
    ScopedLock<Spinlock> sl (m_async_spin);
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
  ScopedLock<Spinlock> sl (m_async_spin);
  const bool notify = m_async_event_queue.empty();
  m_async_event_queue.push_back (event);
  if (notify && m_async_wakeup)
    m_async_wakeup();
}

void
ScreenWindow::push_event (Event *event)
{
  critical_unless (event);
  ScopedLock<Spinlock> sl (m_async_spin);
  const bool notify = m_async_event_queue.empty();
  m_async_event_queue.push_front (event);
  if (notify && m_async_wakeup)
    m_async_wakeup();
}

Event*
ScreenWindow::pop_event ()
{
  ScopedLock<Spinlock> sl (m_async_spin);
  if (m_async_event_queue.empty())
    return NULL;
  Event *event = m_async_event_queue.front();
  m_async_event_queue.pop_front();
  return event;
}

bool
ScreenWindow::has_event ()
{
  ScopedLock<Spinlock> sl (m_async_spin);
  return !m_async_event_queue.empty();
}

bool
ScreenWindow::peek_events (const std::function<bool (Event*)> &pred)
{
  ScopedLock<Spinlock> sl (m_async_spin);
  for (auto ep : m_async_event_queue)
    if (pred (ep))
      return true;
  return false;
}

void
ScreenWindow::set_event_wakeup (const std::function<void()> &wakeup)
{
  ScopedLock<Spinlock> sl (m_async_spin);
  m_async_wakeup = wakeup;
}

bool
ScreenWindow::update_state (const State &state)
{
  ScopedLock<Spinlock> sl (m_async_spin);
  const bool accessed = m_async_state_accessed;
  m_async_state_accessed = false;
  m_async_state = state;
  return accessed;
}

ScreenWindow::State
ScreenWindow::get_state ()
{
  ScopedLock<Spinlock> sl (m_async_spin);
  return m_async_state;
}

bool
ScreenWindow::viewable ()
{
  ScopedLock<Spinlock> sl (m_async_spin);
  bool viewable = m_async_state.visible;
  viewable = viewable && !(m_async_state.window_flags & (ICONIFY | HIDDEN | SHADED));
  return viewable;
}

void
ScreenWindow::configure (const Config &config)
{
  queue_command (new ScreenCommand (ScreenCommand::CONFIGURE, this, config));
}

void
ScreenWindow::beep ()
{
  queue_command (new ScreenCommand (ScreenCommand::BEEP, this));
}

void
ScreenWindow::show ()
{
  queue_command (new ScreenCommand (ScreenCommand::SHOW, this));
}

void
ScreenWindow::present ()
{
  queue_command (new ScreenCommand (ScreenCommand::PRESENT, this));
}

void
ScreenWindow::blit_surface (cairo_surface_t *surface, const Rapicorn::Region &region)
{
  queue_command (new ScreenCommand (ScreenCommand::BLIT, this, surface, region));
}

void
ScreenWindow::start_user_move (uint button, double root_x, double root_y)
{
  queue_command (new ScreenCommand (ScreenCommand::MOVE, this, button, root_x, root_y));
}

void
ScreenWindow::start_user_resize (uint button, double root_x, double root_y, AnchorType edge)
{
  queue_command (new ScreenCommand (ScreenCommand::RESIZE, this, button, root_x, root_y));
}

void
ScreenWindow::destroy ()
{
  queue_command (new ScreenCommand (ScreenCommand::DESTROY, this));
}

void
ScreenWindow::queue_command (ScreenCommand *command)
{
  ScreenDriver::Friends::queue_command (screen_driver_async(), command);
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

// == ScreenCommand ==
ScreenCommand::ScreenCommand (Type ctype, ScreenWindow *window) :
  type (ctype), screen_window (window), config (NULL), setup (NULL)
{
  assert (type == OK || type == BEEP || type == SHOW || type == PRESENT || type == DESTROY || type == SHUTDOWN);
}

ScreenCommand::ScreenCommand (Type ctype, ScreenWindow *window, const ScreenWindow::Config &cfg) :
  type (ctype), screen_window (window), config (NULL), setup (NULL)
{
  assert (type == CONFIGURE);
  config = new ScreenWindow::Config (cfg);
}

ScreenCommand::ScreenCommand (Type ctype, ScreenWindow *window, const ScreenWindow::Setup &cs, const ScreenWindow::Config &cfg) :
  type (ctype), screen_window (window), config (NULL), setup (NULL)
{
  assert (type == CREATE);
  setup = new ScreenWindow::Setup (cs);
  config = new ScreenWindow::Config (cfg);
}

ScreenCommand::ScreenCommand (Type ctype, ScreenWindow *window, cairo_surface_t *csurface, const Rapicorn::Region &cregion) :
  type (ctype), screen_window (window), config (NULL), setup (NULL)
{
  assert (type == BLIT);
  surface = cairo_surface_reference (csurface);
  region = new Rapicorn::Region (cregion);
}

ScreenCommand::ScreenCommand (Type ctype, ScreenWindow *window, int cbutton, int croot_x, int croot_y) :
  type (ctype), screen_window (window), config (NULL), setup (NULL)
{
  assert (type == MOVE || type == RESIZE);
  cbutton = button;
  croot_x = root_x;
  croot_y = root_y;
}

ScreenCommand::ScreenCommand (Type type, ScreenWindow *window, const String &result) :
  type (type), screen_window (window), config (NULL), setup (NULL)
{
  assert (type == ERROR);
  result_msg  = new String (result);
}

ScreenCommand::~ScreenCommand()
{
  switch (type)
    {
    case CREATE:
      delete setup;
      delete config;
      break;
    case CONFIGURE:
      delete config;
      assert (!setup);
      break;
    case BLIT:
      cairo_surface_destroy (surface);
      delete region;
      break;
    case MOVE: case RESIZE:
      button = root_x = root_y = 0;
      break;
    case BEEP: case SHOW: case PRESENT: case DESTROY: case SHUTDOWN:
      assert (!config && !setup);
      break;
    case OK:
      assert (!config && !setup);
      break;
    case ERROR:
      delete result_msg;
      assert (!setup);
      break;
    }
}

bool
ScreenCommand::reply_type (Type type)
{
  switch (type)
    {
    case CREATE: case SHUTDOWN: return true; // has reply
    case CONFIGURE: case BLIT:  return false;
    case MOVE: case RESIZE:     return false;
    case BEEP: case SHOW:       return false;
    case PRESENT: case DESTROY: return false;
    case OK: case ERROR:        return true; // is reply
    }
  return false; // silence compiler
}

// == ScreenDriver ==
static Mutex                    screen_driver_mutex;
static ScreenDriver            *screen_driver_chain = NULL;

ScreenDriver::ScreenDriver (const String &name, int priority) :
  m_sibling (NULL), m_name (name), m_priority (priority)
{
  ScopedLock<Mutex> locker (screen_driver_mutex);
  m_sibling = screen_driver_chain;
  screen_driver_chain = this;
}

ScreenDriver::~ScreenDriver ()
{
  assert_return (m_command_queue.pending() == false);
  assert_return (m_reply_queue.pending() == false);
  assert_return (m_thread_handle.get_id() == std::thread::id());
}

bool
ScreenDriver::open_L ()
{
  assert_return (screen_driver_mutex.debug_locked(), false);
  assert_return (m_thread_handle.get_id() == std::thread::id(), false);
  assert_return (m_reply_queue.pending() == false, false);
  m_thread_handle = std::thread (&ScreenDriver::run, this, std::ref (m_command_queue), std::ref (m_reply_queue));
  ScreenCommand *reply = m_reply_queue.pop();
  if (reply->type == ScreenCommand::OK)
    {
      delete reply;
      return true;
    }
  else if (reply->type == ScreenCommand::ERROR)
    {
      delete reply;
      m_thread_handle.join();
      return false;
    }
  else
    assert_unreached();
}

void
ScreenDriver::close_L ()
{
  assert_return (screen_driver_mutex.debug_locked());
  assert_return (m_thread_handle.joinable());
  assert_return (m_reply_queue.pending() == false);
  m_command_queue.push (new ScreenCommand (ScreenCommand::SHUTDOWN, NULL));
  ScreenCommand *reply = m_reply_queue.pop();
  assert (reply->type == ScreenCommand::OK);
  delete reply;
  m_thread_handle.join();
}

ScreenDriver*
ScreenDriver::retrieve_screen_driver (const String &backend_name)
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
      else if (it->m_thread_handle.joinable() || it->open_L())
        r = NULL;
      else
        r = "failed to open";
      SDEBUG ("screen driver %s: %s", CQUOTE (it->m_name), r ? r : "success");
      if (r == NULL)
        return it;
    }
  return NULL;
}

void
ScreenDriver::forcefully_close_all ()
{
  ScopedLock<Mutex> locker (screen_driver_mutex);
  for (ScreenDriver *screen_driver = screen_driver_chain; screen_driver; screen_driver = screen_driver->m_sibling)
    if (screen_driver->m_thread_handle.joinable())
      screen_driver->close_L();
}

bool
ScreenDriver::driver_priority_lesser (const ScreenDriver *d1, const ScreenDriver *d2)
{
  return d1->m_priority < d2->m_priority;
}

void
ScreenDriver::queue_command (ScreenCommand *screen_command)
{
  assert_return (m_thread_handle.joinable());
  assert_return (screen_command->screen_window != NULL);
  assert_return (ScreenCommand::reply_type (screen_command->type) == false);
  m_command_queue.push (screen_command);
}

ScreenWindow*
ScreenDriver::create_screen_window (const ScreenWindow::Setup &setup, const ScreenWindow::Config &config)
{
  assert_return (m_thread_handle.joinable(), NULL);
  assert_return (m_reply_queue.pending() == false, NULL);
  m_command_queue.push (new ScreenCommand (ScreenCommand::CREATE, NULL, setup, config));
  ScreenCommand *reply = m_reply_queue.pop();
  assert (reply->type == ScreenCommand::OK && reply->screen_window);
  ScreenWindow *screen_window = reply->screen_window;
  delete reply;
  return screen_window;
}

} // Rapicorn
