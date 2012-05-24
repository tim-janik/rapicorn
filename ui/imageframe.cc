// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "imageframe.hh"
#include "container.hh"
#include "factory.hh"
#include "../rcore/rsvg/svg.hh"
#include <string.h>

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
  if (once_enter (&lib_list))
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
      once_leave (&lib_list, &lv);
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

class ImageFrameImpl : public virtual SingleContainerImpl, public virtual ImageFrame {
  String        m_element;
  Svg::ElementP m_sel;
  bool          m_overlap_child;
protected:
  virtual void          size_request    (Requisition &requisition);
  virtual void          size_allocate   (Allocation area, bool changed);
  virtual void          do_invalidate   ();
  virtual void          render          (RenderContext &rcontext, const Rect &rect);
public:
  explicit              ImageFrameImpl      ();
  virtual              ~ImageFrameImpl      ();
  virtual String        element         () const                { return m_element; }
  virtual void          element         (const String &id)      { m_element = id; invalidate(); }
  virtual bool          overlap_child   () const                { return m_overlap_child; }
  virtual void          overlap_child   (bool overlap)          { m_overlap_child = overlap; invalidate(); }
};

const PropertyList&
ImageFrame::list_properties()
{
  static Property *properties[] = {
    MakeProperty (ImageFrame, element,        _("Element"),         _("The SVG element ID to be rendered."), "rw"),
    MakeProperty (ImageFrame, overlap_child,  _("Overlap Child"),   _("Draw child on top of container area."), "rw"),
  };
  static const PropertyList property_list (properties, ContainerImpl::_property_list());
  return property_list;
}

ImageFrameImpl::ImageFrameImpl() :
  m_overlap_child (false)
{}

ImageFrameImpl::~ImageFrameImpl()
{}

void
ImageFrameImpl::do_invalidate()
{
  if (element_.empty())
    {
      m_sel = m_sel->none();
      return;
    }
  Svg::ElementP ep = library_lookup (m_element);
  if (!ep)
    return;
  m_sel = ep;
}

void
ImageFrameImpl::size_request (Requisition &requisition)
{
  SingleContainerImpl::size_request (requisition);
  if (sel_)
    {
      requisition.width = m_sel->bbox().width;
      requisition.height = m_sel->bbox().height;
      int thickness = 2; // FIXME: use real border marks
      if (!overlap_child_)
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
ImageFrameImpl::render (RenderContext &rcontext, const Rect &rect)
{
  if (sel_)
    {
      const Allocation &area = allocation();
      const int width = area.width, height = area.height;
      uint8 *pixels = new uint8[int (width * height * 4)];
      memset (pixels, 0, width * height * 4);
      cairo_surface_t *surface = cairo_image_surface_create_for_data (pixels, CAIRO_FORMAT_ARGB32, width, height, 4 * width);
      CHECK_CAIRO_STATUS (cairo_surface_status (surface));
      bool rendered = m_sel->render (surface, Svg::BBox (0, 0, width, height));
      if (rendered)
        {
          cairo_t *cr = cairo_context (rcontext, rect);
          cairo_set_source_surface (cr, surface, area.x, area.y); // shift into allocation area
          cairo_paint (cr);
        }
      else
        critical ("Failed to render SVG element: %s", element_.c_str());
      cairo_surface_destroy (surface);
      delete[] pixels;
    }
}

static const ItemFactory<ImageFrameImpl> image_frame_factory ("Rapicorn::Factory::ImageFrame");

} // Rapicorn
