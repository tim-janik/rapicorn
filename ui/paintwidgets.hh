// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_PAINT_WIDGETS_HH__
#define __RAPICORN_PAINT_WIDGETS_HH__

#include <ui/widget.hh>

namespace Rapicorn {

class ArrowImpl : public virtual WidgetImpl, public virtual ArrowIface {
  Direction dir_;
protected:
  virtual void           size_request   (Requisition &requisition) override;
  virtual void           size_allocate (Allocation area, bool changed) override;
  virtual void           render (RenderContext &rcontext, const DRect &rect) override;
public:
  explicit               ArrowImpl      ();
  virtual               ~ArrowImpl      () override;
  virtual void           arrow_dir      (Direction dir) override;
  virtual Direction      arrow_dir      () const override;
  virtual void           size_policy    (SizePolicy spol) override;
  virtual SizePolicy     size_policy    () const override;
};

class DotGridImpl : public virtual WidgetImpl, public virtual DotGridIface {
  DrawFrame             normal_dot_, active_dot_;
  uint                  n_hdots_, n_vdots_;
  uint16                right_padding_dots_, top_padding_dots_, left_padding_dots_, bottom_padding_dots_;
  virtual DrawFrame     dot_type            () const override;
protected:
  virtual void          size_request        (Requisition &requisition) override;
  virtual void          size_allocate       (Allocation area, bool changed) override;
  virtual void          render              (RenderContext &rcontext, const DRect &rect) override;
public:
  explicit              DotGridImpl         ();
  virtual              ~DotGridImpl         () override;
  virtual void          dot_type            (DrawFrame ft) override;
  virtual void          normal_dot          (DrawFrame ft) override;
  virtual DrawFrame     normal_dot          () const override;
  virtual void          active_dot          (DrawFrame ft) override;
  virtual DrawFrame     active_dot          () const override;
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
  virtual DrawFrame     current_dot         () override;
};

class DrawableImpl : public virtual WidgetImpl, public virtual DrawableIface {
  Pixbuf pixbuf_; int x_, y_;
protected:
  virtual void  draw_rect       (int x, int y, const Pixbuf &pixbuf);
  virtual void  size_request    (Requisition &requisition);
  virtual void  size_allocate   (Allocation area, bool changed);
  virtual void  render          (RenderContext &rcontext, const DRect &rect);
public:
  explicit      DrawableImpl    ();
};

} // Rapicorn

#endif  /* __RAPICORN_PAINT_WIDGETS_HH__ */
