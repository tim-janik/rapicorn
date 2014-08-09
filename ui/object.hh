// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_OBJECT_HH_
#define __RAPICORN_OBJECT_HH_

#include <ui/serverapi.hh>

namespace Rapicorn {

// == Forward Declarations ==
typedef std::shared_ptr<ObjectIface> ObjectIfaceP;

/// ObjectImpl is the base type for all server side objects in Rapicorn and implements the IDL base type ObjectIface.
class ObjectImpl : public virtual ObjectIface, public virtual DataListContainer {
protected:
  virtual /*dtor*/                 ~ObjectImpl    () override;
  virtual void                      do_changed    (const String &name);
public:
  explicit                          ObjectImpl    ();
  Signal<void (const String &name)> sig_changed;
  void                              changed       (const String &name);
};
inline bool operator== (const ObjectImpl &object1, const ObjectImpl &object2) { return &object1 == &object2; }
inline bool operator!= (const ObjectImpl &object1, const ObjectImpl &object2) { return &object1 != &object2; }

} // Rapicorn

#endif  /* __RAPICORN_OBJECT_HH_ */
