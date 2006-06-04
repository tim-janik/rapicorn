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

namespace Rapicorn {

/* --- Root --- */
class Root : public virtual Container {
protected:
  explicit      Root            ();
  virtual void  cancel_item_events      (Item               &item) = 0;
public:
  Signal<Container, void (const Allocation&)> sig_expose;
  virtual void  render                  (Plane &plane) = 0;
  /* events */
  virtual void  dispatch_cancel_events  () = 0;
  virtual bool  dispatch_enter_event    (const EventContext &econtext) = 0;
  virtual bool  dispatch_move_event     (const EventContext &econtext) = 0;
  virtual bool  dispatch_leave_event    (const EventContext &econtext) = 0;
  virtual bool  dispatch_button_event   (const EventContext &econtext,
                                         bool                is_press,
                                         uint                button) = 0;
  virtual bool  dispatch_focus_event    (const EventContext &econtext,
                                         bool                is_in) = 0;
  virtual bool  dispatch_key_event      (const EventContext &econtext,
                                         bool                is_press,
                                         KeyValue            key,
                                         const char         *key_name) = 0;
  virtual bool  dispatch_scroll_event   (const EventContext &econtext,
                                         EventType           scroll_type) = 0;
  virtual void  add_grab                (Item   &child,
                                         bool    unconfined = false) = 0;
  void          add_grab                (Item   *child,
                                         bool    unconfined = false)
  {
    throw_if_null (child);
    return add_grab (*child, unconfined);
  }
  virtual void  remove_grab             (Item               &child) = 0;
  void          remove_grab             (Item               *child)     { throw_if_null (child); return remove_grab (*child); }
  virtual Item* get_grab                (bool   *unconfined = NULL) = 0;
};

} // Rapicorn

#endif  /* __RAPICORN_ROOT_HH__ */
