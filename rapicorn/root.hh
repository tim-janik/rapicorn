/* Rapicorn
 * Copyright (C) 2005 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __RAPICORN_ROOT_HH__
#define __RAPICORN_ROOT_HH__

#include <rapicorn/container.hh>
#include <rapicorn/handle.hh>
#include <rapicorn/loop.hh>

namespace Rapicorn {

/* --- Root --- */
class Root : public virtual Container, public virtual MainLoop::Source {
protected:
  explicit      Root                    ();
  virtual void  cancel_item_events      (Item        &item) = 0;
  virtual bool  dispatch_event          (const Event &event) = 0;
  /* loop source (FIXME) */
  virtual bool  prepare                 (uint64 current_time_usecs,
                                         int   *timeout_msecs_p) = 0;
public://FIXME: protected:
  virtual bool  check                   (uint64 current_time_usecs) = 0;
  virtual bool  dispatch                () = 0;
public:
  virtual void  enqueue_async           (Event              *event) = 0;
  Signal<Container, void (const Allocation&)> sig_expose;
  virtual void  render                  (Plane &plane) = 0;
  virtual void  add_grab                (Item  &child,
                                         bool   unconfined = false) = 0;
  void          add_grab                (Item  *child,
                                         bool   unconfined = false)    { throw_if_null (child); return add_grab (*child, unconfined); }
  virtual void  remove_grab             (Item  &child) = 0;
  void          remove_grab             (Item  *child)     { throw_if_null (child); return remove_grab (*child); }
  virtual Item* get_grab                (bool  *unconfined = NULL) = 0;
};

} // Rapicorn

#endif  /* __RAPICORN_ROOT_HH__ */
