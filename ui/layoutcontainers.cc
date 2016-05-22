// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "layoutcontainers.hh"
#include "factory.hh"

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
  bool chspread = false, cvspread = false;
  if (has_visible_child())
    requisition = size_request_child (get_child(), &chspread, &cvspread);
  requisition.width += left_padding() + right_padding();
  requisition.height += top_padding() + bottom_padding();
  set_flag (HSPREAD_CONTAINER, chspread);
  set_flag (VSPREAD_CONTAINER, cvspread);
}

void
AlignmentImpl::size_allocate (Allocation area, bool changed)
{
  if (!has_visible_child())
    return;
  WidgetImpl &child = get_child();
  // pad allocation
  area.x += min (left_padding(), area.width);
  area.width -= min (area.width, left_padding() + right_padding());
  area.y += min (top_padding(), area.height);
  area.height -= min (area.height, bottom_padding() + top_padding());
  // expand/scale child
  const Allocation child_area = layout_child (child, area);
  child.set_child_allocation (child_area);
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
  invalidate_size();
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
  invalidate_size();
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
  invalidate_size();
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
  invalidate_size();
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
}

static const WidgetFactory<AlignmentImpl> alignment_factory ("Rapicorn::Alignment");


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

static const WidgetFactory<HBoxImpl> hbox_factory ("Rapicorn::HBox");

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

static const WidgetFactory<VBoxImpl> vbox_factory ("Rapicorn::VBox");

} // Rapicorn
