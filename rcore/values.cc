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

static inline uint32*
memfind4 (const uint32 *mem, uint32 value, size_t n_values) // FIXME: public header
{
  return (uint32*) wmemchr ((const wchar_t*) mem, value, n_values);
}

static BaseValue::Storage
value_storage (StorageType stt)
{
  return BaseValue::Storage (stt & 0xff);
}

static const char*
storage_name (BaseValue::Storage storage)
{
  switch (storage)
    {
    case BaseValue::INT:        return "INT";
    case BaseValue::FLOAT:      return "FLOAT";
    case BaseValue::STRING:     return "STRING";
    case BaseValue::ARRAY:      return "ARRAY";
    case BaseValue::OBJECT:     return "OBJECT";
    }
  return NULL;
}

static const uint32 value_storage_types[] = {
  BaseValue::INT, BaseValue::FLOAT, BaseValue::STRING, BaseValue::ARRAY, BaseValue::OBJECT,
};

BaseValue::BaseValue (Storage value_type) :
  u(), storage (value_type)
{
  assert (memfind4 (value_storage_types, storage, ARRAY_SIZE (value_storage_types)));
  assert (u.i64 == 0);
  assert (u.ldf == 0);
  assert (u.p == NULL);
}

BaseValue::BaseValue (StorageType value_type) :
  u(), storage (value_storage (value_type))
{
  assert (memfind4 (value_storage_types, storage, ARRAY_SIZE (value_storage_types)));
  assert (u.i64 == 0);
  assert (u.ldf == 0);
  assert (u.p == NULL);
}

void
BaseValue::changed()
{}

void
BaseValue::try_retype (Storage st)
{}

