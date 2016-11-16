// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_WINDOW_HH__
#define __RAPICORN_WINDOW_HH__

#include <ui/viewport.hh>

namespace Rapicorn {

// == WindowImpl ==
class WindowImpl;
typedef std::shared_ptr<WindowImpl> WindowImplP;
typedef std::weak_ptr<WindowImpl>   WindowImplW;

class WindowImpl : public virtual ViewportImpl, public virtual WindowIface {
  const EventLoopP             loop_;
  String                       last_command_;
  Signal_commands::Emission   *commands_emission_;
  virtual void                 create_display_window  () override;
  virtual void                 destroy_display_window () override;
protected:
  virtual const AncestryCache* fetch_ancestry_cache   () override;
  virtual void                 construct              () override;
  virtual void                 dispose                () override;
  virtual void                 set_parent             (ContainerImpl *parent) override;
public:
  explicit                     WindowImpl             ();
  virtual                      ~WindowImpl            ();
  virtual WindowImpl*          as_window_impl         () override       { return this; }
  static  void                 forcefully_close_all   ();
  // event loop
  EventLoopP                   get_event_loop         () const          { return loop_; } ///< See get_loop().
  bool                         custom_command         (const String &command_name, const StringSeq &command_args);
  bool                         command_dispatcher     (const LoopState &state);
  bool                         synthesize_enter       (double xalign = 0.5, double yalign = 0.5) override;
  bool                         synthesize_leave       () override;
  bool                         synthesize_click       (WidgetIface &widget, int button, double xalign = 0.5, double yalign = 0.5) override;
  bool                         synthesize_delete      () override;
  // internal API
  static bool                  widget_is_anchored     (WidgetImpl &widget, Internal = Internal()); ///< See anchored().
};

} // Rapicorn

#endif  /* __RAPICORN_WINDOW_HH__ */
