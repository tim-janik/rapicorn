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
#ifndef __RAPICORN_ARRANGEMENT_HH__
#define __RAPICORN_ARRANGEMENT_HH__

#include <ui/container.hh>

namespace Rapicorn {

class Arrangement : public virtual Container {
protected:
  virtual const PropertyList& list_properties ();
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
