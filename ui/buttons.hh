// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_BUTTONS_HH__
#define __RAPICORN_BUTTONS_HH__

#include <ui/container.hh>
#include <ui/paintcontainers.hh>

namespace Rapicorn {

class ButtonAreaImpl : public virtual SingleContainerImpl, public virtual ButtonAreaIface,
                       public virtual EventHandler, public virtual FocusFrame::Client {
  uint          button_, repeater_, unpress_;
  ClickType     click_type_;
  FocusFrame   *focus_frame_;
  String        on_click_[3];
  virtual void          dump_private_data       (TestStream &tstream);
  bool                  activate_button_command (int button);
  bool                  activate_command        ();
  void                  activate_click          (int button, EventType etype);
  virtual bool          can_focus               () const;
  virtual bool          register_focus_frame    (FocusFrame &frame);
  virtual void          unregister_focus_frame  (FocusFrame &frame);
  virtual void          reset                   (ResetMode mode = RESET_ALL);
  virtual bool          handle_event            (const Event &event);
public:
  explicit              ButtonAreaImpl  ();
  virtual bool          activate_widget   ();
  virtual String        on_click        () const                { return on_click_[0]; }
  virtual void          on_click        (const String &command) { on_click_[0] = string_strip (command); }
  virtual String        on_click2       () const                { return on_click_[1]; }
  virtual void          on_click2       (const String &command) { on_click_[1] = string_strip (command); }
  virtual String        on_click3       () const                { return on_click_[2]; }
  virtual void          on_click3       (const String &command) { on_click_[2] = string_strip (command); }
  virtual ClickType     click_type      () const                { return click_type_; }
  virtual void          click_type      (ClickType  click_type) { reset(); click_type_ = click_type; }
  virtual const PropertyList& __aida_properties__ ();
};

} // Rapicorn

#endif  /* __RAPICORN_BUTTONS_HH__ */
