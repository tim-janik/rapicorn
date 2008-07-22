/* Rapicorn
 * Copyright (C) 2008 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#ifndef __RAPICORN_VALUES_HH__
#define __RAPICORN_VALUES_HH__

#include <rapicorn-core/types.hh>
#include <rapicorn-core/rapicornxml.hh>

namespace Rapicorn {

class Array;
class BaseValue;
typedef ReferenceCountImpl  Object;

/* --- Value handling --- */
class BaseValue {
  union { int64 i64; long double ldf; void *p; } u;
  /*Copy*/              BaseValue (const BaseValue&) : storage (Type::NUM) { /* No Copies */ }
protected:
  void                  assign          (bool                 *vbool)  { set ((int64) *vbool); }
  void                  assign          (int                  *vint)   { set ((int64) *vint); }
  void                  assign          (uint                 *vuint)  { set ((int64) *vuint); }
  void                  assign          (int64                *num);
  void                  assign          (float                *real)   { set ((long double) *real); }
  void                  assign          (double               *vdble)  { set ((long double) *vdble); }
  void                  assign          (long double          *real);
  void                  assign          (const String         *string);
  void                  assign          (const char          **string) { set (String (*string)); }
  void                  assign          (const StringVector   *string_vector);
  void                  assign          (Object               *object);
  void                  assign          (const Array          *array);
  BaseValue&            operator=       (const BaseValue&);
  void                  unset           ();
  explicit              BaseValue       (Type::Storage value_type);
  virtual              ~BaseValue       ();
  virtual void          changed         ();
  virtual void          try_retype      (Type::Storage st);
public:
  const Type::Storage   storage;
  /* non-lvalue getters */
  long double           real          () const;
  int64                 num             () const;
  const String          string          () const;
  /* lvalue getters */
  StringVector&         string_vector   () { return storage == Type::STRING_VECTOR && u.p ? *(StringVector*) u.p : *(StringVector*) 0; }
  Object&               object          () { return storage == Type::OBJECT && u.p ? *(Object*) u.p : *(Object*) 0; }
  Array&                array           () { return storage == Type::ARRAY && u.p ? *(Array*) u.p : *(Array*) 0; }
  /* setters */
  template<typename T>
  void                  set             (T value) { assign (&value); }
  /* conversion */
  bool                  convert         (bool*         = 0)     { return num(); }
  int                   convert         (int*          = 0)     { return num(); }
  uint                  convert         (uint*         = 0)     { return num(); }
  int64                 convert         (int64*        = 0)     { return num(); }
  float                 convert         (float*        = 0)     { return real(); }
  double                convert         (double*       = 0)     { return real(); }
  long double           convert         (long double*  = 0)     { return real(); }
  String                convert         (String*       = 0)     { return string(); }
  StringVector          convert         (StringVector* = 0)     { return string_vector(); }
  Object&               convert         (Object*       = 0)     { return object(); }
  Array&                convert         (Array*        = 0)     { return array(); }
};

class AnyValue : public BaseValue {
  virtual void  try_retype      (Type::Storage st);
public:
  template<typename V>
  explicit      AnyValue        (Type::Storage    strg,
                                 V                value) : BaseValue (strg) { set (value); }
  explicit      AnyValue        (Type::Storage    strg)  : BaseValue (strg) {}
  /*Con*/       AnyValue        (const BaseValue &other) : BaseValue (Type::NUM) { *this = other; }
  /*Copy*/      AnyValue        (const AnyValue  &other) : BaseValue (Type::NUM) { *this = other; }
  template<typename T>
  AnyValue&     operator=       (T tvalue) { set (tvalue); return *this; }
  AnyValue&     operator=       (const BaseValue &other) { this->BaseValue::operator= (other); return *this; }
  AnyValue&     operator=       (const AnyValue  &other) { this->BaseValue::operator= (other); return *this; }
  template<typename T>
  /*T*/         operator T      ()                       { return convert ((T*) 0); }
};

class AutoValue : public AnyValue {
public:
  /*Con*/       AutoValue       (long double           num)           : AnyValue (Type::STRING)        { set (num); }
  /*Con*/       AutoValue       (const char           *cstring)       : AnyValue (Type::STRING)        { set (String (cstring)); }
  /*Con*/       AutoValue       (const String         &string)        : AnyValue (Type::STRING)        { set (string); }
  /*Con*/       AutoValue       (const StringVector   &string_vector) : AnyValue (Type::STRING_VECTOR) { set (string_vector); }
  /*Con*/       AutoValue       (Object               &object)        : AnyValue (Type::OBJECT)        { assign (&object); }
  /*Con*/       AutoValue       (const Array          &array);
};

#define RAPICORN_ARRAY_APPEND(Array, Carray, ElementConstructor)        do { for (uint64_t __ai = 0; __ai < RAPICORN_ARRAY_SIZE (Carray); __ai++) \
                                                                               Array.push_tail ( ElementConstructor ( Carray [__ai] ) ); } while (0)
#define RAPICORN_ARRAY_COMPACT(Carray, ElementConstructor)              ({ Array __t; RAPICORN_ARRAY_APPEND (__t, Carray, ElementConstructor); __t; })

class Array { // ordered key->value map
  class Internal;
  Internal       *array;
  String          nexti         ();
public:
  /*des*/        ~Array         ();
  /*copy*/        Array         (const Array    &other);
  explicit        Array         ();
  template<typename Element, typename CArray>
  static Array    FromCArray    (CArray &a) { Array r; RAPICORN_ARRAY_APPEND (r, a, Element); return r; }
  /* array operations */
  Array&          operator=     (const Array    &other);
  Array           operator+     (const Array    &other) const; /* yield this + other (non replacing) */
  void            update        (const Array    &other);       /* add other (replacing) */
  void            swap          (Array          &other);       /* swap contents */
  bool            operator!=    (const Array    &other) const;
  bool            operator==    (const Array    &other) const { return !operator!= (other); }
  /* core access */
  int64           count         () const;
  StringVector    keys          ();
  Array           values        (); // copies & renumbers
  String          key           (int64                index) const;
  AnyValue&       operator[]    (int64                index);
  AnyValue&       operator[]    (const String        &key);
  const AnyValue& operator[]    (int64                index) const;
  const AnyValue& operator[]    (const String        &key) const;
  void            push_head     (const BaseValue &value);
  void            push_head     (const AutoValue &value) { push_head (*(const BaseValue*) &value); }
  void            push_tail     (const BaseValue &value);
  void            push_tail     (const AutoValue &value) { push_tail (*(const BaseValue*) &value); }
  AnyValue        pop_head      ();
  AnyValue        pop_tail      ();
  bool            remove        (int64           index);
  bool            remove        (const String   &key);
  void            clear         ();
  /* misc methods */
  String          to_string     (const String   &junction = ",");
  XmlNode*        to_xml        ();
  static Array    from_xml      (const String &xmlstring,
                                 const String &inputname = "",
                                 String       *errstrp = NULL);
};

} // Rapicorn

#endif /* __RAPICORN_VALUES_HH__ */
