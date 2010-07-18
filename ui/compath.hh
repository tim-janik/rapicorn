/* Rapicorn
 * Copyright (C) 2010 Tim Janik
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
#ifndef __RAPICORN_COMPATH_HH__
#define __RAPICORN_COMPATH_HH__

#include <ui/item.hh>

namespace Rapicorn {

struct ComponentMatcher : protected NonCopyable {
  enum StepType   { ORIGIN, BELOW, EXPRESSION };
  explicit                 ComponentMatcher (StepType st);
  virtual                 ~ComponentMatcher ();
  inline StepType          step             () { return m_step; }
  static ComponentMatcher* parse_path       (String path);
private:
  const StepType        m_step;
  class Parser;
};

struct ComponentMatcherExpression : public ComponentMatcher {
  enum OpType { INVALID, EQUAL, UNEQUAL, MATCH };
  explicit      ComponentMatcherExpression (String prop, OpType op, String val);
  inline String attribute                  () { return m_attribute; }
  inline OpType operator_                  () { return m_operator; }
  inline String value                      () { return m_value; }
private:
  const String m_attribute, m_value;
  const OpType m_operator;
};

struct ComponentMatcherSegment : public ComponentMatcher {
  typedef vector<ComponentMatcher*> Matchers;
  explicit                  ComponentMatcherSegment (StepType st,
                                                     String   typetag,
                                                     String   idselector);
  /*Des*/                  ~ComponentMatcherSegment ();
  void                      add_predicate    (ComponentMatcher &cmatch);
  inline String             typetag          () { return m_typetag; }
  inline String             idselector       () { return m_idselector; }
  inline const Matchers&    predicates       () { return m_predicates; }
  ComponentMatcherSegment*  next             () { return m_next; }
  void                      link             (ComponentMatcherSegment &cms);
private:
  const String              m_typetag;
  const String              m_idselector;
  Matchers                  m_predicates;
  ComponentMatcherSegment  *m_next;
};

vector<Item*>   collect_items    (Item             &origin,
                                  ComponentMatcher &cmatch);

} // Rapicorn

#endif  /* __RAPICORN_COMPATH_HH__ */
