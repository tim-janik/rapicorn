/* Rapicorn
 * Copyright (C) 2005 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __RAPICORN_PROPERTIES_HH__
#define __RAPICORN_PROPERTIES_HH__

#include <rapicorn/enumdefs.hh>

namespace Rapicorn {

struct Property : ReferenceCountImpl {
  const char *ident;
  char       *label;
  char       *blurb;
  char       *hints;
  Property (const char *cident, const char *clabel, const char *cblurb, const char *chints);
  virtual void   set_value   (Deletable *obj, const String &svalue) = 0;
  virtual String get_value   (Deletable *obj) = 0;
  virtual String get_default (Deletable *obj) = 0;
  virtual bool   get_range   (Deletable *obj, double &minimum, double &start_value, double &maximum, double &stepping) = 0;
protected:
  virtual ~Property();
};

struct PropertyList {
  uint       n_properties;
  Property **properties;
  PropertyList () : n_properties (0), properties (NULL) {}
  template<typename Array>
  PropertyList (Array              &a,
                const PropertyList &chain = PropertyList()) :
    n_properties (0),
    properties (NULL)
  {
    uint array_length = sizeof (a) / sizeof (a[0]);
    n_properties = array_length + chain.n_properties;
    properties = new Property*[n_properties];
    uint i;
    for (i = 0; i < array_length; i++)
      properties[i] = ref_sink (a[i]);
    for (; i < n_properties; i++)
      properties[i] = ref (chain.properties[i - array_length]);
  }
  ~PropertyList()
  {
    for (uint i = 0; i < n_properties; i++)
      unref (properties[i]);
    delete[] properties;
  }
};

#define RAPICORN_MakeProperty(Type, accessor, label, blurb, ...)   \
  create_property (&Type::accessor, &Type::accessor, #accessor, label, blurb, __VA_ARGS__)

template<typename Return, class Class>
inline Return (Class::*
               noconst_getter (Return (Class::*const_getter)() const)
                ) ()
{ return reinterpret_cast<Return (Class::*)()> (const_getter); }

/* --- bool --- */
template<class Class>
struct PropertyBool : Property {
  bool default_value;
  void (Class::*setter) (bool);
  bool (Class::*getter) ();
  PropertyBool (void (Class::*csetter) (bool), bool (Class::*cgetter) (),
                const char *cident, const char *clabel, const char *cblurb,
                bool cdefault_value, const char *chints);
  virtual void   set_value   (Deletable *obj, const String &svalue);
  virtual String get_value   (Deletable *obj);
  virtual String get_default (Deletable *obj);
  virtual bool   get_range   (Deletable *obj, double &minimum, double &start_value, double &maximum, double &stepping) { return false; }
};
template<class Class> inline Property*
create_property (void (Class::*setter) (bool), bool (Class::*getter) (),
                 const char *ident, const char *label, const char *blurb, bool default_value, const char *hints)
{ return new PropertyBool<Class> (setter, getter, ident, label, blurb, default_value, hints); }
template<class Class> inline Property*
create_property (void (Class::*setter) (bool), bool (Class::*getter) () const,
                 const char *ident, const char *label, const char *blurb, bool default_value, const char *hints)
{ return new PropertyBool<Class> (setter, noconst_getter (getter), ident, label, blurb, default_value, hints); }

