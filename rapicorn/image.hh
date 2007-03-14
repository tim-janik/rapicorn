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

#include <rapicorn/item.hh>

namespace Rapicorn {

class PixelImage : public virtual ReferenceCountImpl {
public:
  virtual int           width   () const = 0;
  virtual int           height  () const = 0;
  virtual const uint32* row     (uint y) const = 0; /* endian dependant ARGB integers */
};

class Image : public virtual Item {
  virtual String        image_file        () const { BIRNET_ASSERT_NOT_REACHED(); }
  virtual String        builtin_pixstream () const { BIRNET_ASSERT_NOT_REACHED(); }
public:
  typedef enum {
    NONE        = 0,
    UNKNOWN_FORMAT,
    READ_FAILED,        /* file io problems */
    DATA_CORRUPT,       /* image (partially) corrupted */
    EXCESS_DIMENSIONS,  /* image too large or out of memory */
  } ErrorType;
  virtual ErrorType        load_image_file              (const String         &filename) = 0;
  virtual ErrorType        load_pixstream               (const uint8          *gdkp_pixstream) = 0;
  virtual ErrorType        load_pixel_image             (const PixelImage     *pimage) = 0;
  ErrorType                load_pixel_image             (const PixelImage     &pimage) { return load_pixel_image (&pimage); }
  virtual void             image_file                   (const String         &filename) = 0;
  virtual void             builtin_pixstream            (const String         &builtin_name) = 0;
  static const uint8*      lookup_builtin_pixstream     (const String         &builtin_name);
  static void              register_builtin_pixstream   (const String         &builtin_name,
                                                         const uint8   * const static_const_pixstream);
  static const PixelImage* pixel_image_from_pixstream   (const uint8          *gdkp_pixstream,
                                                         ErrorType            *error_type = NULL);
};

} // Rapicorn

#endif  /* __RAPICORN_IMAGE_HH__ */
