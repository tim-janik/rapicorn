// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_BUTTONS_HH__
#define __RAPICORN_BUTTONS_HH__

#include <ui/container.hh>
#include <ui/paintcontainers.hh>

namespace Rapicorn {

class ButtonAreaImpl : public virtual SingleContainerImpl, public virtual ButtonAreaIface,
                       public virtual EventHandler {
  uint          button_, repeater_, unpress_;
  Click         click_type_;
  String        on_click_[3];
  virtual void          dump_private_data       (TestStream &tstream);
  bool                  activate_button_command (int button);
  bool                  activate_command        ();
  void                  activate_click          (int button, EventType etype);
protected:
  virtual void          construct               () override;
  virtual void          reset                   (ResetMode mode = RESET_ALL);
  virtual bool          handle_event            (const Event &event);
public:
  explicit              ButtonAreaImpl  ();
  virtual bool          activate_widget ();
  virtual Click         click_type      () const override;
  virtual void          click_type      (Click click_type) override;
  virtual String        on_click        () const override;
  virtual void          on_click        (const String &command) override;
  virtual String        on_click2       () const override;
  virtual void          on_click2       (const String &command) override;
  virtual String        on_click3       () const override;
  virtual void          on_click3       (const String &command) override;
};

} // Rapicorn

#endif  /* __RAPICORN_BUTTONS_HH__ */
