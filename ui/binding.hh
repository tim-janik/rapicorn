// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_BINDING_HH__
#define __RAPICORN_BINDING_HH__

#include <ui/widget.hh>

namespace Rapicorn {

class Binding;
typedef std::shared_ptr<Binding> BindingP; ///< Pointer type to manage Binding objects as shared_ptr.

class BindableRelayImpl : public virtual BindableRelayIface, public virtual BindableIface {
  struct Request {
    enum Type { NONE, SET, GET, CACHE };
    Type         type;
    uint64       nonce;
    String       path;
    Any          any;
    explicit     Request (Type t, uint64 n, const String &p, const Any &a = Any());
  };
  vector<Request> requests_;
protected:
  explicit      BindableRelayImpl       ();
  virtual      ~BindableRelayImpl       ();
private: // BindableIface
  virtual void  bindable_set            (const String &bpath, const Any &any) override;
  virtual void  bindable_get            (const String &bpath, Any &any) override;
public: // BindableRelayIface
  virtual void  report_notify           (const String &bpath) override;
  virtual void  report_result           (int64 nonce, const Any &result, const String &error) override;
};

/// Binding class to bind an @a instance @a property to another object's property.
class Binding {
  ObjectImpl    &instance_;
  const String   instance_property_;
  size_t         instance_sigid_ = 0;
  BindableIfaceP binding_context_;
  const String   binding_path_;
  size_t         binding_sigid_ = 0;
  explicit Binding (ObjectImpl &instance, const String &instance_property, const String &binding_path);
  virtual ~Binding ();
  friend class FriendAllocator<Binding>;        // provide make_shared for non-public ctor
  void            bindable_to_object ();
  void            object_to_bindable ();
  void            object_notify      (const String &property);
  void            bindable_notify    (const String &property);
public:
  String          instance_property () const     { return instance_property_; }
  void            bind_context      (ObjectIfaceP binding_context);
  void            reset             ();
  static BindingP make_shared       (ObjectImpl &instance, const String &instance_property, const String &binding_path)
  { return FriendAllocator<Binding>::make_shared (instance, instance_property, binding_path); }
};

} // Rapicorn

#endif  /* __RAPICORN_BINDING_HH__ */