/* --- range --- */
template<class Class, typename Type>
struct PropertyRange : Property {
  Type default_value;
  Type minimum_value;
  Type maximum_value;
  Type stepping;
  void (Class::*setter) (Type);
  Type (Class::*getter) ();
  PropertyRange (void (Class::*csetter) (Type), Type (Class::*cgetter) (),
                 const char *cident, const char *clabel, const char *cblurb,
                 Type cdefault_value, Type cminimum_value, Type cmaximum_value,
                 Type cstepping, const char *chints);
  virtual void   set_value   (Deletable *obj, const String &svalue);
  virtual String get_value   (Deletable *obj);
  virtual String get_default (Deletable *obj);
  virtual bool   get_range   (Deletable *obj, double &minimum, double &start_value, double &maximum, double &stepping);
};
/* int */
template<class Class> inline Property*
create_property (void (Class::*setter) (int), int (Class::*getter) (),
                 const char *ident, const char *label, const char *blurb, int default_value,
                 int min_value, int max_value, int stepping, const char *hints)
{ return new PropertyRange<Class,int> (setter, getter, ident, label, blurb, default_value, min_value, max_value, stepping, hints); }
template<class Class> inline Property*
create_property (void (Class::*setter) (int), int (Class::*getter) () const,
                 const char *ident, const char *label, const char *blurb, int default_value,
                 int min_value, int max_value, int stepping, const char *hints)
{ return new PropertyRange<Class,int> (setter, noconst_getter (getter), ident, label, blurb, default_value, min_value, max_value, stepping, hints); }
/* int16 */
template<class Class> inline Property*
create_property (void (Class::*setter) (int16), int16 (Class::*getter) (),
                 const char *ident, const char *label, const char *blurb, int16 default_value,
                 int16 min_value, int16 max_value, int16 stepping, const char *hints)
{ return new PropertyRange<Class,int16> (setter, getter, ident, label, blurb, default_value, min_value, max_value, stepping, hints); }
template<class Class> inline Property*
create_property (void (Class::*setter) (int16), int16 (Class::*getter) () const,
                 const char *ident, const char *label, const char *blurb, int16 default_value,
                 int16 min_value, int16 max_value, int16 stepping, const char *hints)
{ return new PropertyRange<Class,int16> (setter, noconst_getter (getter), ident, label, blurb, default_value, min_value, max_value, stepping, hints); }
/* uint */
template<class Class> inline Property*
create_property (void (Class::*setter) (uint), uint (Class::*getter) (),
                 const char *ident, const char *label, const char *blurb, uint default_value,
                 uint min_value, uint max_value, uint stepping, const char *hints)
{ return new PropertyRange<Class,uint> (setter, getter, ident, label, blurb, default_value, min_value, max_value, stepping, hints); }
template<class Class> inline Property*
create_property (void (Class::*setter) (uint), uint (Class::*getter) () const,
                 const char *ident, const char *label, const char *blurb, uint default_value,
                 uint min_value, uint max_value, uint stepping, const char *hints)
{ return new PropertyRange<Class,uint> (setter, noconst_getter (getter), ident, label, blurb, default_value, min_value, max_value, stepping, hints); }
/* uint16 */
template<class Class> inline Property*
create_property (void (Class::*setter) (uint16), uint16 (Class::*getter) (),
                 const char *ident, const char *label, const char *blurb, uint16 default_value,
                 uint16 min_value, uint16 max_value, uint16 stepping, const char *hints)
{ return new PropertyRange<Class,uint16> (setter, getter, ident, label, blurb, default_value, min_value, max_value, stepping, hints); }
template<class Class> inline Property*
create_property (void (Class::*setter) (uint16), uint16 (Class::*getter) () const,
                 const char *ident, const char *label, const char *blurb, uint16 default_value,
                 uint16 min_value, uint16 max_value, uint16 stepping, const char *hints)
{ return new PropertyRange<Class,uint16> (setter, noconst_getter (getter), ident, label, blurb, default_value, min_value, max_value, stepping, hints); }
/* float */
template<class Class> inline Property*
create_property (void (Class::*setter) (float), float (Class::*getter) (),
                 const char *ident, const char *label, const char *blurb, float default_value,
                 float min_value, float max_value, float stepping, const char *hints)
{ return new PropertyRange<Class,float> (setter, getter, ident, label, blurb, default_value, min_value, max_value, stepping, hints); }
template<class Class> inline Property*
create_property (void (Class::*setter) (float), float (Class::*getter) () const,
                 const char *ident, const char *label, const char *blurb, float default_value,
                 float min_value, float max_value, float stepping, const char *hints)
{ return new PropertyRange<Class,float> (setter, noconst_getter (getter), ident, label, blurb, default_value, min_value, max_value, stepping, hints); }
/* double */
template<class Class> inline Property*
create_property (void (Class::*setter) (double), double (Class::*getter) (),
                 const char *ident, const char *label, const char *blurb, double default_value,
                 double min_value, double max_value, double stepping, const char *hints)
{ return new PropertyRange<Class,double> (setter, getter, ident, label, blurb, default_value, min_value, max_value, stepping, hints); }
template<class Class> inline Property*
create_property (void (Class::*setter) (double), double (Class::*getter) () const,
                 const char *ident, const char *label, const char *blurb, double default_value,
                 double min_value, double max_value, double stepping, const char *hints)
{ return new PropertyRange<Class,double> (setter, noconst_getter (getter), ident, label, blurb, default_value, min_value, max_value, stepping, hints); }

