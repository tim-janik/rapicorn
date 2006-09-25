/* Rapicorn
 * Copyright (C) 2006 Tim Janik
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
#ifndef __RAPICORN_PAINT_GADGETS_HH__
#define __RAPICORN_PAINT_GADGETS_HH__

#include <rapicorn/item.hh>

namespace Rapicorn {

class Arrow : public virtual Item {
public:
  virtual void           arrow_dir      (DirType dir) = 0;
  virtual DirType        arrow_dir      () const = 0;
  virtual void           size_policy    (SizePolicyType spol) = 0;
  virtual SizePolicyType size_policy    () const = 0;
};

class DotGrid : public virtual Item {
  FrameType             dot_type        () const { BIRNET_ASSERT_NOT_REACHED(); }
public:
  void                  dot_type            (FrameType ft);
  virtual void          normal_dot          (FrameType ft) = 0;
  virtual FrameType     normal_dot          () const = 0;
  virtual void          impressed_dot       (FrameType ft) = 0;
  virtual FrameType     impressed_dot       () const = 0;
  virtual void          n_hdots             (uint   num) = 0;
  virtual uint          n_hdots             () const = 0;
  virtual void          n_vdots             (uint   num) = 0;
  virtual uint          n_vdots             () const = 0;
  virtual uint          right_padding_dots  () const  = 0;
  virtual void          right_padding_dots  (uint c)  = 0;
  virtual uint          top_padding_dots    () const  = 0;
  virtual void          top_padding_dots    (uint c)  = 0;
  virtual uint          left_padding_dots   () const  = 0;
  virtual void          left_padding_dots   (uint c)  = 0;
  virtual uint          bottom_padding_dots () const  = 0;
  virtual void          bottom_padding_dots (uint c)  = 0;
};

class Drawable : public virtual Item {
protected:
  explicit      Drawable();
  virtual void  draw    (Display        &display) = 0;
public:
  Signal<Drawable, void (Display &display)> sig_draw;
};

} // Rapicorn

#endif  /* __RAPICORN_PAINT_GADGETS_HH__ */
