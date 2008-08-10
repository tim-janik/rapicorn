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
  uint16                default_col_spacing;
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
  virtual void          repack_child    (Item           &item,
                                         const PackInfo &orig,
                                         const PackInfo &pnew);
public:
  explicit              TableImpl       ();
  virtual uint          get_n_rows      ()                              { return rows.size(); }
  virtual uint          get_n_cols      ()                              { return cols.size(); }
  void                  resize          (uint n_cols, uint n_rows, bool force);
  virtual void          resize          (uint n_cols, uint n_rows) { resize (n_cols, n_rows, false); }
  virtual bool          is_row_used     (uint row);
  virtual bool          is_col_used     (uint col);
  virtual void          insert_rows     (uint first_row, uint n_rows);
  virtual void          insert_cols     (uint first_col, uint n_cols);
  virtual               ~TableImpl      ();
public:
  virtual bool          homogeneous     () const                        { return homogeneous_items; }
  virtual void          homogeneous     (bool chomogeneous_items)       { homogeneous_items = chomogeneous_items; invalidate(); }
  virtual uint          col_spacing     ()                              { return default_col_spacing; }
  virtual void          col_spacing     (uint cspacing);
  virtual uint          row_spacing     ()                              { return default_row_spacing; }
  virtual void          row_spacing     (uint rspacing);
};

} // Rapicorn

#endif  /* __RAPICORN_TABLE_IMPL_HH__ */
