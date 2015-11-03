// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_EVALUATOR_HH__
#define __RAPICORN_EVALUATOR_HH__

#include <ui/widget.hh>
#include <list>

namespace Rapicorn {

/** Simple infix expression parser and evaluator.
 * For a sample read-eval-print loop, see: <tt>ui/tests/servertests --shell</tt>.
 */
struct Evaluator {
  typedef std::vector<String>           ArgumentList; /* elements: key=utf8string */
  typedef std::map<String,String>       VariableMap;
  typedef std::list<const VariableMap*> VariableMapList;
  static String     canonify_name   (const String       &key); /* chars => [A-Za-z0-9_] */
  static String     canonify_key    (const String       &key); /* canonify, id=>name, strip leading '_' */
  static bool       split_argument  (const String       &argument,
                                     String             &key,
                                     String             &value);
  static void       populate_map    (VariableMap        &vmap,
                                     const ArgumentList &args);
  static void       populate_map    (VariableMap &vmap, const String &variable_name, const String &variable_value);
  static void       populate_map    (VariableMap        &vmap,
                                     const ArgumentList &variable_names,
                                     const ArgumentList &variable_values);
  void              push_map        (const VariableMap  &vmap);
  void              pop_map         (const VariableMap  &vmap);
  String            parse_eval      (const String       &expression);
private:
  VariableMapList   env_maps;
};

} // Rapicorn

#endif /* __RAPICORN_EVALUATOR_HH__ */
