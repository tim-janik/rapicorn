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
  String         source_, normal_image_, prelight_image_, impressed_image_;
  String         insensitive_image_, default_image_, focus_image_, state_image_;
  ImagePainter   source_painter_, state_painter_;
  String         current_source      ();
  void           update_source      (String &member, const String &value, const char *name);
protected:
  virtual void   do_changed          (const String &name) override;
  virtual void   size_request        (Requisition &requisition) override;
  virtual void   size_allocate       (Allocation area, bool changed) override;
  virtual void   render              (RenderContext &rcontext, const Rect &rect) override;
public:
  virtual String source              () const override          { return source_; }
  virtual void   source              (const String &resource) override;
  virtual String normal_image        () const override          { return normal_image_; }
  virtual void   normal_image        (const String &e) override { update_source (normal_image_, e, "normal_image"); }
  virtual String prelight_image      () const override          { return prelight_image_; }
  virtual void   prelight_image      (const String &e) override { update_source (prelight_image_, e, "prelight_image"); }
  virtual String impressed_image     () const override          { return impressed_image_; }
  virtual void   impressed_image     (const String &e) override { update_source (impressed_image_, e, "impressed_image"); }
  virtual String insensitive_image   () const override          { return insensitive_image_; }
  virtual void   insensitive_image   (const String &e) override { update_source (insensitive_image_, e, "insensitive_image"); }
  virtual String default_image       () const override          { return default_image_; }
  virtual void   default_image       (const String &e) override { update_source (default_image_, e, "default_image"); }
  virtual String focus_image         () const override          { return focus_image_; }
  virtual void   focus_image         (const String &e) override { update_source (focus_image_, e, "focus_image"); }
};

} // Rapicorn

#endif  // __RAPICORN_IMAGE_HH__
