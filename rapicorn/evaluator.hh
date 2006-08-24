/* Rapicorn
 * Copyright (C) 2002-2006 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
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
private:
  const char*   expand_variable                 (const char   *expression,
                                                 String       &result);
  const char*   expand_formula                  (const char   *expression,
                                                 String       &result);
  String        lookup                          (const String &var);
  VariableMap   default_map;
  std::list<VariableMap*> env_maps;
public:
  explicit      Evaluator                      ();
  static String canonify_key                   (const String       &key); /* chars => [A-Za-z0-9-], 'id' => 'name' */
  static void   populate_map                   (VariableMap        &vmap,
                                                const ArgumentList &args);
  void          push_map                       (VariableMap        *vmap);
  VariableMap*  pop_map                        ();
  void          set                            (const String &key_eq_utf8string);
  String        expand_expression_variables    (const String &expression);
  String        expand_expression              (const String &expression);
};

} // Rapicorn

#endif /* __RAPICORN_EVALUATOR_HH__ */
