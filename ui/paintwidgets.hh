// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_PAINT_WIDGETS_HH__
#define __RAPICORN_PAINT_WIDGETS_HH__

#include <ui/widget.hh>

namespace Rapicorn {

class ArrowImpl : public virtual WidgetImpl, public virtual ArrowIface {
  DirType dir_;
protected:
  virtual               ~ArrowImpl      () override;
  virtual void           size_request   (Requisition &requisition) override;
  virtual void           size_allocate (Allocation area, bool changed) override;
  virtual void           render (RenderContext &rcontext, const Rect &rect) override;
public:
  explicit               ArrowImpl      ();
  virtual void           arrow_dir      (DirType dir) override;
  virtual DirType        arrow_dir      () const override;
  virtual void           size_policy    (SizePolicyType spol) override;
  virtual SizePolicyType size_policy    () const override;
};

class DotGrid : public virtual WidgetImpl {
  FrameType             dot_type        () const { RAPICORN_ASSERT_UNREACHED(); }
protected:
  virtual
  const PropertyList&   __aida_properties__      ();
public:
  void                  dot_type            (FrameType ft);
  virtual void          normal_dot          (FrameType ft) = 0;
  virtual FrameType     normal_dot          () const = 0;
  virtual void          impressed_dot       (FrameType ft) = 0;
  virtual FrameType     impressed_dot       () const = 0;
  virtual void          n_hdots             (int   num) = 0;
  virtual int           n_hdots             () const = 0;
  virtual void          n_vdots             (int   num) = 0;
  virtual int           n_vdots             () const = 0;
  virtual int           right_padding_dots  () const  = 0;
  virtual void          right_padding_dots  (int c)  = 0;
  virtual int           top_padding_dots    () const  = 0;
  virtual void          top_padding_dots    (int c)  = 0;
  virtual int           left_padding_dots   () const  = 0;
  virtual void          left_padding_dots   (int c)  = 0;
  virtual int           bottom_padding_dots () const  = 0;
  virtual void          bottom_padding_dots (int c)  = 0;
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
