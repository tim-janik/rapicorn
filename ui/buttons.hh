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

#include <ui/container.hh>
#include <ui/paintcontainers.hh>

namespace Rapicorn {

struct Activatable : virtual BaseObject { // FIXME: remove this? /* ActivateModel */
  SignalVoid<Activatable>       sig_changed;
  virtual bool                  check_activate ();
  virtual void                  activate       ();
};

class ButtonAreaImpl : public virtual SingleContainerImpl, public virtual ButtonAreaIface,
                       public virtual EventHandler, public virtual FocusFrame::Client {
  typedef Signal<ButtonAreaImpl, bool (), CollectorUntil0<bool> > SignalCheckActivate;
  typedef Signal<ButtonAreaImpl, void ()>                         SignalActivate;
  uint          m_button, m_repeater;
  ClickType     m_click_type;
  FocusFrame   *m_focus_frame;
  String        m_on_click[3];
  virtual void          dump_private_data       (TestStream &tstream);
  bool                  activate_button_command (int button);
  bool                  activate_command        ();
  void                  activate_click          (int button, EventType etype);
  bool                  can_focus               () const;
  bool                  register_focus_frame    (FocusFrame &frame);
  void                  unregister_focus_frame  (FocusFrame &frame);
  void                  reset                   (ResetMode mode = RESET_ALL);
  bool                  handle_event            (const Event &event);
public:
  explicit              ButtonAreaImpl  ();
  SignalCheckActivate   sig_check_activate;
  SignalActivate        sig_activate;
  virtual String        on_click        () const                { return m_on_click[0]; }
  virtual void          on_click        (const String &command) { m_on_click[0] = string_strip (command); }
  virtual String        on_click2       () const                { return m_on_click[1]; }
  virtual void          on_click2       (const String &command) { m_on_click[1] = string_strip (command); }
  virtual String        on_click3       () const                { return m_on_click[2]; }
  virtual void          on_click3       (const String &command) { m_on_click[2] = string_strip (command); }
  virtual ClickType     click_type      () const                { return m_click_type; }
  virtual void          click_type      (ClickType  click_type) { reset(); m_click_type = click_type; }
  virtual const PropertyList& list_properties ();
};

} // Rapicorn

#endif  /* __RAPICORN_BUTTONS_HH__ */
