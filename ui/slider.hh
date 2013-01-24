// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_SLIDER_HH__
#define __RAPICORN_SLIDER_HH__

#include <ui/adjustment.hh>
#include <ui/container.hh>

namespace Rapicorn {

class SliderArea : public virtual ContainerImpl {
  bool                  move              (int);
protected:
  virtual const CommandList&    list_commands  ();
  virtual const PropertyList&   _property_list ();
  explicit              SliderArea        ();
  virtual void          control           (const String   &command_name,
                                           const String   &arg) = 0;
  virtual void          slider_changed    ();
  typedef Aida::Signal<void ()> SignalSliderChanged;
public:
  virtual bool          flipped           () const = 0;
  virtual void          flipped           (bool flip) = 0;
  virtual Adjustment*   adjustment        () const = 0;
  virtual void          adjustment        (Adjustment     &adjustment) = 0;
  virtual
  AdjustmentSourceType  adjustment_source () const = 0;
  virtual void          adjustment_source (AdjustmentSourceType adj_source) = 0;
  SignalSliderChanged   sig_slider_changed;
};

} // Rapicorn

#endif  /* __RAPICORN_SLIDER_HH__ */
