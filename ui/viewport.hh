// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_VIEWPORT_HH__
#define __RAPICORN_VIEWPORT_HH__

#include <ui/container.hh>
#include <ui/displaywindow.hh>

namespace Rapicorn {

class ViewportImpl : public virtual ResizeContainerImpl {
protected:
  virtual const AncestryCache* fetch_ancestry_cache () override;
public:
  explicit              ViewportImpl            ();
  virtual              ~ViewportImpl            ();
};

} // Rapicorn

#endif  /* __RAPICORN_VIEWPORT_HH__ */