/* --- string --- */
template<class Class>
struct PropertyString : Property {
  String default_value;
  void   (Class::*setter) (const String&);
  String (Class::*getter) ();
  PropertyString (void (Class::*csetter) (const String&), String (Class::*cgetter) (),
                  const char *cident, const char *clabel, const char *cblurb,
                  const String &cdefault_value, const char *chints);
  virtual void   set_value   (Deletable *obj, const String &svalue);
  virtual String get_value   (Deletable *obj);
  virtual String get_default (Deletable *obj);
  virtual bool   get_range   (Deletable *obj, double &minimum, double &start_value, double &maximum, double &stepping) { return false; }
};
template<class Class> inline Property*
create_property (void (Class::*setter) (const String&), String (Class::*getter) (),
                 const char *ident, const char *label, const char *blurb, String default_value, const char *hints)
{ return new PropertyString<Class> (setter, getter, ident, label, blurb, default_value, hints); }
template<class Class> inline Property*
create_property (void (Class::*setter) (const String&), String (Class::*getter) () const,
                 const char *ident, const char *label, const char *blurb, String default_value, const char *hints)
{ return new PropertyString<Class> (setter, noconst_getter (getter), ident, label, blurb, default_value, hints); }

/* --- enum --- */
template<class Class, typename Type>
struct PropertyEnum : Property {
  int64 default_value;
  const EnumClass &enum_class;
  void (Class::*setter) (Type);
  Type (Class::*getter) ();
  PropertyEnum (void (Class::*csetter) (Type), Type (Class::*cgetter) (),
                const char *cident, const char *clabel, const char *cblurb,
                int64 cdefault_value, const EnumClass &eclass, const char *chints);
  virtual void   set_value   (Deletable *obj, const String &svalue);
  virtual String get_value   (Deletable *obj);
  virtual String get_default (Deletable *obj);
  virtual bool   get_range   (Deletable *obj, double &minimum, double &start_value, double &maximum, double &stepping) { return false; }
};
template<class Class, typename Type> inline Property*
create_property (void (Class::*setter) (Type), Type (Class::*getter) (),
                 const char *ident, const char *label, const char *blurb, Type default_value, const char *hints)
{
  static const EnumType<Type> enum_class;
  return new PropertyEnum<Class,Type> (setter, getter, ident, label, blurb, default_value, enum_class, hints);
}
template<class Class, typename Type> inline Property*
create_property (void (Class::*setter) (Type), Type (Class::*getter) () const,
                 const char *ident, const char *label, const char *blurb, Type default_value, const char *hints)
{
  static const EnumType<Type> enum_class;
  return new PropertyEnum<Class,Type> (setter, noconst_getter (getter), ident, label, blurb, default_value, enum_class, hints);
}

/* --- implementations --- */
template<class Class>
PropertyBool<Class>::PropertyBool (void (Class::*csetter) (bool), bool (Class::*cgetter) (),
                                   const char *cident, const char *clabel, const char *cblurb,
                                   bool cdefault_value, const char *chints) :
  Property (cident, clabel, cblurb, chints),
  default_value (cdefault_value),
  setter (csetter),
  getter (cgetter)
{}

template<class Class> void
PropertyBool<Class>::set_value (Deletable *obj, const String &svalue)
{
  bool b = string_to_bool (svalue);
  Class *instance = dynamic_cast<Class*> (obj);
  (instance->*setter) (b);
}

template<class Class> String
PropertyBool<Class>::get_value (Deletable *obj)
{
  Class *instance = dynamic_cast<Class*> (obj);
  bool b = (instance->*getter) ();
  return string_from_bool (b);
}

template<class Class> String
PropertyBool<Class>::get_default (Deletable *obj)
{
  return string_from_bool (default_value);
}

