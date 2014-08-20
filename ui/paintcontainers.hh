// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_PAINT_CONTAINERS_HH__
#define __RAPICORN_PAINT_CONTAINERS_HH__

#include <ui/container.hh>

namespace Rapicorn {

class Ambience : public virtual ContainerImpl {
protected:
  virtual const PropertyList&   __aida_properties__         ();
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

class FrameImpl : public virtual SingleContainerImpl, public virtual FrameIface {
  FrameType         normal_frame_, impressed_frame_;
  bool              overlap_child_, tight_focus_;
  bool              is_tight_focus    () const;
protected:
  bool              tap_tight_focus (int onoffx);
  virtual          ~FrameImpl       () override;
  virtual void      do_changed      (const String &name) override;
  virtual void      size_request    (Requisition &requisition) override;
  virtual void      size_allocate   (Allocation area, bool changed) override;
  virtual void      render          (RenderContext &rcontext, const Rect &rect) override;
public: // FrameIface
  explicit          FrameImpl       ();
  virtual FrameType current_frame   () override;
  virtual FrameType normal_frame    () const override;
  virtual void      normal_frame    (FrameType) override;
  virtual FrameType impressed_frame () const override;
  virtual void      impressed_frame (FrameType) override;
  virtual FrameType frame_type      () const override;
  virtual void      frame_type      (FrameType) override;
  virtual bool      overlap_child   () const override;
  virtual void      overlap_child   (bool) override;
};

class FocusFrameImpl : public virtual FrameImpl, public virtual FocusFrameIface {
protected:
  virtual          ~FocusFrameImpl    () override;
  virtual void      set_focus_child   (WidgetImpl *widget) override;
  virtual void      hierarchy_changed (WidgetImpl *old_toplevel) override;
public:
  explicit          FocusFrameImpl    ();
  /** FocusFrame registers itself with ancestors that implement the FocusFrameImpl::Client interface.
   * This is useful for ancestors to be notified about a FocusFrameImpl descendant to implement
   * ::can_focus efficiently and for a FocusFrame to reflect its ancestor's ::has_focus state.
   */
  struct Client : public virtual WidgetImpl {
    virtual bool    register_focus_frame    (FocusFrameImpl &frame) = 0;
    virtual void    unregister_focus_frame  (FocusFrameImpl &frame) = 0;
  };
  virtual FrameType current_frame () override;
  // FocusFrameIface
  virtual FrameType focus_frame   () const override;
  virtual void      focus_frame   (FrameType) override;
  virtual bool      tight_focus   () const override;
  virtual void      tight_focus   (bool) override;
private:
  FrameType         focus_frame_;
  Client           *client_;
  size_t            conid_client_;
  void              client_changed (const String &name);
};

} // Rapicorn

#endif  /* __RAPICORN_PAINT_CONTAINERS_HH__ */
