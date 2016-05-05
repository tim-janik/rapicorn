// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "viewport.hh"
#include "factory.hh"

#define VDEBUG(...)     RAPICORN_KEY_DEBUG ("Viewport", __VA_ARGS__)

namespace Rapicorn {

ViewportImpl::ViewportImpl ()
{
  const_cast<AnchorInfo*> (force_anchor_info())->viewport = this;
}

ViewportImpl::~ViewportImpl ()
{
  const_cast<AnchorInfo*> (force_anchor_info())->viewport = NULL;
}

void
ViewportImpl::expose_child_region (const Region &region)
{
  if (!region.empty())
    {
      expose_region_.add (region);
      collapse_expose_region();
      if (parent())
        {
          // propagate exposes, to make child rendering changes visible at toplevel
          expose (region);
        }
    }
}

void
ViewportImpl::collapse_expose_region ()
{
  // check for excess expose fragment scenarios
  uint n_erects = expose_region_.count_rects();
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
      expose_region_.add (expose_region_.extents());
      VDEBUG ("collapsing expose rectangles due to overflow: %u -> %u", n_erects, expose_region_.count_rects());
    }
}

} // Rapicorn
