/* Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html */
#include "coffer.hh"
#include "containerimpl.hh"
#include "factory.hh"
#include "../rcore/rsvg/svg.hh"

namespace Rapicorn {

class CofferImpl : public virtual Coffer, public virtual SingleContainerImpl {
  String        m_element;
  Svg::Element  m_sel;
  bool          m_overlap_child;
protected:
  virtual void          size_request    (Requisition &requisition);
  virtual void          size_allocate   (Allocation area);
  virtual void          do_invalidate   ();
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
CofferImpl::size_request (Requisition &requisition)
{
  SingleContainerImpl::size_request (requisition);
  int thickness = 2; // FIXME: use real border marks
  if (!m_overlap_child)
    {
      requisition.width += 2 * thickness;
      requisition.height += 2 * thickness;
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

static const ItemFactory<CofferImpl> coffer_factory ("Rapicorn::Factory::Coffer");

} // Rapicorn
