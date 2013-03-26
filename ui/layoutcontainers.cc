// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "layoutcontainers.hh"
#include "tableimpl.hh"
#include "factory.hh"

namespace Rapicorn {

const PropertyList&
Alignment::__aida_properties__()
{
  static Property *properties[] = {
    MakeProperty (Alignment, left_padding,   _("Left Padding"),   _("Amount of padding to add at the child's left side"), 0, 65535, 3, "rw"),
    MakeProperty (Alignment, right_padding,  _("Right Padding"),  _("Amount of padding to add at the child's right side"), 0, 65535, 3, "rw"),
    MakeProperty (Alignment, bottom_padding, _("Bottom Padding"), _("Amount of padding to add at the child's bottom side"), 0, 65535, 3, "rw"),
    MakeProperty (Alignment, top_padding,    _("Top Padding"),    _("Amount of padding to add at the child's top side"), 0, 65535, 3, "rw"),
    MakeProperty (Alignment, padding,        _("Padding"),        _("Amount of padding to add at the child's sides"), 0, 65535, 3, "w"),
  };
  static const PropertyList property_list (properties, ContainerImpl::__aida_properties__());
  return property_list;
}

class AlignmentImpl : public virtual SingleContainerImpl, public virtual Alignment {
  uint16 left_padding_, right_padding_;
  uint16 bottom_padding_, top_padding_;
public:
  AlignmentImpl() :
    left_padding_ (0), right_padding_ (0),
    bottom_padding_ (0), top_padding_ (0)
  {}
  virtual void
  size_request (Requisition &requisition)
  {
    // FIXME: account for child's PackInfo like SingleContainerImpl::size_request
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
  virtual void
  size_allocate (Allocation area, bool changed)
  {
    // FIXME: account for child's PackInfo like SingleContainerImpl::size_allocate
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
  virtual uint  left_padding   () const  { return left_padding_; }
  virtual void  left_padding   (uint c)  { left_padding_ = c; invalidate(); }
  virtual uint  right_padding  () const  { return right_padding_; }
  virtual void  right_padding  (uint c)  { right_padding_ = c; invalidate(); }
  virtual uint  bottom_padding () const  { return bottom_padding_; }
  virtual void  bottom_padding (uint c)  { bottom_padding_ = c; invalidate(); }
  virtual uint  top_padding    () const  { return top_padding_; }
  virtual void  top_padding    (uint c)  { top_padding_ = c; invalidate(); }
  virtual uint  padding        () const  { assert_unreached(); return 0; }
  virtual void  padding        (uint c)
  {
    left_padding_ = right_padding_ = bottom_padding_ = top_padding_ = c;
    invalidate();
  }
};
static const WidgetFactory<AlignmentImpl> alignment_factory ("Rapicorn::Factory::Alignment");

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
    uint col = get_n_cols();
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
  virtual void  homogeneous     (bool chomogeneous_widgets)       { TableImpl::homogeneous (chomogeneous_widgets); }
  virtual uint  spacing  () const                               { return col_spacing(); }
  virtual void  spacing  (uint cspacing)                        { col_spacing (cspacing); }
public:
  explicit
  HBoxImpl()
  {}
  ~HBoxImpl()
  {}
};
static const WidgetFactory<HBoxImpl> hbox_factory ("Rapicorn::Factory::HBox");

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
    uint row = get_n_rows();
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
  virtual void  homogeneous     (bool chomogeneous_widgets)       { TableImpl::homogeneous (chomogeneous_widgets); }
  virtual uint  spacing  () const                               { return row_spacing(); }
  virtual void  spacing  (uint cspacing)                        { row_spacing (cspacing); }
public:
  explicit
  VBoxImpl()
  {}
  ~VBoxImpl()
  {}
};
static const WidgetFactory<VBoxImpl> vbox_factory ("Rapicorn::Factory::VBox");

} // Rapicorn
