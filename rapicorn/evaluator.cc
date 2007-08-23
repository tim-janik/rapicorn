/* Rapicorn
 * Copyright (C) 2002-2006 Tim Janik
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
#include "evaluator.hh"
#include "../configure.h"
using namespace std;

namespace { // Anon
using namespace Rapicorn;

typedef String (*Function)      (const list<String> args,
                                 Evaluator         &evaluator);
static Function lookup_function (const String &name);

} // Anon

namespace Rapicorn {

Evaluator::~Evaluator ()
{}

Evaluator::Evaluator()
{
  default_map["RAPICORN_VERSION"] = RAPICORN_VERSION;
  default_map["RAPICORN_ARCHITECTURE"] = RAPICORN_ARCH_NAME;
}

static inline const char*
advance_arg (const char *c)
{
  while (*c && *c != ')' && *c != ',')
    if (*c == '$' && c[1] == '(')
      {
        c += 2;
        uint level = 1;
        while (level)   /* read til ')' */
          switch (*c++)
            {
            case 0:     level = 0;      c--;    break;
            case '(':   level++;                break;
            case ')':   level--;                break;
            }
      }
    else
      c++;
  return c;
}

const char*
Evaluator::expand_formula (const char  *expression,
                           String      &result)
{
  const char *c = expression, *last = c;
  list<String> args;
  /* read up function name */
  while (*c && *c != ')' && *c != ',' && *c != ' ' && *c != '\t')
    c++;
  args.push_back (String (last, c - last));
  while (*c == ' ' || *c == '\t')
    c++;        /* ignore spaces after function name */
  /* parse args if any */
  if (*c && *c != ')')
    {
      last = *c == ',' ? ++c : c;
      /* read up ','-seperated args */
      while (*c && *c != ')')
        {
          if (*c == ',')
            {
              args.push_back (String (last, c - last));
              last = ++c;
            }
          else
            c = advance_arg (c);
        }
      args.push_back (String (last, c - last));
    }
  if (!*c)
    throw Exception ("malformed formula: $(", expression);
  else
    c++;        /* skip ')' */
  result += lookup_function (*args.begin()) (args, *this);
  return c;
}

