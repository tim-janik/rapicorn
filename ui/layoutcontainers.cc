// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "layoutcontainers.hh"
#include "table.hh"
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

static const WidgetFactory<AlignmentImpl> alignment_factory ("Rapicorn::Factory::Alignment");

// == HBoxImpl ==
const PropertyList&
HBox::__aida_properties__()
{
  static Property *properties[] = {
    MakeProperty (HBox, homogeneous, _("Homogeneous"), _("Whether all children get the same size"), "rw"),
    MakeProperty (HBox, spacing,     _("Spacing"),     _("The amount of space between two consecutive columns"), 0, 65535, 10, "rw"),
  };
  static const PropertyList property_list (properties, ContainerImpl::__aida_properties__());
  return property_list;
}

class HBoxImpl : public virtual TableImpl, public virtual HBox {
  virtual const PropertyList& __aida_properties__() { return HBox::__aida_properties__(); }
  virtual void
  add_child (WidgetImpl &widget)
  {
    uint col = n_cols();
    while (col > 0 && !is_col_used (col - 1))
      col--;
    if (is_col_used (col))
      insert_cols (col, 1);     // should never be triggered
    widget.hposition (col);
    widget.hspan (1);
    TableImpl::add_child (widget); /* ref, sink, set_parent, insert */
  }
protected:
  virtual bool  homogeneous     () const                        { return TableImpl::homogeneous(); }
  virtual void  homogeneous     (bool chomogeneous_widgets)     { TableImpl::homogeneous (chomogeneous_widgets); }
  virtual int   spacing         () const                        { return col_spacing(); }
  virtual void  spacing         (int cspacing)                  { col_spacing (MIN (INT_MAX, cspacing)); }
public:
  explicit
  HBoxImpl()
  {}
  ~HBoxImpl()
  {}
};
static const WidgetFactory<HBoxImpl> hbox_factory ("Rapicorn::Factory::HBox");

// == VBoxImpl ==
const PropertyList&
VBox::__aida_properties__()
{
  static Property *properties[] = {
    MakeProperty (VBox, homogeneous, _("Homogeneous"), _("Whether all children get the same size"), "rw"),
    MakeProperty (VBox, spacing,     _("Spacing"),     _("The amount of space between two consecutive rows"), 0, 65535, 10, "rw"),
  };
  static const PropertyList property_list (properties, ContainerImpl::__aida_properties__());
  return property_list;
}

class VBoxImpl : public virtual TableImpl, public virtual VBox {
  /* pack properties */
  virtual const PropertyList& __aida_properties__() { return VBox::__aida_properties__(); }
  virtual void
  add_child (WidgetImpl &widget)
  {
    uint row = n_rows();
    while (row > 0 && !is_row_used (row - 1))
      row--;
    if (is_row_used (row))
      insert_rows (row, 1);     // should never be triggered
    widget.vposition (row);
    widget.vspan (1);
    TableImpl::add_child (widget); /* ref, sink, set_parent, insert */
  }
protected:
  virtual bool  homogeneous     () const                        { return TableImpl::homogeneous(); }
  virtual void  homogeneous     (bool chomogeneous_widgets)     { TableImpl::homogeneous (chomogeneous_widgets); }
  virtual int   spacing         () const                        { return row_spacing(); }
  virtual void  spacing         (int cspacing)                  { row_spacing (MIN (INT_MAX, cspacing)); }
public:
  explicit
  VBoxImpl()
  {}
  ~VBoxImpl()
  {}
};
static const WidgetFactory<VBoxImpl> vbox_factory ("Rapicorn::Factory::VBox");

} // Rapicorn
