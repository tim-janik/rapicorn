/* Rapicorn
 * Copyright (C) 2005 Tim Janik
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
#include "enumdefs.hh"
#include <cstring>

namespace Rapicorn {

static bool
isalnum (uint8 c)
{
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
}

static inline char
char_canon (char c)
{
  if (c >= '0' && c <= '9')
    return c;
  else if (c >= 'A' && c <= 'Z')
    return c - 'A' + 'a';
  else if (c >= 'a' && c <= 'z')
    return c;
  else
    return '-';
}

static inline bool
eval_match (const char *str1,
            const char *str2)
{
  while (*str1 && *str2)
    {
      uint8 s1 = char_canon (*str1++);
      uint8 s2 = char_canon (*str2++);
      if (s1 != s2)
        return false;
    }
  return *str1 == 0 && *str2 == 0;
}

static bool
name_match_detailed (const char *value_name1,
                     uint        name1_length,
                     const char *value_name2,
                     uint        name2_length)
{
  if (name2_length > name1_length)
    {
      const char *ts = value_name2;
      value_name2 = value_name1;
      value_name1 = ts;
      uint tl = name2_length;
      name2_length = name1_length;
      name1_length = tl;
    }
  const char *name1 = value_name1 + name1_length - min (name1_length, name2_length);
  const char *name2 = value_name2 + name2_length - min (name1_length, name2_length);
  if (name1 > value_name1)      /* allow partial matches on word boundary only */
    {
      if (isalnum (name1[-1]) && isalnum (name1[0])) /* no word boundary */
        return false;
    }
  return name2[0] && eval_match (name1, name2);
}

bool
EnumClass::match_partial (const char *value_name1, const char *partial_value_name) const
{
  return_val_if_fail (value_name1 != NULL, false);
  return_val_if_fail (partial_value_name != NULL, false);
  uint length1 = strlen (value_name1);
  uint length2 = strlen (partial_value_name);
  if (length1 < length2)        /* length1 must be >= length2 for partial matches */
    return false;
  return name_match_detailed (value_name1, length1, partial_value_name, length2);
}

bool
EnumClass::match (const char *value_name1, const char *value_name2) const
{
  return_val_if_fail (value_name1 != NULL, false);
  return_val_if_fail (value_name2 != NULL, false);
  return name_match_detailed (value_name1, strlen (value_name1), value_name2, strlen (value_name2));
}

const EnumClass::Value*
EnumClass::find_first (int64 value) const
{
  value = constrain (value);
  uint n_values = 0;
  const Value *values;
  list_values (n_values, values);
  for (uint i = 0; i < n_values; i++)
    if (values[i].value == value)
      return &values[i];
  return NULL;
}

const EnumClass::Value*
EnumClass::find_first (const String &value_name) const
{
  const char *vname = value_name.c_str();
  uint n_values = 0;
  const Value *values;
  list_values (n_values, values);
  uint length2 = strlen (vname);
  for (uint i = 0; i < n_values; i++)
    {
      uint length1 = values[i].name_length;
      if (length1 < length2)    /* length1 must be >= length2 for partial matches */
        continue;
      if (name_match_detailed (values[i].value_name, length1, vname, length2))
        return &values[i];
    }
  return NULL;
}

int64
EnumClass::parse (const char *value_string,
                  String     *error_string) const
{
  if (!flag_combinable())
    {
      const Value *v = find_first (value_string);
      if (v)
        return v->value;
      if (error_string)
        *error_string = value_string;
      return constrain (0);
    }
  /* parse flags */
  const char *sep = strchr (value_string, '|');
  uint64 value = 0;
  while (sep)
    {
      String s (value_string, sep - value_string);
      value |= parse (s.c_str(), error_string);
      value_string = sep + 1;
    }
  const Value *v = find_first (value_string);
  if (v)
    value |= v->value;
  else if (error_string)
    *error_string = sep;
  return constrain (value);
}

String
EnumClass::string (int64 orig_value) const
{
  int64 value = constrain (orig_value);
  if (!flag_combinable())
    {
      const Value *v = find_first (value);
      return v ? v->value_name : "";
    }
  uint n_values = 0;
  const Value *values;
  list_values (n_values, values);
  String s;
  /* combine flags */
  while (value)
    {
      const Value *match1 = NULL;
      for (uint i = 0; i < n_values; i++)
        if ((value & values[i].value) == values[i].value)
          /* choose the greatest match, needed by mixed flags/nume types (StateMask) */
          match1 = match1 && match1->value > values[i].value ? match1 : &values[i];
      if (match1)
        {
          if (s[0])
            s = String (match1->value_name) + "|" + s;
          else
            s = String (match1->value_name);
          value &= match1->value;
        }
      else
        value = 0; /* no match */
    }
  if (!s[0] && value == 0)
    for (uint i = 0; i < n_values; i++)
      if (values[i].value == 0)
        {
          s = values[i].value_name;
          break;
        }
  return s;
}

/* include generated enum definitions */
#include "gen-enums.cc"

} // Rapicorn
