// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_SCROLL_ITEMS_HH__
#define __RAPICORN_SCROLL_ITEMS_HH__

#include <ui/adjustment.hh>
#include <ui/container.hh>

namespace Rapicorn {

/* --- ScrollArea --- */
class ScrollArea : public virtual ContainerImpl, public virtual AdjustmentSource {
protected:
  explicit              ScrollArea();
public:
  virtual double        xoffset         () const = 0;
  virtual double        yoffset         () const = 0;
  virtual void          scroll_to       (double x,
                                         double y) = 0;
};

} // Rapicorn

#endif  /* __RAPICORN_SCROLL_ITEMS_HH__ */
