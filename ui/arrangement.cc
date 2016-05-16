// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "arrangement.hh"
#include "factory.hh"

namespace Rapicorn {

ArrangementImpl::ArrangementImpl() :
  origin_ (0, 0),
  origin_hanchor_ (0.5),
  origin_vanchor_ (0.5),
  child_area_()
{}

ArrangementImpl::~ArrangementImpl()
{}

double
ArrangementImpl::xorigin () const
{
  return origin_.x;
}

void
ArrangementImpl::xorigin (double v)
{
  origin_.x = v;
  invalidate();
  changed ("xorigin");
}

double
ArrangementImpl::yorigin () const
{
  return origin_.y;
}

void
ArrangementImpl::yorigin (double v)
{
  origin_.y = v;
  invalidate();
  changed ("yorigin");
}

double
ArrangementImpl::origin_hanchor () const
{
  return origin_hanchor_;
}

void
ArrangementImpl::origin_hanchor (double align)
{
  origin_hanchor_ = align;
  invalidate();
  changed ("origin_hanchor");
}

double
ArrangementImpl::origin_vanchor () const
{
  return origin_vanchor_;
}

void
ArrangementImpl::origin_vanchor (double align)
{
  origin_vanchor_ = align;
  invalidate();
  changed ("origin_vanchor");
}

Allocation
ArrangementImpl::local_child_allocation (WidgetImpl &child,
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

DRect
ArrangementImpl::child_area ()
{
  DRect rect; /* empty */
  Allocation parea = allocation();
  for (auto childp : *this)
    {
      WidgetImpl &child = *childp;
      Allocation area = local_child_allocation (child, parea.width, parea.height);
      rect.rect_union (DRect (Point (area.x, area.y), 1, 1));
      rect.rect_union (DRect (Point (area.x + area.width - 1, area.y + area.height - 1), 1, 1));
    }
  return rect;
}

void
ArrangementImpl::size_request (Requisition &requisition)
{
  DRect rect; /* empty */
  bool chspread = false, cvspread = false, need_origin = false;
  for (auto childp : *this)
    {
      WidgetImpl &child = *childp;
      /* size request all children */
      // Requisition rq = child.requisition();
      if (!child.visible())
        continue;
      chspread |= child.hspread();
      cvspread |= child.vspread();
      Allocation area = local_child_allocation (child, 0, 0);
      rect.rect_union (DRect (Point (area.x, area.y), 1, 1));
      rect.rect_union (DRect (Point (area.x + area.width - 1, area.y + area.height - 1), 1, 1));
      need_origin = true;
    }
  if (need_origin)
    rect.rect_union (DRect (Point (0, 0), 1, 1));
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
  for (auto childp : *this)
    {
      WidgetImpl &child = *childp;
      if (!child.visible())
        continue;
      Allocation carea = local_child_allocation (child, area.width, area.height);
      carea.x += area.x;
      carea.y += area.y;
      /* allocate child */
      child.set_allocation (carea);
    }
}

static const WidgetFactory<ArrangementImpl> arrangement_factory ("Rapicorn::Arrangement");

} // Rapicorn
