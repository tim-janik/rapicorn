// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_BUTTONS_HH__
#define __RAPICORN_BUTTONS_HH__

#include <ui/container.hh>
#include <ui/paintcontainers.hh>

namespace Rapicorn {

class ButtonAreaImpl : public virtual SingleContainerImpl, public virtual ButtonAreaIface,
                       public virtual EventHandler, public virtual FocusFrame::Client {
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
  virtual String        on_click        () const                { return m_on_click[0]; }
  virtual void          on_click        (const String &command) { m_on_click[0] = string_strip (command); }
  virtual String        on_click2       () const                { return m_on_click[1]; }
  virtual void          on_click2       (const String &command) { m_on_click[1] = string_strip (command); }
  virtual String        on_click3       () const                { return m_on_click[2]; }
  virtual void          on_click3       (const String &command) { m_on_click[2] = string_strip (command); }
  virtual ClickType     click_type      () const                { return m_click_type; }
  virtual void          click_type      (ClickType  click_type) { reset(); m_click_type = click_type; }
  virtual const PropertyList& _property_list ();
};

} // Rapicorn

#endif  /* __RAPICORN_BUTTONS_HH__ */
