/* Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html */
#ifndef __RAPICORN_COFFER_HH__
#define __RAPICORN_COFFER_HH__

#include <ui/container.hh>

namespace Rapicorn {

class Coffer : public virtual ContainerImpl {
protected:
  virtual const PropertyList& _property_list ();
public:
  virtual String element        () const = 0;
  virtual void   element        (const String &id) = 0;
  virtual bool   overlap_child  () const = 0;
  virtual void   overlap_child  (bool overlap) = 0;
};

} // Rapicorn

#endif  /* __RAPICORN_COFFER_HH__ */
