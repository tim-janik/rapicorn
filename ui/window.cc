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

/* --- implementation --- */
WinPtr::WinPtr (Root &root) :
  m_root (ref_sink (root)),
  commands (*this)
{}

WinPtr::WinPtr (const WinPtr &window) :
  m_root (ref_sink (window.m_root)),
  commands (*this)
{}

WinPtr::~WinPtr ()
{
  m_root.unref();
}

WinPtr*
WinPtr::from_locatable_id (uint64 objid)
{
  Locatable *locatable = Locatable::from_locatable_id (objid);
  if (locatable)
    {
      Item *item = dynamic_cast<Item*> (locatable);
      if (item)
        {
          Root *root = item->get_root();
          if (root)
            return new WinPtr (*root);
        }
    }
  return NULL;
}

void
WinPtr::ref () const
{
  return m_root.ref();
}

Root&
WinPtr::root () const
{
  assert (rapicorn_thread_entered());
  return m_root;
}

void
WinPtr::unref () const
{
  return m_root.unref();
}

bool
WinPtr::viewable ()
{
  assert (rapicorn_thread_entered());
  return m_root.viewable();
}

void
WinPtr::show ()
{
  assert (rapicorn_thread_entered());
  m_root.create_viewport();
}

bool
WinPtr::closed ()
{
  assert (rapicorn_thread_entered());
  return m_root.has_viewport();
}

void
WinPtr::close ()
{
  assert (rapicorn_thread_entered());
  m_root.destroy_viewport();
}

WinPtr::Commands::Commands (WinPtr &w) :
  window (w)
{}

void
WinPtr::Commands::operator+= (const CmdSlot &s)
{
  window.m_root.sig_command += s;
}

void
WinPtr::Commands::operator-= (const CmdSlot &s)
{
  window.m_root.sig_command -= s;
}

void
WinPtr::Commands::operator+= (const CmdSlotW &s)
{
  window.m_root.sig_window_command += s;
}

void
WinPtr::Commands::operator-= (const CmdSlotW &s)
{
  window.m_root.sig_window_command -= s;
}

void
WinPtr::Commands::operator+= (bool (*callback) (const String&, const StringVector&))
{
  return operator+= (slot (callback));
}

void
WinPtr::Commands::operator-= (bool (*callback) (const String&, const StringVector&))
{
  return operator-= (slot (callback));
}

void
WinPtr::Commands::operator+= (bool (*callback) (WinPtr&, const String&, const StringVector&))
{
  return operator+= (slot (callback));
}

void
WinPtr::Commands::operator-= (bool (*callback) (WinPtr&, const String&, const StringVector&))
{
  return operator-= (slot (callback));
}

} // Rapicorn
