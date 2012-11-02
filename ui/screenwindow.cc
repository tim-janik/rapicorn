// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "screenwindow.hh"
#include <list>

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
ScreenWindow*
ScreenWindow::create_screen_window (const String                &backend_name,
                                    WindowType                   screen_window_type,
                                    ScreenWindow::EventReceiver &receiver)
{
  std::list<ScreenWindow::FactoryBase*>::iterator it;
  for (it = screen_window_backends->begin(); it != screen_window_backends->end(); it++)
    if (backend_name == (*it)->m_name)
      return (*it)->create_screen_window (screen_window_type, receiver);
  if (backend_name == "auto")
    {
      /* cruel approximation of automatic selection logic */
      it = screen_window_backends->begin();
      if (it != screen_window_backends->end())
        return (*it)->create_screen_window (screen_window_type, receiver);
    }
  return NULL;
}

} // Rapicorn
