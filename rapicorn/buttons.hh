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
#ifndef __RAPICORN_BUTTONS_HH__
#define __RAPICORN_BUTTONS_HH__

#include <rapicorn/item.hh>

namespace Rapicorn {

struct Disposable : virtual Convertible { // FIXME: move out of here
  SignalVoid<Disposable> sig_dispose;
};

struct Activatable : virtual Convertible { /* ActivateModel */
  SignalVoid<Activatable>       sig_changed;
  virtual bool                  check_activate ();
  virtual void                  activate       ();
};

class ButtonController : public virtual Controller {
protected:
  explicit      ButtonController();
  virtual void  unset_model() = 0;
public:
  virtual void  set_model (Activatable  &activatable) = 0;
  virtual void  update    () = 0;
public: /* convenience API to spare model */
  typedef Signal<ButtonController, bool (), CollectorUntil0<bool> > SignalCheckActivate;
  typedef Signal<ButtonController, void ()>                         SignalActivate;
  SignalCheckActivate   sig_check_activate;
  SignalActivate        sig_activate;
};

class ButtonView : public virtual Item {
public:
  virtual void  set_model       (Activatable            &activatable) = 0;
  virtual void  set_controller  (ButtonController       &bcontroller) = 0;
};

} // Rapicorn

#endif  /* __RAPICORN_BUTTONS_HH__ */
