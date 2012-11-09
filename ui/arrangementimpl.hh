// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_ARRANGEMENT_IMPL_HH__
#define __RAPICORN_ARRANGEMENT_IMPL_HH__

#include <ui/arrangement.hh>
#include <ui/container.hh>

namespace Rapicorn {

class ArrangementImpl : public virtual MultiContainerImpl, public virtual Arrangement {
  Point         m_origin;
  float         m_origin_hanchor;
  float         m_origin_vanchor;
  Rect          m_child_area;
public:
  explicit                      ArrangementImpl         ();
  virtual                       ~ArrangementImpl        ();
  virtual double                xorigin                 ()                      { return m_origin.x; }
  virtual void                  xorigin                 (double v)              { m_origin.x = v; invalidate(); }
  virtual double                yorigin                 ()                      { return m_origin.y; }
  virtual void                  yorigin                 (double v)              { m_origin.y = v; invalidate(); }
  virtual float                 origin_hanchor          ()                      { return m_origin_hanchor; }
  virtual void                  origin_hanchor          (float align)           { m_origin_hanchor = align; invalidate(); }
  virtual float                 origin_vanchor          ()                      { return m_origin_vanchor; }
  virtual void                  origin_vanchor          (float align)           { m_origin_vanchor = align; invalidate(); }
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
