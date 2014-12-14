// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#include "layoutcontainers.hh"
#include "factory.hh"
#include "../rcore/rsvg/svg.hh" // Svg::Span

namespace Rapicorn {

// == AlignmentImpl ==
AlignmentImpl::AlignmentImpl() :
  left_padding_ (0), right_padding_ (0),
  bottom_padding_ (0), top_padding_ (0)
{}

AlignmentImpl::~AlignmentImpl ()
{}

void
AlignmentImpl::size_request (Requisition &requisition)
{
  /// @BUG: account for child's PackInfo like SingleContainerImpl::size_request
  bool chspread = false, cvspread = false;
  if (has_children())
    {
      WidgetImpl &child = get_child();
      if (child.visible())
        {
          Requisition cr = child.requisition();
          requisition.width = left_padding() + cr.width + right_padding();
          requisition.height = bottom_padding() + cr.height + top_padding();
          chspread = child.hspread();
          cvspread = child.vspread();
        }
    }
  set_flag (HSPREAD_CONTAINER, chspread);
  set_flag (VSPREAD_CONTAINER, cvspread);
}

void
AlignmentImpl::size_allocate (Allocation area, bool changed)
{
  /// @BUG: account for child's PackInfo like SingleContainerImpl::size_allocate
  if (!has_visible_child())
    return;
  WidgetImpl &child = get_child();
  Requisition rq = child.requisition();
  /* pad allocation */
  area.x += left_padding();
  area.width -= left_padding() + right_padding();
  area.y += bottom_padding();
  area.height -= bottom_padding() + top_padding();
  /* expand/scale child */
  if (area.width > rq.width && !child.hexpand())
    {
      int width = iround (rq.width + child.hscale() * (area.width - rq.width));
      area.x += iround (child.halign() * (area.width - width));
      area.width = width;
    }
  if (area.height > rq.height && !child.vexpand())
    {
      int height = iround (rq.height + child.vscale() * (area.height - rq.height));
      area.y += iround (child.valign() * (area.height - height));
      area.height = height;
    }
  child.set_allocation (area);
}

static inline int
u16 (int v)
{
  return CLAMP (v, 0, 65535);
}

int
AlignmentImpl::left_padding () const
{
  return left_padding_;
}

void
AlignmentImpl::left_padding (int c)
{
  left_padding_ = u16 (c);
  invalidate();
  changed ("left_padding");
}

int
AlignmentImpl::right_padding () const
{
  return right_padding_;
}

void
AlignmentImpl::right_padding (int c)
{
  right_padding_ = u16 (c);
  invalidate();
  changed ("right_padding");
}

int
AlignmentImpl::bottom_padding () const
{
  return bottom_padding_;
}

void
AlignmentImpl::bottom_padding (int c)
{
  bottom_padding_ = u16 (c);
  invalidate();
  changed ("bottom_padding");
}

int
AlignmentImpl::top_padding () const
{
  return top_padding_;
}

void
AlignmentImpl::top_padding (int c)
{
  top_padding_ = u16 (c);
  invalidate();
  changed ("top_padding");
}

int
AlignmentImpl::padding () const
{
  assert_unreached();
  return 0;
}

void
AlignmentImpl::padding (int c)
{
  const int v = u16 (c);
  left_padding (v);
  right_padding (v);
  bottom_padding (v);
  top_padding (v);
  invalidate();
}

static const WidgetFactory<AlignmentImpl> alignment_factory ("Rapicorn_Factory:Alignment");

// == FillAreaContainerImpl ==
FillAreaContainerImpl::FillAreaContainerImpl ()
{}

FillAreaContainerImpl::~FillAreaContainerImpl ()
{}

void
FillAreaContainerImpl::source (const String &uri)
{
  source_ = uri;
  image_painter_ = ImagePainter (source_);
  invalidate();
  changed ("source");
}

String
FillAreaContainerImpl::source () const
{
  return source_;
}

void
FillAreaContainerImpl::size_request (Requisition &requisition)
{
  bool chspread = false, cvspread = false;
  if (has_visible_child())
    {
      WidgetImpl &child = get_child();
      Requisition requisition;
      size_request_child (requisition, &chspread, &cvspread);
    }
  set_flag (HSPREAD_CONTAINER, chspread);
  set_flag (VSPREAD_CONTAINER, cvspread);
}

void
FillAreaContainerImpl::size_allocate (Allocation area, bool changed)
{
  if (has_visible_child())
    {
      WidgetImpl &child = get_child();
      Allocation child_area;
      if (image_painter_)
        {
          const Requisition image_size = image_painter_.image_size ();
          Rect fill = image_painter_.fill_area();
          assert_return (fill.x + fill.width <= image_size.width);
          assert_return (fill.y + fill.height <= image_size.height);
          Svg::Span spans[3] = { { 0, 0 }, { 0, 1 }, { 0, 0 } };
          // horizontal distribution & allocation
          spans[0].length = fill.x;
          spans[1].length = fill.width;
          spans[2].length = image_size.width - fill.x - fill.width;
          ssize_t dremain = Svg::Span::distribute (ARRAY_SIZE (spans), spans, area.width - image_size.width, 1);
          if (dremain < 0)
            Svg::Span::distribute (ARRAY_SIZE (spans), spans, dremain, 0); // shrink *any* segment
          child_area.x = area.x + spans[0].length;
          child_area.width = spans[1].length;
          // vertical distribution & allocation
          spans[0].length = fill.y;
          spans[1].length = fill.height;
          spans[2].length = image_size.height - fill.y - fill.height;
          dremain = Svg::Span::distribute (ARRAY_SIZE (spans), spans, area.height - image_size.height, 1);
          if (dremain < 0)
            Svg::Span::distribute (ARRAY_SIZE (spans), spans, dremain, 0); // shrink *any* segment
          child_area.y = area.y + spans[0].length;
          child_area.height = spans[1].length;
        }
      else
        child_area = area;
      child_area = layout_child (child, child_area);
      child.set_allocation (child_area);
    }
}

static const WidgetFactory<FillAreaContainerImpl> fill_area_container_factory ("Rapicorn_Factory:FillAreaContainer");


// == HBoxImpl ==
HBoxImpl::HBoxImpl()
{}

HBoxImpl::~HBoxImpl()
{}

void
HBoxImpl::add_child (WidgetImpl &widget)
{
  uint col = n_cols();
  while (col > 0 && !is_col_used (col - 1))
    col--;
  if (is_col_used (col))
    expand_table (col, 1, UINT_MAX, 0); // grow for appending
  widget.hposition (col);
  widget.hspan (1);
  TableLayoutImpl::add_child (widget); // ref, sink, set_parent, insert
}

bool
HBoxImpl::homogeneous () const
{
  return TableLayoutImpl::homogeneous();
}

void
HBoxImpl::homogeneous (bool homogeneous_widgets)
{
  TableLayoutImpl::homogeneous (homogeneous_widgets);
}

int
HBoxImpl::spacing () const
{
  return col_spacing();
}

void
HBoxImpl::spacing (int vspacing)
{
  if (vspacing != col_spacing())
    {
      col_spacing (vspacing);
      changed ("spacing");
    }
}

static const WidgetFactory<HBoxImpl> hbox_factory ("Rapicorn_Factory:HBox");

// == VBoxImpl ==
VBoxImpl::VBoxImpl()
{}

VBoxImpl::~VBoxImpl()
{}

void
VBoxImpl::add_child (WidgetImpl &widget)
{
  uint row = n_rows();
  while (row > 0 && !is_row_used (row - 1))
    row--;
  if (is_row_used (row))
    expand_table (UINT_MAX, 0, row, 1); // grow for appending
  widget.vposition (row);
  widget.vspan (1);
  TableLayoutImpl::add_child (widget); // ref, sink, set_parent, insert
}

bool
VBoxImpl::homogeneous () const
{
  return TableLayoutImpl::homogeneous();
}

void
VBoxImpl::homogeneous (bool homogeneous_widgets)
{
  TableLayoutImpl::homogeneous (homogeneous_widgets);
}

int
VBoxImpl::spacing () const
{
  return row_spacing();
}

void
VBoxImpl::spacing (int vspacing)
{
  if (vspacing != row_spacing())
    {
      row_spacing (vspacing);
      changed ("spacing");
    }
}

static const WidgetFactory<VBoxImpl> vbox_factory ("Rapicorn_Factory:VBox");

} // Rapicorn
