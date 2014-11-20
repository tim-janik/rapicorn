// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_IMAGE_HH__
#define __RAPICORN_IMAGE_HH__

#include <ui/imagerenderer.hh>

namespace Rapicorn {

class ImageImpl : public virtual ImageRendererImpl, public virtual ImageIface {
  String                source_, element_, stock_id_;
  ImageBackendP         image_backend_;
protected:
  void                  stock           (const String &stock_id);
  String                stock           () const;
  virtual void          size_request    (Requisition &requisition) override;
  virtual void          size_allocate   (Allocation area, bool changed) override;
  virtual void          render          (RenderContext &rcontext, const Rect &rect) override;
public:
  virtual void          pixbuf          (const Pixbuf &pixbuf) override;
  virtual Pixbuf        pixbuf          () const override;
  virtual void          source          (const String &uri) override;
  virtual String        source          () const override;
  virtual String        element         () const override;
  virtual void          element         (const String &id) override;
};

} // Rapicorn

#endif  // __RAPICORN_IMAGE_HH__
