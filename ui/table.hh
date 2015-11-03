// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_TABLE_HH__
#define __RAPICORN_TABLE_HH__

#include <ui/container.hh>

namespace Rapicorn {

class TableLayoutImpl : public virtual MultiContainerImpl {
  uint16         default_col_spacing_, default_row_spacing_;
  uint           homogeneous_widgets_ : 1;
protected:
  explicit       TableLayoutImpl     ();
  virtual       ~TableLayoutImpl     ();
  virtual void   size_request        (Requisition &requisition);
  virtual void   size_allocate       (Allocation area, bool changed);
  virtual void   repack_child        (WidgetImpl &widget, const PackInfo &orig, const PackInfo &pnew);
  void           size_request_init   ();
  void           size_request_pass1  ();
  void           size_request_pass2  ();
  void           size_request_pass3  ();
  void           size_allocate_init  ();
  void           size_allocate_pass1 ();
  void           size_allocate_pass2 ();
  void           resize_table        (uint n_cols, uint n_rows);
  void           expand_table        (uint first_col, uint n_cols, uint first_row, uint n_rows);
  bool           is_col_used         (int col);
  bool           is_row_used         (int row);
  int            n_cols              () const           { return cols_.size(); }
  int            n_rows              () const           { return rows_.size(); }
  bool           homogeneous         () const           { return homogeneous_widgets_; }
  void           homogeneous         (bool h);
  uint16         col_spacing         () const           { return default_col_spacing_; }
  void           col_spacing         (uint16 s);
  uint16         row_spacing         () const           { return default_row_spacing_; }
  void           row_spacing         (uint16 s);
  struct RowCol {
    uint   requisition;
    uint   allocation;
    uint16 spacing;
    uint   need_expand : 1;
    uint   need_shrink : 1;
    uint   expand : 1;
    uint   shrink : 1;
    uint   empty : 1;
    explicit    RowCol            ()                                 { memset (this, 0, sizeof (*this)); }
    static bool lesser_allocation (const RowCol *a, const RowCol *b) { return a->allocation < b->allocation; }
  };
private:
  vector<RowCol> rows_, cols_;
};

class TableImpl : public virtual TableLayoutImpl, public virtual TableIface {
public:
  virtual bool homogeneous () const override                    { return TableLayoutImpl::homogeneous(); }
  virtual void homogeneous (bool) override;
  virtual int  col_spacing () const override                    { return TableLayoutImpl::col_spacing(); }
  virtual void col_spacing (int) override;
  virtual int  row_spacing () const override                    { return TableLayoutImpl::row_spacing(); }
  virtual void row_spacing (int) override;
  virtual int  n_cols      () const override                    { return TableLayoutImpl::n_cols(); }
  virtual void n_cols      (int nc) override                    { resize (nc, n_rows()); }
  virtual int  n_rows      () const override                    { return TableLayoutImpl::n_rows(); }
  virtual void n_rows      (int nr) override                    { resize (n_cols(), nr); }
  virtual bool is_col_used (int col) override                   { return TableLayoutImpl::is_col_used (col); }
  virtual bool is_row_used (int row) override                   { return TableLayoutImpl::is_row_used (row); }
  virtual void resize      (int ncols, int nrows) override;
  virtual void insert_cols (int first_col, int ncols) override;
  virtual void insert_rows (int first_row, int nrows) override;
};

} // Rapicorn

#endif  /* __RAPICORN_TABLE_HH__ */
