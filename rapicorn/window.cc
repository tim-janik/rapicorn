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
Window::Window (Root           &root,
                OwnedMutex     &omutex) :
  m_root (ref_sink (root)),
  m_omutex (omutex)
{}

Window::Window (const Window &window) :
  m_root (ref_sink (window.m_root)),
  m_omutex (window.m_omutex)
{}

Window::~Window ()
{
  unref (m_root);
}

Root&
Window::root ()
{
  if (m_omutex.mine())
    return m_root;
  error ("Unsyncronized window handle access in thread: %s", Thread::Self::name().c_str());
}

Root*
Window::peek_root ()
{
  if (m_omutex.mine())
    return &m_root;
  else
    return NULL;
}

bool
Window::visible ()
{
  AutoLocker locker (m_omutex);
  return m_root.visible();
}

void
Window::show ()
{
  AutoLocker locker (m_omutex);
  m_root.show();
}

void
Window::hide ()
{
  AutoLocker locker (m_omutex);
  m_root.hide();
}

bool
Window::closed ()
{
  AutoLocker locker (m_omutex);
  return m_root.closed();
}

void
Window::close ()
{
  AutoLocker locker (m_omutex);
  m_root.close();
}

} // Rapicorn
