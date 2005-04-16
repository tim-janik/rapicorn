/* Rapicorn
 * Copyright (C) 2005 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "layoutcontainers.hh"
#include "tableimpl.hh"
#include "factory.hh"

namespace Rapicorn {

class AlignmentImpl : public virtual SingleContainerImpl, public virtual Alignment {
  int    m_width, m_height;
  uint16 m_left_padding, m_right_padding;
  uint16 m_bottom_padding, m_top_padding;
  float m_halign, m_hscale;
  float m_valign, m_vscale;
public:
  AlignmentImpl() :
    m_width (-1), m_height (-1),
    m_left_padding (0), m_right_padding (0),
    m_bottom_padding (0), m_top_padding (0),
    m_halign (0.5), m_hscale (1),
    m_valign (0.5), m_vscale (1)
  {}
  virtual void
  size_request (Requisition &requisition)
  {
    bool chspread = false, cvspread = false;
    if (has_children())
      {
        Item &child = get_child();
        if (child.visible())
          {
            Requisition cr = child.size_request ();
            requisition.width = left_padding() + cr.width + right_padding();
            requisition.height = bottom_padding() + cr.height + top_padding();
            chspread = child.hspread();
            cvspread = child.vspread();
          }
      }
    set_flag (HSPREAD_CONTAINER, chspread);
    set_flag (VSPREAD_CONTAINER, cvspread);
    if (width() >= 0)
      requisition.width = width();
    if (height() >= 0)
      requisition.height = height();
  }
  virtual void
  size_allocate (Allocation area)
  {
    allocation (area);
    if (!has_visible_child())
      return;
    Item &child = get_child();
    Requisition rq = child.size_request();
    /* pad allocation */
    area.x += left_padding();
    area.width -= left_padding() + right_padding();
    area.y += bottom_padding();
    area.height -= bottom_padding() + top_padding();
    /* expand/scale child */
    if (area.width > rq.width && !child.hexpand())
      {
        int width = iround (rq.width + hscale() * (area.width - rq.width));
        area.x += iround (halign() * (area.width - width));
        area.width = width;
      }
    if (area.height > rq.height && !child.vexpand())
      {
        int height = iround (rq.height + vscale() * (area.height - rq.height));
        area.y += iround (valign() * (area.height - height));
        area.height = height;
      }
    child.set_allocation (area);
  }
  virtual int   width          () const  { return m_width; }
  virtual void  width          (int w)   { m_width = MAX (-1, w); invalidate(); }
  virtual int   height         () const  { return m_width; }
  virtual void  height         (int h)   { m_height = MAX (-1, h); invalidate(); }
  virtual float halign         () const  { return m_halign; }
  virtual void  halign         (float f) { m_halign = CLAMP (f, 0, 1); invalidate(); }
  virtual float hscale         () const  { return m_hscale; }
  virtual void  hscale         (float f) { m_hscale = CLAMP (f, 0, 1); invalidate(); }
  virtual float valign         () const  { return m_valign; }
  virtual void  valign         (float f) { m_valign = CLAMP (f, 0, 1); invalidate(); }
  virtual float vscale         () const  { return m_vscale; }
  virtual void  vscale         (float f) { m_vscale = CLAMP (f, 0, 1); invalidate(); }
  virtual uint  left_padding   () const  { return m_left_padding; }
  virtual void  left_padding   (uint c)  { m_left_padding = c; invalidate(); }
  virtual uint  right_padding  () const  { return m_right_padding; }
  virtual void  right_padding  (uint c)  { m_right_padding = c; invalidate(); }
  virtual uint  bottom_padding () const  { return m_bottom_padding; }
  virtual void  bottom_padding (uint c)  { m_bottom_padding = c; invalidate(); }
  virtual uint  top_padding    () const  { return m_top_padding; }
  virtual void  top_padding    (uint c)  { m_top_padding = c; invalidate(); }
