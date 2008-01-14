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
#include "values.hh"
#include "rapicornmath.hh"
#include <deque>

namespace Rapicorn {

#define IS_RAPICORN_TYPE_STORAGE(strg)  (strchr (RAPICORN_TYPE_STORAGE_CHARS, strg))

BaseValue::BaseValue (Type::Storage value_type) :
  u(), storage (value_type)
{
  assert (IS_RAPICORN_TYPE_STORAGE (storage));
  assert (u.i64 == 0);
  assert (u.ldf == 0);
  assert (u.p == NULL);
}

void
BaseValue::changed()
{}

void
BaseValue::try_retype (Type::Storage st)
{}

BaseValue&
BaseValue::operator= (const BaseValue &other)
{
  try_retype (other.storage);
  if (other.storage == storage)
    {
      switch (storage)
        {
        case Type::NUM: case Type::FLOAT:
          u = other.u;
          break;
        case Type::STRING:
          if (other.u.p)
            u.p = new String (*(String*) other.u.p);
          else
            u.p = NULL;
          break;
        case Type::STRING_VECTOR:
          if (other.u.p)
            u.p = new vector<String> (*(vector<String>*) other.u.p);
          else
            u.p = NULL;
          break;
        case Type::ARRAY:
          if (other.u.p)
            u.p = new Array (*(Array*) other.u.p);
          else
            u.p = NULL;
          break;
        case Type::OBJECT:
          if (other.u.p)
            u.p = ref ((Object*) other.u.p);
          else
            u.p = NULL;
          break;
        }
      changed();
    }
  else
    switch (other.storage)
      {
      case Type::NUM: case Type::FLOAT:
        this->set (other.u.i64);
        break;
      case Type::STRING:
        this->set (other.u.p ? *(String*) other.u.p : "");
        break;
      case Type::STRING_VECTOR:
        this->set (other.u.p ? *(vector<String>*) other.u.p : vector<String>());
        break;
      case Type::ARRAY:
        this->set (other.u.p ? *(Array*) other.u.p : Array());
        break;
      case Type::OBJECT:
        assign ((Object*) other.u.p);
        break;
      }
  return *this;
}

int64
BaseValue::num () const
{
  switch (storage)
    {
    case Type::NUM:
      return u.i64;
    case Type::FLOAT:
      return (int64) roundl (u.ldf);
    case Type::STRING:
      if (!u.p)
        return 0;
      else
        return string_to_int (*(String*) u.p);
    case Type::ARRAY:
      return u.p ? ((Array*) u.p)->count() : 0;
    case Type::STRING_VECTOR:
      return u.p ? ((vector<String>*) u.p)->size() : 0;
    case Type::OBJECT:
      return u.p != NULL;
    }
  return 0;
}

long double
BaseValue::vfloat () const
{
  switch (storage)
    {
    case Type::FLOAT:
      return u.ldf;
    case Type::STRING:
      if (!u.p)
        return 0;
      else
        return string_to_float (*(String*) u.p);
    case Type::NUM:
    case Type::ARRAY:
    case Type::STRING_VECTOR:
    case Type::OBJECT:
      return num();
    }
  return 0.0;
}

const String
BaseValue::string () const
{
  switch (storage)
    {
    case Type::NUM:
      return string_from_int (u.i64);
    case Type::FLOAT:
      return string_from_double (u.ldf);
    case Type::STRING:
      return u.p ? *(String*) u.p : "";
    case Type::ARRAY:
      return u.p ? ((Array*) u.p)->to_string() : "";
    case Type::STRING_VECTOR:
      if (u.p)
        {
          vector<String> *vs = (vector<String>*) u.p;
          String s;
          for (uint i = 0; i < s.size(); i++)
            if (i)
              s += ", " + (*vs)[i];
            else
              s += (*vs)[i];
          return s;
        }
      else
        return "";
    case Type::OBJECT:
      return "";
    }
  return "";
}

void
BaseValue::assign (int64 *nump)
{
  int64 num = *nump;
  try_retype (Type::NUM);
  switch (storage)
    {
    case Type::NUM:
      u.i64 = num;
      break;
    case Type::FLOAT:
      u.ldf = num;
      break;
    case Type::STRING:
      if (!u.p)
        u.p = new String;
      *(String*) u.p = string_from_int (num);
      break;
    case Type::ARRAY:
    case Type::STRING_VECTOR:
    case Type::OBJECT:
      if (num == 0) // emulate 'NULL Pointer'
        {
          unset();
          break;
        }
      goto default_error;
    default_error:
      RAPICORN_ERROR ("value type mismatch for %s setter: %s", "num", Type::type_name (storage));
    }
  changed();
}

void
BaseValue::assign (long double *ldfp)
{
  long double ldf = *ldfp;
  try_retype (Type::FLOAT);
  switch (storage)
    {
    case Type::NUM:
      u.i64 = (int64) roundl (ldf);
      break;
    case Type::FLOAT:
      u.ldf = ldf;
      break;
    case Type::STRING:
      if (!u.p)
        u.p = new String;
      *(String*) u.p = string_from_double (ldf);
      break;
    case Type::ARRAY:
    case Type::STRING_VECTOR:
    case Type::OBJECT:
      if (ldf == 0) // emulate 'NULL Pointer'
        {
          unset();
          break;
        }
      goto default_error;
    default_error:
      RAPICORN_ERROR ("value type mismatch for %s setter: %s", "float", Type::type_name (storage));
    }
  changed();
}

void
BaseValue::assign (const String *sp)
{
  const String &s = *sp;
  try_retype (Type::STRING);
  switch (storage)
    {
    case Type::NUM:
      u.i64 = string_to_int (s);
      break;
    case Type::FLOAT:
      u.ldf = string_to_double (s);
      break;
    case Type::STRING:
      if (!u.p)
        u.p = new String;
      *(String*) u.p = s;
      break;
    case Type::ARRAY:
      goto default_error;
    case Type::STRING_VECTOR:
      goto default_error;
    case Type::OBJECT:
      goto default_error;
    default_error:
      RAPICORN_ERROR ("value type mismatch for %s setter: %s", "String", Type::type_name (storage));
    }
  changed();
}

void
BaseValue::assign (const StringVector *svp)
{
  const StringVector &sv = *svp;
  try_retype (Type::STRING_VECTOR);
  switch (storage)
    {
    case Type::NUM:
    case Type::FLOAT:
    case Type::STRING:
    case Type::ARRAY:
    case Type::OBJECT:
      goto default_error;
    case Type::STRING_VECTOR:
      if (!u.p)
        u.p = new vector<String>;
      *(vector<String>*) u.p = sv;
      break;
    default_error:
      RAPICORN_ERROR ("value type mismatch for %s setter: %s", "StringVector", Type::type_name (storage));
    }
  changed();
}

void
BaseValue::assign (const Array *ap)
{
  const Array &a = *ap;
  try_retype (Type::ARRAY);
  switch (storage)
    {
    case Type::NUM:
    case Type::FLOAT:
    case Type::STRING:
    case Type::STRING_VECTOR:
    case Type::OBJECT:
      goto default_error;
    case Type::ARRAY:
      if (!u.p)
        u.p = new Array;
      *(Array*) u.p = a;
      break;
    default_error:
      RAPICORN_ERROR ("value type mismatch for %s setter: %s", "Array", Type::type_name (storage));
    }
  changed();
}

void
BaseValue::assign (Object *orefp)
{
  Object &oref = *orefp;
  try_retype (Type::OBJECT);
  Object *object = &oref;
  switch (storage)
    {
    case Type::OBJECT:
      if (object)
        ref (object);
      if (u.p)
        unref ((Object*) u.p);
      u.p = object;
      break;
    case Type::NUM:
    case Type::FLOAT:
    case Type::STRING:
    case Type::STRING_VECTOR:
    case Type::ARRAY:
      goto default_error;
    default_error:
      RAPICORN_ERROR ("value type mismatch for %s setter: %s", "Object", Type::type_name (storage));
    }
  changed();
}

void
BaseValue::unset()
{
  switch (storage)
    {
    case Type::NUM:
    case Type::FLOAT:
      break;            // no release
    case Type::STRING:
      if (u.p)
        delete (String*) u.p;
      break;
    case Type::ARRAY:
      if (u.p)
        delete (Array*) u.p;
      break;
    case Type::STRING_VECTOR:
      if (u.p)
        delete (vector<String>*) u.p;
      break;
    case Type::OBJECT:
      if (u.p)
        unref ((Object*) u.p);
      break;
    }
  u.ldf = 0.0;
  u.i64 = 0;
  u.p = NULL;
}

BaseValue::~BaseValue ()
{
  unset();
  assert (u.i64 == 0);
}

AutoValue::AutoValue (const Array &array) :
  AnyValue (Type::ARRAY)
{
  set (array);
}

void
AnyValue::try_retype (Type::Storage strg)
{
  if (storage != strg)
    {
      unset();
      *((Type::Storage*) &storage) = strg;
    }
}

typedef map<String,AnyValue> StringValueMap;

struct Aval {
  const char *key;
  AnyValue    value;
};

struct Array::Internal : public std::deque<Aval> {
  StringValueMap     vmap;
  std::deque<String> strings;
  size_t             last;
  Internal      () : last (0) {}
};

Array::Array () :
  array (new Array::Internal())
{}

Array::Array (const Array &other) :
  array (new Array::Internal())
{
  *array = *other.array;
}

Array::~Array ()
{
  delete array;
  array = NULL;
}

Array&
Array::operator= (const Array &other)
{
  *array = *other.array;
  return *this;
}

int64
Array::count () const
{
  return array->strings.size();
}

StringVector
Array::keys ()
{
  return StringVector (array->strings.begin(), array->strings.end());
}

Array
Array::values ()
{
  Array anew;
  for (size_t i = 0; i < array->strings.size(); i++)
    {
      StringValueMap::const_iterator it = array->vmap.find (array->strings[i]);
      anew.push_tail (it->second);
    }
  return anew;
}

String
Array::nexti ()
{
  array->last = MAX (array->last, count());
  String idx = string_from_int (array->last++);
  while (array->vmap.find (idx) != array->vmap.end())
    {
      idx = string_from_int (array->last++);
      if (array->last == 0)
        {
          RAPICORN_WARNING ("Array index out of bounds: %zu", array->last - 1);
          idx = "Rapicorn::Array::OOB";
          break;
        }
    }
  return idx;
}

void
Array::push_tail (const BaseValue &value)
{
  String idx = nexti();
  array->strings.push_back (idx);
  std::pair<String,AnyValue> pnew (idx, value);
  array->vmap.insert (pnew);
}

void
Array::push_head (const BaseValue &value)
{
  String idx = nexti();
  array->strings.push_front (idx);
  std::pair<String,AnyValue> pnew (idx, value);
  array->vmap.insert (pnew);
}

AnyValue
Array::pop_head ()
{
  if (array->strings.size() == 0)               // OOB access
    return AnyValue (Type::NUM, 0);
  StringValueMap::iterator it = array->vmap.find (array->strings[0]);
  array->strings.erase (array->strings.begin() + 0);
  AnyValue v = it->second;
  array->vmap.erase (it);
  return v;
}

AnyValue
Array::pop_tail ()
{
  if (array->strings.size() == 0)               // OOB access
    return AnyValue (Type::NUM, 0);
  size_t ilast = array->strings.size() - 1;
  StringValueMap::iterator it = array->vmap.find (array->strings[ilast]);
  array->strings.erase (array->strings.begin() + ilast);
  AnyValue v = it->second;
  array->vmap.erase (it);
  return v;
}

String
Array::key (int64 index) const
{
  if (index >= 0 && index < array->strings.size())
    return array->strings[index];
  else if (ABS (index) <= array->strings.size())
    return array->strings[array->strings.size() + index];
  String idx = string_from_int (ABS (index));
  return idx;
}

AnyValue&
Array::operator[] (int64 index)
{
  return this->operator[] (key (index));
}

const AnyValue&
Array::operator[] (int64 index) const
{
  return this->operator[] (key (index));
}

static const AnyValue array_default_value (Type::NUM, 0);

AnyValue&
Array::operator[] (const String &key)
{
  StringValueMap::iterator it = array->vmap.find (key);
  if (it == array->vmap.end())
    {
      array->strings.push_back (key);
      std::pair<String,AnyValue> pnew (key, array_default_value);
      array->vmap.insert (pnew);
      it = array->vmap.find (key);
    }
  return it->second;
}

const AnyValue&
Array::operator[] (const String &key) const
{
  StringValueMap::const_iterator it = array->vmap.find (key);
  if (it == array->vmap.end())
    return array_default_value;
  else
    return it->second;
}

bool
Array::remove (int64 index)
{
  if (index >= 0)                               // index name access
    {
      String idx = string_from_int (index);
      return remove (idx);
    }
  else if (-index > array->strings.size())      // OOB access
    return false;
  else                                          // reverse order access
    return remove (array->strings[array->strings.size() + index]);
}

bool
Array::remove (const String &key)
{
  StringValueMap::iterator it = array->vmap.find (key);
  if (it == array->vmap.end())
    return false;
  for (size_t i = 0; i < array->strings.size(); i++)
    {
      /* transform index so we search head, tail, head+1, tail-1, ... */
      ssize_t t = i >> 1;
      if (i & 1)
        t = array->strings.size() - t;
      /* remove string entry */
      if (array->strings[t] == it->first)
        {
          array->strings.erase (array->strings.begin() + t);
          break;
        }
    }
  /* remove map entry after string entry to keep it->first alive */
  array->vmap.erase (it);
  return true;
}

void
Array::clear ()
{
  array->strings.clear();
  array->vmap.clear();
  array->last = 0;
}

String
Array::to_string (const String &junction)
{
  int64 imax = count();
  String result;
  for (int64 i = 0; i < imax; i++)
    {
      if (i)
        result += junction;
      result += (*this)[i].string();
    }
  return result;
}

} // Rapicorn
