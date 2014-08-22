// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_SCROLL_WIDGETS_HH__
#define __RAPICORN_SCROLL_WIDGETS_HH__

#include <ui/adjustment.hh>
#include <ui/container.hh>

namespace Rapicorn {

class ScrollAreaImpl : public virtual SingleContainerImpl, public virtual ScrollAreaIface, public virtual AdjustmentSource {
  Adjustment           *hadjustment_, *vadjustment_;
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

} // Rapicorn

#endif  /* __RAPICORN_SCROLL_WIDGETS_HH__ */
