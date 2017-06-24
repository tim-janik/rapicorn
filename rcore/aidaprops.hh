// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_AIDA_PROPS_HH__
#define __RAPICORN_AIDA_PROPS_HH__

#include <rcore/aida.hh>
#include <rcore/strings.hh>

namespace Rapicorn { namespace Aida {

// == Parameter ==
/// Parameter encapsulates the logic and data involved in editing an object property.
class Parameter {
  typedef std::function<void (const String &what)>           ChangedFunction;
  const String                                               name_;
  const Any                                                  instance_; // keeps Handle/Iface reference count
  const std::function<void (const Any&)>                     setter_;
  const std::function<Any ()>                                getter_;
  const std::function<String (const String&, const String&)> getaux_;
  const std::function<size_t (const ChangedFunction&)>       connect_;
  const std::function<bool (size_t)>                         disconnect_;
  vector<size_t>                                             ids_;
  class Connector {
    Parameter &parameter_;
    Connector& operator= (const Connector&) = delete;
  public:
    /*copy*/   Connector (const Connector &con) : parameter_ (con.parameter_) {}
    explicit   Connector (Parameter &parameter) : parameter_ (parameter) {}
    /// Operator to remove a signal handler through its connection ID, returns if a handler was removed.
    bool       operator-= (size_t connection_id);
    /// Operator to add a new function or lambda as changed() signal handler, returns a handler connection ID.
    size_t     operator+= (const ChangedFunction &callback);
  };
protected:
  /// Helper to implement get_aux().
  static String find_aux (const std::vector<String> &vec, const String &field_name, const String &key, const String &fallback);
public:
  virtual ~Parameter();
  /// Create a Parameter to wrap RemoteHandle accessors for plain values (bool, int, float).
  template<class Klass, class Value, REQUIRES< IsRemoteHandleDerived<Klass>::value > = true>
  Parameter (Klass &instance, const String &name, void (Klass::*setter) (Value), Value (Klass::*getter) () const) :
    name_ (name), instance_ (({ Any a; a.set (instance); a; })),
    setter_ ([instance, setter] (const Any &any) -> void { return (Klass (instance) .* setter) (any.get<Value>()); }),
    getter_ ([instance, getter] ()               -> Any  { Any a; a.set ((instance .* getter) ()); return a; }),
    getaux_ ([instance, name] (const String &k, const String &f) -> String { return find_aux (instance.__aida_aux_data__(), name, k, f); }),
    connect_ ([instance] (const ChangedFunction &cb) -> size_t { return Klass (instance).sig_changed() += cb; }),
    disconnect_ ([instance] (size_t id) -> bool { return Klass (instance).sig_changed() -= id; })
  {}
  /// Create a Parameter to wrap ImplicitBase accessors for plain values (bool, int, float).
  template<class Klass, class Value, REQUIRES< IsImplicitBaseDerived<Klass>::value > = true>
  Parameter (Klass &instance, const String &name, void (Klass::*setter) (Value), Value (Klass::*getter) () const) :
    name_ (name), instance_ (({ Any a; a.set (instance); a; })),
    setter_ ([&instance, setter] (const Any &any) -> void { return (instance .* setter) (any.get<Value>()); }),
    getter_ ([&instance, getter] ()               -> Any  { Any a; a.set ((instance .* getter) ()); return a; }),
    getaux_ ([&instance, name] (const String &k, const String &f) -> String { return find_aux (instance.__aida_aux_data__(), name, k, f); }),
    connect_ ([&instance] (const ChangedFunction &cb) -> size_t { return instance.sig_changed() += cb; }),
    disconnect_ ([&instance] (size_t id) -> bool { return instance.sig_changed() -= id; })
  {}
  /// Create a Parameter to wrap RemoteHandle accessors for struct values (std::string, record).
  template<class Klass, class Value, REQUIRES< IsRemoteHandleDerived<Klass>::value > = true>
  Parameter (Klass &instance, const String &name, void (Klass::*setter) (const Value&), Value (Klass::*getter) () const) :
    name_ (name), instance_ (({ Any a; a.set (instance); a; })),
    setter_ ([instance, setter] (const Any &any) -> void { return (Klass (instance) .* setter) (any.get<Value>()); }),
    getter_ ([instance, getter] ()               -> Any  { Any a; a.set ((instance .* getter) ()); return a; }),
    getaux_ ([instance, name] (const String &k, const String &f) -> String { return find_aux (instance.__aida_aux_data__(), name, k, f); }),
    connect_ ([instance] (const ChangedFunction &cb) -> size_t { return Klass (instance).sig_changed() += cb; }),
    disconnect_ ([instance] (size_t id) -> bool { return Klass (instance).sig_changed() -= id; })
  {}
  /// Create a Parameter to wrap ImplicitBase accessors for struct values (std::string, record).
  template<class Klass, class Value, REQUIRES< IsImplicitBaseDerived<Klass>::value > = true>
  Parameter (Klass &instance, const String &name, void (Klass::*setter) (const Value&), Value (Klass::*getter) () const) :
    name_ (name), instance_ (({ Any a; a.set (instance); a; })),
    setter_ ([&instance, setter] (const Any &any) -> void { return (instance .* setter) (any.get<Value>()); }),
    getter_ ([&instance, getter] ()               -> Any  { Any a; a.set ((instance .* getter) ()); return a; }),
    getaux_ ([&instance, name] (const String &k, const String &f) -> String { return find_aux (instance.__aida_aux_data__(), name, k, f); }),
    connect_ ([&instance] (const ChangedFunction &cb) -> size_t { return instance.sig_changed() += cb; }),
    disconnect_ ([&instance] (size_t id) -> bool { return instance.sig_changed() -= id; })
  {}
  /// Create a Parameter to wrap (ImplicitBase derived) interface accessors for instance/interface values.
  template<class Klass, class Value>
  Parameter (Klass &instance, const String &name, void (Klass::*setter) (Value*), std::shared_ptr<Value> (Klass::*getter) () const) :
    name_ (name), instance_ (({ Any a; a.set (instance); a; })),
    setter_ ([&instance, setter] (const Any &any) -> void { return (instance .* setter) (any.get<std::shared_ptr<Value> >().get()); }),
    getter_ ([&instance, getter] ()               -> Any  { Any a; a.set ((instance .* getter) ()); return a; }),
    getaux_ ([&instance, name] (const String &k, const String &f) -> String { return find_aux (instance.__aida_aux_data__(), name, k, f); }),
    connect_ ([&instance] (const ChangedFunction &cb) -> size_t { return instance.sig_changed() += cb; }),
    disconnect_ ([&instance] (size_t id) -> bool { return instance.sig_changed() -= id; })
  {}
  String                                  name    () const;          ///< Retrieve the wrapped value's field or property name.
  void                                    set     (const Any &any);  ///< Set the wrapped value to the contents of @a any.
  Any                                     get     () const;          ///< Retrieve the wrapped value as @a Any.
  /// Fetch auxillary parameter information.
  template<typename Value = String> Value get_aux (const String &key, const String &fallback = "")
  { return string_to_type<Value> (getaux_ (key, fallback)); }
  /// Signal for change notifications, used to notify property changes.
  Connector     sig_changed()                               { return Connector (*this); }
  class ListVisitor;
};

/// Visitor used in conjunction with __accept_accessor__() to build a Parameter list from instance properties.
class Parameter::ListVisitor {
  std::vector<Parameter>  default_vector_;
  std::vector<Parameter> &parameters_;
public:
  /// Construct a parameter list visitor to add parameters to @a parameter_vector.
  explicit ListVisitor (std::vector<Parameter> &parameter_vector) :
    parameters_ (parameter_vector)
  {}
  /// Construct a parameter list visitor, access the resulting parameter list through parameters().
  explicit ListVisitor () : parameters_ (default_vector_) {}
  /// Visitation method called for each @a Klass @a instance property accessors.
  template<class Klass, typename SetterType, typename GetterType> void
  operator() (Klass &instance, const char *field_name, void (Klass::*setter) (SetterType), GetterType (Klass::*getter) () const)
  {
    parameters_.push_back (Parameter (instance, field_name, setter, getter));
  }
  /// Retrieve the resulting parameter list.
  std::vector<Parameter>& parameters() { return parameters_; }
};


// == PropertyHostInterface ==
typedef ImplicitBase PropertyHostInterface;

// == Property ==
class Property {
protected:
  virtual ~Property();
public:
  const char *ident;
  char       *label;
  char       *blurb;
  char       *hints;
  Property (const char *cident, const char *clabel, const char *cblurb, const char *chints);
  virtual void   set_value   (PropertyHostInterface &obj, const String &svalue) = 0;
  virtual String get_value   (PropertyHostInterface &obj) = 0;
  virtual bool   get_range   (PropertyHostInterface &obj, double &minimum, double &maximum, double &stepping) = 0;
  bool           readable    () const;
  bool           writable    () const;
};

// == PropertyList ==
struct PropertyList /// Container structure for property descriptions.
{
  typedef Aida::Property Property; // make Property available as class member
private:
  size_t     n_properties_;
  Property **properties_;
  void       append_properties (size_t n_props, Property **props, const PropertyList &c0, const PropertyList &c1,
                                const PropertyList &c2, const PropertyList &c3, const PropertyList &c4, const PropertyList &c5,
                                const PropertyList &c6, const PropertyList &c7, const PropertyList &c8, const PropertyList &c9);
public:
  Property** list_properties   (size_t *n_properties) const;
  /*dtor*/  ~PropertyList      ();
  explicit   PropertyList      () : n_properties_ (0), properties_ (NULL) {}
  template<typename Array>
  explicit   PropertyList      (Array &a, const PropertyList &c0 = PropertyList(), const PropertyList &c1 = PropertyList(),
                                const PropertyList &c2 = PropertyList(), const PropertyList &c3 = PropertyList(),
                                const PropertyList &c4 = PropertyList(), const PropertyList &c5 = PropertyList(),
                                const PropertyList &c6 = PropertyList(), const PropertyList &c7 = PropertyList(),
                                const PropertyList &c8 = PropertyList(), const PropertyList &c9 = PropertyList()) :
    n_properties_ (0), properties_ (NULL)
  {
    const size_t n_props = sizeof (a) / sizeof (a[0]);
    Property *props[n_props];
    for (size_t i = 0; i < sizeof (a) / sizeof (a[0]); i++)
      props[i] = a[i];
    append_properties (n_props, props, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9);
  }
};

// == Property Creation ==
#define RAPICORN_AIDA_PROPERTY(Type, accessor, label, blurb, ...)   \
  Rapicorn::Aida::create_property (&Type::accessor, &Type::accessor, #accessor, label, blurb, __VA_ARGS__)

#define RAPICORN_AIDA_PROPERTY_CHAIN(first,...)                    (*({ \
  static Property *__dummy_[] = {};                                     \
  static const PropertyList property_list (__dummy_, first, __VA_ARGS__); \
  &property_list; }))

