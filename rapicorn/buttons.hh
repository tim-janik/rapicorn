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
#ifndef __RAPICORN_BUTTONS_HH__
#define __RAPICORN_BUTTONS_HH__

#include <rapicorn/container.hh>
#include <rapicorn/paintcontainers.hh>

namespace Rapicorn {

struct Activatable : virtual Convertible { // FIXME: remove this? /* ActivateModel */
  SignalVoid<Activatable>       sig_changed;
  virtual bool                  check_activate ();
  virtual void                  activate       ();
};

class ButtonArea : public virtual Container, public virtual FocusFrame::Client {
  typedef Signal<ButtonArea, bool (), CollectorUntil0<bool> > SignalCheckActivate;
  typedef Signal<ButtonArea, void ()>                         SignalActivate;
protected:
  explicit              ButtonArea();
public:
  SignalCheckActivate   sig_check_activate;
  SignalActivate        sig_activate;
  virtual String        on_click        () const = 0;
  virtual void          on_click        (const String &command) = 0;
  virtual String        on_click2       () const = 0;
  virtual void          on_click2       (const String &command) = 0;
  virtual String        on_click3       () const = 0;
  virtual void          on_click3       (const String &command) = 0;
  virtual ClickType     click_type      () const = 0;
  virtual void          click_type      (ClickType     click_type) = 0;
  virtual
  const PropertyList&   list_properties ();
};

} // Rapicorn

#endif  /* __RAPICORN_BUTTONS_HH__ */
