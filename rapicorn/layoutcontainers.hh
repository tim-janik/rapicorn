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
#ifndef __RAPICORN_LAYOUT_CONTAINERS_HH__
#define __RAPICORN_LAYOUT_CONTAINERS_HH__

#include <rapicorn/utilities.hh>

namespace Rapicorn {

class Alignment : public virtual Convertible {
  virtual int   width          () const  = 0;
  virtual void  width          (int w)   = 0;
  virtual int   height         () const  = 0;
  virtual void  height         (int h)   = 0;
  virtual float halign         () const  = 0;
  virtual void  halign         (float f) = 0;
  virtual float hscale         () const  = 0;
  virtual void  hscale         (float f) = 0;
  virtual float valign         () const  = 0;
  virtual void  valign         (float f) = 0;
  virtual float vscale         () const  = 0;
  virtual void  vscale         (float f) = 0;
  virtual uint  left_padding   () const  = 0;
  virtual void  left_padding   (uint c)  = 0;
  virtual uint  right_padding  () const  = 0;
  virtual void  right_padding  (uint c)  = 0;
  virtual uint  bottom_padding () const  = 0;
  virtual void  bottom_padding (uint c)  = 0;
  virtual uint  top_padding    () const  = 0;
  virtual void  top_padding    (uint c)  = 0;
};

class HBox : public virtual Convertible {
public:
  virtual bool  homogeneous     () const = 0;
  virtual void  homogeneous     (bool chomogeneous_items) = 0;
  virtual uint  spacing         () = 0;
  virtual void  spacing         (uint cspacing) = 0;
};

class VBox : public virtual Convertible {
public:
  virtual bool  homogeneous     () const = 0;
  virtual void  homogeneous     (bool chomogeneous_items) = 0;
  virtual uint  spacing         () = 0;
  virtual void  spacing         (uint cspacing) = 0;
};

} // Rapicorn

#endif  /* __RAPICORN_LAYOUT_CONTAINERS_HH__ */
