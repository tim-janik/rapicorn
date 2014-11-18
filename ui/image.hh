// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_IMAGE_HH__
#define __RAPICORN_IMAGE_HH__

#include <ui/imagerenderer.hh>

namespace Rapicorn {

class ImageImpl : public virtual ImageRendererImpl, public virtual ImageIface {
private:
  String                image_url_, stock_id_;
protected:
  void                  stock           (const String &stock_id);
  String                stock           () const;
  virtual void          size_request    (Requisition &requisition);
  virtual void          size_allocate   (Allocation area, bool changed);
  virtual void          render          (RenderContext &rcontext, const Rect &rect);
public:
  virtual void          pixbuf          (const Pixbuf &pixbuf);
  virtual Pixbuf        pixbuf          () const;
  virtual void          source          (const String &uri);
  virtual String        source          () const;
};

} // Rapicorn

#endif  // __RAPICORN_IMAGE_HH__
