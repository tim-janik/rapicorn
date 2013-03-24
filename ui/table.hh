// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_TABLE_HH__
#define __RAPICORN_TABLE_HH__

#include <ui/container.hh>

namespace Rapicorn {

class Table : public virtual ContainerImpl {
protected:
  virtual const PropertyList&   _property_list ();
public:
  virtual bool  homogeneous     () const = 0;
  virtual void  homogeneous     (bool chomogeneous_widgets) = 0;
  virtual uint  col_spacing     () const = 0;
  virtual void  col_spacing     (uint cspacing) = 0;
  virtual uint  row_spacing     () const = 0;
  virtual void  row_spacing     (uint rspacing) = 0;
  virtual void  resize          (uint n_cols, uint n_rows) = 0;
  virtual uint  get_n_rows      () = 0;
  virtual uint  get_n_cols      () = 0;
  virtual bool  is_row_used     (uint row) = 0;
  virtual bool  is_col_used     (uint col) = 0;
  virtual void  insert_rows     (uint first_row, uint n_rows) = 0;
  virtual void  insert_cols     (uint first_col, uint n_cols) = 0;
};

} // Rapicorn

#endif  /* __RAPICORN_TABLE_HH__ */
