// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "screenwindow.hh"
#include <list>
#include <algorithm>

#define SDEBUG(...)     RAPICORN_KEY_DEBUG ("ScreenDriver", __VA_ARGS__)

namespace Rapicorn {

/* --- ScreenWindow::FactoryBase --- */
static std::list<ScreenWindow::FactoryBase*> *screen_window_backends = NULL;

ScreenWindow::FactoryBase::~FactoryBase ()
{}

void
ScreenWindow::FactoryBase::register_backend (FactoryBase &factory)
{
  do_once { screen_window_backends = new std::list<ScreenWindow::FactoryBase*>(); }
  screen_window_backends->push_back (&factory);
}

/* --- ScreenWindow --- */
template<class InputIterator, class Function> InputIterator
find_match (InputIterator first, InputIterator last, Function f)
{
  for (; first != last; ++first)
    if (f (first))
      break;
  return first;
}

ScreenWindow*
ScreenWindow::create_screen_window (const String                &backend_name,
                                    WindowType                   screen_window_type,
                                    ScreenWindow::EventReceiver &receiver)
{
  auto itb = screen_window_backends->begin(), ite = screen_window_backends->end(), it = ite;
  if (backend_name != "auto")
    it = find_match (itb, ite, [backend_name] (decltype (itb) it) { return (*it)->m_name == backend_name; });
  else
    {
      // rough implementation of automatic backend selection
      it = find_match (itb, ite, [backend_name] (decltype (itb) it) { return (*it)->m_name == "X11Window"; });
      if (it == ite)
        it = find_match (itb, ite, [backend_name] (decltype (itb) it) { return (*it)->m_name == "GtkWindow"; });
    }
  if (it != ite)
    return (*it)->create_screen_window (screen_window_type, receiver);
  return NULL;
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
