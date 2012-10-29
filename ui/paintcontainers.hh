// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_PAINT_CONTAINERS_HH__
#define __RAPICORN_PAINT_CONTAINERS_HH__

#include <ui/container.hh>

namespace Rapicorn {

class Ambience : public virtual ContainerImpl {
protected:
  virtual const PropertyList&   list_properties         ();
public:
  virtual void                  insensitive_background  (const String &color) = 0;
  virtual String                insensitive_background  () const = 0;
  virtual void                  prelight_background     (const String &color) = 0;
  virtual String                prelight_background     () const = 0;
  virtual void                  impressed_background    (const String &color) = 0;
  virtual String                impressed_background    () const = 0;
  virtual void                  normal_background       (const String &color) = 0;
  virtual String                normal_background       () const = 0;
  virtual void                  insensitive_lighting    (LightingType sh) = 0;
  virtual LightingType          insensitive_lighting    () const = 0;
  virtual void                  prelight_lighting       (LightingType sh) = 0;
  virtual LightingType          prelight_lighting       () const = 0;
  virtual void                  impressed_lighting      (LightingType sh) = 0;
  virtual LightingType          impressed_lighting      () const = 0;
  virtual void                  normal_lighting         (LightingType sh) = 0;
  virtual LightingType          normal_lighting         () const = 0;
  virtual void                  insensitive_shade       (LightingType sh) = 0;
  virtual LightingType          insensitive_shade       () const = 0;
  virtual void                  prelight_shade          (LightingType sh) = 0;
  virtual LightingType          prelight_shade          () const = 0;
  virtual void                  impressed_shade         (LightingType sh) = 0;
  virtual LightingType          impressed_shade         () const = 0;
  virtual void                  normal_shade            (LightingType sh) = 0;
  virtual LightingType          normal_shade            () const = 0;
  /* group setters */
  void                          background              (const String &color);
  void                          lighting                (LightingType sh);
  void                          shade                   (LightingType sh);
private:
  String                        background              () const { RAPICORN_ASSERT_UNREACHED(); }
  LightingType                  lighting                () const { RAPICORN_ASSERT_UNREACHED(); }
  LightingType                  shade                   () const { RAPICORN_ASSERT_UNREACHED(); }
};

class Frame : public virtual ContainerImpl {
  FrameType                     frame_type      () const        { RAPICORN_ASSERT_UNREACHED(); }
protected:
  virtual const PropertyList&   list_properties ();
public:
  void                          frame_type      (FrameType ft);
  virtual FrameType             normal_frame    () const = 0;
  virtual void                  normal_frame    (FrameType ft) = 0;
  virtual FrameType             impressed_frame () const = 0;
  virtual void                  impressed_frame (FrameType ft) = 0;
  virtual bool                  overlap_child   () const = 0;
  virtual void                  overlap_child   (bool ovc) = 0;
  virtual bool                  tight_focus     () const = 0;
  virtual void                  tight_focus     (bool ovc) = 0;
};

class FocusFrame : public virtual Frame {
protected:
  virtual const PropertyList&   list_properties         ();
public:
  virtual void                  focus_frame             (FrameType ft) = 0;
  virtual FrameType             focus_frame             () const = 0;
  struct Client : public virtual ItemImpl {
    virtual bool                register_focus_frame    (FocusFrame &frame) = 0;
    virtual void                unregister_focus_frame  (FocusFrame &frame) = 0;
  };
};

} // Rapicorn

#endif  /* __RAPICORN_PAINT_CONTAINERS_HH__ */
