// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_PAINT_CONTAINERS_HH__
#define __RAPICORN_PAINT_CONTAINERS_HH__

#include <ui/container.hh>

namespace Rapicorn {

class AmbienceImpl : public virtual SingleContainerImpl, public virtual AmbienceIface {
  String normal_background_, prelight_background_, impressed_background_, insensitive_background_;
  LightingType normal_lighting_, prelight_lighting_, impressed_lighting_, insensitive_lighting_;
  LightingType normal_shade_, prelight_shade_, impressed_shade_, insensitive_shade_;
protected:
  void                 render_shade            (cairo_t *cairo, int x, int y, int width, int height, LightingType st);
  virtual void         render                  (RenderContext &rcontext, const Rect &rect) override;
  virtual             ~AmbienceImpl            () override;
public:
  explicit             AmbienceImpl            ();
  virtual void         insensitive_background  (const String &color) override;
  virtual String       insensitive_background  () const override;
  virtual void         prelight_background     (const String &color) override;
  virtual String       prelight_background     () const override;
  virtual void         impressed_background    (const String &color) override;
  virtual String       impressed_background    () const override;
  virtual void         normal_background       (const String &color) override;
  virtual String       normal_background       () const override;
  virtual void         insensitive_lighting    (LightingType sh) override;
  virtual LightingType insensitive_lighting    () const override;
  virtual void         prelight_lighting       (LightingType sh) override;
  virtual LightingType prelight_lighting       () const override;
  virtual void         impressed_lighting      (LightingType sh) override;
  virtual LightingType impressed_lighting      () const override;
  virtual void         normal_lighting         (LightingType sh) override;
  virtual LightingType normal_lighting         () const override;
  virtual void         insensitive_shade       (LightingType sh) override;
  virtual LightingType insensitive_shade       () const override;
  virtual void         prelight_shade          (LightingType sh) override;
  virtual LightingType prelight_shade          () const override;
  virtual void         impressed_shade         (LightingType sh) override;
  virtual LightingType impressed_shade         () const override;
  virtual void         normal_shade            (LightingType sh) override;
  virtual LightingType normal_shade            () const override;
  // group setters
  virtual void         background              (const String &color) override;
  virtual void         lighting                (LightingType sh) override;
  virtual void         shade                   (LightingType sh) override;
private:
  virtual String       background              () const override;
  virtual LightingType lighting                () const override;
  virtual LightingType shade                   () const override;
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
   * .can_focus() efficiently and for a FocusFrame to reflect its ancestor's .has_focus() state.
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
