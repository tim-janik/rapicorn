// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_BINDABLE_HH__
#define __RAPICORN_BINDABLE_HH__

#include <rcore/utilities.hh>
#include <rcore/strings.hh>

namespace Rapicorn {

/// BindablePath - A dot separated list of objects and property names to identify a bindable property.
struct BindablePath {
  const std::string        path;
  std::vector<std::string> plist;
  bool                     match    (const std::string &name) const;
};

/// BindableIface - An interface implemented by custom user objects to be accessible to data bindings.
struct BindableIface {
  void          bindable_notify   (const std::string  &name) const;                ///< Notify listeners that property @a name changed.
  virtual void  bindable_set      (const BindablePath &bpath, const Any &any) = 0; ///< Set the property matched by @a path to @a any.
  virtual void  bindable_get      (const BindablePath &bpath, Any &any) = 0;       ///< Get the property matched by @a path as @a any.
};
typedef std::shared_ptr<BindableIface> BindableIfaceP;

/// Global function template to be overridden to implement non-intrusive property getters via BindableAdaptor.
template<class Source> void
bindable_accessor_get (const BindableIface &paccessible, const BindablePath &bpath, Any &any, Source&);

/// Global function template to be overridden to implement non-intrusive property setters via BindableAdaptor.
template<class Source> void
bindable_accessor_set (const BindableIface &paccessible, const BindablePath &bpath, const Any &any, Source&);

/// BindableAdaptorBase - Base template for BindableAdaptor.
class BindableAdaptorBase : virtual public BindableIface {
public:
  virtual ~BindableAdaptorBase () {}
};

/// BindableAdaptor - Glue template to implement the BindableIface for client objects that don't derive BindableIface.
template<class Source> class BindableAdaptor {
  static_assert (IsSharedPtr<Source>::value || IsWeakPtr<Source>::value,
                 "Rapicorn::BindableAdaptor<T>: type T is not std::shared_ptr or std::weak_ptr");
};

/// BindableAdaptor specialisation for std::shared_ptr<>.
template<class Source>
class BindableAdaptor<std::shared_ptr<Source> > : virtual public BindableAdaptorBase {
protected:
  std::shared_ptr<Source> self_;
public:
  /*ctor*/ BindableAdaptor (std::shared_ptr<Source> sp) : self_ (sp) {}
protected: // BindableIface
  virtual void
  bindable_set (const BindablePath &bpath, const Any &any)
  {
    bindable_accessor_set (*this, bpath, any, *self_.get());
  }
  virtual void
  bindable_get (const BindablePath &bpath, Any &any)
  {
    bindable_accessor_get (*this, bpath, any, *self_.get());
  }
};

/// BindableAdaptor specialisation for std::weak_ptr<>.
template<class Source>
class BindableAdaptor<std::weak_ptr<Source> > : virtual public BindableAdaptorBase {
protected:
  std::weak_ptr<Source> self_;
public:
  /*ctor*/ BindableAdaptor (std::weak_ptr<Source> sp) : self_ (sp) {}
protected: // BindableIface
  virtual void
  bindable_set (const BindablePath &bpath, const Any &any)
  {
    std::shared_ptr<Source> sp = self_.lock();
    Source *source = sp.get();
    if (source)
      bindable_accessor_set (*this, bpath, any, *source);
  }
  virtual void
  bindable_get (const BindablePath &bpath, Any &any)
  {
    std::shared_ptr<Source> sp = self_.lock();
    Source *source = sp.get();
    if (source)
      bindable_accessor_get (*this, bpath, any, *source);
  }
};

/// BinadableAccessor - A client side driver object that uses the BindableIface API on behalf of a BindableRelay.
class BinadableAccessor {
  BindableAdaptorBase *adaptor_; // type erasure
  BindableIface    &pa_;
  static void   notify_property_change  (const BindableIface *pa, const String &name);
  friend struct BindableIface;
public:
  typedef std::vector<std::string> StringList;

  template<class Source,
           typename std::enable_if<!std::is_base_of<BindableIface, Source>::value>::type* = nullptr>
  explicit BinadableAccessor (Source &source) : adaptor_ (new BindableAdaptor<Source> (source)), pa_ (*adaptor_) {}
  explicit BinadableAccessor (BindableIface &pa) : adaptor_ (NULL), pa_ (pa)        {}
  virtual ~BinadableAccessor ()                                                          { if (adaptor_) delete adaptor_; }
  Any        get_property            (const String &name);
  void       set_property            (const String &name, const Any &any);
  void       notify_property         (const String &name);
  StringList list_propertis          ();
};

} // Rapicorn

#endif  // __RAPICORN_BINDABLE_HH__
