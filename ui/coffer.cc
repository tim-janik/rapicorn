/* Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html */
#include "coffer.hh"
#include "container.hh"
#include "factory.hh"
#include "../rcore/rsvg/svg.hh"
#include <string.h>

#define CHECK_CAIRO_STATUS(status)      do { \
  cairo_status_t ___s = (status);            \
  if (___s != CAIRO_STATUS_SUCCESS)          \
    DEBUG ("%s: %s", cairo_status_to_string (___s), #status);   \
  } while (0)

namespace Rapicorn {

class CofferImpl : public virtual SingleContainerImpl, public virtual Coffer {
  String        m_element;
  Svg::Element  m_sel;
  bool          m_overlap_child;
protected:
  virtual void          size_request    (Requisition &requisition);
  virtual void          size_allocate   (Allocation area, bool changed);
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
  static const PropertyList property_list (properties, ContainerImpl::list_properties());
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
CofferImpl::size_allocate (Allocation area, bool changed)
{
  if (has_allocatable_child())
    {
      Allocation carea = area;
      if (!m_overlap_child)
        {
          int thickness = 2; // FIXME: use real border marks
          carea.x += thickness;
          carea.y += thickness;
          carea.width -= 2 * thickness;
          carea.height -= 2 * thickness;
        }
      ItemImpl &child = get_child();
      Allocation child_area = layout_child (child, carea);
      child.set_allocation (child_area);
    }
}

void
CofferImpl::render (Display &display)
{
  Allocation area = allocation();
  if (m_sel)
    {
      const int aw = area.width, ah = area.height;
      uint8 *pixels = new uint8[int (aw * ah * 4)];
      memset (pixels, 0, aw * ah * 4);
      cairo_surface_t *surface = cairo_image_surface_create_for_data (pixels, CAIRO_FORMAT_ARGB32, aw, ah, 4 * aw);
      CHECK_CAIRO_STATUS (cairo_surface_status (surface));
      bool rendered = m_sel.render (surface, Svg::Allocation (0, 0, aw, ah));
      if (rendered)
        {
          cairo_t *cr = display.create_cairo ();
          cairo_set_source_surface (cr, surface, 0, 0); // (x,y) are set in the matrix below
          cairo_matrix_t matrix;
          cairo_matrix_init_identity (&matrix);
          cairo_matrix_translate (&matrix, -area.x, (area.y + area.height)); // -x, y + height
          cairo_matrix_scale (&matrix, 1, -1);
          cairo_pattern_set_matrix (cairo_get_source (cr), &matrix);
          cairo_paint (cr);
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
