// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_CORE_OBJECTS_HH__
#define __RAPICORN_CORE_OBJECTS_HH__

#include <rcore/utilities.hh>
#include <rcore/thread.hh>
#include <rcore/aidaprops.hh>

namespace Rapicorn {

// == VirtualTypeid ==
class VirtualTypeid {
protected:
  virtual      ~VirtualTypeid      ();
public:
  String        typeid_name        ();
};
String          cxx_demangle       (const char *mangled_identifier);

// == ClassDoctor (used for private class copies) ==
#ifdef  __RAPICORN_BUILD__
class ClassDoctor;
#else
class ClassDoctor {};
#endif

// == Deletable ==
/**
 * Deletable is a virtual base class that can be derived from (usually with
 * public virtual) to ensure an object has a vtable and a virtual destructor.
 * Also, it allows deletion hooks to be called during the objects destructor,
 * by deriving from Rapicorn::Deletable::DeletionHook. No extra per-object space is
 * consumed to allow deletion hooks, which makes Deletable a suitable base
 * type for classes that may or may not need this feature (e.g. objects that
 * can but often aren't used for signal handler connections).
 */
struct Deletable : public virtual VirtualTypeid {
  /**
   * DeletionHook is the base implementation class for hooks which are hooked
   * up into the deletion phase of a Rapicorn::Deletable.
   */
  class DeletionHook {
    DeletionHook    *prev;
    DeletionHook    *next;
    friend class Deletable;
  protected:
    virtual     ~DeletionHook          (); /* { if (deletable) deletable_remove_hook (deletable); deletable = NULL; } */
    virtual void monitoring_deletable  (Deletable &deletable) = 0;
    virtual void dismiss_deletable     () = 0;
  public:
    explicit     DeletionHook          () : prev (NULL), next (NULL) {}
    bool         deletable_add_hook    (void      *any)              { return false; }
    bool         deletable_add_hook    (Deletable *deletable);
    bool         deletable_remove_hook (void      *any)              { return false; }
    bool         deletable_remove_hook (Deletable *deletable);
  };
private:
  void           add_deletion_hook     (DeletionHook *hook);
  void           remove_deletion_hook  (DeletionHook *hook);
protected:
  void           invoke_deletion_hooks ();
  virtual       ~Deletable             ();
};

// == DataListContainer ==
/**
 * By using a DataKey, DataListContainer objects allow storage and retrieval of custom data members in a typesafe fashion.
 * The custom data members will initially default to DataKey::fallback and are deleted by the DataListContainer destructor.
 * Example: @snippet rcore/tests/datalist.cc DataListContainer-EXAMPLE
 */
class DataListContainer {
  DataList data_list;
public: /// @name Accessing custom data members
  /// Assign @a data to the custom keyed data member, deletes any previously set data.
  template<typename Type> inline void set_data    (DataKey<Type> *key, Type data) { data_list.set (key, data); }
  /// Retrieve contents of the custom keyed data member, returns DataKey::fallback if nothing was set.
  template<typename Type> inline Type get_data    (DataKey<Type> *key) const      { return data_list.get (key); }
  /// Swap @a data with the current contents of the custom keyed data member, returns the current contents.
  template<typename Type> inline Type swap_data   (DataKey<Type> *key, Type data) { return data_list.swap (key, data); }
  /// Removes and returns the current contents of the custom keyed data member without deleting it.
  template<typename Type> inline Type swap_data   (DataKey<Type> *key)            { return data_list.swap (key); }
  /// Delete the current contents of the custom keyed data member, invokes DataKey::destroy.
  template<typename Type> inline void delete_data (DataKey<Type> *key)            { data_list.del (key); }
};

// == ReferenceCountable ==
class ReferenceCountable : public virtual Deletable {
  volatile mutable uint32 ref_field;
  static const uint32     FLOATING_FLAG = 1 << 31;
  inline uint32 ref_get    () const                             { return Lib::atomic_load (&ref_field); }
  inline bool   ref_cas    (uint32 oldv, uint32 newv) const     { return __sync_bool_compare_and_swap (&ref_field, oldv, newv); }
  inline void   fast_ref   () const;
  inline void   fast_unref () const;
  void          real_unref () const;
  RAPICORN_CLASS_NON_COPYABLE (ReferenceCountable);
protected:
  virtual      ~ReferenceCountable ();
  virtual void  delete_this        ();
  virtual void  pre_finalize       ();
  virtual void  finalize           ();
  inline uint32 ref_count          () const                     { return ref_get() & ~FLOATING_FLAG; }
public:
  bool     floating           () const                          { return 0 != (ref_get() & FLOATING_FLAG); }
  void     ref                () const                          { fast_ref(); }
  void     ref_sink           () const;
  bool     finalizing         () const                          { return ref_count() < 1; }
  void     unref              () const                          { fast_unref(); }
  void     ref_diag           (const char *msg = NULL) const;
  explicit ReferenceCountable (uint allow_stack_magic = 0);
  template<class Obj> static Obj& ref      (Obj &obj) { obj.ref();       return obj; }
  template<class Obj> static Obj* ref      (Obj *obj) { obj->ref();      return obj; }
  template<class Obj> static Obj& ref_sink (Obj &obj) { obj.ref_sink();  return obj; }
  template<class Obj> static Obj* ref_sink (Obj *obj) { obj->ref_sink(); return obj; }
  template<class Obj> static void unref    (Obj &obj) { obj.unref(); }
  template<class Obj> static void unref    (Obj *obj) { obj->unref(); }
};
template<class Obj> static Obj& ref      (Obj &obj) { obj.ref();       return obj; }
template<class Obj> static Obj* ref      (Obj *obj) { obj->ref();      return obj; }
template<class Obj> static Obj& ref_sink (Obj &obj) { obj.ref_sink();  return obj; }
template<class Obj> static Obj* ref_sink (Obj *obj) { obj->ref_sink(); return obj; }
template<class Obj> static void unref    (Obj &obj) { obj.unref(); }
template<class Obj> static void unref    (Obj *obj) { obj->unref(); }

// == BaseObject ==
/// Legacy type, will be merged/dissolved into ObjectImpl and ImplicitBase.
class BaseObject : public virtual ReferenceCountable, public virtual Aida::ImplicitBase {
  static void shared_ptr_deleter (BaseObject*);
protected:
  class                    InterfaceMatcher;
  template<class C>  class InterfaceMatch;
  virtual void                 dispose   ();
public:
  template<class Class, typename std::enable_if<std::is_base_of<BaseObject, Class>::value>::type* = nullptr>
  static std::shared_ptr<Class> shared_ptr (Class *object) ///< Wrap BaseObject or derived type into a std::shared_ptr<>().
  {
    return object ? std::shared_ptr<Class> (ref (object), shared_ptr_deleter) : std::shared_ptr<Class>();
  }
  Aida::ImplicitBaseP shared_from_this () { return shared_ptr (this); }
  // keep clear, add new API to ObjectImpl or ObjectIface
};
typedef Aida::PropertyList PropertyList; // import PropertyList from Aida namespace
typedef Aida::Property     Property;     // import Property from Aida namespace
class NullInterface : std::exception {};

struct BaseObject::InterfaceMatcher {
  explicit      InterfaceMatcher (const String &ident) : ident_ (ident), match_found_ (false) {}
  bool          done             () const { return match_found_; }
  virtual  bool match            (BaseObject *object, const String &ident = String()) = 0;
  RAPICORN_CLASS_NON_COPYABLE (InterfaceMatcher);
protected:
  const String &ident_;
  bool          match_found_;
};

template<class C>
struct BaseObject::InterfaceMatch : BaseObject::InterfaceMatcher {
  typedef C&    Result;
  explicit      InterfaceMatch  (const String &ident) : InterfaceMatcher (ident), instance_ (NULL) {}
  C&            result          (bool may_throw) const;
  virtual bool  match           (BaseObject *obj, const String &ident);
protected:
  C            *instance_;
};
template<class C>
struct BaseObject::InterfaceMatch<C&> : InterfaceMatch<C> {
  explicit      InterfaceMatch  (const String &ident) : InterfaceMatch<C> (ident) {}
};
template<class C>
struct BaseObject::InterfaceMatch<C*> : InterfaceMatch<C> {
  typedef C*    Result;
  explicit      InterfaceMatch  (const String &ident) : InterfaceMatch<C> (ident) {}
  C*            result          (bool may_throw) const { return InterfaceMatch<C>::instance_; }
};

template<class C> bool
BaseObject::InterfaceMatch<C>::match (BaseObject *obj, const String &ident)
{
  if (!instance_)
    {
      const String &id = ident_;
      if (id.empty() || id == ident)
        {
          instance_ = dynamic_cast<C*> (obj);
          match_found_ = instance_ != NULL;
        }
    }
  return match_found_;
}

template<class C> C&
BaseObject::InterfaceMatch<C>::result (bool may_throw) const
{
  if (!this->instance_ && may_throw)
    throw NullInterface();
  return *this->instance_;
}

// == Implementation Details ==
inline void
ReferenceCountable::fast_ref () const
{
  // fast-path: use one atomic op and deferred checks
  uint32 old_ref = __sync_fetch_and_add (&ref_field, 1);
  uint32 new_ref = old_ref + 1;                       // ...and_add (,1)
  RAPICORN_ASSERT (old_ref & ~FLOATING_FLAG);         // check dead objects
  RAPICORN_ASSERT (new_ref & ~FLOATING_FLAG);         // check overflow
}

inline void
ReferenceCountable::fast_unref () const
{
  RAPICORN_CFENCE;
  uint32 old_ref = ref_field; // skip read-barrier for fast-path
  if (RAPICORN_LIKELY (old_ref & ~(FLOATING_FLAG | 1)) && // old_ref >= 2
      RAPICORN_LIKELY (ref_cas (old_ref, old_ref - 1)))
    return; // trying fast-path with single atomic op
  real_unref();
}

} // Rapicorn

#endif // __RAPICORN_CORE_OBJECTS_HH__
