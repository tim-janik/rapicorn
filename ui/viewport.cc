// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "viewport.hh"
#include "factory.hh"

namespace Rapicorn {

ViewportImpl::ViewportImpl ()
{}

ViewportImpl::~ViewportImpl ()
{}

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

} // Rapicorn