protected:
  virtual const PropertyList&
  list_properties()
  {
    static Property *properties[] = {
      MakeProperty (AlignmentImpl, width,          _("Requested Width"), _("The supposed width for the alignment, -1=automatic"), -1, -1, MAXINT, 5, "rw"),
      MakeProperty (AlignmentImpl, height,         _("Requested Height"), _("The supposed height for the alignment, -1=automatic"), -1, -1, MAXINT, 5, "rw"),
      MakeProperty (AlignmentImpl, halign,         _("Horizontal Alignment"), _("Horizontal position of unexpanded child, 0=left, 1=right"), 0, 0, 1, 0.5, "rw"),
      MakeProperty (AlignmentImpl, hscale,         _("Horizontal Scale"),     _("Fractional expansion of unexpanded child, 0=unexpanded, 1=expanded"), 1, 0, 1, 0.5, "rw"),
      MakeProperty (AlignmentImpl, valign,         _("Vertical Alignment"),   _("Vertical position of unexpanded child, 0=bottom, 1=top"), 0, 0, 1, 0.5, "rw"),
      MakeProperty (AlignmentImpl, vscale,         _("Vertical Scale"),       _("Fractional expansion of unexpanded child, 0=unexpanded, 1=expanded"), 1, 0, 1, 0.5, "rw"),
      MakeProperty (AlignmentImpl, left_padding,   _("Left Padding"),   _("Amount of padding to add at the child's left side"), 0, 0, 65535, 3, "rw"),
      MakeProperty (AlignmentImpl, right_padding,  _("Right Padding"),  _("Amount of padding to add at the child's right side"), 0, 0, 65535, 3, "rw"),
      MakeProperty (AlignmentImpl, bottom_padding, _("Bottom Padding"), _("Amount of padding to add at the child's bottom side"), 0, 0, 65535, 3, "rw"),
      MakeProperty (AlignmentImpl, top_padding,    _("Top Padding"),    _("Amount of padding to add at the child's top side"), 0, 0, 65535, 3, "rw"),
    };
    static const PropertyList property_list (properties, SingleContainerImpl::list_properties());
    return property_list;
  }
};
static const ItemFactory<AlignmentImpl> alignment_factory ("Rapicorn::Alignment");

class HBoxImpl : public virtual HBox, public virtual TableImpl {
  /* pack properties */
  class HBoxPacker : public virtual TablePacker {
  public:
    virtual const PropertyList&
    list_properties ()
    {
      static Property *properties[] = {
        MakeProperty (TablePacker, left_padding,   _("Left Padding"),   _("Amount of padding to add at the child's left side"), 0, 0, 65535, 3, "rw"),
        MakeProperty (TablePacker, right_padding,  _("Right Padding"),  _("Amount of padding to add at the child's right side"), 0, 0, 65535, 3, "rw"),
        MakeProperty (TablePacker, bottom_padding, _("Bottom Padding"), _("Amount of padding to add at the child's bottom side"), 0, 0, 65535, 3, "rw"),
        MakeProperty (TablePacker, top_padding,    _("Top Padding"),    _("Amount of padding to add at the child's top side"), 0, 0, 65535, 3, "rw"),
        MakeProperty (TablePacker, hshrink,        _("Horizontal Shrink"), _("Whether the child may be shrunken horizontally"), false, "rw"),
        MakeProperty (TablePacker, vshrink,        _("Vertical Shrink"),   _("Whether the child may be shrunken vertically"), false, "rw"),
        MakeProperty (TablePacker, hfill,          _("Horizontal Fill"),   _("Whether the child may fill all extra horizontal space"), true, "rw"),
        MakeProperty (TablePacker, vfill,          _("Vertical Fill"),     _("Whether the child may fill all extra vertical space"), true, "rw"),
      };
      static const PropertyList property_list (properties);
      return property_list;
    }
    HBoxPacker (Item &citem) : TablePacker (citem) {}
  };
  virtual Packer
  create_packer (Item &item)
  {
    if (item.parent() == this)
      return Packer (new HBoxPacker (item));
    else
      throw Exception ("foreign child: ", item.name());
  }
  virtual bool
  add_child (Item &item, const PackPropertyList &pack_plist)
  {
    if (MultiContainerImpl::add_child (item, pack_plist)) /* ref, sink, set_parent, insert */
      {
        uint col = get_n_cols();
        insert_cols (col, 1);
        while (col > 0 && !is_col_used (col - 1))
          col--;
        Packer packer = create_packer (item);
        TablePacker *tpacker = extract_child_packer<TablePacker*> (packer);
        tpacker->update();
        diag ("hboxcolumn: %d used:%d,%d,%d (%d)\n", col, is_col_used(0),is_col_used(1),is_col_used(2),get_n_cols());
        tpacker->left_attach (col);
        tpacker->right_attach (col + 1);
        tpacker->commit();
        packer.apply_properties (pack_plist);
        return true;
      }
    return false;
  }
protected:
  virtual bool  homogeneous     () const                        { return TableImpl::homogeneous(); }
  virtual void  homogeneous     (bool chomogeneous_items)       { TableImpl::homogeneous (chomogeneous_items); }
  virtual uint  spacing  ()                                     { return column_spacing(); }
  virtual void  spacing  (uint cspacing)                        { column_spacing (cspacing); }
public:
  virtual const PropertyList&
  list_properties()
  {
    static Property *properties[] = {
      MakeProperty (HBox, homogeneous, _("Homogeneous"), _("Whether all children get the same size"), false, "rw"),
      MakeProperty (HBox, spacing,     _("Spacing"),     _("The amount of space between two consecutive columns"), 0, 0, 65535, 10, "rw"),
    };
    static const PropertyList property_list (properties, Container::list_properties());
    return property_list;
  }
  explicit
  HBoxImpl()
  {}
  ~HBoxImpl()
  {}
};