const char*
Evaluator::expand_variable (const char  *expression,
                            String      &result)
{
  static const char ident_start[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ" "abcdefghijklmnopqrstuvwxyz" "_";
  static const char ident_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ" "abcdefghijklmnopqrstuvwxyz" "_." "0123456789";
  const char *c = expression;
  if (strchr (ident_start, *c))
    {
      c++;
      while (*c && strchr (ident_chars, *c))
        c++;
      String var (expression, c - expression);
      String val = lookup (var);        // FIXME: shortcut expansion of env->name here?
      result += expand_expression (val);
    }
  else if (*c)  /* eat one non-ident char */
    c++;
  return c;
}

String
Evaluator::expand_expression (const String &expression)
{
  const char *c = expression.c_str();
  const char *d = strchr (c, '$');
  String result;
  while (d)
    {
      result += String (c, d - c);
      if (d[1] == '$')
        {
          result += '$';
          c = d + 2;
        }
      else if (d[1] == '(')
        c = expand_formula (d + 2, result);
      else
        c = expand_variable (d + 1, result);
      d = strchr (c, '$');
    }
  result += c;
  return result;
}

String
Evaluator::lookup (const String &var)
{
  String key = canonify_key (var);
  for (list<const VariableMap*>::const_iterator it = env_maps.begin(); it != env_maps.end(); it++)
    {
      VariableMap::const_iterator cit = (*it)->find (key);
      if (cit != (*it)->end())
        return cit->second;
    }
  VariableMap::const_iterator cit = default_map.find (key);
  if (cit != default_map.end())
    return cit->second;
  if (key == "RANDOM")
    return string_from_uint (lrand48());
  return "";
}

void
Evaluator::push_map (const VariableMap &vmap)
{
  env_maps.push_back (&vmap);
}

void
Evaluator::pop_map (const VariableMap  &vmap)
{
  if (!env_maps.size() || &vmap != env_maps.back())
    error ("invalid map for pop(): %p != %p", &vmap, env_maps.back());
  env_maps.pop_back();
}

String
Evaluator::canonify (const String &key)
{
  /* chars => [A-Za-z0-9_] */
  String s = key;
  for (uint i = 0; i < s.size(); i++)
    if (!((s[i] >= 'A' && s[i] <= 'Z') ||
          (s[i] >= 'a' && s[i] <= 'z') ||
          (s[i] >= '0' && s[i] <= '9') ||
          s[i] == '_'))
      s[i] = '_';
  return s;
}

String
Evaluator::canonify_key (const String &key)
{
  /* chars => [A-Za-z0-9_], 'id' => 'name' */
  if (key == "id")
    return "name";
  else
    return canonify (key);
}

void
Evaluator::populate_map (VariableMap        &vmap,
                         const ArgumentList &args)
{
  for (ArgumentList::const_iterator it = args.begin(); it != args.end(); it++)
    {
      const char *key_value = it->c_str();
      const char *equal = strchr (key_value, '=');
      if (!equal || equal <= key_value)
        throw Exception ("Invalid 'argument=value' syntax: ", *it);
      String key = it->substr (0, equal - key_value);
      vmap[canonify_key (key)] = equal + 1;
    }
}

void
Evaluator::populate_map (VariableMap       &vmap,
                         const VariableMap &args)
{
  for (VariableMap::const_iterator it = args.begin(); it != args.end(); it++)
    {
      const String &key = it->first;
      const String &value = it->second;
      vmap[canonify_key (key)] = value;
    }
}

void
Evaluator::replenish_map (VariableMap        &vmap,
                          const ArgumentList &args)
{
  for (ArgumentList::const_iterator it = args.begin(); it != args.end(); it++)
    {
      const char *key_value = it->c_str();
      const char *equal = strchr (key_value, '=');
      if (!equal || equal <= key_value)
        throw Exception ("Invalid 'argument=value' syntax: ", *it);
      String key = it->substr (0, equal - key_value);
      key = canonify_key (key);
      if (vmap.find (key) == vmap.end())
        vmap[key] = equal + 1;
    }
}

void
Evaluator::replenish_map (VariableMap       &vmap,
                          const VariableMap &args)
{
  for (VariableMap::const_iterator it = args.begin(); it != args.end(); it++)
    {
      const String &mkey = it->first;
      const String &value = it->second;
      String key = canonify_key (mkey);
      if (vmap.find (key) == vmap.end())
        vmap[key] = value;
    }
}

void
Evaluator::set (const String &key_eq_utf8string)
{
  const char *key_value = key_eq_utf8string.c_str();
  const char *equal = strchr (key_value, '=');
  if (!equal || equal <= key_value)
    throw Exception ("Invalid 'argument=value' syntax: ", key_eq_utf8string);
  String key = key_eq_utf8string.substr (0, equal - key_value);
  key = canonify_key (key);
  if (!equal[1])
    default_map.erase (key);
  else
    default_map[key] = equal + 1;
}

String
Evaluator::debug_dump (void)
{
  String result;
  result += "[\n";
  for (list<const VariableMap*>::const_iterator it = env_maps.begin(); it != env_maps.end(); it++)
    {
      result += "  {";
      for (VariableMap::const_iterator cit = (*it)->begin(); cit != (*it)->end(); cit++)
        result += "\n    " + cit->first + " = " + cit->second + ",";
      result += " },\n";
    }
  result += "  { # default_map";
  for (VariableMap::const_iterator cit = default_map.begin(); cit != default_map.end(); cit++)
    result += "\n    " + cit->first + " = " + cit->second + ",";
  if (!default_map.size())
    result += "\n ";
  result += " },\n";
  result += "]";
  return result;
}

} // Rapicorn

