// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_ARRANGEMENT_HH__
#define __RAPICORN_ARRANGEMENT_HH__

#include <ui/container.hh>

namespace Rapicorn {

class Arrangement : public virtual ContainerImpl {
protected:
  virtual const PropertyList& __aida_properties__ ();
public:
  virtual double xorigin         () const = 0;
  virtual void   xorigin         (double v) = 0;
  virtual double yorigin         () const = 0;
  virtual void   yorigin         (double v) = 0;
  virtual float  origin_hanchor  () const = 0;
  virtual void   origin_hanchor  (float align) = 0;
  virtual float  origin_vanchor  () const = 0;
  virtual void   origin_vanchor  (float align) = 0;
  virtual Rect   child_area      () = 0;
};

} // Rapicorn

#endif  /* __RAPICORN_ARRANGEMENT_HH__ */
