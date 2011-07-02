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
#ifndef __RAPICORN_SCROLL_ITEMS_HH__
#define __RAPICORN_SCROLL_ITEMS_HH__

#include <ui/adjustment.hh>
#include <ui/container.hh>

namespace Rapicorn {

/* --- ScrollArea --- */
class ScrollArea : public virtual ContainerImpl, public virtual AdjustmentSource {
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
