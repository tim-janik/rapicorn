// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_IMAGE_RENDERER_HH__
#define __RAPICORN_IMAGE_RENDERER_HH__

#include <ui/widget.hh>

namespace Rapicorn {

class ImageRendererImpl : public virtual WidgetImpl {
public:
  struct        ImageBackend;
protected:
  typedef std::shared_ptr<ImageBackend> ImageBackendP;
  explicit ImageRendererImpl ();
  virtual ~ImageRendererImpl () override;
  void     get_image_size    (const ImageBackendP &image_backend, Requisition &image_size);
  void     get_fill_area     (const ImageBackendP &image_backend, Allocation &fill_area);
  void     paint_image       (const ImageBackendP &image_backend, RenderContext &rcontext, const Rect &rect);
  ImageBackendP load_source  (const String &resource, const String &element_id) __attribute__ ((warn_unused_result));
  ImageBackendP load_pixmap  (Pixmap pixmap) __attribute__ ((warn_unused_result));
};

} // Rapicorn

#endif  /* __RAPICORN_IMAGE_RENDERER_HH__ */
