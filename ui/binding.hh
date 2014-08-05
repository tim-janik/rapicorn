// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_BINDING_HH__
#define __RAPICORN_BINDING_HH__

#include <ui/widget.hh>

namespace Rapicorn {

class Binding;
typedef std::shared_ptr<Binding> BindingP; ///< Pointer type to manage Binding objects as shared_ptr.

/// Binding class to bind an @a instance @a property to another object's property.
class Binding {
  ObjectIface  &instance_;
  const String  instance_property_;
  ObjectIfaceP  binding_context_;
  const String  binding_path_;
  explicit Binding (ObjectIface &instance, const String &instance_property, const String &binding_path);
  virtual ~Binding ();
  friend class FriendAllocator<Binding>;        // provide make_shared for non-public ctor
public:
  String          instance_property () const     { return instance_property_; }
  void            bind_context      (ObjectIfaceP binding_context);
  void            reset             ();
  static BindingP make_shared       (ObjectIface &instance, const String &instance_property, const String &binding_path)
  { return FriendAllocator<Binding>::make_shared (instance, instance_property, binding_path); }
};

} // Rapicorn

#endif  /* __RAPICORN_BINDING_HH__ */
