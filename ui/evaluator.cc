// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#include "evaluator.hh"
#include "sinfex.hh"
#include "../configure.h"

#include <stdlib.h>

namespace Rapicorn {

typedef Evaluator::VariableMapList VariableMapList;

static Sinfex::Value
variable_map_list_lookup (const VariableMapList &env_maps,
                          const String          &var,
                          bool                  &unknown)
{
  String key = Evaluator::canonify_key (var);
  for (VariableMapList::const_iterator it = env_maps.begin(); it != env_maps.end(); it++)
    {
      Evaluator::VariableMap::const_iterator cit = (*it)->find (key);
      if (cit != (*it)->end())
        return Sinfex::Value (cit->second);
    }
  if (key == "RAPICORN_VERSION")
    return Sinfex::Value (RAPICORN_VERSION);
  else if (key == "RAPICORN_ARCHITECTURE")
    return Sinfex::Value (RAPICORN_ARCH_NAME);
  else if (key == "RANDOM")
    return Sinfex::Value (lrand48());
  else
    {
      unknown = true;
      return Sinfex::Value ("");
    }
}

class VariableMapListScope : public Sinfex::Scope {
  const VariableMapList &vml_;
public:
  typedef Sinfex::Value Value;
  VariableMapListScope (const VariableMapList &vml) :
    vml_ (vml)
  {}
  virtual Value
  resolve_variable (const String &entity,
                    const String &name)
  {
    bool unknown = false;
    const String fullname = entity == "" ? name : entity + "." + name;
    Value result = variable_map_list_lookup (vml_, fullname, unknown);
    if (unknown)
      critical ("Evaluator: failed to resolve variable reference: %s", fullname.c_str());
    return result;
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
    critical ("%s: unknown function call: %s", "Rapicorn::Evaluator::Scope", funcstring.c_str());
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
    critical ("%s: unable to pop map: %p != %p", STRFUNC, &vmap, env_maps.back());
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
        critical ("%s: invalid 'key=value' syntax: %s", STRFUNC, it->c_str());
    }
}

void
Evaluator::populate_map (VariableMap        &vmap,
                         const ArgumentList &variable_names,
                         const ArgumentList &variable_values)
{
  const size_t cmax = min (variable_names.size(), variable_values.size());
  for (size_t i = 0; i < cmax; i++)
    vmap[variable_names[i]] = variable_values[i];
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
              critical ("%s: unterminated expression in: %s", warning_entity, expression.c_str());
              return result;
            }
          if (d + 1 >= e)
            result += '`';
          else
            {
              const String expr = expression.substr (d + 1, e - d - 1);
              SinfexP sinfex = Sinfex::parse_string (expr);
              Sinfex::Value value = sinfex->eval (scope);
              result += value.string();
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
