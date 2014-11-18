// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_IMAGE_RENDERER_HH__
#define __RAPICORN_IMAGE_RENDERER_HH__

#include <ui/widget.hh>

namespace Rapicorn {

class ImageRendererImpl : public virtual WidgetImpl {
public:
  struct        ImageBackend;
private:
  std::shared_ptr<ImageBackend> image_backend_;
protected:
  explicit ImageRendererImpl ();
  virtual ~ImageRendererImpl () override;
  void     get_image_size   (Requisition &image_size);
  void     get_fill_area    (Allocation &fill_area);
  void     paint_image      (RenderContext &rcontext, const Rect &rect);
  bool     load_source      (const String &resource, const String &element_id);
  void     load_pixmap      (Pixmap pixmap);
};

} // Rapicorn

#endif  /* __RAPICORN_IMAGE_RENDERER_HH__ */
