// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_PAINT_CONTAINERS_HH__
#define __RAPICORN_PAINT_CONTAINERS_HH__

#include <ui/container.hh>
#include <ui/painter.hh>

namespace Rapicorn {

class AmbienceImpl : public virtual SingleContainerImpl, public virtual AmbienceIface {
  String   normal_background_, hover_background_, active_background_, insensitive_background_;
  Lighting normal_lighting_, hover_lighting_, active_lighting_, insensitive_lighting_;
  Lighting normal_shade_, hover_shade_, active_shade_, insensitive_shade_;
protected:
  void                 render_shade            (cairo_t *cairo, int x, int y, int width, int height, Lighting st);
  virtual void         render                  (RenderContext &rcontext) override;
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
  virtual Lighting     insensitive_lighting    () const override;
  virtual void         hover_lighting          (Lighting sh) override;
  virtual Lighting     hover_lighting          () const override;
  virtual void         active_lighting         (Lighting sh) override;
  virtual Lighting     active_lighting         () const override;
  virtual void         normal_lighting         (Lighting sh) override;
  virtual Lighting     normal_lighting         () const override;
  virtual void         insensitive_shade       (Lighting sh) override;
  virtual Lighting     insensitive_shade       () const override;
  virtual void         hover_shade             (Lighting sh) override;
  virtual Lighting     hover_shade             () const override;
  virtual void         active_shade            (Lighting sh) override;
  virtual Lighting     active_shade            () const override;
  virtual void         normal_shade            (Lighting sh) override;
  virtual Lighting     normal_shade            () const override;
  // group setters
  virtual void         background              (const String &color) override;
  virtual void         lighting                (Lighting sh) override;
  virtual void         shade                   (Lighting sh) override;
private:
  virtual String       background              () const override;
  virtual Lighting     lighting                () const override;
  virtual Lighting     shade                   () const override;
};

class FrameImpl : public virtual SingleContainerImpl, public virtual FrameIface {
  DrawFrame         normal_frame_, active_frame_;
  bool              overlap_child_, tight_focus_;
  bool              is_tight_focus    () const;
protected:
  bool              tap_tight_focus (int onoffx);
  virtual void      size_request    (Requisition &requisition) override;
  virtual void      size_allocate   (Allocation area) override;
  virtual void      render          (RenderContext &rcontext) override;
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
  virtual void hierarchy_changed          (WindowImpl *old_toplevel) override;
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
  virtual void   size_allocate           (Allocation area);
};

class ElementPainterImpl : public virtual SingleContainerImpl, public virtual ElementPainterIface {
  Allocation                    child_area_;
  String                        svg_source_;
  mutable Svg::FileP            cached_svg_;
  struct PaintEntry {
    String                      element;
    uint64                      state_bit;
  };
  std::vector<PaintEntry>       paint_elements_;
  static constexpr uint64       FILLING_PAINTER = 1ull << 61;
  static constexpr uint64       SIZING_PAINTER  = 1ull << 62;
  Svg::FileP           svg_file        () const;
  PaintEntry&          fetch_entry     (uint64 state_bit);
  const PaintEntry&    peek_entry      (uint64 state_bit) const;
  const PaintEntry&    peek_entry      (WidgetState state_bit) const                            { return peek_entry (uint64 (state_bit)); }
  void                 assign_entry    (uint64 state_bit, const String &element, const String &property);
  void                 assign_entry    (WidgetState sbit, const String &e, const String &p)     { assign_entry (uint64 (sbit), e, p); }
  ImagePainter         load_painter    (const String &fragment) const;
protected:
  String               size_fragment   () const;
  String               state_fragment  (WidgetState widget_state) const;
  String               filling_fragment(WidgetState widget_state) const;
  ImagePainter         size_painter    () const;
  ImagePainter         state_painter   () const;
  ImagePainter         filling_painter () const;
  virtual WidgetState  render_state    () const;
  virtual void         do_changed      (const String &name) override;
  virtual void         size_request    (Requisition &requisition) override;
  virtual void         size_allocate   (Allocation area) override;
  virtual void         render          (RenderContext &rcontext) override;
public:
  explicit       ElementPainterImpl   ();
  virtual       ~ElementPainterImpl   () override;
  virtual String svg_source            () const override                { return svg_source_; }
  virtual void   svg_source            (const String &source) override;
  virtual String sizing_element        () const override                { return peek_entry (SIZING_PAINTER).element; }
  virtual void   sizing_element        (const String &element) override { assign_entry (SIZING_PAINTER, element, "sizing_element"); }
  virtual String normal_element        () const override                { return peek_entry (WidgetState::NORMAL).element; }
  virtual void   normal_element        (const String &element) override { assign_entry (WidgetState::NORMAL, element, "normal_element"); }
  virtual String acceleratable_element () const override                { return peek_entry (WidgetState::ACCELERATABLE).element; }
  virtual void   acceleratable_element (const String &element) override { assign_entry (WidgetState::ACCELERATABLE, element, "acceleratable_element"); }
  virtual String hover_element         () const override                { return peek_entry (WidgetState::HOVER).element; }
  virtual void   hover_element         (const String &element) override { assign_entry (WidgetState::HOVER, element, "hover_element"); }
  virtual String panel_element         () const override                { return peek_entry (WidgetState::PANEL).element; }
  virtual void   panel_element         (const String &element) override { assign_entry (WidgetState::PANEL, element, "panel_element"); }
  virtual String default_element       () const override                { return peek_entry (WidgetState::DEFAULT).element; }
  virtual void   default_element       (const String &element) override { assign_entry (WidgetState::DEFAULT, element, "default_element"); }
  virtual String selected_element      () const override                { return peek_entry (WidgetState::SELECTED).element; }
  virtual void   selected_element      (const String &element) override { assign_entry (WidgetState::SELECTED, element, "selected_element"); }
  virtual String focused_element       () const override                { return peek_entry (WidgetState::FOCUSED).element; }
  virtual void   focused_element       (const String &element) override { assign_entry (WidgetState::FOCUSED, element, "focused_element"); }
  virtual String insensitive_element   () const override                { return peek_entry (WidgetState::INSENSITIVE).element; }
  virtual void   insensitive_element   (const String &element) override { assign_entry (WidgetState::INSENSITIVE, element, "insensitive_element"); }
  virtual String active_element        () const override                { return peek_entry (WidgetState::ACTIVE).element; }
  virtual void   active_element        (const String &element) override { assign_entry (WidgetState::ACTIVE, element, "active_element"); }
  virtual String toggled_element       () const override                { return peek_entry (WidgetState::TOGGLED).element; }
  virtual void   toggled_element       (const String &element) override { assign_entry (WidgetState::TOGGLED, element, "toggled_element"); }
  virtual String retained_element      () const override                { return peek_entry (WidgetState::RETAINED).element; }
  virtual void   retained_element      (const String &element) override { assign_entry (WidgetState::RETAINED, element, "retained_element"); }
  virtual String stashed_element       () const override                { return peek_entry (WidgetState::STASHED).element; }
  virtual void   stashed_element       (const String &element) override { assign_entry (WidgetState::STASHED, element, "stashed_element"); }
  virtual String filling_element       () const override                { return peek_entry (FILLING_PAINTER).element; }
  virtual void   filling_element       (const String &element) override { assign_entry (FILLING_PAINTER, element, "filling_element"); }
};

class FocusPainterImpl : public virtual ElementPainterImpl, public virtual FocusPainterIface, public virtual FocusIndicator {
  ContainerImpl *focus_container_;
  bool           container_has_focus_, tight_;
protected:
  virtual WidgetState render_state               () const override;
  virtual void        set_focus_child            (WidgetImpl *widget) override;
  virtual void        hierarchy_changed          (WindowImpl *old_toplevel) override;
  virtual void        focusable_container_change (ContainerImpl &focus_container) override;
public:
  explicit            FocusPainterImpl           ();
  virtual            ~FocusPainterImpl           () override;
};

class ShapePainterImpl : public virtual SingleContainerImpl, public virtual ShapePainterIface {
  String        shape_style_normal_;
  Allocation    fill_area_;
protected:
  virtual WidgetState  render_state    () const;
  virtual void         size_request    (Requisition &requisition) override;
  virtual void         size_allocate   (Allocation area) override;
  virtual void         render          (RenderContext &rcontext) override;
public:
  explicit       ShapePainterImpl     ();
  virtual       ~ShapePainterImpl     () override;
  virtual String shape_style_normal   () const override                 { return shape_style_normal_; }
  virtual void   shape_style_normal   (const String &kind) override;
};

} // Rapicorn

#endif  /* __RAPICORN_PAINT_CONTAINERS_HH__ */
