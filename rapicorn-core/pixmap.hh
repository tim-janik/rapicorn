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

#include <rapicorn-core/rapicornutils.hh>

namespace Rapicorn {

class Pixbuf {
  uint32         *m_pixels;
  int             m_rowstride;
protected:
  const int       m_width, m_height;
  virtual        ~Pixbuf    ();
  explicit        Pixbuf    (uint _width, uint _height);
public:
  int             width     () const { return m_width; }
  int             height    () const { return m_height; }
  const uint32*   row       (uint y) const; /* endian dependant ARGB integers */
  bool            compare   (const Pixbuf &other,
                             uint x, uint y, int width, int height,
                             uint tx, uint ty,
                             double *averrp = NULL, double *maxerrp = NULL,
                             double *nerrp = NULL, double *npixp = NULL) const;
  static bool     try_alloc (uint width, uint height);
};

class Pixmap : public Pixbuf, public virtual ReferenceCountImpl {
  String          m_comment;
protected:
  virtual        ~Pixmap    ();
public:
  explicit        Pixmap    (uint _width, uint _height);
  String          comment   () const    { return m_comment; }
  void            comment   (const String &_comment);
  uint32*         row       (uint y)    { return const_cast<uint32*> (Pixbuf::row (y)); }
  bool            save_png  (const String &filename);   /* assigns errno */
  static Pixmap*  load_png  (const String &filename,    /* assigns errno */
                             bool          tryrepair = false);
};

} // Rapicorn

#endif /* __RAPICORN_PIXMAP_HH__ */
