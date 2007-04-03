/* Rapicorn
 * Copyright (C) 2005 Tim Janik
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
#ifndef __RAPICORN_ROOT_HH__
#define __RAPICORN_ROOT_HH__

#include <rapicorn/container.hh>
#include <rapicorn/window.hh>
#include <rapicorn/loop.hh>

namespace Rapicorn {

/* --- Root --- */
class Root : public virtual Container, public virtual EventLoop::Source {
  friend class  Item;
  void          uncross_focus           (Item        &fitem);
  Window        m_window; // for WindowCommandSignal
protected:
  virtual void  beep                    (void) = 0;
  void          set_focus               (Item         *item);
  virtual void  expose_root_region      (const Region &region) = 0;     // root item coords
  virtual void  copy_area               (const Rect   &src,
                                         const Point  &dest) = 0;
  virtual void  cancel_item_events      (Item         *item) = 0;
  void          cancel_item_events      (Item         &item)            { cancel_item_events (&item); }
  virtual bool  dispatch_event          (const Event  &event) = 0;
  virtual void  set_parent              (Item         *parent);
  virtual bool  custom_command          (const String &command_name,
                                         const String &command_args);
  /* loop source (FIXME) */
  virtual bool  prepare                 (uint64 current_time_usecs,
                                         int64 *timeout_usecs_p) = 0;
  explicit      Root                    ();
public://FIXME: protected:
  virtual bool  check                   (uint64 current_time_usecs) = 0;
  virtual bool  dispatch                () = 0;
  virtual void  draw_now                () = 0;
public:
  Item*         get_focus               () const;
  virtual bool  tunable_requisitions    () = 0;
  virtual void  render                  (Plane &plane) = 0;
  virtual void  add_grab                (Item  &child,
                                         bool   unconfined = false) = 0;
  void          add_grab                (Item  *child,
                                         bool   unconfined = false)    { throw_if_null (child); return add_grab (*child, unconfined); }
  virtual void  remove_grab             (Item  &child) = 0;
  void          remove_grab             (Item  *child)     { throw_if_null (child); return remove_grab (*child); }
  virtual Item* get_grab                (bool  *unconfined = NULL) = 0;
  /* commands */
  typedef Signal<Root, bool (const String&, const String&), CollectorWhile0<bool> >   CommandSignal;
  CommandSignal sig_command;
  typedef Signal<Window, bool (const String&, const String&), CollectorWhile0<bool> > WindowCommandSignal;
  WindowCommandSignal sig_window_command;
  /* window */
  virtual void  show                    () = 0;
  virtual void  hide                    () = 0;
  virtual bool  closed                  () = 0;
  virtual void  close                   () = 0;
  /* main loop functions */
  virtual EventLoop* get_loop           () = 0;
  /* MT-safe */
  virtual Window window                 () = 0;
};

} // Rapicorn

#endif  /* __RAPICORN_ROOT_HH__ */
