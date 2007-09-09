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
#ifndef __RAPICORN_PAINT_CONTAINERS_HH__
#define __RAPICORN_PAINT_CONTAINERS_HH__

#include <rapicorn/utilities.hh>
#include <rapicorn/item.hh>

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
  virtual String        background              () const { RAPICORN_ASSERT_NOT_REACHED(); }
  virtual LightingType  lighting                () const { RAPICORN_ASSERT_NOT_REACHED(); }
  virtual LightingType  shade                   () const { RAPICORN_ASSERT_NOT_REACHED(); }
};

class Frame : public virtual Convertible {
  FrameType             frame_type      () const        { RAPICORN_ASSERT_NOT_REACHED(); }
public:
  void                  frame_type      (FrameType ft);
  virtual FrameType     normal_frame    () const = 0;
  virtual void          normal_frame    (FrameType ft) = 0;
  virtual FrameType     impressed_frame () const = 0;
  virtual void          impressed_frame (FrameType ft) = 0;
  virtual bool          overlap_child   () const = 0;
  virtual void          overlap_child   (bool ovc) = 0;
};

class FocusFrame : public virtual Frame {
public:
  virtual void          focus_frame        (FrameType ft) = 0;
  virtual FrameType     focus_frame        () const = 0;
  struct Client : public virtual Item {
    virtual bool        register_focus_frame   (FocusFrame &frame) = 0;
    virtual void        unregister_focus_frame (FocusFrame &frame) = 0;
  };
};

} // Rapicorn

#endif  /* __RAPICORN_PAINT_CONTAINERS_HH__ */
