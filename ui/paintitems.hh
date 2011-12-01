/* Rapicorn
 * Copyright (C) 2006 Tim Janik
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
#ifndef __RAPICORN_PAINT_ITEMS_HH__
#define __RAPICORN_PAINT_ITEMS_HH__

#include <ui/item.hh>

namespace Rapicorn {

class Arrow : public virtual ItemImpl {
protected:
  virtual
  const PropertyList&    list_properties();
public:
  virtual void           arrow_dir      (DirType dir) = 0;
  virtual DirType        arrow_dir      () const = 0;
  virtual void           size_policy    (SizePolicyType spol) = 0;
  virtual SizePolicyType size_policy    () const = 0;
};

class DotGrid : public virtual ItemImpl {
  FrameType             dot_type        () const { RAPICORN_ASSERT_NOT_REACHED(); }
protected:
  virtual
  const PropertyList&   list_properties     ();
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

class DrawableImpl : public virtual ItemImpl, public virtual DrawableIface {
  PixelRectImpl m_pic;
protected:
  virtual void  draw_rect       (const PixelRectImpl &pixrect);
  virtual void  size_request    (Requisition &requisition);
  virtual void  size_allocate   (Allocation area, bool changed);
  virtual void  render          (RenderContext &rcontext, const Rect &rect);
public:
  explicit      DrawableImpl    ();
};

} // Rapicorn

#endif  /* __RAPICORN_PAINT_ITEMS_HH__ */
