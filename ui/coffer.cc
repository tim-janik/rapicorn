/* Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html */
#include "coffer.hh"
#include "containerimpl.hh"
#include "factory.hh"
#include "../rcore/rsvg/svg.hh"
#include <string.h>

#define CHECK_CAIRO_STATUS(status)      do { \
  cairo_status_t ___s = (status);            \
  if (___s != CAIRO_STATUS_SUCCESS)          \
    DEBUG ("%s: %s", cairo_status_to_string (___s), #status);   \
  } while (0)

namespace Rapicorn {

class CofferImpl : public virtual Coffer, public virtual SingleContainerImpl {
  String        m_element;
  Svg::Element  m_sel;
  bool          m_overlap_child;
protected:
  virtual void          size_request    (Requisition &requisition);
  virtual void          size_allocate   (Allocation area);
  virtual void          do_invalidate   ();
  virtual void          render          (Display &display);
public:
  explicit              CofferImpl      ();
  virtual              ~CofferImpl      ();
  virtual String        element         () const                { return m_element; }
  virtual void          element         (const String &id)      { m_element = id; invalidate(); }
  virtual bool          overlap_child   () const                { return m_overlap_child; }
  virtual void          overlap_child   (bool overlap)          { m_overlap_child = overlap; invalidate(); }
};

const PropertyList&
Coffer::list_properties()
{
  static Property *properties[] = {
    MakeProperty (Coffer, element,        _("Element"),         _("The SVG element ID to be rendered."), "rw"),
    MakeProperty (Coffer, overlap_child,  _("Overlap Child"),   _("Draw child on top of container area."), "rw"),
  };
  static const PropertyList property_list (properties, Container::list_properties());
  return property_list;
}

CofferImpl::CofferImpl() :
  m_overlap_child (false)
{}

CofferImpl::~CofferImpl()
{}

void
CofferImpl::do_invalidate()
{
  if (m_element.empty())
    {
      m_sel = m_sel.none();
      return;
    }
  Svg::Element e = Svg::Library::lookup_element (m_element);
  if (e.is_null())
    return;
  m_sel = e;
}

void
CofferImpl::size_request (Requisition &requisition)
{
  SingleContainerImpl::size_request (requisition);
  if (m_sel)
    {
      requisition.width = m_sel.allocation().width;
      requisition.height = m_sel.allocation().height;
      int thickness = 2; // FIXME: use real border marks
      if (!m_overlap_child)
        {
          requisition.width += 2 * thickness;
          requisition.height += 2 * thickness;
        }
    }
}

void
CofferImpl::size_allocate (Allocation area)
{
  Allocation carea = area;
  if (has_allocatable_child() && !m_overlap_child)
    {
      int thickness = 2; // FIXME: use real border marks
      carea.x += thickness;
      carea.y += thickness;
      carea.width -= 2 * thickness;
      carea.height -= 2 * thickness;
    }
  SingleContainerImpl::size_allocate (carea);
  allocation (area);
}

void
CofferImpl::render (Display &display)
{
  Allocation area = allocation();
  if (m_sel)
    {
      Svg::Allocation a = m_sel.allocation();
      uint8 *pixels = new uint8[int (a.width * a.height * 4)];
      memset (pixels, 0, a.width * a.height * 4);
      cairo_surface_t *surface = cairo_image_surface_create_for_data (pixels, CAIRO_FORMAT_ARGB32, a.width, a.height, 4 * a.width);
      CHECK_CAIRO_STATUS (cairo_surface_status (surface));
      bool rendered = m_sel.render (surface, Svg::Allocation (0, 0, a.width, a.height));
      if (rendered)
        {
          cairo_t *cr = display.create_cairo ();
          cairo_set_source_surface (cr, surface, 0, 0); // (x,y) are set in the matrix below
          cairo_matrix_t matrix;
          cairo_matrix_init_identity (&matrix);
          double sx = a.width / area.width;
          double sy = a.height / area.height;
          cairo_matrix_translate (&matrix, -area.x * sx, (area.y + area.height) * sy); // -x, y + height
          cairo_matrix_scale (&matrix, sx, -sy);
          cairo_pattern_set_matrix (cairo_get_source (cr), &matrix);
          cairo_paint (cr);
          printerr ("%s: scale: %g %g\n", STRFUNC, 1.0 / sx, 1.0 / sy);
        }
      else
        critical ("Failed to render SVG element: %s", m_element.c_str());
      cairo_surface_destroy (surface);
      delete[] pixels;
    }
  SingleContainerImpl::render (display);
}

static const ItemFactory<CofferImpl> coffer_factory ("Rapicorn::Factory::Coffer");

} // Rapicorn