/* --- bool --- */
template<class Class>
struct PropertyBool : Property {
  void (Class::*setter) (bool);
  bool (Class::*getter) () const;
  PropertyBool (void (Class::*csetter) (bool), bool (Class::*cgetter) () const,
                const char *cident, const char *clabel, const char *cblurb,
                const char *chints);
  virtual void   set_value   (PropertyHostInterface &obj, const String &svalue);
  virtual String get_value   (PropertyHostInterface &obj);
  virtual bool   get_range   (PropertyHostInterface &obj, double &minimum, double &maximum, double &stepping) { return false; }
};
template<class Class> inline Property*
create_property (void (Class::*setter) (bool), bool (Class::*getter) () const,
                 const char *ident, const char *label, const char *blurb, const char *hints)
{ return new PropertyBool<Class> (setter, getter, ident, label, blurb, hints); }

/* --- range --- */
template<class Class, typename Type>
struct PropertyRange : Property {
  Type minimum_value;
  Type maximum_value;
  Type stepping;
  void (Class::*setter) (Type);
  Type (Class::*getter) () const;
  PropertyRange (void (Class::*csetter) (Type), Type (Class::*cgetter) () const,
                 const char *cident, const char *clabel, const char *cblurb,
                 Type cminimum_value, Type cmaximum_value,
                 Type cstepping, const char *chints);
  virtual void   set_value   (PropertyHostInterface &obj, const String &svalue);
  virtual String get_value   (PropertyHostInterface &obj);
  virtual bool   get_range   (PropertyHostInterface &obj, double &minimum, double &maximum, double &stepping);
};
/* int */
template<class Class> inline Property*
create_property (void (Class::*setter) (int), int (Class::*getter) () const,
                 const char *ident, const char *label, const char *blurb,
                 int min_value, int max_value, int stepping, const char *hints)
{ return new PropertyRange<Class,int> (setter, getter, ident, label, blurb, min_value, max_value, stepping, hints); }
/* int64 */
template<class Class> inline Property*
create_property (void (Class::*setter) (int64), int64 (Class::*getter) () const,
                 const char *ident, const char *label, const char *blurb,
                 int64 min_value, int64 max_value, int64 stepping, const char *hints)
{ return new PropertyRange<Class,int64> (setter, getter, ident, label, blurb, min_value, max_value, stepping, hints); }
/* double */
template<class Class> inline Property*
create_property (void (Class::*setter) (double), double (Class::*getter) () const,
                 const char *ident, const char *label, const char *blurb,
                 double min_value, double max_value, double stepping, const char *hints)
{ return new PropertyRange<Class,double> (setter, getter, ident, label, blurb, min_value, max_value, stepping, hints); }

