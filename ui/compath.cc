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
#include "compath.hh"
#include "container.hh"
#include "factory.hh"
#include <algorithm>
#include <memory>
#include <string.h>

namespace Rapicorn {
using std::auto_ptr;

/* Grammer:
  path          ::= '/' segment [ '/' segment ]*
  segment       ::= match                                       // | '..'
  match         ::= selector [ '[' predicate ']' ]*
  selector      ::= typetag [ '#' idselector ] | '#' idselector
  typetag       ::= '*' | word
  idselector    ::= word
  word          ::= alpha [alnum | ':_-']*
  predicate     ::= path | expression
  expression    ::= '@' attribute op value
  attribute     ::= word
  op            ::= '==' | '!=' | '=~'
  value         ::= string
  string        ::= '"' [^"]* '"'  |  "'" [^']* "'"
  escstring     ::= '"' ( [^"\\]+ | \\. )* '"'  |  "'" ( [^'\\]+ | \\. )* "'"
*/

struct ComponentMatcher::Parser {
  static ComponentMatcher*
  parse_expression (const char *&c)
  {
    const char *start = c;
    if (*c != '@')
      return NULL;
    c++;
    eat_spaces (c);
    String attribute = parse_word (c);
    eat_spaces (c);
    ComponentMatcherExpression::OpType op = parse_op (c);
    eat_spaces (c);
    const char *mark = c;
    String value = parse_string (c);
    const bool nostring = mark == c;
    if (attribute.empty() || op == ComponentMatcherExpression::INVALID || nostring)
      {
        c = start;
        return NULL;
      }
    return new ComponentMatcherExpression (attribute, op, value);
  }
  static ComponentMatcherExpression::OpType
  parse_op (const char *&c)
  {
    eat_spaces (c);
    if (c[0] == '=' and c[1] == '=')
      {
        c += 2;
        return ComponentMatcherExpression::EQUAL;
      }
    if (c[0] == '!' and c[1] == '=')
      {
        c += 2;
        return ComponentMatcherExpression::UNEQUAL;
      }
    if (c[0] == '=' and c[1] == '~')
      {
        c += 2;
        return ComponentMatcherExpression::MATCH;
      }
    return ComponentMatcherExpression::INVALID;
  }
  static String
  parse_string (const char *&c)
  {
    const char *start = c;
    if (*c != '"' && *c != '\'')
      {
      nostring:
        c = start;
        return "";
      }
    const char starter = *c;
    c++;
    String s;
    while (*c)
      if (*c == starter)
        {
          c++;
          return s;
        }
      else
        {
          s += *c;
          c++;
        }
    goto nostring; // missing end quote
  }
  static String
  parse_escstring (const char *&c)
  {
    const char *start = c;
    if (*c != '"' && *c != '\'')
      {
      nostring:
        c = start;
        return "";
      }
    const char starter = *c;
    c++;
    String s;
    while (*c)
      switch (*c)
        {
        case '\\':
          if (!c[1])
            goto nostring;
          s += c[1];
          c += 2;
          break;
        case '"': case '\'':
          if (*c == starter)
            {
              c++;
              return s;
            }
          // fall through
        default:
          s += *c;
          c++;
        }
    goto nostring; // missing end quote
  }
  static ComponentMatcherSegment*
  parse_path (const char *&c)
  {
    auto_ptr<ComponentMatcherSegment> cmsptr;
    ComponentMatcherSegment *last = NULL;
    eat_spaces (c);
    while (*c == '/')
      {
        c++;
        eat_spaces (c);
        ComponentMatcherSegment *comp = parse_component (c, last ? BELOW : ORIGIN);
        if (!comp)
          return NULL; // slash without valid component
        if (!last)
          last = comp, cmsptr.reset (last);
        else
          last->link (*comp), last = comp;
        eat_spaces (c);
      }
    return cmsptr.release();
  }
  static ComponentMatcherSegment*
  parse_component (const char *&c, StepType stept)
  {
    String typetag = parse_typetag (c);
    eat_spaces (c);
    if (typetag.empty() and *c == '#')
      typetag = "*"; // alows plain idselector matches
    String idselector = parse_hash_idselector (c);
    if (typetag.empty() and idselector.empty())
      return NULL; // missing selector
    ComponentMatcherSegment &cms = *new ComponentMatcherSegment (stept, typetag, idselector);
    auto_ptr<ComponentMatcherSegment> cmsptr (&cms); // auto-clean on returns
    eat_spaces (c);
    while (*c == '[')
      {
        c++;
        eat_spaces (c);
        const char *saved = c;
        ComponentMatcher *pred = parse_path (c);
        if (!pred and saved == c)
          pred = parse_expression (c);
        if (!pred)
          return NULL; // no valid path or expression
        eat_spaces (c);
        if (*c != ']')
          return NULL; // unclosed predicate braces
        c++;
        cms.add_predicate (*pred);
        eat_spaces (c);
      }
    return cmsptr.release();
  }
  static String
  parse_typetag (const char *&c)
  {
    if (*c == '*')
      {
        c++;
        return "*";
      }
    else
      return parse_word (c);
  }
  static String
  parse_hash_idselector (const char *&c)
  {
    if (*c == '#')
      {
        const char *start = c;
        c++;
        eat_spaces (c);
        String w = parse_word (c);
        if (!w.empty())
          return w;
        c = start;
      }
    return ""; // invalid or empty idselector
  }
  static String
  parse_word (const char *&c)
  {
    String s;
    if ((*c >= 'A' && *c <= 'Z') or
        (*c >= 'a' && *c <= 'z') or
        (*c >= '0' && *c <= '9'))
      {
        s += *c;
        c++;
        while ((*c >= 'A' && *c <= 'Z') or
               (*c >= 'a' && *c <= 'z') or
               (*c >= '0' && *c <= '9') or
               *c == ':' or *c == '_' or *c == '-')
          {
            s += *c;
            c++;
          }
      }
    return s;
  }
  static void
  eat_spaces (const char *&c)
  {
    while (*c && strchr (" \t\r\n\v\f", *c))
      c++;
  }
};

ComponentMatcher*
ComponentMatcher::parse_path (String path)
{
  const char *c = path.c_str();
  ComponentMatcher *cm = Parser::parse_path (c);
  Parser::eat_spaces (c);
  if (c[0] == 0 && cm)
    return cm; // success
  if (cm)
    delete cm;
  return NULL; // invalid path or junk after path
}

ComponentMatcher::ComponentMatcher (StepType st) :
  m_step (st)
{}

ComponentMatcher::~ComponentMatcher ()
{}

ComponentMatcherExpression::ComponentMatcherExpression (String prop, OpType op, String val) :
  ComponentMatcher (EXPRESSION), m_attribute (prop), m_value (val), m_operator (op)
{}

ComponentMatcherSegment::ComponentMatcherSegment (StepType st,
                                                  String   typetag,
                                                  String   idselector) :
  ComponentMatcher (st), m_typetag (typetag), m_idselector (idselector), m_next (NULL)
{}

ComponentMatcherSegment::~ComponentMatcherSegment ()
{
  while (!m_predicates.empty())
    {
      ComponentMatcher *p = m_predicates.back();
      m_predicates.pop_back();
      if (p)
        delete p;
    }
  if (m_next)
    {
      ComponentMatcher *p = m_next;
      m_next = NULL;
      if (p)
        delete p;
    }
}

void
ComponentMatcherSegment::add_predicate (ComponentMatcher &cmatch)
{
  m_predicates.push_back (&cmatch);
}

void
ComponentMatcherSegment::link (ComponentMatcherSegment &cms)
{
  return_if_fail (m_next == NULL);
  m_next = &cms;
}

static bool
match_tags (const String     &typetag,
            const StringList &tags)
{
  bool result = false;
  for (StringList::const_iterator it = tags.begin(); it != tags.end() && !result; it++)
    {
      const String &tag = *it;
      String::size_type i = tag.rfind (typetag);
      if (i == String::npos)
        continue;       // no match
      if (i + typetag.size() == tag.size() and  // tail match?
          (i == 0 || tag.data()[i - 1] == ':')) // namespace boundary?
        result = true;
    }
  if (Rapicorn::Logging::debugging())
    {
      String stags;
      for (StringList::const_iterator it = tags.begin(); it != tags.end(); it++)
        stags += string_printf ("%s%s", it == tags.begin() ? "" : " ", it->c_str());
      DEBUG ("MATCH: %s in (%s): %u", typetag.c_str(), stags.c_str(), result);
    }
  return result;
}

static bool
match_predicate (ComponentMatcher &cm,
                 ItemImpl         &item)
{
  switch (cm.step())
    {
    case ComponentMatcher::ORIGIN:
      {
        ComponentMatcherSegment &cmseg = *dynamic_cast<ComponentMatcherSegment*> (&cm);
        vector<ItemImpl*> result = collect_items (item, cmseg);
        return !result.empty();
      }
    case ComponentMatcher::EXPRESSION:
      {
        ComponentMatcherExpression &cmex = *dynamic_cast<ComponentMatcherExpression*> (&cm);
        if (!item.lookup_property (cmex.attribute()))
          return false;
        String pvalue = item.get_property (cmex.attribute());
        bool result;
        switch (cmex.operator_())
          {
          case ComponentMatcherExpression::EQUAL:
            result = pvalue == cmex.value();
            break;
          case ComponentMatcherExpression::UNEQUAL:
            result = pvalue != cmex.value();
            break;
          case ComponentMatcherExpression::MATCH:
            result = Regex::match_simple (cmex.value(), pvalue,
                                          Regex::MULTILINE, Regex::MATCH_NORMAL);
            break;
          default:
            assert_unreached();
          }
        DEBUG ("PREDICATE: %s %u %s (%s): %u\n", cmex.attribute().c_str(), cmex.operator_(),
               cmex.value().c_str(), pvalue.c_str(), result);
        return result;
      }
    default: ;
    }
  assert_unreached();
}

static bool
match_item (ComponentMatcherSegment &cms,
            ItemImpl                &item)
{
  String idselector = cms.idselector();
  if (not (idselector.empty() || item.name().find (idselector) != String::npos))
    return false;
  String typetag = cms.typetag();
  if (typetag != "*")
    {
      StringList tags = Factory::factory_context_tags (item.factory_context());
      if (not match_tags (typetag, tags))
        return false;
    }
  const ComponentMatcherSegment::Matchers &predicates = cms.predicates();
  for (ComponentMatcherSegment::Matchers::const_iterator ci = predicates.begin();
       ci != predicates.end(); ci++)
    if (!match_predicate (**ci, item))
      return false;
  return true;
}

static vector<ItemImpl*>
match_segment (ComponentMatcherSegment &cms,
               ItemImpl                &item)
{
  vector<ItemImpl*> result;
  // either match self
  if (match_item (cms, item))
    {
      result.push_back (&item);
      return result;
    }
  // or match children recursively
  Container *container = dynamic_cast<Container*> (&item);
  if (container)
    for (Container::ChildWalker cw = container->local_children(); cw.has_next(); cw++)
      {
        vector<ItemImpl*> more = match_segment (cms, *cw);
        if (result.empty())
          result.swap (more);
        else
          result.insert (result.end(), more.begin(), more.end());
      }
  return result;
}

vector<ItemImpl*>
collect_items (ItemImpl         &origin,
               ComponentMatcher &cmatch)
{
  vector<ItemImpl*> candidates;
  return_val_if_fail (cmatch.step() == ComponentMatcher::ORIGIN, candidates);

  ComponentMatcherSegment *cmseg = dynamic_cast<ComponentMatcherSegment*> (&cmatch);

  candidates.push_back (&origin);
  for (ComponentMatcherSegment *cms = cmseg; cms; cms = cms->next())
    {
      vector<ItemImpl*> result;
      if (cms->step() == ComponentMatcher::BELOW)
        {
          vector<ItemImpl*> children;
          for (uint j = 0; j < candidates.size(); j++)
            {
              Container *container = dynamic_cast<Container*> (candidates[j]);
              if (!container)
                continue;
              for (Container::ChildWalker cw = container->local_children(); cw.has_next(); cw++)
                children.push_back (&*cw);
            }
          candidates.swap (children);
        }
      for (uint j = 0; j < candidates.size(); j++)
        {
          vector<ItemImpl*> more = match_segment (*cms, *candidates[j]);
          if (result.empty())
            result.swap (more);
          else
            result.insert (result.end(), more.begin(), more.end());
        }
      candidates.swap (result);
    }

  return candidates;
}

} // Rapicorn
