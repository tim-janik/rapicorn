// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_AIDA_PROPS_HH__
#define __RAPICORN_AIDA_PROPS_HH__

#include <rcore/enumdefs.hh>
#include <rcore/strings.hh>
#include <rcore/aida.hh>

namespace Rapicorn { namespace Aida {

class PropertyList;

// == PropertyHostInterface ==
class PropertyHostInterface {
protected:
  virtual                    ~PropertyHostInterface() {}
  virtual const PropertyList& _property_list ();
};

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
class PropertyList /// Container structure for property descriptions.
{
  size_t     m_n_properties;
  Property **m_properties;
  void       append_properties (size_t n_props, Property **props, const PropertyList &c0, const PropertyList &c1,
                                const PropertyList &c2, const PropertyList &c3, const PropertyList &c4, const PropertyList &c5,
                                const PropertyList &c6, const PropertyList &c7, const PropertyList &c8, const PropertyList &c9);
public:
  Property** list_properties   (size_t *n_properties) const;
  /*dtor*/  ~PropertyList      ();
  explicit   PropertyList      () : m_n_properties (0), m_properties (NULL) {}
  template<typename Array>
  explicit   PropertyList      (Array &a, const PropertyList &c0 = PropertyList(), const PropertyList &c1 = PropertyList(),
                                const PropertyList &c2 = PropertyList(), const PropertyList &c3 = PropertyList(),
                                const PropertyList &c4 = PropertyList(), const PropertyList &c5 = PropertyList(),
                                const PropertyList &c6 = PropertyList(), const PropertyList &c7 = PropertyList(),
                                const PropertyList &c8 = PropertyList(), const PropertyList &c9 = PropertyList()) :
    m_n_properties (0), m_properties (NULL)
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

template<typename Return, class Class>
inline Return (Class::*
               noconst_getter (Return (Class::*const_getter)() const)
                ) ()
{ return reinterpret_cast<Return (Class::*)()> (const_getter); }

/* --- bool --- */
template<class Class>
struct PropertyBool : Property {
  void (Class::*setter) (bool);
  bool (Class::*getter) ();
  PropertyBool (void (Class::*csetter) (bool), bool (Class::*cgetter) (),
                const char *cident, const char *clabel, const char *cblurb,
                const char *chints);
  virtual void   set_value   (PropertyHostInterface &obj, const String &svalue);
  virtual String get_value   (PropertyHostInterface &obj);
  virtual bool   get_range   (PropertyHostInterface &obj, double &minimum, double &maximum, double &stepping) { return false; }
};
template<class Class> inline Property*
create_property (void (Class::*setter) (bool), bool (Class::*getter) (),
                 const char *ident, const char *label, const char *blurb, const char *hints)
{ return new PropertyBool<Class> (setter, getter, ident, label, blurb, hints); }
template<class Class> inline Property*
create_property (void (Class::*setter) (bool), bool (Class::*getter) () const,
                 const char *ident, const char *label, const char *blurb, const char *hints)
{ return new PropertyBool<Class> (setter, noconst_getter (getter), ident, label, blurb, hints); }

/* --- range --- */
template<class Class, typename Type>
struct PropertyRange : Property {
  Type minimum_value;
  Type maximum_value;
  Type stepping;
  void (Class::*setter) (Type);
  Type (Class::*getter) ();
  PropertyRange (void (Class::*csetter) (Type), Type (Class::*cgetter) (),
                 const char *cident, const char *clabel, const char *cblurb,
                 Type cminimum_value, Type cmaximum_value,
                 Type cstepping, const char *chints);
  virtual void   set_value   (PropertyHostInterface &obj, const String &svalue);
  virtual String get_value   (PropertyHostInterface &obj);
  virtual bool   get_range   (PropertyHostInterface &obj, double &minimum, double &maximum, double &stepping);
};
/* int */
template<class Class> inline Property*
create_property (void (Class::*setter) (int), int (Class::*getter) (),
                 const char *ident, const char *label, const char *blurb,
                 int min_value, int max_value, int stepping, const char *hints)
{ return new PropertyRange<Class,int> (setter, getter, ident, label, blurb, min_value, max_value, stepping, hints); }
template<class Class> inline Property*
create_property (void (Class::*setter) (int), int (Class::*getter) () const,
                 const char *ident, const char *label, const char *blurb,
                 int min_value, int max_value, int stepping, const char *hints)
{ return new PropertyRange<Class,int> (setter, noconst_getter (getter), ident, label, blurb, min_value, max_value, stepping, hints); }
/* int16 */
template<class Class> inline Property*
create_property (void (Class::*setter) (int16), int16 (Class::*getter) (),
                 const char *ident, const char *label, const char *blurb,
                 int16 min_value, int16 max_value, int16 stepping, const char *hints)
{ return new PropertyRange<Class,int16> (setter, getter, ident, label, blurb, min_value, max_value, stepping, hints); }
template<class Class> inline Property*
create_property (void (Class::*setter) (int16), int16 (Class::*getter) () const,
                 const char *ident, const char *label, const char *blurb,
                 int16 min_value, int16 max_value, int16 stepping, const char *hints)
{ return new PropertyRange<Class,int16> (setter, noconst_getter (getter), ident, label, blurb, min_value, max_value, stepping, hints); }
/* uint */
template<class Class> inline Property*
create_property (void (Class::*setter) (uint), uint (Class::*getter) (),
                 const char *ident, const char *label, const char *blurb,
                 uint min_value, uint max_value, uint stepping, const char *hints)
{ return new PropertyRange<Class,uint> (setter, getter, ident, label, blurb, min_value, max_value, stepping, hints); }
template<class Class> inline Property*
create_property (void (Class::*setter) (uint), uint (Class::*getter) () const,
                 const char *ident, const char *label, const char *blurb,
                 uint min_value, uint max_value, uint stepping, const char *hints)
{ return new PropertyRange<Class,uint> (setter, noconst_getter (getter), ident, label, blurb, min_value, max_value, stepping, hints); }
/* uint16 */
template<class Class> inline Property*
create_property (void (Class::*setter) (uint16), uint16 (Class::*getter) (),
                 const char *ident, const char *label, const char *blurb,
                 uint16 min_value, uint16 max_value, uint16 stepping, const char *hints)
{ return new PropertyRange<Class,uint16> (setter, getter, ident, label, blurb, min_value, max_value, stepping, hints); }
template<class Class> inline Property*
create_property (void (Class::*setter) (uint16), uint16 (Class::*getter) () const,
                 const char *ident, const char *label, const char *blurb,
                 uint16 min_value, uint16 max_value, uint16 stepping, const char *hints)
{ return new PropertyRange<Class,uint16> (setter, noconst_getter (getter), ident, label, blurb, min_value, max_value, stepping, hints); }
/* float */
template<class Class> inline Property*
create_property (void (Class::*setter) (float), float (Class::*getter) (),
                 const char *ident, const char *label, const char *blurb,
                 float min_value, float max_value, float stepping, const char *hints)
{ return new PropertyRange<Class,float> (setter, getter, ident, label, blurb, min_value, max_value, stepping, hints); }
template<class Class> inline Property*
create_property (void (Class::*setter) (float), float (Class::*getter) () const,
                 const char *ident, const char *label, const char *blurb,
                 float min_value, float max_value, float stepping, const char *hints)
{ return new PropertyRange<Class,float> (setter, noconst_getter (getter), ident, label, blurb, min_value, max_value, stepping, hints); }
/* double */
template<class Class> inline Property*
create_property (void (Class::*setter) (double), double (Class::*getter) (),
                 const char *ident, const char *label, const char *blurb,
                 double min_value, double max_value, double stepping, const char *hints)
{ return new PropertyRange<Class,double> (setter, getter, ident, label, blurb, min_value, max_value, stepping, hints); }
template<class Class> inline Property*
create_property (void (Class::*setter) (double), double (Class::*getter) () const,
                 const char *ident, const char *label, const char *blurb,
                 double min_value, double max_value, double stepping, const char *hints)
{ return new PropertyRange<Class,double> (setter, noconst_getter (getter), ident, label, blurb, min_value, max_value, stepping, hints); }

/* --- string --- */
template<class Class>
struct PropertyString : Property {
  void   (Class::*setter) (const String&);
  String (Class::*getter) ();
  PropertyString (void (Class::*csetter) (const String&), String (Class::*cgetter) (),
                  const char *cident, const char *clabel, const char *cblurb,
                  const char *chints);
  virtual void   set_value   (PropertyHostInterface &obj, const String &svalue);
  virtual String get_value   (PropertyHostInterface &obj);
  virtual bool   get_range   (PropertyHostInterface &obj, double &minimum, double &maximum, double &stepping) { return false; }
};
template<class Class> inline Property*
create_property (void (Class::*setter) (const String&), String (Class::*getter) (),
                 const char *ident, const char *label, const char *blurb, const char *hints)
{ return new PropertyString<Class> (setter, getter, ident, label, blurb, hints); }
template<class Class> inline Property*
create_property (void (Class::*setter) (const String&), String (Class::*getter) () const,
                 const char *ident, const char *label, const char *blurb, const char *hints)
{ return new PropertyString<Class> (setter, noconst_getter (getter), ident, label, blurb, hints); }

/* --- enum --- */
template<class Class, typename Type>
struct PropertyEnum : Property {
  const EnumClass &enum_class;
  void (Class::*setter) (Type);
  Type (Class::*getter) ();
  PropertyEnum (void (Class::*csetter) (Type), Type (Class::*cgetter) (),
                const char *cident, const char *clabel, const char *cblurb,
                const EnumClass &eclass, const char *chints);
  virtual void   set_value   (PropertyHostInterface &obj, const String &svalue);
  virtual String get_value   (PropertyHostInterface &obj);
  virtual bool   get_range   (PropertyHostInterface &obj, double &minimum, double &maximum, double &stepping) { return false; }
};
template<class Class, typename Type> inline Property*
create_property (void (Class::*setter) (Type), Type (Class::*getter) (),
                 const char *ident, const char *label, const char *blurb, const char *hints)
{
  static const EnumType<Type> enum_class;
  return new PropertyEnum<Class,Type> (setter, getter, ident, label, blurb, enum_class, hints);
}
template<class Class, typename Type> inline Property*
create_property (void (Class::*setter) (Type), Type (Class::*getter) () const,
                 const char *ident, const char *label, const char *blurb, const char *hints)
{
  static const EnumType<Type> enum_class;
  return new PropertyEnum<Class,Type> (setter, noconst_getter (getter), ident, label, blurb, enum_class, hints);
}

/* --- implementations --- */
/* bool property implementation */
template<class Class>
PropertyBool<Class>::PropertyBool (void (Class::*csetter) (bool), bool (Class::*cgetter) (),
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
PropertyRange<Class,Type>::PropertyRange (void (Class::*csetter) (Type), Type (Class::*cgetter) (),
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
  RAPICORN_ASSERT (minimum_value <= maximum_value);
  RAPICORN_ASSERT (minimum_value + stepping <= maximum_value);
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
PropertyString<Class>::PropertyString (void (Class::*csetter) (const String&), String (Class::*cgetter) (),
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
PropertyEnum<Class,Type>::PropertyEnum (void (Class::*csetter) (Type), Type (Class::*cgetter) (),
                                        const char *cident, const char *clabel, const char *cblurb,
                                        const EnumClass &eclass, const char *chints) :
  Property (cident, clabel, cblurb, chints),
  enum_class (eclass),
  setter (csetter),
  getter (cgetter)
{}

template<class Class, typename Type> void
PropertyEnum<Class,Type>::set_value (PropertyHostInterface &obj, const String &svalue)
{
  String error_string;
  uint64 value = enum_class.parse (svalue.c_str(), &error_string);
  if (0 && error_string.size() && !value && string_has_int (svalue))
    value = enum_class.constrain (string_to_int (svalue));
  else if (error_string.size())
    critical ("%s: invalid enum value name '%s': %s", RAPICORN_SIMPLE_FUNCTION, enum_class.enum_name(), error_string.c_str());
  Type v = Type (value);
  Class *instance = dynamic_cast<Class*> (&obj);
  (instance->*setter) (v);
}

template<class Class, typename Type> String
PropertyEnum<Class,Type>::get_value (PropertyHostInterface &obj)
{
  Class *instance = dynamic_cast<Class*> (&obj);
  Type v = (instance->*getter) ();
  return enum_class.string (v);
}

} } // Rapicorn::Aida

#endif  // __RAPICORN_AIDA_PROPS_HH__
