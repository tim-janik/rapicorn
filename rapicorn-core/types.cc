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
#include "types.hh"
#include "rapicornthread.hh"
#include <errno.h>

namespace Rapicorn {

const char*
Type::storage_name (Type::Storage storage)
{
  switch (storage)
    {
    case Type::NUM:             return "NUM";
    case Type::REAL:            return "REAL";
    case Type::STRING:          return "STRING";
    case Type::CHOICE:          return "CHOICE";
    case Type::SEQUENCE:        return "SEQUENCE";
    case Type::RECORD:          return "RECORD";
    case Type::INTERFACE:       return "INTERFACE";
    case Type::STRING_VECTOR:   return "STRING_VECTOR";
    case Type::ARRAY:           return "ARRAY";
    case Type::TYPE_REFERENCE:  return "TYPE_REFERENCE";
    }
  return NULL;
}

namespace Plic {
#include "types-ptpp.cc" // PLIC TypePackage Parser
}

struct Type::Info : public ReferenceCountImpl {
  Plic::TypeInfo plic_type_info;
  explicit Info (const Plic::TypeInfo &tmpl) :
    plic_type_info (tmpl)
  {}
};

String
Type::name () const
{
  return m_info.plic_type_info.name();
}

Type::Storage
Type::storage () const
{
  return Storage (m_info.plic_type_info.storage);
}

String
Type::aux_string (const String &auxname) const
{
  uint naux = m_info.plic_type_info.n_aux_strings ();
  String key = auxname + "=";
  uint kl = key.size();
  for (uint i = 0; i < naux; i++)
    {
      uint al = 0;
      const char *as = m_info.plic_type_info.aux_string (i, &al);
      if (kl <= al && strncmp (key.c_str(), as, kl) == 0)
        return String (as + kl, al - kl);
    }
  return "";
}

double
Type::aux_float (const String &auxname) const
{
  return string_to_double (aux_string (auxname));
}

int64
Type::aux_num (const String &auxname) const
{
  return string_to_int (aux_string (auxname));
}

String
Type::label () const
{
  String str = aux_string ("label");
  return str.size() == 0 ? ident() : str;
}

String
Type::hints () const
{
  String str = aux_string ("hints");
  if (str.size() == 0 || str[0] != ':')
    str = ":" + str;
  if (str[str.size() - 1] != ':')
    str = str + ":";
  return str;
}

Type::Type (Info &tinfo) :
  m_info (ref_sink (tinfo))
{}

Type::Type (const Type &src) :
  m_info (ref_sink (src.m_info))
{}

Type::~Type ()
{
  unref (m_info);
}

bool
Type::istype () const // false for notype()
{
  return storage() != 0;
}

static Plic::TypeRegistry type_registry;
static Mutex              type_registry_mutex;
static Type              *type_notype = NULL;

Type
Type::notype ()
{
  if (!type_notype)
    {
      struct Null_TypeInfo : public Plic::TypeInfo {};
      Null_TypeInfo nt;
      static const uint8 nullname[] = "\200\200\200\200";
      nt.namep = nullname;
      nt.rtypep = nullname;
      Type::Info *rawti = new Type::Info (nt);
      ref_sink (rawti);
      {
        AutoLocker locker (type_registry_mutex);
        if (!type_notype)
          type_notype = new Type (*rawti);
      }
      unref (rawti);
    }
  return *type_notype;
}

Type
Type::lookup (const String &full_name)
{
  String::size_type sl = full_name.rfind (':');
  if (sl != full_name.npos && sl > 0 && full_name[sl-1] == ':')
    {
      String nspace = full_name.substr (0, sl - 1), tname = full_name.substr (sl + 1);
      vector<Plic::TypeNamespace> tnl;
      AutoLocker locker (type_registry_mutex);
      tnl = type_registry.list_namespaces ();
      for (uint i = 0; i < tnl.size(); i++)
        if (tnl[i].fullname() == nspace)
          {
            vector<Plic::TypeInfo> til = tnl[i].list_types ();
            for (uint j = 0; j < til.size(); j++)
              if (til[j].name () == tname)
                {
                  Type::Info *rawti = new Type::Info (til[j]);
                  ref_sink (rawti);
                  Type t (*rawti);
                  unref (rawti);
                  return t;
                }
          }
    }
  // type_registry_mutex must be unlocked for ::notype()
  return Type::notype();
}

void
Type::register_package (uint        static_data_length,
                        const char *static_data)
{
  AutoLocker locker (type_registry_mutex);
  String err = type_registry.register_type_package (static_data_length,
                                                    reinterpret_cast<const uint8*> (static_data));
  if (err != "")
    error ("%s", err.c_str());
}

static char* /* return malloc()-ed buffer containing a full read of FILE */
file_memread (FILE   *stream,
              size_t *lengthp)
{
  size_t sz = 256;
  char *malloc_string = (char*) malloc (sz);
  if (!malloc_string)
    return NULL;
  char *start = malloc_string;
  errno = 0;
  while (!feof (stream))
    {
      size_t bytes = fread (start, 1, sz - (start - malloc_string), stream);
      if (bytes <= 0 && ferror (stream) && errno != EAGAIN)
        {
          start = malloc_string; // error/0-data
          break;
        }
      start += bytes;
      if (start == malloc_string + sz)
        {
          bytes = start - malloc_string;
          sz *= 2;
          char *newstring = (char*) realloc (malloc_string, sz);
          if (!newstring)
            {
              start = malloc_string; // error/0-data
              break;
            }
          malloc_string = newstring;
          start = malloc_string + bytes;
        }
    }
  int savederr = errno;
  *lengthp = start - malloc_string;
  if (!*lengthp)
    {
      free (malloc_string);
      malloc_string = NULL;
    }
  errno = savederr;
  return malloc_string;
}

void
Type::register_package_file (const String &filename)
{
  FILE *file = fopen (filename.c_str(), "r");
  if (!file)
    error ("failed to open \"%s\": %s", filename.c_str(), strerror (errno));
  size_t len = 0;
  char *contents = file_memread (file, &len);
  if (len)
    {
      register_package (len, contents);
      contents = NULL; // make contents static by "leak"-ing
    }
  else
    error ("failed to read \"%s\": %s", filename.c_str(), strerror (errno ? errno : ENODATA));
  fclose (file);
  if (contents)
    free (contents);
}

} // Rapicorn
