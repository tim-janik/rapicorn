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
  m_info (ref (tinfo))
{}

Type::Type (const Type &src) :
  m_info (ref (src.m_info))
{}

Type::~Type ()
{
  Info *old_info = &m_info;
  *(Info**) &m_info = NULL;
  if (old_info)
    unref (old_info);
}

} // Rapicorn
