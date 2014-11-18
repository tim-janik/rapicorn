// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_IMAGE_RENDERER_HH__
#define __RAPICORN_IMAGE_RENDERER_HH__

#include <ui/container.hh>

namespace Rapicorn {

class ImageRendererImpl : public virtual SingleContainerImpl {
  class SvgElementP;
  union { uint64 mem_[(sizeof (std::shared_ptr<void>) + 7) / 8]; char dummy_; } svgelep_; // SvgElementP memory
  String                element_;
  bool                  overlap_child_;
  SvgElementP&          svg_element_ptr () const;
protected:
  virtual              ~ImageRendererImpl() override;
  virtual void          size_request    (Requisition &requisition) override;
  virtual void          size_allocate   (Allocation area, bool changed) override;
  virtual void          do_invalidate   () override;
  virtual void          render          (RenderContext &rcontext, const Rect &rect) override;
public:
  explicit              ImageRendererImpl();
  void                  set_element     (const String &id);
  void                  set_overlap     (bool overlap);
};

} // Rapicorn

#endif  /* __RAPICORN_IMAGE_RENDERER_HH__ */
