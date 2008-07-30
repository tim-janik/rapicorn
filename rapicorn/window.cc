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
  m_root (ref_sink (root)),
  commands (*this)
{}

Window::Window (const Window &window) :
  m_root (ref_sink (window.m_root)),
  commands (*this)
{}

Window::~Window ()
{
  m_root.unref();
}

Window*
Window::from_object_url (const String &rooturl)
{
  struct DeletableAccessor : public Deletable {
    using Deletable::from_object_url; // gain access to protected Deletable method
  };
  Deletable *deletable = DeletableAccessor::from_object_url (rooturl);
  if (deletable)
    {
      Item *item = dynamic_cast<Item*> (deletable);
      if (item)
        {
          Root *root = item->get_root();
          if (root)
            return new Window (*root);
        }
    }
  return NULL;
}

void
Window::ref () const
{
  return m_root.ref();
}

Root&
Window::root () const
{
  assert (rapicorn_thread_entered());
  return m_root;
}

void
Window::unref () const
{
  return m_root.unref();
}

bool
Window::viewable ()
{
  assert (rapicorn_thread_entered());
  return m_root.viewable();
}

void
Window::show ()
{
  assert (rapicorn_thread_entered());
  m_root.create_viewport();
}

bool
Window::closed ()
{
  assert (rapicorn_thread_entered());
  return m_root.has_viewport();
}

void
Window::close ()
{
  assert (rapicorn_thread_entered());
  m_root.destroy_viewport();
}

Window::Commands::Commands (Window &w) :
  window (w)
{}

void
Window::Commands::operator+= (const CmdSlot &s)
{
  window.m_root.sig_command += s;
}

void
Window::Commands::operator-= (const CmdSlot &s)
{
  window.m_root.sig_command -= s;
}

void
Window::Commands::operator+= (const CmdSlotW &s)
{
  window.m_root.sig_window_command += s;
}

void
Window::Commands::operator-= (const CmdSlotW &s)
{
  window.m_root.sig_window_command -= s;
}

void
Window::Commands::operator+= (bool (*callback) (const String&, const StringVector&))
{
  return operator+= (slot (callback));
}

void
Window::Commands::operator-= (bool (*callback) (const String&, const StringVector&))
{
  return operator-= (slot (callback));
}

void
Window::Commands::operator+= (bool (*callback) (Window&, const String&, const StringVector&))
{
  return operator+= (slot (callback));
}

void
Window::Commands::operator-= (bool (*callback) (Window&, const String&, const StringVector&))
{
  return operator-= (slot (callback));
}

} // Rapicorn
