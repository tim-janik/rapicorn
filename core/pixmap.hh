/* Rapicorn - experimental UI toolkit
 * Copyright (C) 2008 Tim Janik
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
#ifndef __RAPICORN_PIXMAP_HH__
#define __RAPICORN_PIXMAP_HH__

#include <core/rapicornutils.hh>

namespace Rapicorn {

class Pixbuf {
  uint32         *m_pixels;
protected:
  const int       m_rowstride;
  const int       m_width, m_height;
  virtual        ~Pixbuf    ();
  explicit        Pixbuf    (uint _width, uint _height, int alignment = -1);
public:
  int             width     () const { return m_width; }
  int             height    () const { return m_height; }
  const uint32*   row       (uint y) const; /* endian dependant ARGB integers */
  bool            compare   (const Pixbuf &source,
                             uint sx, uint sy, int swidth, int sheight,
                             uint tx, uint ty,
                             double *averrp = NULL, double *maxerrp = NULL,
                             double *nerrp = NULL, double *npixp = NULL) const;
  static bool     try_alloc (uint width, uint height, int alignment = -1);
  static bool     save_png  (const String &filename,    /* assigns errno */
                             const Pixbuf &pixbuf,
                             const String &comment);
};

class Pixmap : public Pixbuf, public virtual ReferenceCountImpl {
  String          m_comment;
protected:
  virtual        ~Pixmap    ();
public:
  explicit        Pixmap    (uint _width, uint _height, int alignment = -1);
  String          comment   () const    { return m_comment; }
  void            comment   (const String &_comment);
  uint32*         row       (uint y)    { return const_cast<uint32*> (Pixbuf::row (y)); }
  using           Pixbuf::row;
  bool            save_png  (const String &filename);   /* assigns errno */
  void            copy      (const Pixmap &source,
                             uint sx, uint sy, int swidth, int sheight,
                             uint tx, uint ty);
  static Pixmap*  load_png  (const String &filename,    /* assigns errno */
                             bool          tryrepair = false);
  static Pixmap*  pixstream (const uint8  *pixstream);  /* assigns errno */
  static Pixmap*  stock     (const String &stock_name);
  static void     add_stock (const String &stock_name,
                             const uint8  *pixstream);
};

} // Rapicorn

#endif /* __RAPICORN_PIXMAP_HH__ */
