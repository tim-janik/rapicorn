/* Rapicorn
 * Copyright (C) 2008 Tim Janik
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
#ifndef __RAPICORN_LIST_AREA_HH__
#define __RAPICORN_LIST_AREA_HH__

#include <rapicorn/container.hh>

namespace Rapicorn {

class ItemList : public virtual Container {
protected:
  virtual const PropertyList&   list_properties ();
public:
  virtual bool   browse       () const  = 0;
  virtual void   browse       (bool b) = 0;
  virtual void   model        (const String &modelurl) = 0;
  virtual String model        () const = 0;
};

} // Rapicorn

#endif  /* __RAPICORN_LIST_AREA_HH__ */
