/* RapicornSignal
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
#include "rapicornsignal.hh"

namespace Rapicorn {
namespace Signals {

/* --- TrampolineLink --- */
void
TrampolineLink::owner_ref ()
{
  return_if_fail (m_linking_owner == false);
  ref_sink();
  m_linking_owner = true;
}

void
TrampolineLink::unlink()
{
  callable (false);
  if (m_linking_owner)
    {
      m_linking_owner = false;
      next->prev = prev;
      prev->next = next;
      // leave ->next, ->prev intact for iterators still holding a ref
      unref();
    }
}

TrampolineLink::~TrampolineLink()
{
  return_if_fail (m_linking_owner == false);
  if (next || prev)
    {
      // unlink() might have left leave next and prev
      prev = next = NULL;
    }
}

/* --- SignalBase --- */
bool
SignalBase::EmbeddedLink::operator== (const TrampolineLink &other) const
{
  return false;
}

ConId
SignalBase::connect_link (TrampolineLink *link,
                          bool            with_emitter)
{
  return_val_if_fail (link->m_linking_owner == false && link->prev == NULL && link->next == NULL, NULL);
  link->owner_ref();
  link->prev = start.prev;
  link->next = &start;
  start.prev->next = link;
  start.prev = link;
  link->with_emitter (with_emitter);
  return link->con_id();
}

uint
SignalBase::disconnect_equal_link (const TrampolineLink &link,
                                   bool                  with_emitter)
{
  for (TrampolineLink *walk = start.next; walk != &start; walk = walk->next)
    if (walk->with_emitter() == with_emitter and *walk == link)
      {
        return_val_if_fail (walk->m_linking_owner == true, 0);
        walk->unlink(); // unrefs
        return 1;
      }
  return 0;
}

uint
SignalBase::disconnect_link_id (ConId con_id)
{
  for (TrampolineLink *walk = start.next; walk != &start; walk = walk->next)
    if (con_id == walk->con_id())
      {
        return_val_if_fail (walk->m_linking_owner == true, 0);
        walk->unlink(); // unrefs
        return 1;
      }
  return 0;
}

SignalBase::~SignalBase()
{
  while (start.next != &start)
    {
      return_if_fail (start.next->m_linking_owner == true);
      start.next->unlink();
    }
  RAPICORN_ASSERT (start.next == &start);
  RAPICORN_ASSERT (start.prev == &start);
  start.prev = start.next = NULL;
  start.check_last_ref();
  start.unref();
}

void
SignalBase::EmbeddedLink::delete_this ()
{
  /* not deleting, because this structure is always embedded as SignalBase::start */
}

} // Signals
} // Rapicorn