namespace { // Anon

static inline const String
next_arg (Evaluator                    &evaluator,
          const list<String>           &args,
          list<String>::const_iterator &it,
          const String                 &fallback = "")
{
  const String &arg = it != args.end() ? *it++ : fallback;
  if (arg.find ('$') != arg.npos)
    return evaluator.expand_expression (arg);
  return arg;
}

static String
defun_bool (const list<String> args,
            Evaluator         &evaluator)
{
  list<String>::const_iterator it = args.begin();
  const String &func_name = next_arg (evaluator, args, it);
  const String &arg1 = next_arg (evaluator, args, it);
  bool b = string_to_bool (arg1);
  if (func_name == "not")
    b = !b;
  return b ? "1" : "0";
}

static String
defun_count (const list<String> args,
             Evaluator         &evaluator)
{
  list<String>::const_iterator it = args.begin();
  next_arg (evaluator, args, it);  // skip function name
  uint64 count = 0;
  while (it != args.end())
    {
      count++;
      it++;
    }
  return string_from_uint (count);
}

static String
defun_bool_logic (const list<String> args,
                  Evaluator         &evaluator)
{
  list<String>::const_iterator it = args.begin();
  const String &func_name = next_arg (evaluator, args, it);
  bool result = func_name == "and" || func_name == "nand";
  while (it != args.end())
    {
      bool arg = string_to_bool (next_arg (evaluator, args, it));
      if      (func_name == "xor" || func_name == "xnor")
        result ^= arg;
      else if (func_name == "or"  || func_name == "nor")
        result |= arg;
      else if (func_name == "and" || func_name == "nand")
        result &= arg;
    }
  if (func_name == "xnor" || func_name == "nor" || func_name == "nand")
    result = !result;
  return result ? "1" : "0";
}

static String
defun_float_comparisons (const list<String> args,
                         Evaluator         &evaluator)
{
  list<String>::const_iterator it = args.begin();
  const String &func_name = next_arg (evaluator, args, it);
  double last = string_to_double (next_arg (evaluator, args, it));
  do
    {
      double arg = string_to_double (next_arg (evaluator, args, it));
      bool match = false;
      if      (func_name == "lt")
        match = last < arg;
      else if (func_name == "le")
        match = last <= arg;
      else if (func_name == "eq")
        match = last == arg;
      else if (func_name == "ge")
        match = last >= arg;
      else if (func_name == "gt")
        match = last > arg;
      else if (func_name == "ne")
        match = last != arg;
      if (!match)
        return "0";
      last = arg;
    }
  while (it != args.end());
  return "1";
}

static String
defun_string_comparisons (const list<String> args,
                          Evaluator         &evaluator)
{
  list<String>::const_iterator it = args.begin();
  const String &func_name = next_arg (evaluator, args, it);
  String last = next_arg (evaluator, args, it);
  do
    {
      const String &arg = next_arg (evaluator, args, it);
      bool match = false;
      if      (func_name == "strlt")
        match = last.compare (arg) < 0;
      else if (func_name == "strle")
        match = last.compare (arg) <= 0;
      else if (func_name == "streq")
        match = last.compare (arg) == 0;
      else if (func_name == "strge")
        match = last.compare (arg) >= 0;
      else if (func_name == "strgt")
        match = last.compare (arg) > 0;
      else if (func_name == "strne")
        match = last.compare (arg) != 0;
      // g_printerr ("\ndefun_string_comparisons: %s: \"%s\" \"%s\" = %d\n", func_name.c_str(), last.c_str(), arg.c_str(), match);
      if (!match)
        return "0";
      last = arg;
    }
  while (it != args.end());
  return "1";
}

static String
defun_println (const list<String> args,
               Evaluator         &evaluator)
{
  list<String>::const_iterator it = args.begin();
  next_arg (evaluator, args, it);  // skip function name
  String msg;
  while (it != args.end())
    msg += *it++;
  errmsg ("Evaluator", msg);
  return "";
}

static String
unknown_function (const list<String> args,
                  Evaluator         &evaluator)
{
  warning ("Evaluator: no such function: %s", args.begin()->c_str());
  return "";
}

static Function
lookup_function (const String &name)
{
  static const struct { const char *name; Function func; } funcs[] = {
    { "count",          defun_count },
    { "bool",           defun_bool },
    { "not",            defun_bool },
    { "xor",            defun_bool_logic },
    { "xnor",           defun_bool_logic },
    { "or",             defun_bool_logic },
    { "nor",            defun_bool_logic },
    { "and",            defun_bool_logic },
    { "nand",           defun_bool_logic },
    { "lt",             defun_float_comparisons },
    { "le",             defun_float_comparisons },
    { "eq",             defun_float_comparisons },
    { "ge",             defun_float_comparisons },
    { "gt",             defun_float_comparisons },
    { "ne",             defun_float_comparisons },
    { "strlt",          defun_string_comparisons },
    { "strle",          defun_string_comparisons },
    { "streq",          defun_string_comparisons },
    { "strge",          defun_string_comparisons },
    { "strgt",          defun_string_comparisons },
    { "strne",          defun_string_comparisons },
    { "println",        defun_println },
  };
  for (uint i = 0; i < ARRAY_SIZE (funcs); i++)
    if (name == funcs[i].name)
      return funcs[i].func;
  return unknown_function;
}

} // Anon
