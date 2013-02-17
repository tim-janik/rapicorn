// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_ARRANGEMENT_IMPL_HH__
#define __RAPICORN_ARRANGEMENT_IMPL_HH__

#include <ui/arrangement.hh>
#include <ui/container.hh>

namespace Rapicorn {

class ArrangementImpl : public virtual MultiContainerImpl, public virtual Arrangement {
  Point         origin_;
  float         origin_hanchor_;
  float         origin_vanchor_;
  Rect          child_area_;
public:
  explicit                      ArrangementImpl         ();
  virtual                       ~ArrangementImpl        ();
  virtual double                xorigin                 () const                { return origin_.x; }
  virtual void                  xorigin                 (double v)              { origin_.x = v; invalidate(); }
  virtual double                yorigin                 () const                { return origin_.y; }
  virtual void                  yorigin                 (double v)              { origin_.y = v; invalidate(); }
  virtual float                 origin_hanchor          () const                { return origin_hanchor_; }
  virtual void                  origin_hanchor          (float align)           { origin_hanchor_ = align; invalidate(); }
  virtual float                 origin_vanchor          () const                { return origin_vanchor_; }
  virtual void                  origin_vanchor          (float align)           { origin_vanchor_ = align; invalidate(); }
  virtual Rect                  child_area              ();
protected:
  virtual void                  size_request            (Requisition &requisition);
  virtual void                  size_allocate           (Allocation area, bool changed);
  Allocation                    local_child_allocation  (ItemImpl &child,
                                                         double    width,
                                                         double    height);
};

} // Rapicorn

#endif  /* __RAPICORN_ARRANGEMENT_IMPL_HH__ */