template<class Class, typename Type>
PropertyRange<Class,Type>::PropertyRange (void (Class::*csetter) (Type), Type (Class::*cgetter) (),
                                          const char *cident, const char *clabel, const char *cblurb,
                                          Type cdefault_value, Type cminimum_value, Type cmaximum_value,
                                          Type cstepping, const char *chints) :
  Property (cident, clabel, cblurb, chints),
  default_value (cdefault_value),
  minimum_value (cminimum_value),
  maximum_value (cmaximum_value),
  stepping (cstepping),
  setter (csetter),
  getter (cgetter)
{
  assert (minimum_value <= maximum_value);
  assert (minimum_value <= default_value);
  assert (default_value <= maximum_value);
  assert (minimum_value + stepping <= maximum_value);
}

template<class Class, typename Type> void
PropertyRange<Class,Type>::set_value (Deletable *obj, const String &svalue)
{
  Type v = string_to_type<Type> (svalue);
  Class *instance = dynamic_cast<Class*> (obj);
  (instance->*setter) (v);
}

template<class Class, typename Type> String
PropertyRange<Class,Type>::get_value (Deletable *obj)
{
  Class *instance = dynamic_cast<Class*> (obj);
  Type v = (instance->*getter) ();
  return string_from_type<Type> (v);
}

template<class Class, typename Type> String
PropertyRange<Class,Type>::get_default (Deletable *obj)
{
  return string_from_type<Type> (default_value);
}

template<class Class, typename Type> bool
PropertyRange<Class,Type>::get_range (Deletable *obj, double &minimum, double &start_value, double &maximum, double &vstepping)
{
  minimum = minimum_value, start_value = default_value, maximum = maximum_value, vstepping = stepping;
  return true;
}

template<class Class>
PropertyString<Class>::PropertyString (void (Class::*csetter) (const String&), String (Class::*cgetter) (),
                                       const char *cident, const char *clabel, const char *cblurb,
                                       const String &cdefault_value, const char *chints) :
  Property (cident, clabel, cblurb, chints),
  default_value (cdefault_value),
  setter (csetter),
  getter (cgetter)
{}

template<class Class> void
PropertyString<Class>::set_value (Deletable *obj, const String &svalue)
{
  Class *instance = dynamic_cast<Class*> (obj);
  (instance->*setter) (svalue);
}

template<class Class> String
PropertyString<Class>::get_value (Deletable *obj)
{
  Class *instance = dynamic_cast<Class*> (obj);
  return (instance->*getter) ();
}

template<class Class> String
PropertyString<Class>::get_default (Deletable *obj)
{
  return default_value;
}

template<class Class, typename Type>
PropertyEnum<Class,Type>::PropertyEnum (void (Class::*csetter) (Type), Type (Class::*cgetter) (),
                                        const char *cident, const char *clabel, const char *cblurb,
                                        int64 cdefault_value, const EnumClass &eclass, const char *chints) :
  Property (cident, clabel, cblurb, chints),
  default_value (cdefault_value),
  enum_class (eclass),
  setter (csetter),
  getter (cgetter)
{}

template<class Class, typename Type> void
PropertyEnum<Class,Type>::set_value (Deletable *obj, const String &svalue)
{
  String error_string;
  uint64 value = enum_class.parse (svalue.c_str(), &error_string);
  if (0 && error_string[0] && !value && string_has_int (svalue))
    value = enum_class.constrain (string_to_int (svalue));
  else if (error_string[0])
    throw Exception ("no such value name in enum ", enum_class.enum_name(), ": ", error_string);
  Type v = Type (value);
  Class *instance = dynamic_cast<Class*> (obj);
  (instance->*setter) (v);
}

template<class Class, typename Type> String
PropertyEnum<Class,Type>::get_value (Deletable *obj)
{
  Class *instance = dynamic_cast<Class*> (obj);
  Type v = (instance->*getter) ();
  return enum_class.string (v);
}

template<class Class, typename Type> String
PropertyEnum<Class,Type>::get_default (Deletable *obj)
{
  return enum_class.string (default_value);
}

} // Rapicorn

#endif  /* __RAPICORN_PROPERTIES_HH__ */
