// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_VIEWPORT_HH__
#define __RAPICORN_VIEWPORT_HH__

#include <ui/container.hh>
#include <ui/displaywindow.hh>

namespace Rapicorn {

class ViewportImpl : public virtual ResizeContainerImpl {
  Region                expose_region_;        // maintained in child coord space
  void                  collapse_expose_region  ();
protected:
  const Region&         peek_expose_region      () const { return expose_region_; }
  void                  discard_expose_region   () { expose_region_.clear(); }
  bool                  exposes_pending         () const { return !expose_region_.empty(); }
public:
  void                  expose_child_region     (const Region &region);
  explicit              ViewportImpl            ();
  virtual              ~ViewportImpl            ();
};

} // Rapicorn

#endif  /* __RAPICORN_VIEWPORT_HH__ */
