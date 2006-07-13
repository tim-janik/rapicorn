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
#ifndef __RAPICORN_SCROLL_ITEMS_HH__
#define __RAPICORN_SCROLL_ITEMS_HH__

#include <rapicorn/adjustment.hh>
#include <rapicorn/container.hh>

namespace Rapicorn {

/* --- ScrollArea --- */
class ScrollArea : public virtual Container, public virtual AdjustmentSource {
protected:
  explicit              ScrollArea();
public:
  virtual double        xoffset         () const = 0;
  virtual double        yoffset         () const = 0;
  virtual void          scroll_to       (double x,
                                         double y) = 0;
};

} // Rapicorn

#endif  /* __RAPICORN_SCROLL_ITEMS_HH__ */
