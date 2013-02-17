// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_PAINT_WIDGETS_HH__
#define __RAPICORN_PAINT_WIDGETS_HH__

#include <ui/item.hh>

namespace Rapicorn {

class Arrow : public virtual WidgetImpl {
protected:
  virtual
  const PropertyList&    _property_list();
public:
  virtual void           arrow_dir      (DirType dir) = 0;
  virtual DirType        arrow_dir      () const = 0;
  virtual void           size_policy    (SizePolicyType spol) = 0;
  virtual SizePolicyType size_policy    () const = 0;
};

class DotGrid : public virtual WidgetImpl {
  FrameType             dot_type        () const { RAPICORN_ASSERT_UNREACHED(); }
protected:
  virtual
  const PropertyList&   _property_list      ();
public:
  void                  dot_type            (FrameType ft);
  virtual void          normal_dot          (FrameType ft) = 0;
  virtual FrameType     normal_dot          () const = 0;
  virtual void          impressed_dot       (FrameType ft) = 0;
  virtual FrameType     impressed_dot       () const = 0;
  virtual void          n_hdots             (uint   num) = 0;
  virtual uint          n_hdots             () const = 0;
  virtual void          n_vdots             (uint   num) = 0;
  virtual uint          n_vdots             () const = 0;
  virtual uint          right_padding_dots  () const  = 0;
  virtual void          right_padding_dots  (uint c)  = 0;
  virtual uint          top_padding_dots    () const  = 0;
  virtual void          top_padding_dots    (uint c)  = 0;
  virtual uint          left_padding_dots   () const  = 0;
  virtual void          left_padding_dots   (uint c)  = 0;
  virtual uint          bottom_padding_dots () const  = 0;
  virtual void          bottom_padding_dots (uint c)  = 0;
};

class DrawableImpl : public virtual WidgetImpl, public virtual DrawableIface {
  Pixbuf pixbuf_; int x_, y_;
protected:
  virtual void  draw_rect       (int x, int y, const Pixbuf &pixbuf);
  virtual void  size_request    (Requisition &requisition);
  virtual void  size_allocate   (Allocation area, bool changed);
  virtual void  render          (RenderContext &rcontext, const Rect &rect);
  virtual const PropertyList&   _property_list ();
public:
  explicit      DrawableImpl    ();
};

} // Rapicorn

#endif  /* __RAPICORN_PAINT_WIDGETS_HH__ */
