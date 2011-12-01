// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "viewport.hh"
#include "factory.hh"

namespace Rapicorn {

ViewportImpl::ViewportImpl () :
  m_tunable_requisition_counter (0)
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
      change_flags_silently (INVALID_ALLOCATION, true);
    }
  else // !have_allocation
    area = allocation(); // keep x,y
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
ViewportImpl::render (Display &display)
{
  const IRect ia = allocation();
  display.push_clip_rect (ia.x, ia.y, ia.width, ia.height);
  // paint background
  cairo_t *cr = display.create_cairo (background());
  cairo_destroy (cr);
  // paint children
  SingleContainerImpl::render (display);
  display.pop_clip_rect();
}

void
ViewportImpl::queue_expose_region (const Region &region)
{
  if (!region.empty())
    {
      m_expose_region.add (region);
      collapse_expose_region();
    }
}

void
ViewportImpl::queue_expose_rect (const Rect &rect)
{
  m_expose_region.add (rect);
  collapse_expose_region();
}

void
ViewportImpl::expose_child (const Region &region)
{
  // FIXME: need affine handling here?
  queue_expose_region (region);
}

void
ViewportImpl::collapse_expose_region ()
{
  /* check for excess expose fragment scenarios */
  uint n_erects = m_expose_region.count_rects();
  if (n_erects > 999)           /* workable limit, considering O(n^2) collision complexity */
    {
      /* aparently the expose fragments we're combining are too small,
       * so we can end up with spending too much time on expose rectangle
       * compression (much more than needed for rendering).
       * as a workaround, we simply force everything into a single expose
       * rectangle which is good enough to avoid worst case explosion.
       * note that this is only triggered in seldom pathological cases.
       */
      m_expose_region.add (m_expose_region.extents());
      // printerr ("collapsing due to too many expose rectangles: %u -> %u\n", n_erects, m_expose_region.count_rects());
    }
}

} // Rapicorn
