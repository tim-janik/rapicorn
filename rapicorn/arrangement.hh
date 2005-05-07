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
#ifndef __RAPICORN_ARRANGEMENT_HH__
#define __RAPICORN_ARRANGEMENT_HH__

#include <rapicorn/item.hh>

namespace Rapicorn {

class Arrangement : public virtual Convertible {
public:
  virtual Point origin          () = 0;
  virtual void  origin          (Point p) = 0;
  virtual float origin_hanchor  () = 0;
  virtual void  origin_hanchor  (float align) = 0;
  virtual float origin_vanchor  () = 0;
  virtual void  origin_vanchor  (float align) = 0;
  virtual Rect  child_area      () = 0;
};

} // Rapicorn

#endif  /* __RAPICORN_ARRANGEMENT_HH__ */
