// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "screenwindow.hh"
#include <list>
#include <algorithm>

#define SDEBUG(...)     RAPICORN_KEY_DEBUG ("ScreenDriver", __VA_ARGS__)

namespace Rapicorn {

// == ScreenWindow ==
ScreenWindow::ScreenWindow () :
  async_state_accessed_ (0)
{}

ScreenWindow::~ScreenWindow ()
{
  std::list<Event*> events;
  {
    ScopedLock<Spinlock> sl (async_spin_);
    events.swap (async_event_queue_);
  }
  while (!events.empty())
    {
      Event *e = events.front ();
      events.pop_front();
      delete e;
    }
  critical_unless (async_event_queue_.empty()); // this queue must not be accessed at this point
}

void
ScreenWindow::enqueue_event (Event *event)
{
  critical_unless (event);
  ScopedLock<Spinlock> sl (async_spin_);
  const bool notify = async_event_queue_.empty();
  async_event_queue_.push_back (event);
  if (notify && async_wakeup_)
    async_wakeup_();
}

void
ScreenWindow::push_event (Event *event)
{
  critical_unless (event);
  ScopedLock<Spinlock> sl (async_spin_);
  const bool notify = async_event_queue_.empty();
  async_event_queue_.push_front (event);
  if (notify && async_wakeup_)
    async_wakeup_();
}

Event*
ScreenWindow::pop_event ()
{
  ScopedLock<Spinlock> sl (async_spin_);
  if (async_event_queue_.empty())
    return NULL;
  Event *event = async_event_queue_.front();
  async_event_queue_.pop_front();
  return event;
}

bool
ScreenWindow::has_event ()
{
  ScopedLock<Spinlock> sl (async_spin_);
  return !async_event_queue_.empty();
}

bool
ScreenWindow::peek_events (const std::function<bool (Event*)> &pred)
{
  ScopedLock<Spinlock> sl (async_spin_);
  for (auto ep : async_event_queue_)
    if (pred (ep))
      return true;
  return false;
}

void
ScreenWindow::set_event_wakeup (const std::function<void()> &wakeup)
{
  ScopedLock<Spinlock> sl (async_spin_);
  async_wakeup_ = wakeup;
}

bool
ScreenWindow::update_state (const State &state)
{
  ScopedLock<Spinlock> sl (async_spin_);
  const bool accessed = async_state_accessed_;
  async_state_accessed_ = false;
  async_state_ = state;
  return accessed;
}

ScreenWindow::State
ScreenWindow::get_state ()
{
  ScopedLock<Spinlock> sl (async_spin_);
  return async_state_;
}

bool
ScreenWindow::viewable ()
{
  ScopedLock<Spinlock> sl (async_spin_);
  bool viewable = async_state_.visible;
  viewable = viewable && !(async_state_.window_flags & (ICONIFY | HIDDEN | SHADED));
  return viewable;
}

void
ScreenWindow::configure (const Config &config, bool sizeevent)
{
  ScreenCommand *cmd = new ScreenCommand (ScreenCommand::CONFIGURE, this);
  cmd->config = new ScreenWindow::Config (config);
  cmd->need_resize = sizeevent;
  queue_command (cmd);
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
  ScreenCommand *cmd = new ScreenCommand (ScreenCommand::BLIT, this);
  cmd->surface = cairo_surface_reference (surface);
  cmd->region = new Rapicorn::Region (region);
  queue_command (cmd);
}

void
ScreenWindow::start_user_move (uint button, double root_x, double root_y)
{
  ScreenCommand *cmd = new ScreenCommand (ScreenCommand::UMOVE, this);
  cmd->button = button;
  cmd->root_x = root_x;
  cmd->root_y = root_y;
  queue_command (cmd);
}

void
ScreenWindow::start_user_resize (uint button, double root_x, double root_y, AnchorType edge)
{
  ScreenCommand *cmd = new ScreenCommand (ScreenCommand::URESIZE, this);
  cmd->button = button;
  cmd->root_x = root_x;
  cmd->root_y = root_y;
  queue_command (cmd);
}

void
ScreenWindow::set_content_owner (ContentSourceType source, const StringVector &data_types)
{
  ScreenCommand *cmd = new ScreenCommand (ScreenCommand::OWNER, this);
  cmd->source = source;
  cmd->string_list = data_types;
  queue_command (cmd);
}

void
ScreenWindow::provide_content (const String &data_type, const String &data, uint64 request_id)
{
  ScreenCommand *cmd = new ScreenCommand (ScreenCommand::PROVIDE, this);
  cmd->nonce = request_id;
  cmd->string_list.push_back (data_type);
  cmd->string_list.push_back (data);
  queue_command (cmd);
}

void
ScreenWindow::request_content (ContentSourceType source, uint64 nonce, const String &data_type)
{
  ScreenCommand *cmd = new ScreenCommand (ScreenCommand::CONTENT, this);
  cmd->source = source;
  cmd->nonce = nonce;
  cmd->string_list.push_back (data_type);
  queue_command (cmd);
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
  type (ctype), screen_window (window), config (NULL), setup (NULL), surface (NULL), region (NULL),
  nonce (0), root_x (-1), root_y (-1), button (-1), source (ContentSourceType (0)), need_resize (false)
{}

ScreenCommand::~ScreenCommand()
{
  if (surface)
    cairo_surface_destroy (surface);
  if (config)
    delete config;
  if (setup)
    delete setup;
  if (region)
    delete region;
}

bool
ScreenCommand::reply_type (Type type)
{
  switch (type)
    {
    case CREATE: case SHUTDOWN: return true; // has reply
    case OK: case ERROR:        return true; // is reply
    case CONFIGURE: case BLIT:
    case UMOVE: case URESIZE:
    case OWNER:
    case PROVIDE: case CONTENT:
    case BEEP: case SHOW:
    case PRESENT: case DESTROY:
      return false;
    }
  return false; // silence compiler
}

// == ScreenDriver ==
static Mutex                    screen_driver_mutex;
static ScreenDriver            *screen_driver_chain = NULL;

ScreenDriver::ScreenDriver (const String &name, int priority) :
  sibling_ (NULL), name_ (name), priority_ (priority)
{
  ScopedLock<Mutex> locker (screen_driver_mutex);
  sibling_ = screen_driver_chain;
  screen_driver_chain = this;
}

ScreenDriver::~ScreenDriver ()
{
  assert_return (command_queue_.pending() == false);
  assert_return (reply_queue_.pending() == false);
  assert_return (thread_handle_.get_id() == std::thread::id());
}

bool
ScreenDriver::open_L ()
{
  assert_return (screen_driver_mutex.debug_locked(), false);
  assert_return (thread_handle_.get_id() == std::thread::id(), false);
  assert_return (reply_queue_.pending() == false, false);
  thread_handle_ = std::thread (&ScreenDriver::run, this, std::ref (command_queue_), std::ref (reply_queue_));
  ScreenCommand *reply = reply_queue_.pop();
  if (reply->type == ScreenCommand::OK)
    {
      delete reply;
      return true;
    }
  else if (reply->type == ScreenCommand::ERROR)
    {
      delete reply;
      thread_handle_.join();
      return false;
    }
  else
    assert_unreached();
}

void
ScreenDriver::close_L ()
{
  assert_return (screen_driver_mutex.debug_locked());
  assert_return (thread_handle_.joinable());
  assert_return (reply_queue_.pending() == false);
  command_queue_.push (new ScreenCommand (ScreenCommand::SHUTDOWN, NULL));
  ScreenCommand *reply = reply_queue_.pop();
  assert (reply->type == ScreenCommand::OK);
  delete reply;
  thread_handle_.join();
}

ScreenDriver*
ScreenDriver::retrieve_screen_driver (const String &backend_name)
{
  ScopedLock<Mutex> locker (screen_driver_mutex);
  vector<ScreenDriver*> screen_driver_array;
  for (ScreenDriver *it = screen_driver_chain; it; it = it->sibling_)
    screen_driver_array.push_back (it);
  SDEBUG ("trying to open 1/%d screen drivers...", screen_driver_array.size());
  sort (screen_driver_array.begin(), screen_driver_array.end(), driver_priority_lesser);
  for (auto it : screen_driver_array)
    {
      const char *r;
      if (it->name_ != backend_name && backend_name != "auto")
        r = "not selected";
      else if (it->thread_handle_.joinable() || it->open_L())
        r = NULL;
      else
        r = "failed to open";
      SDEBUG ("screen driver %s: %s", CQUOTE (it->name_), r ? r : "success");
      if (r == NULL)
        return it;
    }
  return NULL;
}

void
ScreenDriver::forcefully_close_all ()
{
  ScopedLock<Mutex> locker (screen_driver_mutex);
  for (ScreenDriver *screen_driver = screen_driver_chain; screen_driver; screen_driver = screen_driver->sibling_)
    if (screen_driver->thread_handle_.joinable())
      screen_driver->close_L();
}

bool
ScreenDriver::driver_priority_lesser (const ScreenDriver *d1, const ScreenDriver *d2)
{
  return d1->priority_ < d2->priority_;
}

void
ScreenDriver::queue_command (ScreenCommand *screen_command)
{
  assert_return (thread_handle_.joinable());
  assert_return (screen_command->screen_window != NULL);
  assert_return (ScreenCommand::reply_type (screen_command->type) == false);
  command_queue_.push (screen_command);
}

ScreenWindow*
ScreenDriver::create_screen_window (const ScreenWindow::Setup &setup, const ScreenWindow::Config &config)
{
  assert_return (thread_handle_.joinable(), NULL);
  assert_return (reply_queue_.pending() == false, NULL);

  ScreenCommand *cmd = new ScreenCommand (ScreenCommand::CREATE, NULL);
  cmd->setup = new ScreenWindow::Setup (setup);
  cmd->config = new ScreenWindow::Config (config);
  command_queue_.push (cmd);
  ScreenCommand *reply = reply_queue_.pop();
  assert (reply->type == ScreenCommand::OK && reply->screen_window);
  ScreenWindow *screen_window = reply->screen_window;
  delete reply;
  return screen_window;
}

} // Rapicorn
