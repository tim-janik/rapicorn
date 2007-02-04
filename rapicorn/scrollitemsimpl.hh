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
#ifndef __RAPICORN_SCROLL_ITEMS_IMPL_HH__
#define __RAPICORN_SCROLL_ITEMS_IMPL_HH__

#include <rapicorn/scrollitems.hh>
#include <rapicorn/containerimpl.hh>

namespace Rapicorn {

class ScrollAreaImpl : public virtual ScrollArea, public virtual SingleContainerImpl {
  mutable Adjustment   *m_hadjustment, *m_vadjustment;
  Adjustment&           hadjustment() const;
  Adjustment&           vadjustment() const;
public:
  explicit              ScrollAreaImpl  ();
  virtual Adjustment*   get_adjustment  (AdjustmentSourceType adj_source,
                                         const String        &name = "");
  virtual double        xoffset         () const;
  virtual double        yoffset         () const;
  virtual void          scroll_to       (double x,
                                         double y);
};

} // Rapicorn

#endif  /* __RAPICORN_SCROLL_ITEMS_IMPL_HH__ */
