// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_OBJECT_HH_
#define __RAPICORN_OBJECT_HH_

#include <ui/primitives.hh>

namespace Rapicorn {

// == Forward Declarations ==
class FactoryContext;
class ObjectImpl;
typedef std::shared_ptr<ObjectIface> ObjectIfaceP;
typedef std::shared_ptr<ObjectImpl>  ObjectImplP;

/// ObjectImpl is the base type for all server side objects in Rapicorn and implements the IDL base type ObjectIface.
class ObjectImpl : public virtual ObjectIface, public virtual DataListContainer {
  static void                       ctor_factory_context (FactoryContext *fc);
protected:
  virtual /*dtor*/                 ~ObjectImpl    () override;
  virtual void                      construct     ();
  virtual void                      dispose       ();
  virtual void                      do_changed    (const String &name);
  class                    InterfaceMatcher;
  template<class C>  class InterfaceMatch;
  FactoryContext&                   ctor_factory_context ();
public:
  explicit                          ObjectImpl    ();
  Signal<void (const String &name)> sig_changed;
  virtual void                      changed       (const String &name);
  String                            typeid_name   ();
  template<class InstanceType> static
  std::shared_ptr<InstanceType>     make_instance (FactoryContext &fc)
  {
    ctor_factory_context (&fc);
    std::shared_ptr<InstanceType> sptr = std::make_shared<InstanceType>();
    ctor_factory_context (NULL);
    static_cast<ObjectImpl*> (&*sptr)->construct();
    return sptr;
  }
};
inline bool operator== (const ObjectImpl &object1, const ObjectImpl &object2) { return &object1 == &object2; }
inline bool operator!= (const ObjectImpl &object1, const ObjectImpl &object2) { return &object1 != &object2; }

// Implementation details
struct ObjectImpl::InterfaceMatcher {
  explicit      InterfaceMatcher (const String &ident) : ident_ (ident), match_found_ (false) {}
  bool          done             () const { return match_found_; }
  virtual  bool match            (ObjectImpl *object, const String &ident = String()) = 0;
  RAPICORN_CLASS_NON_COPYABLE (InterfaceMatcher);
protected:
  const String &ident_;
  bool          match_found_;
};

template<class ClassPtr> struct ObjectImpl::InterfaceMatch : ObjectImpl::InterfaceMatcher {
  static_assert (std::is_pointer<ClassPtr>::value, "invalid InterfaceMatch<> cast to non-pointer type");
};

template<class Class>
struct ObjectImpl::InterfaceMatch<Class*> : ObjectImpl::InterfaceMatcher {
  typedef Class* Result;
  explicit       InterfaceMatch  (const String &ident) : InterfaceMatcher (ident), instance_ (NULL) {}
  Class*         result          () const { return instance_; }
  virtual bool   match           (ObjectImpl *object, const String &ident = String()) final
  {
    if (!instance_)
      {
        const String &id = ident_;
        if (id.empty() || id == ident)
          {
            instance_ = dynamic_cast<Class*> (object);
            match_found_ = instance_ != NULL;
          }
      }
    return match_found_;
  }
private:
  Class         *instance_;
};

} // Rapicorn

#endif  /* __RAPICORN_OBJECT_HH_ */
