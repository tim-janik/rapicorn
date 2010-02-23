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

#include <ui/item.hh>
#include <list>

namespace Rapicorn {

struct Evaluator {
  typedef std::list<String>             ArgumentList; /* elements: key=utf8string */
  typedef std::map<String,String>       VariableMap;
  typedef std::list<const VariableMap*> VariableMapList;
  static String     canonify_name   (const String       &key); /* chars => [A-Za-z0-9_] */
  static String     canonify_key    (const String       &key); /* canonify, id=>name, strip leading '_' */
  static bool       split_argument  (const String       &argument,
                                     String             &key,
                                     String             &value);
  static void       populate_map    (VariableMap        &vmap,
                                     const ArgumentList &args);
  void              push_map        (const VariableMap  &vmap);
  void              pop_map         (const VariableMap  &vmap);
  String            parse_eval      (const String       &expression);
private:
  VariableMapList   env_maps;
};

} // Rapicorn

#endif /* __RAPICORN_EVALUATOR_HH__ */
