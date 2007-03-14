/* Rapicorn
 * Copyright (C) 2007 Tim Janik
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
#include "window.hh"
#include "root.hh"

namespace Rapicorn {
using namespace std;

/* --- implementation --- */
Window::Window (Root &root) :
  m_root (ref_sink (root))
{}

Window::Window (const Window &window) :
  m_root (ref_sink (window.m_root))
{}

Window::~Window ()
{
  unref (m_root);
}

Root&
Window::root ()
{
  assert (rapicorn_thread_entered());
  return m_root;
}

bool
Window::visible ()
{
  assert (rapicorn_thread_entered());
  return m_root.visible();
}

void
Window::show ()
{
  assert (rapicorn_thread_entered());
  m_root.show();
}

void
Window::hide ()
{
  assert (rapicorn_thread_entered());
  m_root.hide();
}

bool
Window::closed ()
{
  assert (rapicorn_thread_entered());
  return m_root.closed();
}

void
Window::close ()
{
  assert (rapicorn_thread_entered());
  m_root.close();
}

} // Rapicorn
