// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_PAINT_WIDGETS_HH__
#define __RAPICORN_PAINT_WIDGETS_HH__

#include <ui/widget.hh>

namespace Rapicorn {

class ArrowImpl : public virtual WidgetImpl, public virtual ArrowIface {
  DirType dir_;
protected:
  virtual void           size_request   (Requisition &requisition) override;
  virtual void           size_allocate (Allocation area, bool changed) override;
  virtual void           render (RenderContext &rcontext, const Rect &rect) override;
public:
  explicit               ArrowImpl      ();
  virtual               ~ArrowImpl      () override;
  virtual void           arrow_dir      (DirType dir) override;
  virtual DirType        arrow_dir      () const override;
  virtual void           size_policy    (SizePolicyType spol) override;
  virtual SizePolicyType size_policy    () const override;
};

class DotGridImpl : public virtual WidgetImpl, public virtual DotGridIface {
  FrameType             normal_dot_, active_dot_;
  uint                  n_hdots_, n_vdots_;
  uint16                right_padding_dots_, top_padding_dots_, left_padding_dots_, bottom_padding_dots_;
  virtual FrameType     dot_type            () const override;
protected:
  virtual void          size_request        (Requisition &requisition) override;
  virtual void          size_allocate       (Allocation area, bool changed) override;
  virtual void          render              (RenderContext &rcontext, const Rect &rect) override;
public:
  explicit              DotGridImpl         ();
  virtual              ~DotGridImpl         () override;
  virtual void          dot_type            (FrameType ft) override;
  virtual void          normal_dot          (FrameType ft) override;
  virtual FrameType     normal_dot          () const override;
  virtual void          active_dot          (FrameType ft) override;
  virtual FrameType     active_dot          () const override;
  virtual void          n_hdots             (int   num) override;
  virtual int           n_hdots             () const override;
  virtual void          n_vdots             (int   num) override;
  virtual int           n_vdots             () const override;
  virtual int           right_padding_dots  () const  override;
  virtual void          right_padding_dots  (int c)  override;
  virtual int           top_padding_dots    () const  override;
  virtual void          top_padding_dots    (int c)  override;
  virtual int           left_padding_dots   () const  override;
  virtual void          left_padding_dots   (int c)  override;
  virtual int           bottom_padding_dots () const  override;
  virtual void          bottom_padding_dots (int c)  override;
  virtual FrameType     current_dot         () override;
};

class DrawableImpl : public virtual WidgetImpl, public virtual DrawableIface {
  Pixbuf pixbuf_; int x_, y_;
protected:
  virtual void  draw_rect       (int x, int y, const Pixbuf &pixbuf);
  virtual void  size_request    (Requisition &requisition);
  virtual void  size_allocate   (Allocation area, bool changed);
  virtual void  render          (RenderContext &rcontext, const Rect &rect);
public:
  explicit      DrawableImpl    ();
};

} // Rapicorn

#endif  /* __RAPICORN_PAINT_WIDGETS_HH__ */
