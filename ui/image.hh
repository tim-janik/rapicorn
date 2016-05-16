// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
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
  virtual void          render          (RenderContext &rcontext, const IRect &rect) override;
public:
  virtual void          pixbuf          (const Pixbuf &pixbuf) override;
  virtual Pixbuf        pixbuf          () const override;
  virtual void          source          (const String &uri) override;
  virtual String        source          () const override;
  virtual void          stock           (const String &stock_id) override;
  virtual String        stock           () const override;
};

} // Rapicorn

#endif  // __RAPICORN_IMAGE_HH__
