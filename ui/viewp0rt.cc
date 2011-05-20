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

/* --- Viewp0rt::FactoryBase --- */
static std::list<Viewp0rt::FactoryBase*> *viewp0rt_backends = NULL;

Viewp0rt::FactoryBase::~FactoryBase ()
{}

void
Viewp0rt::FactoryBase::register_backend (FactoryBase &factory)
{
  if (once_enter (&viewp0rt_backends))
    once_leave (&viewp0rt_backends, new std::list<Viewp0rt::FactoryBase*>());
  viewp0rt_backends->push_back (&factory);
}

/* --- Viewp0rt --- */
Viewp0rt*
Viewp0rt::create_viewp0rt (const String            &backend_name,
                           WindowType               viewp0rt_type,
                           Viewp0rt::EventReceiver &receiver)
{
  std::list<Viewp0rt::FactoryBase*>::iterator it;
  for (it = viewp0rt_backends->begin(); it != viewp0rt_backends->end(); it++)
    if (backend_name == (*it)->m_name)
      return (*it)->create_viewp0rt (viewp0rt_type, receiver);
  if (backend_name == "auto")
    {
      /* cruel approximation of automatic selection logic */
      it = viewp0rt_backends->begin();
      if (it != viewp0rt_backends->end())
        return (*it)->create_viewp0rt (viewp0rt_type, receiver);
    }
  return NULL;
}

} // Rapicorn
