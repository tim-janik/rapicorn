/* Rapicorn
 * Copyright (C) 2005-2006 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "viewp0rt.hh"
#include <list>

namespace Rapicorn {

/* --- ScreenWindow::FactoryBase --- */
static std::list<ScreenWindow::FactoryBase*> *screen_window_backends = NULL;

ScreenWindow::FactoryBase::~FactoryBase ()
{}

void
ScreenWindow::FactoryBase::register_backend (FactoryBase &factory)
{
  if (once_enter (&screen_window_backends))
    once_leave (&screen_window_backends, new std::list<ScreenWindow::FactoryBase*>());
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
