// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "screenwindow.hh"
#include <list>
#include <algorithm>

#define SDEBUG(...)     RAPICORN_KEY_DEBUG ("ScreenDriver", __VA_ARGS__)

namespace Rapicorn {

// == ScreenWindow ==
static const char*
flag_name (uint64 flag)
{
  switch (flag)
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
    case ScreenWindow::UNFOCUSED:	return "UNFOCUSED";
    case ScreenWindow::ACCEPT_FOCUS:	return "ACCEPT_FOCUS";
    case ScreenWindow::ICONIFY:	        return "ICONIFY";
    case ScreenWindow::DELETABLE:	return "DELETABLE";
    default:                            return "UNKNOWN";
    }
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
