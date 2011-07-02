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
#ifndef __RAPICORN_IMAGE_HH__
#define __RAPICORN_IMAGE_HH__

#include <ui/item.hh>

namespace Rapicorn {

class Image : public virtual ItemImpl {
  virtual String        image_file      () const { RAPICORN_ASSERT_NOT_REACHED(); }
  virtual String        stock_pixmap    () const { RAPICORN_ASSERT_NOT_REACHED(); }
protected:
  const PropertyList&   list_properties ();
public:
  virtual void             pixmap       (Pixmap       *pixmap) = 0;
  virtual Pixmap*          pixmap       (void) = 0;
  virtual void /*errno*/   stock_pixmap (const String &stock_name) = 0;
  virtual void /*errno*/   image_file   (const String &filename) = 0;
};

} // Rapicorn

#endif  /* __RAPICORN_IMAGE_HH__ */
