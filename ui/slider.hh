// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_SLIDER_HH__
#define __RAPICORN_SLIDER_HH__

#include <ui/adjustment.hh>
#include <ui/table.hh>

namespace Rapicorn {

class SliderAreaImpl : public virtual TableLayoutImpl, public virtual SliderAreaIface {
  Adjustment          *adjustment_;
  size_t               avc_id_, arc_id_;
  AdjustmentSourceType adjustment_source_;
  bool                 flip_;
  void                         unset_adjustment  ();
  bool                         move              (MoveType);
protected:
  virtual                     ~SliderAreaImpl    () override;
  virtual void                 hierarchy_changed (WidgetImpl *old_toplevel) override;
  virtual const CommandList&   list_commands     () override;
  virtual void                 slider_changed    ();
  typedef Aida::Signal<void ()> SignalSliderChanged;
public:
  explicit                     SliderAreaImpl    ();
  Adjustment*                  adjustment        () const;
  void                         adjustment        (Adjustment &adjustment);
  virtual AdjustmentSourceType adjustment_source () const override;
  virtual void                 adjustment_source (AdjustmentSourceType adj_source) override;
  virtual bool                 flipped           () const override;
  virtual void                 flipped           (bool flip) override;
  SignalSliderChanged          sig_slider_changed;
};

} // Rapicorn

#endif  /* __RAPICORN_SLIDER_HH__ */
