// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_IMAGE_HH__
#define __RAPICORN_IMAGE_HH__

#include <ui/widget.hh>
#include <ui/painter.hh>

namespace Rapicorn {

class ImageImpl : public virtual WidgetImpl, public virtual ImageIface {
  String                source_, stock_id_;
  ImagePainter          image_painter_;
protected:
  void                  broken_image    ();
  virtual void          size_request    (Requisition &requisition) override;
  virtual void          size_allocate   (Allocation area, bool changed) override;
  virtual void          render          (RenderContext &rcontext, const Rect &rect) override;
public:
  virtual void          pixbuf          (const Pixbuf &pixbuf) override;
  virtual Pixbuf        pixbuf          () const override;
  virtual void          source          (const String &uri) override;
  virtual String        source          () const override;
  virtual void          stock           (const String &stock_id) override;
  virtual String        stock           () const override;
};

class StatePainterImpl : public virtual WidgetImpl, public virtual StatePainterIface {
  String svg_source_, svg_fragment_;
  ImagePainter size_painter_, state_painter_;
  String cached_painter_;
  String         current_element     ();
  String         state_element       (StateType state);
protected:
  virtual void   do_changed          (const String &name) override;
  virtual void   size_request        (Requisition &requisition) override;
  virtual void   size_allocate       (Allocation area, bool changed) override;
  virtual void   render              (RenderContext &rcontext, const Rect &rect) override;
public:
  explicit       StatePainterImpl     ();
  virtual String svg_source           () const override                 { return svg_source_; }
  virtual void   svg_source           (const String &source) override;
  virtual String svg_element          () const override                 { return svg_fragment_; }
  virtual void   svg_element          (const String &element) override;
};

} // Rapicorn

#endif  // __RAPICORN_IMAGE_HH__
