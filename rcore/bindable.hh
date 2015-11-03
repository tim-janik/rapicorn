// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_BINDABLE_HH__
#define __RAPICORN_BINDABLE_HH__

#include <rcore/utilities.hh>
#include <rcore/strings.hh>

namespace Rapicorn {

/// BindableIface - An interface implemented by custom user objects to be accessible to data bindings.
struct BindableIface {
  virtual     ~BindableIface   ();
  bool         bindable_match  (const String &bpath, const String &name) const; ///< Match the binding path segments against @a name.
  virtual void bindable_set    (const String &bpath, const Any &any) = 0; ///< Set the property matched by @a path to @a any.
  virtual void bindable_get    (const String &bpath, Any &any) = 0;       ///< Get the property matched by @a path as @a any.
  typedef Signal<void (const String &property_name)> BindableNotifySignal;
  BindableNotifySignal::
  Connector    sig_bindable_notify () const;                   ///< Signal to notify listeners, see bindable_notify().
  void         bindable_notify     (const String &name) const; ///< Notify listeners that property @a name changed.
};
typedef std::shared_ptr<BindableIface> BindableIfaceP;

/// Global function template to be overridden to implement non-intrusive property getters via BindableAdaptor.
template<class Source> void
bindable_accessor_get (const BindableIface &paccessible, const String &bpath, Any &any, Source&);

/// Global function template to be overridden to implement non-intrusive property setters via BindableAdaptor.
template<class Source> void
bindable_accessor_set (const BindableIface &paccessible, const String &bpath, const Any &any, Source&);

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
  bindable_set (const String &bpath, const Any &any)
  {
    bindable_accessor_set (*this, bpath, any, *self_.get());
  }
  virtual void
  bindable_get (const String &bpath, Any &any)
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
  bindable_set (const String &bpath, const Any &any)
  {
    std::shared_ptr<Source> sp = self_.lock();
    Source *source = sp.get();
    if (source)
      bindable_accessor_set (*this, bpath, any, *source);
  }
  virtual void
  bindable_get (const String &bpath, Any &any)
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
  BindableIface    &bindable_;
  size_t            notify_id_;
  friend struct BindableIface;
  void          ctor                    ();
public:
  template<class Source,
           typename std::enable_if<!std::is_base_of<BindableIface, Source>::value>::type* = nullptr>
  explicit BinadableAccessor (Source    &source) : adaptor_ (new BindableAdaptor<Source> (source)), bindable_ (*adaptor_) { ctor(); }
  explicit BinadableAccessor (BindableIface &bi) : adaptor_ (NULL), bindable_ (bi)                                        { ctor(); }
  virtual ~BinadableAccessor ();
  Any        get_property            (const String &name);
  void       set_property            (const String &name, const Any &any);
  void       notify_property         (const String &name);
};

} // Rapicorn

#endif  // __RAPICORN_BINDABLE_HH__
