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
#ifndef __RAPICORN_TABLE_HH__
#define __RAPICORN_TABLE_HH__

#include <rapicorn/container.hh>

namespace Rapicorn {

class Table : public virtual Container {
protected:
  virtual const PropertyList&   list_properties ();
public:
  virtual bool  homogeneous     () const = 0;
  virtual void  homogeneous     (bool chomogeneous_items) = 0;
  virtual uint  column_spacing  () = 0;
  virtual void  column_spacing  (uint cspacing) = 0;
  virtual uint  row_spacing     () = 0;
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
