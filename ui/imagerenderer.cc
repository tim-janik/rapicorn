// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#include "imagerenderer.hh"
#include "factory.hh"
#include "../rcore/rsvg/svg.hh"

#define SVGDEBUG(...)   RAPICORN_KEY_DEBUG ("SVG", __VA_ARGS__)

#define CHECK_CAIRO_STATUS(status)      do { \
  cairo_status_t ___s = (status);            \
  if (___s != CAIRO_STATUS_SUCCESS)          \
    SVGDEBUG ("%s: %s", cairo_status_to_string (___s), #status);   \
  } while (0)

namespace Rapicorn {

static const vector<Svg::FileP>&
list_library_files ()
{
  static vector<Svg::FileP> *lib_list = NULL;
  do_once
    {
      static vector<Svg::FileP> lv;
      const char *rp = getenv ("RAPICORN_SVG");
      for (auto s : Path::searchpath_split (rp ? rp : ""))
        if (s.size())
          {
            Svg::FileP fp = Svg::File::load (s);
            SVGDEBUG ("loading: %s: %s", s.c_str(), strerror (errno));
            if (fp)
              lv.push_back (fp);
          }
      lib_list = &lv;
    }
  return *lib_list;
}

static Svg::ElementP
library_lookup (const String &elementid)
{
  const vector<Svg::FileP> &libs = list_library_files();
  Svg::ElementP ep;
  for (auto file : libs)
    {
      ep = file->lookup (elementid);
      if (ep)
        break;
    }
  SVGDEBUG ("lookup: %s: %s", elementid.c_str(), ep ? "OK" : "failed");
  return ep;
}

// == ImageFrameImpl ==
class ImageFrameImpl::SvgElementP : public Svg::ElementP
{};

ImageFrameImpl::ImageFrameImpl() :
  overlap_child_ (false)
{
  static_assert (sizeof (SvgElementP) == sizeof (svgelep_), "std::shared_ptr<> size inveriant");
  new (&svgelep_) SvgElementP();
}

ImageFrameImpl::~ImageFrameImpl()
{
  svg_element_ptr().~SvgElementP();
}

ImageFrameImpl::SvgElementP&
ImageFrameImpl::svg_element_ptr () const
{
  // we use a dedicated accessor for SvgElementP aka Svg::ElementP, to hide SVG headers from our .hh file
  return *(SvgElementP*) &svgelep_;
}

void
ImageFrameImpl::set_element (const String &id)
{
  element_ = id;
  invalidate();
}

void
ImageFrameImpl::set_overlap (bool overlap)
{
  overlap_child_ = overlap;
  invalidate();
}

void
ImageFrameImpl::do_invalidate()
{
  Svg::ElementP &sep = svg_element_ptr();
  if (element_.empty())
    {
      sep = sep->none();
      return;
    }
  Svg::ElementP ep = library_lookup (element_);
  if (!ep)
    return;
  sep = ep;
}

void
ImageFrameImpl::size_request (Requisition &requisition)
{
  SingleContainerImpl::size_request (requisition);
  Svg::ElementP &sep = svg_element_ptr();
  if (sep)
    {
      requisition.width = sep->bbox().width;
      requisition.height = sep->bbox().height;
      const int thickness = 2; // FIXME: use real border marks
      if (!overlap_child_ && has_children())
        {
          requisition.width += 2 * thickness;
          requisition.height += 2 * thickness;
        }
    }
}

void
ImageFrameImpl::size_allocate (Allocation area, bool changed)
{
  if (has_visible_child())
    {
      Allocation carea = area;
      if (!overlap_child_)
        {
          int thickness = 2; // FIXME: use real border marks
          carea.x += thickness;
          carea.y += thickness;
          carea.width -= 2 * thickness;
          carea.height -= 2 * thickness;
        }
      WidgetImpl &child = get_child();
      Allocation child_area = layout_child (child, carea);
      child.set_allocation (child_area);
    }
}

void
ImageFrameImpl::render (RenderContext &rcontext, const Rect &render_rect)
{
  Svg::ElementP &sep = svg_element_ptr();
  const Allocation &area = allocation();
  Rect rect = area;
  rect.intersect (render_rect);
  if (sep && rect.width > 0 && rect.height > 0)
    {
      const uint npixels = rect.width * rect.height;
      uint8 *pixels = new uint8[int (npixels * 4)];
      memset (pixels, 0, npixels * 4);
      cairo_surface_t *surface = cairo_image_surface_create_for_data (pixels, CAIRO_FORMAT_ARGB32,
                                                                      rect.width, rect.height, 4 * rect.width);
      CHECK_CAIRO_STATUS (cairo_surface_status (surface));
      cairo_surface_set_device_offset (surface, -(rect.x - area.x), -(rect.y - area.y)); // offset into intersection
      Svg::BBox bbox = sep->bbox();
      const bool rendered = sep->render (surface, Svg::RenderSize::ZOOM, area.width / bbox.width, area.height / bbox.height);
      if (rendered)
        {
          cairo_t *cr = cairo_context (rcontext, rect);
          cairo_set_source_rgb (cr, 1, 0, 0); cairo_paint (cr);
          cairo_set_source_surface (cr, surface, rect.x, rect.y); // shift into allocation area
          cairo_paint (cr);
        }
      else
        critical ("Failed to render SVG element: %s", element_.c_str());
      cairo_surface_destroy (surface);
      delete[] pixels;
    }
}

static const WidgetFactory<ImageFrameImpl> image_frame_factory ("Rapicorn_Factory:ImageFrame");

} // Rapicorn