BaseValue&
BaseValue::operator= (const BaseValue &other)
{
  try_retype (other.storage);
  if (other.storage == storage)
    {
      switch (storage)
        {
        case INT: case FLOAT:
          u = other.u;
          break;
        case STRING:
          if (other.u.p)
            u.p = new String (*(String*) other.u.p);
          else
            u.p = NULL;
          break;
        case ARRAY:
          if (other.u.p)
            u.p = new Array (*(Array*) other.u.p);
          else
            u.p = NULL;
          break;
        case OBJECT:
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
      case INT: case FLOAT:
        this->set (other.u.i64);
        break;
      case STRING:
        this->set (other.u.p ? *(String*) other.u.p : "");
        break;
      case ARRAY:
        this->set (other.u.p ? *(Array*) other.u.p : Array());
        break;
      case OBJECT:
        assign ((Object*) other.u.p);
        break;
      }
  return *this;
}

int64
BaseValue::asint () const
{
  switch (storage)
    {
    case INT:
      return u.i64;
    case FLOAT:
      return (int64) roundl (u.ldf);
    case STRING:
      if (!u.p)
        return 0;
      else
        return string_to_int (*(String*) u.p);
    case ARRAY:
      return u.p ? ((Array*) u.p)->count() : 0;
    case OBJECT:
      return u.p != NULL;
    }
  return 0;
}

long double
BaseValue::asfloat () const
{
  switch (storage)
    {
    case FLOAT:
      return u.ldf;
    case STRING:
      if (!u.p)
        return 0;
      else
        return string_to_float (*(String*) u.p);
    case INT:
    case ARRAY:
    case OBJECT:
      return asint();
    }
  return 0.0;
}

const String
BaseValue::string () const
{
  switch (storage)
    {
    case INT:
      return string_from_int (u.i64);
    case FLOAT:
      return string_from_double (u.ldf);
    case STRING:
      return u.p ? *(String*) u.p : "";
    case ARRAY:
      return u.p ? ((Array*) u.p)->to_string() : "";
    case OBJECT:
      return "";
    }
  return "";
}

StringVector
BaseValue::string_vector ()
{
#warning FIXME: BaseValue::string_vector
  return StringVector();
}

void
BaseValue::assign (int64 *i64p)
{
  int64 i64 = *i64p;
  try_retype (INT);
  switch (storage)
    {
    case INT:
      u.i64 = i64;
      break;
    case FLOAT:
      u.ldf = i64;
      break;
    case STRING:
      if (!u.p)
        u.p = new String;
      *(String*) u.p = string_from_int (i64);
      break;
    case ARRAY:
    case OBJECT:
      if (i64 == 0) // emulate 'NULL Pointer'
        {
          unset();
          break;
        }
      goto default_error;
    default_error:
      RAPICORN_ERROR ("value type mismatch for %s setter: %s", "int", storage_name (storage));
    }
  changed();
}

void
BaseValue::assign (long double *ldfp)
{
  long double ldf = *ldfp;
  try_retype (FLOAT);
  switch (storage)
    {
    case INT:
      u.i64 = (int64) roundl (ldf);
      break;
    case FLOAT:
      u.ldf = ldf;
      break;
    case STRING:
      if (!u.p)
        u.p = new String;
      *(String*) u.p = string_from_double (ldf);
      break;
    case ARRAY:
    case OBJECT:
      if (ldf == 0) // emulate 'NULL Pointer'
        {
          unset();
          break;
        }
      goto default_error;
    default_error:
      RAPICORN_ERROR ("value type mismatch for %s setter: %s", "float", storage_name (storage));
    }
  changed();
}

void
BaseValue::assign (const String *sp)
{
  const String &s = *sp;
  try_retype (STRING);
  switch (storage)
    {
    case INT:
      u.i64 = string_to_int (s);
      break;
    case FLOAT:
      u.ldf = string_to_double (s);
      break;
    case STRING:
      if (!u.p)
        u.p = new String;
      *(String*) u.p = s;
      break;
    case ARRAY:
      goto default_error;
    case OBJECT:
      goto default_error;
    default_error:
      RAPICORN_ERROR ("value type mismatch for %s setter: %s", "string", storage_name (storage));
    }
  changed();
}

void
BaseValue::assign (const StringVector *svp)
{
  try_retype (ARRAY);
  switch (storage)
    {
    case INT:
    case FLOAT:
    case STRING:
    case OBJECT:
      goto default_error;
    case ARRAY:
#warning FIXME: BaseValue::assign (StringVector)
      break;
    default_error:
      RAPICORN_ERROR ("value type mismatch for %s setter: %s", "StringVector", storage_name (storage));
    }
  changed();
}

void
BaseValue::assign (const Array *ap)
{
  const Array &a = *ap;
  try_retype (ARRAY);
  switch (storage)
    {
    case INT:
    case FLOAT:
    case STRING:
    case OBJECT:
      goto default_error;
    case ARRAY:
      if (!u.p)
        u.p = new Array;
      *(Array*) u.p = a;
      break;
    default_error:
      RAPICORN_ERROR ("value type mismatch for %s setter: %s", "Array", storage_name (storage));
    }
  changed();
}

void
BaseValue::assign (Object *orefp)
{
  Object &oref = *orefp;
  try_retype (OBJECT);
  Object *object = &oref;
  switch (storage)
    {
    case OBJECT:
      if (object)
        ref (object);
      if (u.p)
        unref ((Object*) u.p);
      u.p = object;
      break;
    case INT:
    case FLOAT:
    case STRING:
    case ARRAY:
      goto default_error;
    default_error:
      RAPICORN_ERROR ("value type mismatch for %s setter: %s", "Object", storage_name (storage));
    }
  changed();
}

void
BaseValue::unset()
{
  switch (storage)
    {
    case INT:
    case FLOAT:
      break;            // no release
    case STRING:
      if (u.p)
        delete (String*) u.p;
      break;
    case ARRAY:
      if (u.p)
        delete (Array*) u.p;
      break;
    case OBJECT:
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
  AnyValue (ARRAY)
{
  set (array);
}

AutoValue::AutoValue (const StringVector &string_vector) :
  AnyValue (ARRAY)
{
#warning FIXME: AutoValue::AutoValue()
}

void
AnyValue::try_retype (Storage strg)
{
  if (storage != strg)
    {
      unset();
      *((Storage*) &storage) = strg;
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
      idx = string_from_uint (array->last++);
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
    return AnyValue (INT, 0);
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
    return AnyValue (INT, 0);
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
  else if (index < 0 && -index <= array->strings.size())
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

static const AnyValue array_default_value (INT, 0);

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
      assert (it != array->vmap.end());
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

class XmlNodeWalker {
#warning FIXME: move XmlNodeWalker to rapicornxml.h
protected:
  virtual bool
  process_children (const XmlNode &xnode)
  {
    for (XmlNode::ConstChildIter it = xnode.children_begin(); it != xnode.children_end(); it++)
      if (!process_node (**it))
        return false;
    return true;
  }
  virtual bool
  handle_node (const XmlNode &xnode)
  {
    return true;
  }
public:
  virtual bool
  process_node (const XmlNode &xnode)
  {
    if (!handle_node (xnode))
      return false;
    if (!process_children (xnode))
      return false;
    return true;
  }
};

namespace {
struct XmlArray : public XmlNodeWalker {
  Array m_array;
  virtual bool
  process_node (const XmlNode &xnode)
  {
    bool valid_tree = true, nested_recurse = false;
    if (xnode.istext())
      {
        m_array.push_tail (xnode.text());
      }
    else if (xnode.name() == "Rapicorn::Array")
      {
        valid_tree = xnode.parent() == NULL;
        valid_tree &= process_children (xnode);
      }
    else if (xnode.name() == "row")
      {
        valid_tree &= xnode.parent()->name() == "Rapicorn::Array";
        valid_tree &= process_children (xnode);
      }
    else if (xnode.name() == "col")
      {
        valid_tree &= xnode.parent()->name() == "row";
        nested_recurse = true;
      }
    else if (xnode.name() == "cell")
      {
        valid_tree &= xnode.parent()->name() == "col";
        nested_recurse = true;
      }
    if (valid_tree && nested_recurse)
      {
        Array oarray = m_array;
        m_array = Array();
        valid_tree &= process_children (xnode);
        oarray.push_tail (m_array);
        m_array = oarray;
      }
    return valid_tree;
  }
};
} // Anon

Array
Array::from_xml (const String &xmlstring,
                 const String &inputname,
                 String       *errstrp)
{
  String errstr;
  MarkupParser::Error perror;
  XmlNode *xnode = XmlNode::parse_xml (inputname, xmlstring.data(), xmlstring.size(), &perror, "Rapicorn::Array");
  Array result;
  if (perror.code)
    errstr = string_printf ("%s:%d:%d: %s", inputname.c_str(),
                            perror.line_number, perror.char_number, perror.message.c_str());
  if (xnode)
    {
      ref_sink (xnode);
      if (!errstr.size())
        {
          XmlArray xarray;
          if (xarray.process_node (*xnode) == true)
            result = xarray.m_array;
          else
            errstr = string_printf ("%s: invalid array markup: %s", inputname.c_str(), xmlstring.c_str());
        }
      unref (xnode);
    }
  if (errstrp)
    *errstrp = errstr;
  return result;
}

XmlNode*
Array::to_xml ()
{
  XmlNode *root = XmlNode::create_parent ("Array");
  root->break_within (true);
  StringVector k = keys();
  for (uint64 i = 0; i < k.size(); i++)
    {
      AnyValue &v = (*this)[k[i]];
      XmlNode *parentnode = NULL;
      bool addnull = false;
      String tag, txt;
      switch (v.storage)
        {
          Object *obj;
        case BaseValue::INT:
          tag = "int";
          txt = v.string();
          break;
        case BaseValue::FLOAT:
          tag = "float";
          txt = v.string();
          break;
        case BaseValue::STRING:
          txt = v.string();
          break;
        case BaseValue::ARRAY:
          tag = "Array";
          parentnode = v.array().to_xml();
          break;
        case BaseValue::OBJECT:
          tag = "Object";
          obj = &v.object();
          addnull = !obj;
          if (!addnull)
            txt = obj->object_url();
          break;
        }
      XmlNode *current = root;
      if (1)
        {
          XmlNode *child = XmlNode::create_parent ("row");
          if (i > 2147483647 || string_from_uint (i) != k[i])
            child->set_attribute ("key", k[i]);
          child->break_after (true);
          current->add_child (*child);
          current = child;
        }
      if (tag.size())
        {
          XmlNode *child = XmlNode::create_parent (tag);
          current->add_child (*child);
          current = child;
        }
      if (parentnode)
        {
          ref_sink (parentnode);
          current->steal_children (*parentnode);
          unref (parentnode);
        }
      if (addnull)
        current->add_child (*XmlNode::create_parent ("null"));
      if (txt.size())
        current->add_child (*XmlNode::create_text (txt));
    }
  return root;
}

} // Rapicorn
