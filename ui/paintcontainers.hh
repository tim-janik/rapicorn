// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_PAINT_CONTAINERS_HH__
#define __RAPICORN_PAINT_CONTAINERS_HH__

#include <ui/container.hh>
#include <ui/painter.hh>

namespace Rapicorn {

class AmbienceImpl : public virtual SingleContainerImpl, public virtual AmbienceIface {
  String normal_background_, hover_background_, active_background_, insensitive_background_;
  Lighting normal_lighting_, hover_lighting_, active_lighting_, insensitive_lighting_;
  Lighting normal_shade_, hover_shade_, active_shade_, insensitive_shade_;
protected:
  void                 render_shade            (cairo_t *cairo, int x, int y, int width, int height, Lighting st);
  virtual void         render                  (RenderContext &rcontext, const Rect &rect) override;
public:
  explicit             AmbienceImpl            ();
  virtual             ~AmbienceImpl            () override;
  virtual void         insensitive_background  (const String &color) override;
  virtual String       insensitive_background  () const override;
  virtual void         hover_background        (const String &color) override;
  virtual String       hover_background        () const override;
  virtual void         active_background       (const String &color) override;
  virtual String       active_background       () const override;
  virtual void         normal_background       (const String &color) override;
  virtual String       normal_background       () const override;
  virtual void         insensitive_lighting    (Lighting sh) override;
  virtual Lighting insensitive_lighting    () const override;
  virtual void         hover_lighting          (Lighting sh) override;
  virtual Lighting hover_lighting          () const override;
  virtual void         active_lighting         (Lighting sh) override;
  virtual Lighting active_lighting         () const override;
  virtual void         normal_lighting         (Lighting sh) override;
  virtual Lighting normal_lighting         () const override;
  virtual void         insensitive_shade       (Lighting sh) override;
  virtual Lighting insensitive_shade       () const override;
  virtual void         hover_shade             (Lighting sh) override;
  virtual Lighting hover_shade             () const override;
  virtual void         active_shade            (Lighting sh) override;
  virtual Lighting active_shade            () const override;
  virtual void         normal_shade            (Lighting sh) override;
  virtual Lighting normal_shade            () const override;
  // group setters
  virtual void         background              (const String &color) override;
  virtual void         lighting                (Lighting sh) override;
  virtual void         shade                   (Lighting sh) override;
private:
  virtual String       background              () const override;
  virtual Lighting lighting                () const override;
  virtual Lighting shade                   () const override;
};

class FrameImpl : public virtual SingleContainerImpl, public virtual FrameIface {
  DrawFrame         normal_frame_, active_frame_;
  bool              overlap_child_, tight_focus_;
  bool              is_tight_focus    () const;
protected:
  bool              tap_tight_focus (int onoffx);
  virtual void      do_changed      (const String &name) override;
  virtual void      size_request    (Requisition &requisition) override;
  virtual void      size_allocate   (Allocation area, bool changed) override;
  virtual void      render          (RenderContext &rcontext, const Rect &rect) override;
public: // FrameIface
  explicit          FrameImpl       ();
  virtual          ~FrameImpl       () override;
  virtual DrawFrame current_frame   () override;
  virtual DrawFrame normal_frame    () const override;
  virtual void      normal_frame    (DrawFrame) override;
  virtual DrawFrame active_frame    () const override;
  virtual void      active_frame    (DrawFrame) override;
  virtual DrawFrame frame_type      () const override;
  virtual void      frame_type      (DrawFrame) override;
  virtual bool      overlap_child   () const override;
  virtual void      overlap_child   (bool) override;
};

class FocusFrameImpl : public virtual FrameImpl, public virtual FocusFrameIface, public virtual FocusIndicator {
  ContainerImpl    *focus_container_;
  bool              container_has_focus_;
  DrawFrame         focus_frame_;
protected:
  virtual void set_focus_child            (WidgetImpl *widget) override;
  virtual void hierarchy_changed          (WidgetImpl *old_toplevel) override;
  virtual void focusable_container_change (ContainerImpl &focus_container) override;
public:
  explicit          FocusFrameImpl    ();
  virtual          ~FocusFrameImpl    () override;
  virtual DrawFrame current_frame () override;
  // FocusFrameIface
  virtual DrawFrame focus_frame   () const override;
  virtual void      focus_frame   (DrawFrame) override;
  virtual bool      tight_focus   () const override;
  virtual void      tight_focus   (bool) override;
};

class LayerPainterImpl : public virtual MultiContainerImpl, public virtual LayerPainterIface {
public:
  explicit       LayerPainterImpl ();
  virtual       ~LayerPainterImpl () override;
protected:
  virtual void   size_request            (Requisition &requisition);
  virtual void   size_allocate           (Allocation area, bool changed);
  Allocation     local_child_allocation  (WidgetImpl &child, double width, double height);
};

class ElementPainterImpl : public virtual SingleContainerImpl, public virtual ElementPainterIface {
  String         svg_source_, svg_fragment_;
  ImagePainter   size_painter_, state_painter_;
  String         cached_painter_;
  String         current_element     ();
  String         state_element       (WidgetState state);
protected:
  virtual WidgetState element_state    () const;
  virtual void      do_changed       (const String &name) override;
  virtual void      size_request     (Requisition &requisition) override;
  virtual void      size_allocate    (Allocation area, bool changed) override;
  virtual void      render           (RenderContext &rcontext, const Rect &rect) override;
public:
  explicit       ElementPainterImpl   ();
  virtual       ~ElementPainterImpl   () override;
  virtual String svg_source           () const override                 { return svg_source_; }
  virtual void   svg_source           (const String &source) override;
  virtual String svg_element          () const override                 { return svg_fragment_; }
  virtual void   svg_element          (const String &element) override;
};

class FocusPainterImpl : public virtual ElementPainterImpl, public virtual FocusPainterIface, public virtual FocusIndicator {
  ContainerImpl *focus_container_;
  bool           container_has_focus_, tight_;
protected:
  virtual WidgetState element_state              () const override;
  virtual void      set_focus_child            (WidgetImpl *widget) override;
  virtual void      hierarchy_changed          (WidgetImpl *old_toplevel) override;
  virtual void      focusable_container_change (ContainerImpl &focus_container) override;
public:
  explicit       FocusPainterImpl ();
  virtual       ~FocusPainterImpl () override;
};

} // Rapicorn

#endif  /* __RAPICORN_PAINT_CONTAINERS_HH__ */
