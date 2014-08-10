// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_TABLE_IMPL_HH__
#define __RAPICORN_TABLE_IMPL_HH__

#include <ui/table.hh>
#include <ui/container.hh>
#include <string.h>

namespace Rapicorn {

class TableImpl : public virtual MultiContainerImpl, public virtual Table {
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
  uint                  homogeneous_widgets : 1;
  void                  size_request_init       ();
  void                  size_request_pass1      ();
  void                  size_request_pass2      ();
  void                  size_request_pass3      ();
  void                  size_allocate_init      ();
  void                  size_allocate_pass1     ();
  void                  size_allocate_pass2     ();
  void                  resize_table    (uint n_cols, uint n_rows);
protected:
  virtual               ~TableImpl      ();
  virtual void          size_request    (Requisition &requisition);
  virtual void          size_allocate   (Allocation area, bool changed);
  virtual void          repack_child    (WidgetImpl       &widget,
                                         const PackInfo &orig,
                                         const PackInfo &pnew);
public:
  explicit              TableImpl       ();
  virtual void          resize          (uint n_cols, uint n_rows) { resize (n_cols, n_rows); }
  virtual uint          get_n_rows      ()                              { return rows.size(); }
  virtual uint          get_n_cols      ()                              { return cols.size(); }
  virtual bool          is_row_used     (uint row);
  virtual bool          is_col_used     (uint col);
  virtual void          insert_rows     (uint first_row, uint n_rows);
  virtual void          insert_cols     (uint first_col, uint n_cols);
  virtual bool          homogeneous     () const                        { return homogeneous_widgets; }
  virtual void          homogeneous     (bool chomogeneous_widgets)     { homogeneous_widgets = chomogeneous_widgets; invalidate(); }
  virtual int           col_spacing     () const                        { return default_col_spacing; }
  virtual void          col_spacing     (int cspacing);
  virtual int           row_spacing     () const                        { return default_row_spacing; }
  virtual void          row_spacing     (int rspacing);
};

} // Rapicorn

#endif  /* __RAPICORN_TABLE_IMPL_HH__ */
