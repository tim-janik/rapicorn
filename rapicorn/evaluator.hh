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
#ifndef __RAPICORN_EVALUATOR_HH__
#define __RAPICORN_EVALUATOR_HH__

#include <rapicorn/item.hh>
#include <rapicorn/handle.hh>
#include <list>

namespace Rapicorn {
class Evaluator {
public:
  typedef std::map<String,String> VariableMap;
  typedef std::list<String>       ArgumentList; /* elements: key=utf8string */
  virtual      ~Evaluator                       (); // FIXME: destructor required by gcc-3.4
private:
  const char*   expand_variable                 (const char   *expression,
                                                 String       &result);
  const char*   expand_formula                  (const char   *expression,
                                                 String       &result);
  String        lookup                          (const String &var);
  VariableMap   default_map;
  std::list<const VariableMap*> env_maps;
public:
  explicit      Evaluator                      ();
  static String canonify                       (const String       &key); /* chars => [A-Za-z0-9_] */
  static String canonify_key                   (const String       &key); /* canonify, 'id' => 'name' */
  static void   populate_map                   (VariableMap        &vmap,
                                                const ArgumentList &args);
  static void   populate_map                   (VariableMap        &vmap,
                                                const VariableMap  &args);
  static void   replenish_map                  (VariableMap        &vmap,
                                                const ArgumentList &args);
  static void   replenish_map                  (VariableMap        &vmap,
                                                const VariableMap  &args);
  void          push_map                       (const VariableMap  &vmap);
  void          pop_map                        (const VariableMap  &vmap);
  void          set                            (const String &key_eq_utf8string);
  String        expand_expression              (const String &expression);
};

} // Rapicorn

#endif /* __RAPICORN_EVALUATOR_HH__ */
