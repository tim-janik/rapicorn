// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "arrangementimpl.hh"
#include "factory.hh"

namespace Rapicorn {

const PropertyList&
Arrangement::_property_list()
{
  static Property *properties[] = {
    MakeProperty (Arrangement, xorigin,        _("X-Origin"),           _("The x coordinate origin to be displayed by the arrangement"), -MAXDOUBLE, +MAXDOUBLE, 10, "rw"),
    MakeProperty (Arrangement, yorigin,        _("Y-Origin"),           _("The y coordinate origin to be displayed by the arrangement"), -MAXDOUBLE, +MAXDOUBLE, 10, "rw"),
    MakeProperty (Arrangement, origin_hanchor, _("Horizontal Anchor"), _("Horizontal position of the origin within arrangement, 0=left, 1=right"), 0, 1, 0.1, "rw"),
    MakeProperty (Arrangement, origin_vanchor, _("Vertical Anchor"),   _("Vertical position of the origin within arrangement, 0=bottom, 1=top"), 0, 1, 0.1, "rw"),
  };
  static const PropertyList property_list (properties, ContainerImpl::_property_list());
  return property_list;
}

ArrangementImpl::ArrangementImpl() :
  origin_ (0, 0),
  origin_hanchor_ (0.5),
  origin_vanchor_ (0.5),
  child_area_()
{}

ArrangementImpl::~ArrangementImpl()
{}

Allocation
ArrangementImpl::local_child_allocation (ItemImpl &child,
                                         double    width,
                                         double    height)
{
  Requisition requisition = child.requisition();
  const PackInfo &pi = child.pack_info();
  Allocation area;
  area.width = iceil (requisition.width);
  area.height = iceil (requisition.height);
  double origin_x = width * origin_hanchor_ - origin_.x;
  double origin_y = height * origin_vanchor_ - origin_.y;
  area.x = iround (origin_x + pi.hposition - pi.halign * area.width);
  area.y = iround (origin_y + pi.vposition - pi.valign * area.height);
  if (width > 0 && child.hexpand())
    {
      area.x = 0;
      area.width = iround (width);
    }
  if (height > 0 && child.vexpand())
    {
      area.y = 0;
      area.height = iround (height);
    }
  return area;
}

Rect
ArrangementImpl::child_area ()
{
  Rect rect; /* empty */
  Allocation parea = allocation();
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    {
      ItemImpl &child = *cw;
      Allocation area = local_child_allocation (child, parea.width, parea.height);
      rect.rect_union (Rect (Point (area.x, area.y), 1, 1));
      rect.rect_union (Rect (Point (area.x + area.width - 1, area.y + area.height - 1), 1, 1));
    }
  return rect;
}

void
ArrangementImpl::size_request (Requisition &requisition)
{
  Rect rect; /* empty */
  bool chspread = false, cvspread = false, need_origin = false;
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    {
      ItemImpl &child = *cw;
      /* size request all children */
      // Requisition rq = child.requisition();
      if (!child.allocatable())
        continue;
      chspread |= child.hspread();
      cvspread |= child.vspread();
      Allocation area = local_child_allocation (child, 0, 0);
      rect.rect_union (Rect (Point (area.x, area.y), 1, 1));
      rect.rect_union (Rect (Point (area.x + area.width - 1, area.y + area.height - 1), 1, 1));
      need_origin = true;
    }
  if (need_origin)
    rect.rect_union (Rect (Point (0, 0), 1, 1));
  double side1, side2;
  /* calculate size requisition in the west and east of anchor */
  side1 = MAX (-rect.x, 0);
  side2 = MAX (0, rect.upper_x());
  /* expand proportionally */
  side1 = side1 * 1.0 / CLAMP (origin_hanchor(), 0.5, 1);
  side2 = side2 * 1.0 / (1 - CLAMP (origin_hanchor(), 0, 0.5));
  requisition.width = MAX (side1, side2);
  /* calculate size requisition in the south and north of anchor */
  side1 = MAX (-rect.y, 0);
  side2 = MAX (0, rect.upper_y());
  /* expand proportionally */
  side1 = side1 * 1.0 / CLAMP (origin_vanchor(), 0.5, 1);
  side2 = side2 * 1.0 / (1 - CLAMP (origin_vanchor(), 0, 0.5));
  requisition.height = MAX (side1, side2);
  set_flag (HSPREAD_CONTAINER, chspread);
  set_flag (VSPREAD_CONTAINER, cvspread);
}

void
ArrangementImpl::size_allocate (Allocation area, bool changed)
{
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    {
      ItemImpl &child = *cw;
      if (!child.allocatable())
        continue;
      Allocation carea = local_child_allocation (child, area.width, area.height);
      carea.x += area.x;
      carea.y += area.y;
      /* allocate child */
      child.set_allocation (carea);
    }
}

static const ItemFactory<ArrangementImpl> arrangement_factory ("Rapicorn::Factory::Arrangement");

} // Rapicorn
