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
#ifndef __RAPICORN_PAINT_CONTAINERS_HH__
#define __RAPICORN_PAINT_CONTAINERS_HH__

#include <rapicorn/utilities.hh>
#include <rapicorn/enumdefs.hh>

namespace Rapicorn {

class Ambience : public virtual Convertible {
public:
  virtual void          insensitive_background  (const String &color) = 0;
  virtual String        insensitive_background  () const = 0;
  virtual void          prelight_background     (const String &color) = 0;
  virtual String        prelight_background     () const = 0;
  virtual void          impressed_background    (const String &color) = 0;
  virtual String        impressed_background    () const = 0;
  virtual void          normal_background       (const String &color) = 0;
  virtual String        normal_background       () const = 0;
  virtual void          insensitive_lighting    (LightingType sh) = 0;
  virtual LightingType  insensitive_lighting    () const = 0;
  virtual void          prelight_lighting       (LightingType sh) = 0;
  virtual LightingType  prelight_lighting       () const = 0;
  virtual void          impressed_lighting      (LightingType sh) = 0;
  virtual LightingType  impressed_lighting      () const = 0;
  virtual void          normal_lighting         (LightingType sh) = 0;
  virtual LightingType  normal_lighting         () const = 0;
  virtual void          insensitive_shade       (LightingType sh) = 0;
  virtual LightingType  insensitive_shade       () const = 0;
  virtual void          prelight_shade          (LightingType sh) = 0;
  virtual LightingType  prelight_shade          () const = 0;
  virtual void          impressed_shade         (LightingType sh) = 0;
  virtual LightingType  impressed_shade         () const = 0;
  virtual void          normal_shade            (LightingType sh) = 0;
  virtual LightingType  normal_shade            () const = 0;
  /* group setters */
  void                  background              (const String &color);
  void                  lighting                (LightingType sh);
  void                  shade                   (LightingType sh);
private:
  virtual String        background              () const { assert_not_reached(); }
  virtual LightingType  lighting                () const { assert_not_reached(); }
  virtual LightingType  shade                   () const { assert_not_reached(); }
};

class Frame : public virtual Convertible {
  FrameType             frame_type      () const        { assert_not_reached(); }
public:
  void                  frame_type      (FrameType ft);
  virtual void          normal_frame    (FrameType ft) = 0;
  virtual FrameType     normal_frame    () const = 0;
  virtual void          impressed_frame (FrameType ft) = 0;
  virtual FrameType     impressed_frame () const = 0;
};

} // Rapicorn

#endif  /* __RAPICORN_PAINT_CONTAINERS_HH__ */
