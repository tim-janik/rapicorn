// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_SLIDER_HH__
#define __RAPICORN_SLIDER_HH__

#include <ui/adjustment.hh>
#include <ui/table.hh>

namespace Rapicorn {

class SliderAreaImpl : public virtual TableLayoutImpl, public virtual SliderAreaIface {
  AdjustmentP          adjustment_;
  size_t               avc_id_, arc_id_;
  AdjustmentSourceType adjustment_source_;
  bool                 flip_;
  void                         unset_adjustment  ();
  bool                         move              (MoveType);
protected:
  virtual void                 hierarchy_changed (WindowImpl *old_toplevel) override;
  virtual const CommandList&   list_commands     () override;
  virtual void                 slider_changed    ();
  typedef Aida::Signal<void ()> SignalSliderChanged;
public:
  explicit                     SliderAreaImpl    ();
  virtual                     ~SliderAreaImpl    () override;
  Adjustment*                  adjustment        () const;
  void                         adjustment        (Adjustment &adjustment);
  virtual AdjustmentSourceType adjustment_source () const override;
  virtual void                 adjustment_source (AdjustmentSourceType adj_source) override;
  virtual bool                 flipped           () const override;
  virtual void                 flipped           (bool flip) override;
  SignalSliderChanged          sig_slider_changed;
};

class SliderTroughImpl : public virtual SingleContainerImpl, public virtual SliderTroughIface, public virtual EventHandler {
  SliderAreaImpl *slider_area_;
  size_t conid_slider_changed_;
  double                nvalue                  ();
  void                  reallocate_child        ();
protected:
  virtual void          size_request            (Requisition &requisition) override;
  virtual void          size_allocate           (Allocation area) override;
  virtual void          hierarchy_changed       (WindowImpl *old_toplevel) override;
  virtual bool          handle_event            (const Event &event) override;
  virtual void          reset                   (ResetMode mode = RESET_ALL) override;
public:
  explicit              SliderTroughImpl        ();
  virtual              ~SliderTroughImpl        () override;
  bool                  flipped                 () const;
  Adjustment*           adjustment              () const;
};

class SliderSkidImpl : public virtual SingleContainerImpl, public virtual SliderSkidIface, public virtual EventHandler {
  uint                  button_;
  double                coffset_;
  bool                  vertical_skid_;
  bool                  flipped                 () const;
  virtual bool          vertical_skid           () const override;
  virtual void          vertical_skid           (bool vs) override;
protected:
  virtual void          size_request            (Requisition &requisition) override;
  virtual void          reset                   (ResetMode mode = RESET_ALL) override;
  virtual bool          handle_event            (const Event &event) override;
public:
  explicit              SliderSkidImpl          ();
  virtual              ~SliderSkidImpl          () override;
};


} // Rapicorn

#endif  /* __RAPICORN_SLIDER_HH__ */
