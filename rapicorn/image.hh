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
#ifndef __RAPICORN_IMAGE_HH__
#define __RAPICORN_IMAGE_HH__

#include <rapicorn/item.hh>

namespace Rapicorn {

class PixelImage : public virtual ReferenceCountImpl {
public:
  virtual uint          width   () const = 0;
  virtual uint          height  () const = 0;
  virtual const uint32* row     (uint y) const = 0; /* endian dependant ARGB integers */
};

class Image : public virtual Item {
  virtual String        image_file        () const { assert_not_reached(); }
  virtual String        builtin_pixstream () const { assert_not_reached(); }
public:
  typedef enum {
    NONE        = 0,
    UNKNOWN_FORMAT,
    EXCESS_DIMENSIONS,
    READ_FAILED,
    DATA_CORRUPT,
  } ErrorType;
  virtual ErrorType     load_image_file   (const String         &filename) = 0;
  virtual ErrorType     load_pixstream    (const uint8          *gdkp_pixstream) = 0;
  virtual ErrorType     load_pixel_image  (const PixelImage     *pimage) = 0;
  ErrorType             load_pixel_image  (const PixelImage     &pimage) { return load_pixel_image (&pimage); }
  virtual void          image_file        (const String         &filename) = 0;
  virtual void          builtin_pixstream (const String         &builtin_name) = 0;
  static void register_builtin_pixstream  (const char           *builtin_name,
                                           const char           *builtin_pixstream);
  static const PixelImage* pixel_image_from_pixstream (const uint8 *gdkp_pixstream,
                                                       ErrorType   *error_type = NULL);
};

} // Rapicorn

#endif  /* __RAPICORN_IMAGE_HH__ */
