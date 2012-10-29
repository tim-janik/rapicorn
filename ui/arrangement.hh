// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_ARRANGEMENT_HH__
#define __RAPICORN_ARRANGEMENT_HH__

#include <ui/container.hh>

namespace Rapicorn {

class Arrangement : public virtual ContainerImpl {
protected:
  virtual const PropertyList& list_properties ();
public:
  virtual Point origin          () = 0;
  virtual void  origin          (Point p) = 0;
  virtual float origin_hanchor  () = 0;
  virtual void  origin_hanchor  (float align) = 0;
  virtual float origin_vanchor  () = 0;
  virtual void  origin_vanchor  (float align) = 0;
  virtual Rect  child_area      () = 0;
};

} // Rapicorn

#endif  /* __RAPICORN_ARRANGEMENT_HH__ */
