// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "viewport.hh"
#include "factory.hh"

#define VDEBUG(...)     RAPICORN_KEY_DEBUG ("Viewport", __VA_ARGS__)

namespace Rapicorn {

ViewportImpl::ViewportImpl () :
  m_xoffset (0), m_yoffset (0),
  sig_scrolled (*this, &ViewportImpl::do_scrolled)
{}

ViewportImpl::~ViewportImpl ()
{}


void
ViewportImpl::scroll_offsets (int deltax, int deltay)
{
  if (deltax != m_xoffset || deltay != m_yoffset)
    {
      m_xoffset = deltax;
      m_yoffset = deltay;
      // FIXME: need to issue 0-distance move here
      sig_scrolled.emit();
    }
}

void
ViewportImpl::do_scrolled ()
{
  expose();
}

Allocation
ViewportImpl::child_viewport ()
{
  const Allocation &area = allocation();
  const int xoffset = scroll_offset_x(), yoffset = scroll_offset_y();
  return Allocation (xoffset, yoffset, area.width, area.height);
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
      VDEBUG ("collapsing expose rectangles due to overflow: %u -> %u\n", n_erects, m_expose_region.count_rects());
    }
}

} // Rapicorn
