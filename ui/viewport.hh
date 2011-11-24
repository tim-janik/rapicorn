// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_VIEWPORT_HH__
#define __RAPICORN_VIEWPORT_HH__

#include <ui/container.hh>
#include <ui/screenwindow.hh>

namespace Rapicorn {

class ViewportImpl : public virtual SingleContainerImpl {
  Region                m_expose_region;
  uint                  m_tunable_requisition_counter;
  void                  negotiate_size          (const Allocation *carea);
protected:
  void                  negotiate_size          (void) { negotiate_size (NULL); }
  void                  allocate_size           (const Allocation &area);
  void                  queue_expose_region     (const Region &region);
  void                  queue_expose_rect       (const Rect   &rect);
  const Region&         peek_expose_region      () const { return m_expose_region; }
  void                  collapse_expose_region  ();
  void                  discard_expose_region   () { m_expose_region.clear(); }
  bool                  exposes_pending         () const { return !m_expose_region.empty(); }
  virtual void          render_item             (RenderContext &rcontext);
public:
  void                  expose_child            (const Region &region);
  void                  expose_child            (const Rect   &rect) { expose_child (Region (rect)); }
  bool                  requisitions_tunable    () const { return m_tunable_requisition_counter > 0; }
  explicit              ViewportImpl            ();
  virtual              ~ViewportImpl            ();
};

} // Rapicorn

#endif  /* __RAPICORN_VIEWPORT_HH__ */
