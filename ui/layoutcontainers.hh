// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_LAYOUT_CONTAINERS_HH__
#define __RAPICORN_LAYOUT_CONTAINERS_HH__

#include <ui/container.hh>

namespace Rapicorn {

class Alignment : public virtual ContainerImpl {
  virtual int   padding         () const  = 0;
protected:
  virtual const PropertyList&   __aida_properties__ ();
public:
  virtual int   left_padding    () const  = 0;
  virtual void  left_padding    (int c)  = 0;
  virtual int   right_padding   () const  = 0;
  virtual void  right_padding   (int c)  = 0;
  virtual int   bottom_padding  () const  = 0;
  virtual void  bottom_padding  (int c)  = 0;
  virtual int   top_padding     () const  = 0;
  virtual void  top_padding     (int c)  = 0;
  virtual void  padding         (int c)  = 0;
};

class HBox : public virtual ContainerImpl {
protected:
  virtual const PropertyList&   __aida_properties__ ();
public:
  virtual bool  homogeneous     () const = 0;
  virtual void  homogeneous     (bool chomogeneous_widgets) = 0;
  virtual int   spacing         () const = 0;
  virtual void  spacing         (int cspacing) = 0;
};

class VBox : public virtual ContainerImpl {
protected:
  virtual const PropertyList&   __aida_properties__ ();
public:
  virtual bool  homogeneous     () const = 0;
  virtual void  homogeneous     (bool chomogeneous_widgets) = 0;
  virtual int   spacing         () const = 0;
  virtual void  spacing         (int cspacing) = 0;
};

} // Rapicorn

#endif  /* __RAPICORN_LAYOUT_CONTAINERS_HH__ */
