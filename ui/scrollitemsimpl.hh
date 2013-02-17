// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_SCROLL_ITEMS_IMPL_HH__
#define __RAPICORN_SCROLL_ITEMS_IMPL_HH__

#include <ui/scrollitems.hh>
#include <ui/container.hh>

namespace Rapicorn {

class ScrollAreaImpl : public virtual SingleContainerImpl, public virtual ScrollArea {
  mutable Adjustment   *hadjustment_, *vadjustment_;
  Adjustment&           hadjustment() const;
  Adjustment&           vadjustment() const;
public:
  explicit              ScrollAreaImpl  ();
  virtual Adjustment*   get_adjustment  (AdjustmentSourceType adj_source,
                                         const String        &name = "");
  virtual double        xoffset         () const;
  virtual double        yoffset         () const;
  virtual void          scroll_to       (double x,
                                         double y);
};

} // Rapicorn

#endif  /* __RAPICORN_SCROLL_ITEMS_IMPL_HH__ */
