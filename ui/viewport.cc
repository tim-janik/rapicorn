// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "viewport.hh"
#include "factory.hh"

#define VDEBUG(...)     RAPICORN_KEY_DEBUG ("Viewport", __VA_ARGS__)

namespace Rapicorn {

ViewportImpl::ViewportImpl () :
  xoffset_ (0), yoffset_ (0),
  sig_scrolled (Aida::slot (*this, &ViewportImpl::do_scrolled))
{
  const_cast<AnchorInfo*> (force_anchor_info())->viewport = this;
}

ViewportImpl::~ViewportImpl ()
{
  const_cast<AnchorInfo*> (force_anchor_info())->viewport = NULL;
}

void
ViewportImpl::scroll_offsets (int deltax, int deltay)
{
  if (deltax != xoffset_ || deltay != yoffset_)
    {
      xoffset_ = deltax;
      yoffset_ = deltay;
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
ViewportImpl::child_view_area (const WidgetImpl &child)
{
  const int xoffset = scroll_offset_x(), yoffset = scroll_offset_y();
  Allocation view = allocation();
  view.x = xoffset;
  view.y = yoffset;
  Allocation area = child.clipped_allocation();
  area.intersect (view);
  return area;
}

Affine
ViewportImpl::child_affine (const WidgetImpl &widget)
{
  const Allocation &area = allocation();
  const int xoffset = scroll_offset_x(), yoffset = scroll_offset_y();
  return AffineTranslate (-area.x + xoffset, -area.y + yoffset);
}

void
ViewportImpl::render_recursive (RenderContext &rcontext)
{
  // prevent recursive rendering of children by not calling ResizeContainerImpl::render_recursive
  if (0)
    ResizeContainerImpl::render_recursive (rcontext);
  // viewport children are rendered in render()
}

void
ViewportImpl::render (RenderContext &rcontext, const Rect &rect)
{
  if (!has_drawable_child())
    return;
  const Allocation &area = allocation();
  WidgetImpl &child = get_child();
  const int xoffset = xoffset_, yoffset = yoffset_;
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
      expose_region_.subtract (what);
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
      expose_region_.add (region);
      collapse_expose_region();
      if (parent())
        {
          const Allocation &area = allocation();
          // propagate exposes, to make child rendering changes visible at toplevel
          Region vpregion = region;
          vpregion.translate (area.x - xoffset_, area.y - yoffset_); // translate to viewport coords
          expose (vpregion);
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
