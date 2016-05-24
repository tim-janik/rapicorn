// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_SCROLL_WIDGETS_HH__
#define __RAPICORN_SCROLL_WIDGETS_HH__

#include <ui/adjustment.hh>
#include <ui/viewport.hh>

namespace Rapicorn {

class ScrollAreaImpl : public virtual SingleContainerImpl, public virtual ScrollAreaIface, public virtual AdjustmentSource {
  AdjustmentP           hadjustment_, vadjustment_;
  Adjustment&           hadjustment();
  Adjustment&           vadjustment();
protected:
  virtual Adjustment*   get_adjustment  (AdjustmentSourceType adj_source, const String &name = "") override;
public:
  explicit              ScrollAreaImpl  ();
  virtual double        x_offset        () override;
  virtual double        y_offset        () override;
  virtual void          scroll_to       (double x, double y) override;
};

class ScrollPortImpl : public virtual ResizeContainerImpl, public virtual EventHandler {
  Adjustment *hadjustment_, *vadjustment_;
  size_t conid_hadjustment_, conid_vadjustment_;
  uint   fix_id_;
  void                          adjustment_changed      ();
  bool                          scroll                  (EventType scroll_dir);
  void                          fix_adjustments         ();
  void                          do_scrolled             ();
protected:
  virtual void                  hierarchy_changed       (WidgetImpl *old_toplevel) override;
  virtual void                  size_allocate           (Allocation area, bool changed) override;
  virtual void                  set_focus_child         (WidgetImpl *widget) override;
  virtual const CommandList&    list_commands           () override;
  virtual bool                  handle_event            (const Event &event);
  virtual void                  reset                   (ResetMode mode);
public:
  Aida::Signal<void ()>         sig_scrolled;
  explicit                      ScrollPortImpl          ();
  virtual                      ~ScrollPortImpl          ();
  void                          scroll_to_descendant    (WidgetImpl &widget);
};

} // Rapicorn

#endif  /* __RAPICORN_SCROLL_WIDGETS_HH__ */
