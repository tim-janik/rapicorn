// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_LAYOUT_CONTAINERS_HH__
#define __RAPICORN_LAYOUT_CONTAINERS_HH__

#include <ui/container.hh>

namespace Rapicorn {

class Alignment : public virtual ContainerImpl {
  virtual uint  padding         () const  = 0;
protected:
  virtual const PropertyList&   _property_list ();
public:
  virtual uint  left_padding    () const  = 0;
  virtual void  left_padding    (uint c)  = 0;
  virtual uint  right_padding   () const  = 0;
  virtual void  right_padding   (uint c)  = 0;
  virtual uint  bottom_padding  () const  = 0;
  virtual void  bottom_padding  (uint c)  = 0;
  virtual uint  top_padding     () const  = 0;
  virtual void  top_padding     (uint c)  = 0;
  virtual void  padding         (uint c)  = 0;
};

class HBox : public virtual ContainerImpl {
protected:
  virtual const PropertyList&   _property_list ();
public:
  virtual bool  homogeneous     () const = 0;
  virtual void  homogeneous     (bool chomogeneous_items) = 0;
  virtual uint  spacing         () = 0;
  virtual void  spacing         (uint cspacing) = 0;
};

class VBox : public virtual ContainerImpl {
protected:
  virtual const PropertyList&   _property_list ();
public:
  virtual bool  homogeneous     () const = 0;
  virtual void  homogeneous     (bool chomogeneous_items) = 0;
  virtual uint  spacing         () = 0;
  virtual void  spacing         (uint cspacing) = 0;
};

} // Rapicorn

#endif  /* __RAPICORN_LAYOUT_CONTAINERS_HH__ */
