// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_VIEWPORT_HH__
#define __RAPICORN_VIEWPORT_HH__

#include <ui/container.hh>
#include <ui/screenwindow.hh>

namespace Rapicorn {

class ViewportImpl : public virtual SingleContainerImpl {
protected:
  Region                m_expose_region;        // FIXME: private
  virtual void          render          (Display        &display);
  void                  collapse_expose_region ();
public:
  explicit              ViewportImpl    ();
  virtual              ~ViewportImpl    ();
};

} // Rapicorn

#endif  /* __RAPICORN_VIEWPORT_HH__ */
