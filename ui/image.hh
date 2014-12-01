// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_IMAGE_HH__
#define __RAPICORN_IMAGE_HH__

#include <ui/imagerenderer.hh>

namespace Rapicorn {

class ImageImpl : public virtual ImageRendererImpl, public virtual ImageIface {
  String                source_, element_, stock_id_;
  ImageBackendP         image_backend_;
protected:
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
  virtual void          stock           (const String &stock_id) override;
  virtual String        stock           () const override;
};

class StatePainterImpl : public virtual ImageRendererImpl, public virtual StatePainterIface {
  String         source_, element_, normal_element_, prelight_element_, impressed_element_;
  String         insensitive_element_, default_element_, focus_element_, state_element_;
  ImageBackendP  element_image_, state_image_;
  String         current_element     ();
  void           update_element      (String &member, const String &value, const char *name);
protected:
  virtual void   do_changed          (const String &name) override;
  virtual void   size_request        (Requisition &requisition) override;
  virtual void   size_allocate       (Allocation area, bool changed) override;
  virtual void   render              (RenderContext &rcontext, const Rect &rect) override;
public:
  virtual String source              () const override          { return source_; }
  virtual void   source              (const String &resource) override;
  virtual String element             () const override          { return element_; }
  virtual void   element             (const String &e) override;
  virtual String normal_element      () const override          { return normal_element_; }
  virtual void   normal_element      (const String &e) override { update_element (normal_element_, e, "normal_element"); }
  virtual String prelight_element    () const override          { return prelight_element_; }
  virtual void   prelight_element    (const String &e) override { update_element (prelight_element_, e, "prelight_element"); }
  virtual String impressed_element   () const override          { return impressed_element_; }
  virtual void   impressed_element   (const String &e) override { update_element (impressed_element_, e, "impressed_element"); }
  virtual String insensitive_element () const override          { return insensitive_element_; }
  virtual void   insensitive_element (const String &e) override { update_element (insensitive_element_, e, "insensitive_element"); }
  virtual String default_element     () const override          { return default_element_; }
  virtual void   default_element     (const String &e) override { update_element (default_element_, e, "default_element"); }
  virtual String focus_element       () const override          { return focus_element_; }
  virtual void   focus_element       (const String &e) override { update_element (focus_element_, e, "focus_element"); }
};

} // Rapicorn

#endif  // __RAPICORN_IMAGE_HH__