/* --- string --- */
template<class Class>
struct PropertyString : Property {
  void   (Class::*setter) (const String&);
  String (Class::*getter) () const;
  PropertyString (void (Class::*csetter) (const String&), String (Class::*cgetter) () const,
                  const char *cident, const char *clabel, const char *cblurb,
                  const char *chints);
  virtual void   set_value   (PropertyHostInterface &obj, const String &svalue);
  virtual String get_value   (PropertyHostInterface &obj);
  virtual bool   get_range   (PropertyHostInterface &obj, double &minimum, double &maximum, double &stepping) { return false; }
};
template<class Class> inline Property*
create_property (void (Class::*setter) (const String&), String (Class::*getter) () const,
                 const char *ident, const char *label, const char *blurb, const char *hints)
{ return new PropertyString<Class> (setter, getter, ident, label, blurb, hints); }

// == Enum Properties ==
template<class Class, typename Type>
struct PropertyEnum : Property {
  const EnumInfo enum_;
  void (Class::*setter) (Type);
  Type (Class::*getter) () const;
  PropertyEnum (void (Class::*csetter) (Type), Type (Class::*cgetter) () const,
                const char *cident, const char *clabel, const char *cblurb,
                const EnumInfo enuminfo, const char *chints);
  virtual void   set_value   (PropertyHostInterface &obj, const String &svalue);
  virtual String get_value   (PropertyHostInterface &obj);
  virtual bool   get_range   (PropertyHostInterface &obj, double &minimum, double &maximum, double &stepping) { return false; }
};
template<class Class, typename Type,
         typename std::enable_if<std::is_enum<Type>::value>::type* = nullptr> inline Property*
