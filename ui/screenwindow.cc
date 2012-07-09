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

} // Rapicorn
