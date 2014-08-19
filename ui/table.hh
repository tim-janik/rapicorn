// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_TABLE_HH__
#define __RAPICORN_TABLE_HH__

#include <ui/container.hh>

namespace Rapicorn {

class TableImpl : public virtual MultiContainerImpl, public virtual TableIface {
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
  vector<RowCol>        rows_, cols_;
  uint16                default_row_spacing_;
  uint16                default_col_spacing_;
  uint                  homogeneous_widgets_ : 1;
  void                  size_request_init   ();
  void                  size_request_pass1  ();
  void                  size_request_pass2  ();
  void                  size_request_pass3  ();
  void                  size_allocate_init  ();
  void                  size_allocate_pass1 ();
  void                  size_allocate_pass2 ();
  void                  resize_table        (uint n_cols, uint n_rows);
protected:
  virtual               ~TableImpl      ();
  virtual void          size_request    (Requisition &requisition);
  virtual void          size_allocate   (Allocation area, bool changed);
  virtual void          repack_child    (WidgetImpl &widget, const PackInfo &orig, const PackInfo &pnew);
public:
  explicit     TableImpl       ();
  virtual bool homogeneous () const override                    { return homogeneous_widgets_; }
  virtual void homogeneous (bool) override;
  virtual int  col_spacing () const override                    { return default_col_spacing_; }
  virtual void col_spacing (int) override;
  virtual int  row_spacing () const override                    { return default_row_spacing_; }
  virtual void row_spacing (int) override;
  virtual int  n_cols      () const override                    { return cols_.size(); }
  virtual void n_cols      (int nc) override                    { resize_table (nc, rows_.size()); }
  virtual int  n_rows      () const override                    { return rows_.size(); }
  virtual void n_rows      (int nr) override                    { resize_table (cols_.size(), nr); }
  virtual bool is_col_used (int col) override;
  virtual bool is_row_used (int row) override;
  virtual void resize      (int n_cols, int n_rows) override    { resize_table (n_cols, n_rows); }
  virtual void insert_cols (int first_col, int n_cols) override;
  virtual void insert_rows (int first_row, int n_rows) override;
};

} // Rapicorn

#endif  /* __RAPICORN_TABLE_HH__ */
