// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_IMAGE_FRAME_HH__
#define __RAPICORN_IMAGE_FRAME_HH__

#include <ui/container.hh>

namespace Rapicorn {

class ImageFrameImpl : public virtual SingleContainerImpl, public virtual ImageFrameIface {
  class SvgElementP;
  union { uint64 mem_[(sizeof (std::shared_ptr<void>) + 7) / 8]; char dummy_; } svgelep_; // SvgElementP memory
  String                element_;
  bool                  overlap_child_;
  SvgElementP&          svg_element_ptr () const;
protected:
  virtual              ~ImageFrameImpl  () override;
  virtual void          size_request    (Requisition &requisition) override;
  virtual void          size_allocate   (Allocation area, bool changed) override;
  virtual void          do_invalidate   () override;
  virtual void          render          (RenderContext &rcontext, const Rect &rect) override;
public:
  explicit              ImageFrameImpl  ();
  virtual String        element         () const override;
  virtual void          element         (const String &id) override;
  virtual bool          overlap_child   () const override;
  virtual void          overlap_child   (bool overlap) override;
};

} // Rapicorn

#endif  /* __RAPICORN_IMAGE_FRAME_HH__ */
