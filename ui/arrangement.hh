// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_ARRANGEMENT_HH__
#define __RAPICORN_ARRANGEMENT_HH__

#include <ui/container.hh>

namespace Rapicorn {

class ArrangementImpl : public virtual MultiContainerImpl, public virtual ArrangementIface {
  Point         origin_;
  float         origin_hanchor_;
  float         origin_vanchor_;
  IRect         child_area_;
public:
  explicit       ArrangementImpl ();
  virtual       ~ArrangementImpl ();

  virtual double xorigin         () const override;
  virtual void   xorigin         (double) override;
  virtual double yorigin         () const override;
  virtual void   yorigin         (double) override;
  virtual double origin_hanchor  () const override;
  virtual void   origin_hanchor  (double) override;
  virtual double origin_vanchor  () const override;
  virtual void   origin_vanchor  (double) override;
  virtual IRect  child_area      ();
protected:
  virtual void   size_request            (Requisition &requisition);
  virtual void   size_allocate           (Allocation area);
  Allocation     local_child_allocation  (WidgetImpl &child, int width, int height);
};

} // Rapicorn

#endif  /* __RAPICORN_ARRANGEMENT_HH__ */
