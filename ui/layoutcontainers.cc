/* Rapicorn
 * Copyright (C) 2005 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "layoutcontainers.hh"
#include "tableimpl.hh"
#include "factory.hh"

namespace Rapicorn {

const PropertyList&
Alignment::list_properties()
{
  static Property *properties[] = {
    MakeProperty (Alignment, left_padding,   _("Left Padding"),   _("Amount of padding to add at the child's left side"), 0, 65535, 3, "rw"),
    MakeProperty (Alignment, right_padding,  _("Right Padding"),  _("Amount of padding to add at the child's right side"), 0, 65535, 3, "rw"),
    MakeProperty (Alignment, bottom_padding, _("Bottom Padding"), _("Amount of padding to add at the child's bottom side"), 0, 65535, 3, "rw"),
    MakeProperty (Alignment, top_padding,    _("Top Padding"),    _("Amount of padding to add at the child's top side"), 0, 65535, 3, "rw"),
    MakeProperty (Alignment, padding,        _("Padding"),        _("Amount of padding to add at the child's sides"), 0, 65535, 3, "w"),
  };
  static const PropertyList property_list (properties, ContainerImpl::list_properties());
  return property_list;
}

class AlignmentImpl : public virtual SingleContainerImpl, public virtual Alignment {
  uint16 m_left_padding, m_right_padding;
  uint16 m_bottom_padding, m_top_padding;
public:
  AlignmentImpl() :
    m_left_padding (0), m_right_padding (0),
    m_bottom_padding (0), m_top_padding (0)
  {}
  virtual void
  size_request (Requisition &requisition)
  {
    // FIXME: account for child's PackInfo like SingleContainerImpl::size_request
    bool chspread = false, cvspread = false;
    if (has_children())
      {
        ItemImpl &child = get_child();
        if (child.allocatable())
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
    if (!has_allocatable_child())
      return;
    ItemImpl &child = get_child();
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
  virtual uint  left_padding   () const  { return m_left_padding; }
  virtual void  left_padding   (uint c)  { m_left_padding = c; invalidate(); }
  virtual uint  right_padding  () const  { return m_right_padding; }
  virtual void  right_padding  (uint c)  { m_right_padding = c; invalidate(); }
  virtual uint  bottom_padding () const  { return m_bottom_padding; }
  virtual void  bottom_padding (uint c)  { m_bottom_padding = c; invalidate(); }
  virtual uint  top_padding    () const  { return m_top_padding; }
  virtual void  top_padding    (uint c)  { m_top_padding = c; invalidate(); }
  virtual uint  padding        () const  { assert_unreached(); return 0; }
  virtual void  padding        (uint c)
  {
    m_left_padding = m_right_padding = m_bottom_padding = m_top_padding = c;
    invalidate();
  }
};
static const ItemFactory<AlignmentImpl> alignment_factory ("Rapicorn::Factory::Alignment");

const PropertyList&
HBox::list_properties()
{
  static Property *properties[] = {
    MakeProperty (HBox, homogeneous, _("Homogeneous"), _("Whether all children get the same size"), "rw"),
    MakeProperty (HBox, spacing,     _("Spacing"),     _("The amount of space between two consecutive columns"), 0, 65535, 10, "rw"),
  };
  static const PropertyList property_list (properties, ContainerImpl::list_properties());
  return property_list;
}

class HBoxImpl : public virtual TableImpl, public virtual HBox {
  virtual const PropertyList& list_properties() { return HBox::list_properties(); }
  virtual void
  add_child (ItemImpl &item)
  {
    uint col = get_n_cols();
    while (col > 0 && !is_col_used (col - 1))
      col--;
    if (is_col_used (col))
      insert_cols (col, 1);     // should never be triggered
    item.hposition (col);
    item.hspan (1);
    TableImpl::add_child (item); /* ref, sink, set_parent, insert */
  }
protected:
  virtual bool  homogeneous     () const                        { return TableImpl::homogeneous(); }
  virtual void  homogeneous     (bool chomogeneous_items)       { TableImpl::homogeneous (chomogeneous_items); }
  virtual uint  spacing  ()                                     { return col_spacing(); }
  virtual void  spacing  (uint cspacing)                        { col_spacing (cspacing); }
public:
  explicit
  HBoxImpl()
  {}
  ~HBoxImpl()
  {}
};
static const ItemFactory<HBoxImpl> hbox_factory ("Rapicorn::Factory::HBox");

const PropertyList&
VBox::list_properties()
{
  static Property *properties[] = {
    MakeProperty (VBox, homogeneous, _("Homogeneous"), _("Whether all children get the same size"), "rw"),
    MakeProperty (VBox, spacing,     _("Spacing"),     _("The amount of space between two consecutive rows"), 0, 65535, 10, "rw"),
  };
  static const PropertyList property_list (properties, ContainerImpl::list_properties());
  return property_list;
}

class VBoxImpl : public virtual TableImpl, public virtual VBox {
  /* pack properties */
  virtual const PropertyList& list_properties() { return VBox::list_properties(); }
  virtual void
  add_child (ItemImpl &item)
  {
    uint row = get_n_rows();
    while (row > 0 && !is_row_used (row - 1))
      row--;
    if (is_row_used (row))
      insert_rows (row, 1);     // should never be triggered
    item.vposition (row);
    item.vspan (1);
    TableImpl::add_child (item); /* ref, sink, set_parent, insert */
  }
protected:
  virtual bool  homogeneous     () const                        { return TableImpl::homogeneous(); }
  virtual void  homogeneous     (bool chomogeneous_items)       { TableImpl::homogeneous (chomogeneous_items); }
  virtual uint  spacing  ()                                     { return row_spacing(); }
  virtual void  spacing  (uint cspacing)                        { row_spacing (cspacing); }
public:
  explicit
  VBoxImpl()
  {}
  ~VBoxImpl()
  {}
};
static const ItemFactory<VBoxImpl> vbox_factory ("Rapicorn::Factory::VBox");

} // Rapicorn
