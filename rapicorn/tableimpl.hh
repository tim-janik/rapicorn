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
#ifndef __RAPICORN_TABLE_IMPL_HH__
#define __RAPICORN_TABLE_IMPL_HH__

#include <rapicorn/table.hh>
#include <rapicorn/containerimpl.hh>

namespace Rapicorn {

class TableImpl : public virtual Table, public virtual MultiContainerImpl {
  struct RowCol {
    uint   requisition;
    uint   allocation;
    uint16 spacing;
    uint   need_expand : 1;
    uint   need_shrink : 1;
    uint   expand : 1;
    uint   shrink : 1;
    uint   empty : 1;
    explicit    RowCol() { memset (this, 0, sizeof (*this)); }
    static bool lesser_allocation (const TableImpl::RowCol *const &v1,
                                   const TableImpl::RowCol *const &v2);
  };
  vector<RowCol>        rows, cols;
  uint16                default_row_spacing;
  uint16                default_column_spacing;
  uint                  homogeneous_items : 1;
  void                  size_request_init       ();
  void                  size_request_pass1      ();
  void                  size_request_pass2      ();
  void                  size_request_pass3      ();
  void                  size_allocate_init      ();
  void                  size_allocate_pass1     ();
  void                  size_allocate_pass2     ();
protected:
  virtual void          size_request    (Requisition &requisition);
  virtual void          size_allocate   (Allocation area);
public:
  explicit              TableImpl       ();
  virtual uint          get_n_rows      ()                              { return rows.size(); }
  virtual uint          get_n_cols      ()                              { return cols.size(); }
  void                  resize_grow     (uint n_cols, uint n_rows);
  virtual void          resize          (uint n_cols, uint n_rows);
  virtual bool          is_row_used     (uint row);
  virtual bool          is_col_used     (uint col);
  virtual void          insert_rows     (uint first_row, uint n_rows);
  virtual void          insert_cols     (uint first_col, uint n_cols);
  virtual               ~TableImpl      ();
public:
  virtual
  const PropertyList&   list_properties ();
  virtual bool          homogeneous     () const                        { return homogeneous_items; }
  virtual void          homogeneous     (bool chomogeneous_items)       { homogeneous_items = chomogeneous_items; invalidate(); }
  virtual uint          column_spacing  ()                              { return default_column_spacing; }
  virtual void          column_spacing  (uint cspacing);
  virtual uint          row_spacing     ()                              { return default_row_spacing; }
  virtual void          row_spacing     (uint rspacing);
protected:
  /* child location */
  struct Location {
    uint   left_attach, right_attach;
    uint   bottom_attach, top_attach;
    uint16 left_padding, right_padding;
    uint16 bottom_padding, top_padding;
    uint   xexpand : 1;
    uint   yexpand : 1;
    uint   xshrink : 1;
    uint   yshrink : 1;
    uint   xfill : 1;
    uint   yfill : 1;
    Location() { memset (this, 0, sizeof (*this)); xfill = yfill = true; }
  };
  static Location       child_location  (Item &child);
  static void           child_location  (Item &child, Location loc);
  static DataKey<Location> child_location_key;
  /* pack properties */
  class TablePacker : public virtual ChildPacker {
    Item     &item;
    Location loc;
  public:
    explicit            TablePacker     (Item &citem);
    virtual             ~TablePacker    ();
    virtual
    const PropertyList& list_properties ();
    virtual void        update          (); /* fetch real pack properties */
    virtual void        commit          (); /* assign pack properties */
    uint                left_attach     () const { return loc.left_attach; }
    void 		left_attach     (uint c) { loc.left_attach = c; loc.right_attach = MAX (loc.left_attach + 1, loc.right_attach); }
    uint 		right_attach    () const { return loc.right_attach; }
    void 		right_attach    (uint c) { loc.right_attach = c; loc.left_attach = MIN (loc.left_attach, loc.right_attach - 1); }
    uint 		bottom_attach   () const { return loc.bottom_attach; }
    void 		bottom_attach   (uint c) { loc.bottom_attach = c; loc.top_attach = MAX (loc.top_attach, loc.bottom_attach + 1); }
    uint 		top_attach      () const { return loc.top_attach; }
    void 		top_attach      (uint c) { loc.top_attach = c; loc.bottom_attach = MIN (loc.bottom_attach, loc.top_attach - 1); }
    uint 		left_padding    () const { return loc.left_padding; }
    void 		left_padding    (uint c) { loc.left_padding = c; }
    uint 		right_padding   () const { return loc.right_padding; }
    void 		right_padding   (uint c) { loc.right_padding = c; }
    uint 		bottom_padding  () const { return loc.bottom_padding; }
    void 		bottom_padding  (uint c) { loc.bottom_padding = c; }
    uint 		top_padding     () const { return loc.top_padding; }
    void 		top_padding     (uint c) { loc.top_padding = c; }
    bool 		hshrink         () const { return loc.xshrink; }
    void 		hshrink         (bool b) { loc.xshrink = b; }
    bool 		vshrink         () const { return loc.yshrink; }
    void 		vshrink         (bool b) { loc.yshrink = b; }
    bool 		hfill           () const { return loc.xfill; }
    void 		hfill           (bool b) { loc.xfill = b; }
    bool 		vfill           () const { return loc.yfill; }
    void 		vfill           (bool b) { loc.yfill = b; }
  };
  virtual Packer        create_packer   (Item &item);
};

} // Rapicorn

#endif  /* __RAPICORN_TABLE_IMPL_HH__ */
