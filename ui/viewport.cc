// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "viewport.hh"
#include "factory.hh"

namespace Rapicorn {

ViewportImpl::ViewportImpl () :
  m_tunable_requisition_counter (0), m_xoffset (0), m_yoffset (0)
{}

ViewportImpl::~ViewportImpl ()
{}

void
ViewportImpl::negotiate_size (const Allocation *carea)
{
  return_if_fail (requisitions_tunable() == false);
  const bool have_allocation = carea != NULL;
  Allocation area;
  if (have_allocation)
    {
      area = *carea;
      area.x = area.y = 0;
      change_flags_silently (INVALID_ALLOCATION, true);
    }
  /* this is the core of the resizing loop. via Item.tune_requisition(), we
   * allow items to adjust the requisition from within size_allocate().
   * whether the tuned requisition is honored at all, depends on
   * m_tunable_requisition_counter.
   * currently, we simply freeze the allocation after 3 iterations. for the
   * future it's possible to honor the tuned requisition only partially or
   * proportionally as m_tunable_requisition_counter decreases, so to mimick
   * a simulated annealing process yielding the final layout.
   */
  m_tunable_requisition_counter = 3;
  while (test_flags (INVALID_REQUISITION | INVALID_ALLOCATION))
    {
      const Requisition creq = requisition(); // unsets INVALID_REQUISITION
      if (!have_allocation)
        {
          // seed allocation from requisition
          area.width = creq.width;
          area.height = creq.height;
        }
      // a viewport child is allocated relative to the Viewport origin, normally at 0,0
      set_allocation (area); // unsets INVALID_ALLOCATION, may re-::invalidate_size()
      if (m_tunable_requisition_counter)
        m_tunable_requisition_counter--;
    }
  m_tunable_requisition_counter = 0;
}

void
ViewportImpl::allocate_size (const Allocation &area)
{
  return_if_fail (area.width >= 0 && area.height >= 0);
  negotiate_size (&area);
}

void
ViewportImpl::scroll_offsets (int deltax, int deltay)
{
  if (deltax != m_xoffset || deltay != m_yoffset)
    {
      m_xoffset = deltax;
      m_yoffset = deltay;
      expose();
      // FIXME: need to issue 0-distance move here
    }
}

Affine
ViewportImpl::child_affine (const ItemImpl &item)
{
  const Allocation &area = allocation();
  const int xoffset = scroll_offset_x(), yoffset = scroll_offset_y();
  return AffineTranslate (-area.x + xoffset, -area.y + yoffset);
}

void
ViewportImpl::render_item (RenderContext &rcontext)
{
  // prevent recursive rendering of children
  ItemImpl::render_item (rcontext);
  // viewport children are rendered in render()
}

void
ViewportImpl::render (RenderContext &rcontext, const Rect &rect)
{
  if (!has_drawable_child())
    return;
  const Allocation &area = allocation();
  ItemImpl &child = get_child();
  const int xoffset = m_xoffset, yoffset = m_yoffset;
  // constrain rendering within allocation
  Region what = rect;
  // constrain to child allocation (child is allocated relative to Viewport origin)
  const Allocation carea = child.allocation();
  what.intersect (Rect (area.x + carea.x, area.y + carea.y, carea.width, carea.height));
  // constrain to exposed region
  what.intersect (rendering_region (rcontext));
  // viewport rendering rectangle
  const Allocation rarea = what.extents();
  // translate area into child space, shifting by scroll offsets
  what.translate (xoffset - area.x, yoffset - area.y);
  // render child stack
  if (!what.empty())
    {
      m_expose_region.subtract (what);
      cairo_t *cr = cairo_context (rcontext, rarea);
      cairo_translate (cr, area.x - xoffset, area.y - yoffset);
      child.render_into (cr, what);
    }
}

void
ViewportImpl::expose_child_region (const Region &region)
{
  if (!region.empty())
    {
      m_expose_region.add (region);
      collapse_expose_region();
      if (parent())
        {
          const Allocation &area = allocation();
          // propagate exposes, to make child rendering changes visible at toplevel
          Region vpregion = region;
          vpregion.translate (area.x - m_xoffset, area.y - m_yoffset); // translate to viewport coords
          expose (vpregion);
        }
    }
}

void
ViewportImpl::collapse_expose_region ()
{
  // check for excess expose fragment scenarios
  uint n_erects = m_expose_region.count_rects();
  /* considering O(n^2) collision computation complexity, but also focus frame
   * exposures which easily consist of 4+ fragments, a hundred rectangles turn
   * out to be an emperically suitable threshold.
   */
  if (n_erects > 99)
    {
      /* aparently the expose fragments we're combining are too small,
       * so we can end up with spending too much time on expose rectangle
       * compression (more time than needed for actual rendering).
       * as a workaround, we simply force everything into a single expose
       * rectangle which is good enough to avoid worst case explosion.
       */
      m_expose_region.add (m_expose_region.extents());
      // printerr ("ViewportImpl: collapsing expose rectangles due to overflow: %u -> %u\n", n_erects, m_expose_region.count_rects());
    }
}

} // Rapicorn
