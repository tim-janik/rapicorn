// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_VIEWPORT_HH__
#define __RAPICORN_VIEWPORT_HH__

#include <ui/container.hh>
#include <ui/screenwindow.hh>

namespace Rapicorn {

class ViewportImpl : public virtual ResizeContainerImpl {
  Region                m_expose_region;        // maintained in child coord space
  int                   m_xoffset, m_yoffset;
  void                  collapse_expose_region  ();
protected:
  virtual Affine        child_affine            (const ItemImpl &item);
  const Region&         peek_expose_region      () const { return m_expose_region; }
  void                  discard_expose_region   () { m_expose_region.clear(); }
  bool                  exposes_pending         () const { return !m_expose_region.empty(); }
  virtual void          render_item             (RenderContext &rcontext);
  virtual void          render                  (RenderContext &rcontext, const Rect &rect);
  void                  scroll_offsets          (int deltax, int deltay);
  void                  do_scrolled             ();
  int                   scroll_offset_x         () const { return m_xoffset; }
  int                   scroll_offset_y         () const { return m_yoffset; }
public:
  Signal<ViewportImpl, void ()> sig_scrolled;
  void                  expose_child_region     (const Region &region);
  Allocation            child_viewport          ();
  explicit              ViewportImpl            ();
  virtual              ~ViewportImpl            ();
};

} // Rapicorn

#endif  /* __RAPICORN_VIEWPORT_HH__ */
