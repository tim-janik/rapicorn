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
