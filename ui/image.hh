// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_IMAGE_HH__
#define __RAPICORN_IMAGE_HH__

#include <ui/widget.hh>

namespace Rapicorn {

class ImageImpl : public virtual WidgetImpl, public virtual ImageIface {
public: struct  ImageBackend;
private:
  String                image_url_, stock_id_;
  std::shared_ptr<ImageBackend> image_backend_;
protected:
  void                  load_pixmap     ();
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
