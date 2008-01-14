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

#define IS_RAPICORN_TYPE_STORAGE(strg)  (strchr (RAPICORN_TYPE_STORAGE_CHARS, strg))

const char*
Type::type_name (Type::Storage storage)
{
  switch (storage)
    {
    case Type::NUM:             return "NUM";
    case Type::FLOAT:           return "FLOAT";
    case Type::STRING:          return "STRING";
    case Type::ARRAY:           return "ARRAY";
    case Type::STRING_VECTOR:   return "STRING_VECTOR";
    case Type::OBJECT:          return "OBJECT";
    }
  return NULL;
}

static const char*
parse_hexnum (const char *input,
              uint       &pos,
              int64      &result,
              uint        n_digits)
{
  static const char *NFE = "hexnum parse error";
  result = 0;
  while (n_digits--)
    {
      if (!strchr ("ABCDEFabcdef0123456789", input[pos]))
        return NFE;
      result <<= 4;
      result += input[pos] >= 'a' ? input[pos] - 'a' + 10 : input[pos] >= 'A' ? input[pos] - 'A' + 10 : input[pos] > '0' ? input[pos] - '0' : 0;
      pos++;
    }
  return NULL;
}

static const char*
parse_string (const char *input,
              uint       &pos,
              String     &result,
              uint        n_digits)
{
  static const char *SSE = "short string encountered";
  int64 n;
  const char *err = parse_hexnum (input, pos, n, n_digits);
  if (err)
    return err;
  while (n--)
    {
      if (!input[pos])
        return SSE;
      result += input[pos++];
    }
  return NULL;
}

static const char*
parse_assignment (const char *assignment,
                  uint       *offset)
{
  static const char *IKA = "invalid key=value assignment syntax";
  if (!((assignment[0] >= 'A' && assignment[0] <= 'Z') ||
        (assignment[0] >= 'a' && assignment[0] <= 'z') ||
        (assignment[0] >= '0' && assignment[0] <= '9') ||
        assignment[0] == '_'))
    return IKA;
  uint i = 1;
  while (strchr ("abcdefghijklmnopqrstuvwxyz0123456789_ABCDEFGHIJKLMNOPQRSTUVWXYZ", assignment[i]))
    i++;
  if (assignment[i] != '=')
    return IKA;
  *offset = i + 1;
  return NULL;
}

static const char*
type_parser (char const *input,
             Type::Info &ti)
{
  static const char *ITD = "format error";
  /* format:
   * <StorageChar>US
   */
  uint pos = 0;
  // parse magic indicator
  if (input[pos++] != 'R')
    return ITD;
  if (input[pos++] != 'c')
    return ITD;
  if (input[pos++] != 'T')
    return ITD;
  if (input[pos++] != 'i')
    return ITD;
  if (input[pos++] != ';')
    return ITD;
  // parse type name
  const char *err = parse_string (input, pos, ti.name, 4);
  if (err)
    return err;
  if (input[pos++] != ';')
    return ITD;
  // parse storage type
  ti.storage = Type::Storage (input[pos++]);
  if (!ti.storage || !IS_RAPICORN_TYPE_STORAGE (ti.storage))
    return ITD;
  if (input[pos++] != ';')
    return ITD;
  // parse aux data
  while (input[pos] && input[pos] != ';')
    {
      uint offset = 0;
      String astr;
      err = parse_string (input, pos, astr, 4);
      if (err)
        return err;
      err = parse_assignment (astr.c_str(), &offset);
      if (err)
        return err;
      String name = String (astr, 0, offset - 1);
      String value = String (astr, offset);
      ti.auxdata.push_back (name + "=" + value);
    }
  if (input[pos++] != ';')
    return ITD;
  return NULL;
}

Type::Type (char const *_typedef) :
  m_info()
{
  const char *err = type_parser (_typedef, m_info);
  if (err)
    RAPICORN_ERROR ("invalid type definition (%s): \"%s\"", err, string_to_cquote (_typedef).c_str());
}

String
Type::ident () const
{
  return m_info.name;
}

String
Type::label () const
{
  String l = aux_string ("label");
  if (l.size() == 0)
    return ident();
  return l;
}

String
Type::blurb () const
{
  return aux_string ("blurb");
}

String
Type::hints () const
{
  String h = aux_string ("hints");
  if (h.size() == 0 || h[0] != ':')
    h = ":" + h;
  if (h[h.size()-1] != ':')
    h = h + ":";
  return h;
}

Type::Storage
Type::storage () const
{
  return m_info.storage;
}

String
Type::aux_string (const String &auxname) const
{
  if (!strchr (auxname.c_str(), '='))
    {
      String prefix = auxname + "=";
      for (vector<String>::const_iterator it = m_info.auxdata.begin(); it != m_info.auxdata.end(); it++)
        if ((*it).compare (0, prefix.size(), prefix) == 0)
          return (*it).c_str() + prefix.size();
    }
  return "";
}

long double
Type::aux_float (const String &auxname) const
{
  return string_to_double (aux_string (auxname));
}

int64
Type::aux_num (const String &auxname) const
{
  return string_to_int (aux_string (auxname));
}

} // Rapicorn