static const ItemFactory<HBoxImpl> hbox_factory ("Rapicorn::HBox");

class VBoxImpl : public virtual VBox, public virtual TableImpl {
  /* pack properties */
  class VBoxPacker : public virtual TablePacker {
  public:
    virtual const PropertyList&
    list_properties ()
    {
      static Property *properties[] = {
        MakeProperty (TablePacker, left_padding,   _("Left Padding"),   _("Amount of padding to add at the child's left side"), 0, 0, 65535, 3, "rw"),
        MakeProperty (TablePacker, right_padding,  _("Right Padding"),  _("Amount of padding to add at the child's right side"), 0, 0, 65535, 3, "rw"),
        MakeProperty (TablePacker, bottom_padding, _("Bottom Padding"), _("Amount of padding to add at the child's bottom side"), 0, 0, 65535, 3, "rw"),
        MakeProperty (TablePacker, top_padding,    _("Top Padding"),    _("Amount of padding to add at the child's top side"), 0, 0, 65535, 3, "rw"),
        MakeProperty (TablePacker, hshrink,        _("Horizontal Shrink"), _("Whether the child may be shrunken horizontally"), false, "rw"),
        MakeProperty (TablePacker, vshrink,        _("Vertical Shrink"),   _("Whether the child may be shrunken vertically"), false, "rw"),
        MakeProperty (TablePacker, hfill,          _("Horizontal Fill"),   _("Whether the child may fill all extra horizontal space"), true, "rw"),
        MakeProperty (TablePacker, vfill,          _("Vertical Fill"),     _("Whether the child may fill all extra vertical space"), true, "rw"),
      };
      static const PropertyList property_list (properties);
      return property_list;
    }
    VBoxPacker (Item &citem) : TablePacker (citem) {}
  };
  virtual Packer
  create_packer (Item &item)
  {
    if (item.parent() == this)
      return Packer (new VBoxPacker (item));
    else
      throw Exception ("foreign child: ", item.name());
  }
  virtual bool
  add_child (Item &item, const PackPropertyList &pack_plist)
  {
    if (MultiContainerImpl::add_child (item, pack_plist)) /* ref, sink, set_parent, insert */
      {
        uint row = 0; // get_n_rows();
        insert_rows (row, 1);
        Packer packer = create_packer (item);
        TablePacker *tpacker = extract_child_packer<TablePacker*> (packer);
        tpacker->update();
        tpacker->bottom_attach (row);
        tpacker->top_attach (row + 1);
        tpacker->commit();
        packer.apply_properties (pack_plist);
        return true;
      }
    return false;
  }
protected:
  virtual bool  homogeneous     () const                        { return TableImpl::homogeneous(); }
  virtual void  homogeneous     (bool chomogeneous_items)       { TableImpl::homogeneous (chomogeneous_items); }
  virtual uint  spacing  ()                                     { return row_spacing(); }
  virtual void  spacing  (uint cspacing)                        { row_spacing (cspacing); }
public:
  virtual const PropertyList&
  list_properties()
  {
    static Property *properties[] = {
      MakeProperty (VBox, homogeneous, _("Homogeneous"), _("Whether all children get the same size"), false, "rw"),
      MakeProperty (VBox, spacing,     _("Spacing"),     _("The amount of space between two consecutive rows"), 0, 0, 65535, 10, "rw"),
    };
    static const PropertyList property_list (properties, Container::list_properties());
    return property_list;
  }
  explicit
  VBoxImpl()
  {}
  ~VBoxImpl()
  {}
};

static const ItemFactory<VBoxImpl> vbox_factory ("Rapicorn::VBox");

} // Rapicorn