create_property (void (Class::*setter) (Type), Type (Class::*getter) () const,
                 const char *ident, const char *label, const char *blurb, const char *hints)
{
  return new PropertyEnum<Class,Type> (setter, getter, ident, label, blurb, enum_info<Type>(), hints);
}

/* --- implementations --- */
/* bool property implementation */
template<class Class>
PropertyBool<Class>::PropertyBool (void (Class::*csetter) (bool), bool (Class::*cgetter) () const,
                                   const char *cident, const char *clabel, const char *cblurb,
                                   const char *chints) :
  Property (cident, clabel, cblurb, chints),
  setter (csetter),
  getter (cgetter)
{}

template<class Class> void
PropertyBool<Class>::set_value (PropertyHostInterface &obj, const String &svalue)
{
  bool b = string_to_bool (svalue);
  Class *instance = dynamic_cast<Class*> (&obj);
  (instance->*setter) (b);
}

template<class Class> String
PropertyBool<Class>::get_value (PropertyHostInterface &obj)
{
  Class *instance = dynamic_cast<Class*> (&obj);
  bool b = (instance->*getter) ();
  return string_from_bool (b);
}

/* range property implementation */
template<class Class, typename Type>
PropertyRange<Class,Type>::PropertyRange (void (Class::*csetter) (Type), Type (Class::*cgetter) () const,
                                          const char *cident, const char *clabel, const char *cblurb,
                                          Type cminimum_value, Type cmaximum_value,
                                          Type cstepping, const char *chints) :
  Property (cident, clabel, cblurb, chints),
  minimum_value (cminimum_value),
  maximum_value (cmaximum_value),
  stepping (cstepping),
  setter (csetter),
  getter (cgetter)
{
  AIDA_ASSERT_RETURN (minimum_value <= maximum_value);
  AIDA_ASSERT_RETURN (minimum_value + stepping <= maximum_value);
}

