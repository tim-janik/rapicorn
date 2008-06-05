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
#include "sinfex.hh"
#include "../configure.h"
using namespace std;

namespace Rapicorn {

typedef Evaluator::VariableMapList VariableMapList;

static Sinfex::Value
variable_map_list_lookup (const VariableMapList &env_maps,
                          const String          &var)
{
  String key = Evaluator::canonify_key (var);
  for (VariableMapList::const_iterator it = env_maps.begin(); it != env_maps.end(); it++)
    {
      Evaluator::VariableMap::const_iterator cit = (*it)->find (key);
      if (cit != (*it)->end())
        {
          const String &v = cit->second;
          if (v.size() == 0)
            return Sinfex::Value ("");
          if (v[0] == '0' && v.size() > 1 && (v[1] == 'x' || v[1] == 'X'))
            return Sinfex::Value (string_to_uint (v));
          if (v[0] >= '0' && v[0] <= '9')
            return Sinfex::Value (string_to_double (v));
          if (v.size() > 1 && v[0] == '.' && v[1] >= '0' && v[1] <= '9')
            return Sinfex::Value (string_to_double (v));
          if (v[0] == '"' || v[0] == '\'')
            return Sinfex::Value (string_from_cquote (v));
          warning ("invalid variable value syntax: %s", v.c_str());
          return Sinfex::Value (nanl ("0xbadfeed"));
        }
    }
  if (key == "RAPICORN_VERSION")
    return Sinfex::Value (RAPICORN_VERSION);
  else if (key == "RAPICORN_ARCHITECTURE")
    return Sinfex::Value (RAPICORN_ARCH_NAME);
  else if (key == "RANDOM")
    return Sinfex::Value (lrand48());
  else
    return Sinfex::Value ("");
}

class VariableMapListScope : public Sinfex::Scope {
  const VariableMapList &m_vml;
public:
  typedef Sinfex::Value Value;
  VariableMapListScope (const VariableMapList &vml) :
    m_vml (vml)
  {}
  virtual Value
  resolve_variable (const String &entity,
                    const String &name)
  {
    if (entity.size())
      return variable_map_list_lookup (m_vml, entity + "." + name);
    else
      return variable_map_list_lookup (m_vml, name);
  }
  virtual Value
  call_function (const String        &entity,
                 const String        &name,
                 const vector<Value> &args)
  {
    String funcstring = name + " (";
    for (uint i = 0; i < args.size(); i++)
      {
        if (i)
          funcstring += ", ";
        funcstring += args[i].string();
      }
    funcstring += ")";
    warning ("%s: unknown function call: %s", "Rapicorn::Evaluator::Scope", funcstring.c_str());
    return Value (-1);
  }
};

void
Evaluator::push_map (const VariableMap &vmap)
{
  env_maps.push_back (&vmap);
}

void
Evaluator::pop_map (const VariableMap  &vmap)
{
  if (env_maps.size() || &vmap == env_maps.back())
    env_maps.pop_back();
  else
    warning ("%s: unable to pop map: %p != %p", STRFUNC, &vmap, env_maps.back());
}

String
Evaluator::canonify_name (const String &key) /* chars => [A-Za-z0-9_] */
{
  /* chars => [A-Za-z0-9_] */
  String s = key;
  for (uint i = 0; i < s.size(); i++)
    if ((s[i] >= '0' && s[i] <= '9') ||
        (s[i] >= 'A' && s[i] <= 'Z') ||
        (s[i] >= 'a' && s[i] <= 'z') ||
        s[i] == '_')
      ; // keep char
    else
      s[i] = '_';
  return s;
}

String
Evaluator::canonify_key (const String &key) /* canonify, id=>name, strip leading '_' */
{
  /* skip gettext prefix */
  String s = key[0] == '_' ? String (key, 1) : key;
  /* canonify aliases */
  if (s == "id")
    return "name";
  return canonify_name (s);
}

bool
Evaluator::split_argument (const String       &argument,
                           String             &key,
                           String             &value)
{
  String::size_type e = argument.find ('=');
  if (e == String::npos || e == 0)
    return false;
  key = Evaluator::canonify_key (argument.substr (0, e));
  value = argument.substr (e + 1);
  return true;
}

void
Evaluator::populate_map (VariableMap        &vmap,
                         const ArgumentList &args)
{
  for (ArgumentList::const_iterator it = args.begin(); it != args.end(); it++)
    {
      String key, value;
      if (Evaluator::split_argument (*it, key, value))
        vmap[key] = value;
      else
        warning ("%s: invalid 'key=value' syntax: %s", STRFUNC, it->c_str());
    }
}

static String
expand_eval_expressions (const char           *warning_entity,
                         VariableMapListScope &scope,
                         const String         &expression)
{
  String result;
  std::list<String> strl;
  String::size_type i = 0;
  while (i < expression.size())
    {
      String::size_type d = expression.find ('`', i);
      if (d != String::npos) // found backtick
        {
          result += expression.substr (0, d);
          /* scan until next backtick */
          String::size_type e = d + 1;
          bool terminated = false, escape1 = false, squotes = false, dquotes = false;
          while (e < expression.size())
            {
              bool escapenext = false;
              if (expression.at (e) == '`' && !squotes && !dquotes)
                {
                  terminated = true;
                  break;
                }
              else if (expression.at (e) == '"' && !squotes && !escape1)
                dquotes = !dquotes;
              else if (expression.at (e) == '\'' && !dquotes && !escape1)
                squotes = !squotes;
              else if (expression.at (e) == '\\' && !escape1 && (squotes || dquotes))
                escapenext = true;
              escape1 = escapenext;
              e++;
            }
          if (!terminated)
            {
              warning ("%s: unterminated expression in: %s", warning_entity, expression.c_str());
              return result;
            }
          if (d + 1 >= e)
            result += '`';
          else
            {
              const String expr = expression.substr (d + 1, e - d - 1);
              Sinfex *sinfex = Sinfex::parse_string (expr);
              ref_sink (sinfex);
              Sinfex::Value value = sinfex->eval (scope);
              result += value.string();
              unref (sinfex);
            }
          i = e + 1;
        }
      else
        {
          result += expression.substr (i);
          break;
        }
    }
  return result;
}

String
Evaluator::parse_eval (const String &expression)
{
  VariableMapListScope scope (env_maps);
  return expand_eval_expressions (STRFUNC, scope, expression);
}

} // Rapicorn
