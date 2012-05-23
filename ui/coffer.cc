/* Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html */
#include "coffer.hh"
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
            if (!fp->error())
              lv.push_back (fp);
            SVGDEBUG ("loading: %s: %s", s.c_str(), strerror (fp->error()));
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

class CofferImpl : public virtual SingleContainerImpl, public virtual Coffer {
  String        m_element;
  Svg::ElementP m_sel;
  bool          m_overlap_child;
protected:
  virtual void          size_request    (Requisition &requisition);
  virtual void          size_allocate   (Allocation area, bool changed);
  virtual void          do_invalidate   ();
  virtual void          render          (RenderContext &rcontext, const Rect &rect);
public:
  explicit              CofferImpl      ();
  virtual              ~CofferImpl      ();
  virtual String        element         () const                { return element_; }
  virtual void          element         (const String &id)      { element_ = id; invalidate(); }
  virtual bool          overlap_child   () const                { return overlap_child_; }
  virtual void          overlap_child   (bool overlap)          { overlap_child_ = overlap; invalidate(); }
};

const PropertyList&
Coffer::_property_list()
{
  static Property *properties[] = {
    MakeProperty (Coffer, element,        _("Element"),         _("The SVG element ID to be rendered."), "rw"),
    MakeProperty (Coffer, overlap_child,  _("Overlap Child"),   _("Draw child on top of container area."), "rw"),
  };
  static const PropertyList property_list (properties, ContainerImpl::_property_list());
  return property_list;
}

CofferImpl::CofferImpl() :
  overlap_child_ (false)
{}

CofferImpl::~CofferImpl()
{}

void
CofferImpl::do_invalidate()
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
CofferImpl::size_request (Requisition &requisition)
{
  SingleContainerImpl::size_request (requisition);
  if (sel_)
    {
      requisition.width = m_sel->allocation().width;
      requisition.height = m_sel->allocation().height;
      int thickness = 2; // FIXME: use real border marks
      if (!overlap_child_)
        {
          requisition.width += 2 * thickness;
          requisition.height += 2 * thickness;
        }
    }
}

void
CofferImpl::size_allocate (Allocation area, bool changed)
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
CofferImpl::render (RenderContext &rcontext, const Rect &rect)
{
  if (sel_)
    {
      const Allocation &area = allocation();
      const int width = area.width, height = area.height;
      uint8 *pixels = new uint8[int (width * height * 4)];
      memset (pixels, 0, width * height * 4);
      cairo_surface_t *surface = cairo_image_surface_create_for_data (pixels, CAIRO_FORMAT_ARGB32, width, height, 4 * width);
      CHECK_CAIRO_STATUS (cairo_surface_status (surface));
      bool rendered = m_sel->render (surface, Svg::Allocation (0, 0, width, height));
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

static const WidgetFactory<CofferImpl> coffer_factory ("Rapicorn::Factory::Coffer");

} // Rapicorn