template<class Class, typename Type> void
PropertyRange<Class,Type>::set_value (PropertyHostInterface &obj, const String &svalue)
{
  Type v = string_to_type<Type> (svalue);
  Class *instance = dynamic_cast<Class*> (&obj);
  (instance->*setter) (v);
}

template<class Class, typename Type> String
PropertyRange<Class,Type>::get_value (PropertyHostInterface &obj)
{
  Class *instance = dynamic_cast<Class*> (&obj);
  Type v = (instance->*getter) ();
  return string_from_type<Type> (v);
}

template<class Class, typename Type> bool
PropertyRange<Class,Type>::get_range (PropertyHostInterface &obj, double &minimum, double &maximum, double &vstepping)
{
  minimum = minimum_value, maximum = maximum_value, vstepping = stepping;
  return true;
}

/* string property implementation */
template<class Class>
PropertyString<Class>::PropertyString (void (Class::*csetter) (const String&), String (Class::*cgetter) () const,
                                       const char *cident, const char *clabel, const char *cblurb,
                                       const char *chints) :
  Property (cident, clabel, cblurb, chints),
  setter (csetter),
  getter (cgetter)
{}

template<class Class> void
PropertyString<Class>::set_value (PropertyHostInterface &obj, const String &svalue)
{
  Class *instance = dynamic_cast<Class*> (&obj);
  (instance->*setter) (svalue);
}

template<class Class> String
PropertyString<Class>::get_value (PropertyHostInterface &obj)
{
  Class *instance = dynamic_cast<Class*> (&obj);
  return (instance->*getter) ();
}

/* enum property implementation */
template<class Class, typename Type>
PropertyEnum<Class,Type>::PropertyEnum (void (Class::*csetter) (Type), Type (Class::*cgetter) () const,
                                        const char *cident, const char *clabel, const char *cblurb,
                                        const EnumInfo enuminfo, const char *chints) :
  Property (cident, clabel, cblurb, chints),
  enum_ (enuminfo),
  setter (csetter),
  getter (cgetter)
{}

template<class Class, typename Type> void
PropertyEnum<Class,Type>::set_value (PropertyHostInterface &obj, const String &svalue)
{
  String error_string;
  EnumValue ev = enum_.find_value (svalue);
  const char *const enum_value_name = ev.ident;
  AIDA_ASSERT_RETURN (enum_value_name != NULL);
  Type v = Type (ev.value);
  Class *instance = dynamic_cast<Class*> (&obj);
  (instance->*setter) (v);
}

template<class Class, typename Type> String
PropertyEnum<Class,Type>::get_value (PropertyHostInterface &obj)
{
  Class *instance = dynamic_cast<Class*> (&obj);
  Type v = (instance->*getter) ();
  EnumValue ev = enum_.find_value (v);
  const char *const enum_value_name = ev.ident;
  AIDA_ASSERT_RETURN (enum_value_name != NULL, "");
  return ev.ident ? ev.ident : "";
}

} } // Rapicorn::Aida

#endif  // __RAPICORN_AIDA_PROPS_HH__
