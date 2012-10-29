// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_LIST_AREA_HH__
#define __RAPICORN_LIST_AREA_HH__

#include <ui/container.hh>

namespace Rapicorn {

class ItemList : public virtual ContainerImpl {
protected:
  virtual const PropertyList&   list_properties ();
public:
  virtual bool   browse       () const  = 0;
  virtual void   browse       (bool b) = 0;
  virtual void   model        (const String &modelurl) = 0;
  virtual String model        () const = 0;
};

} // Rapicorn

#endif  /* __RAPICORN_LIST_AREA_HH__ */
