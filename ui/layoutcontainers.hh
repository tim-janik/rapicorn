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
#ifndef __RAPICORN_LAYOUT_CONTAINERS_HH__
#define __RAPICORN_LAYOUT_CONTAINERS_HH__

#include <ui/container.hh>

namespace Rapicorn {

class Alignment : public virtual ContainerImpl {
  virtual uint  padding         () const  = 0;
protected:
  virtual const PropertyList&   list_properties ();
public:
  virtual uint  left_padding    () const  = 0;
  virtual void  left_padding    (uint c)  = 0;
  virtual uint  right_padding   () const  = 0;
  virtual void  right_padding   (uint c)  = 0;
  virtual uint  bottom_padding  () const  = 0;
  virtual void  bottom_padding  (uint c)  = 0;
  virtual uint  top_padding     () const  = 0;
  virtual void  top_padding     (uint c)  = 0;
  virtual void  padding         (uint c)  = 0;
};

class HBox : public virtual ContainerImpl {
protected:
  virtual const PropertyList&   list_properties ();
public:
  virtual bool  homogeneous     () const = 0;
  virtual void  homogeneous     (bool chomogeneous_items) = 0;
  virtual uint  spacing         () = 0;
  virtual void  spacing         (uint cspacing) = 0;
};

class VBox : public virtual ContainerImpl {
protected:
  virtual const PropertyList&   list_properties ();
public:
  virtual bool  homogeneous     () const = 0;
  virtual void  homogeneous     (bool chomogeneous_items) = 0;
  virtual uint  spacing         () = 0;
  virtual void  spacing         (uint cspacing) = 0;
};

} // Rapicorn

#endif  /* __RAPICORN_LAYOUT_CONTAINERS_HH__ */
