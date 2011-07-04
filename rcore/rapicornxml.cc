/* RapicornXmlNode
 * Copyright (C) 2007 Tim Janik
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
#include "rapicornxml.hh"
#include "strings.hh"
#include <string.h>
#include <algorithm>

namespace Rapicorn {

XmlNode::XmlNode (const String &element_name,
                  uint          line,
                  uint          _char) :
  m_name (element_name),
  m_parent (NULL),
  m_line (line), m_char (_char)
{}

XmlNode::~XmlNode ()
{
  /* since parents own a reference on their children, m_parent must be NULL */
  assert (m_parent == NULL);
}

static inline vector<String>::const_iterator
find_attribute (const vector<String> &attribute_names,
                const String         &name,
                bool                  case_insensitive)
{
  if (case_insensitive)
    {
      const char *cname = name.c_str();
      for (vector<String>::const_iterator it = attribute_names.begin(); it != attribute_names.end(); it++)
        if (strcasecmp (cname, it->c_str()) == 0)
          return it;
      return attribute_names.end();
    }
  else
    return find (attribute_names.begin(), attribute_names.end(), name);
}

bool
XmlNode::set_attribute (const String &name,
                        const String &value,
                        bool          replace)
{
  vector<String>::const_iterator it = find_attribute (m_attribute_names, name, false);
  if (it == m_attribute_names.end())
    {
      m_attribute_names.push_back (name);
      m_attribute_values.push_back (value);
      return true;
    }
  else if (replace)
    {
      m_attribute_values[it - m_attribute_names.begin()] = value;
      return true;
    }
  else
    return false;
}

String
XmlNode::get_attribute (const String &name,
                        bool          case_insensitive) const
{
  vector<String>::const_iterator it = find_attribute (m_attribute_names, name, case_insensitive);
  if (it != m_attribute_names.end())
    return m_attribute_values[it - m_attribute_names.begin()];
  else
    return "";
}

bool
XmlNode::has_attribute (const String &name,
                        bool          case_insensitive) const
{
  vector<String>::const_iterator it = find_attribute (m_attribute_names, name, case_insensitive);
  return it != m_attribute_names.end();
}

bool
XmlNode::del_attribute (const String &name)
{
  vector<String>::const_iterator it = find_attribute (m_attribute_names, name, false);
  if (it == m_attribute_names.end())
    return false;
  vector<String>::iterator eit = m_attribute_names.begin() + (it - m_attribute_names.begin());
  m_attribute_names.erase (eit);
  m_attribute_values.erase (eit);
  return true;
}

void
XmlNode::set_parent (XmlNode *c,
                     XmlNode *p)
{
  if (p)
    ref_sink (c);
  c->m_parent = p;
  if (!p)
    unref (c);
}

const XmlNode*
XmlNode::first_child (const String &element_name) const
{
  ConstNodes *c = &children();
  if (c)
    for (ConstNodes::const_iterator it = c->begin(); it != c->end(); it++)
      if (element_name == (*it)->name())
        return *it;
  return NULL;
}

void
XmlNode::steal_children (XmlNode &parent)
{
  ConstNodes cl = parent.children();
  for (uint i = 0; i < cl.size(); i++)
    ref (cl[i]);
  for (uint i = cl.size(); i > 0; i++)
    parent.del_child (*cl[i-1]);
  for (uint i = 0; i < cl.size(); i++)
    add_child (*cl[i]);
  for (uint i = 0; i < cl.size(); i++)
    unref (cl[i]);
}

void
XmlNode::break_after (bool newline_after_tag)
{
  uint64 f = flags (NULL);
  f |= 4;
  flags (&f);
}

bool
XmlNode::break_after () const
{
  uint64 f = const_cast<XmlNode*> (this)->flags (NULL);
  return !!(f & 4);
}

void
XmlNode::break_within (bool newlines_around_chidlren)
{
  uint64 f = flags (NULL);
  f |= 8;
  flags (&f);
}

bool
XmlNode::break_within () const
{
  uint64 f = const_cast<XmlNode*> (this)->flags (NULL);
  return !!(f & 8);
}

} // Rapicorn

namespace { // Anon
using namespace Rapicorn;

class XmlNodeText : public virtual XmlNode {
  String                m_text;
  uint64                m_flags;
  virtual uint64        flags           (uint64 *fp) { if (fp) m_flags = *fp; return m_flags; }
  /* XmlNodeText */
  virtual String        text            () const         { return m_text; }
  /* XmlNodeParent */
  virtual ConstNodes&   children        () const         { return *(ConstNodes*) NULL; }
  virtual bool          add_child       (XmlNode &child) { return false; }
  virtual bool          del_child       (XmlNode &child) { return false; }
public:
  XmlNodeText (const String &utf8text,
               uint          line,
               uint          _char) :
    XmlNode ("", line, _char), m_text (utf8text), m_flags (0)
  {}
};

class XmlNodeParent : public virtual XmlNode {
  vector<XmlNode*>      m_children;
  uint64                m_flags;
  virtual uint64        flags           (uint64 *fp) { if (fp) m_flags = *fp; return m_flags; }
  /* XmlNodeText */
  virtual String
  text () const
  {
    String result;
    for (vector<XmlNode*>::const_iterator it = m_children.begin(); it != m_children.end(); it++)
      result.append ((*it)->text());
    return result;
  }
  /* XmlNodeParent */
  virtual ConstNodes&   children        () const        { return m_children; }
  virtual bool
  add_child (XmlNode &child)
  {
    return_val_if_fail (child.parent() == NULL, false);
    m_children.push_back (&child);
    set_parent (&child, this);
    return true;
  }
  virtual bool
  del_child (XmlNode &child)
  {
    /* walk backwards so removing the last child is O(1) */
    for (vector<XmlNode*>::reverse_iterator rit = m_children.rbegin(); rit != m_children.rend(); rit++)
      if (&child == *rit)
        {
          vector<XmlNode*>::iterator it = (++rit).base(); // see reverse_iterator.base() documentation
          assert (&child == *it);
          m_children.erase (it);
          assert (child.parent() == this);
          set_parent (&child, NULL);
          return true;
        }
    return false;
  }
  ~XmlNodeParent()
  {
    while (m_children.size())
      del_child (*m_children[m_children.size() - 1]);
  }
public:
  XmlNodeParent (const String &element_name,
                 uint          line,
                 uint          _char) :
    XmlNode (element_name, line, _char), m_flags (0)
  {}
};

class XmlNodeParser : public Rapicorn::MarkupParser {
  vector<XmlNode*> m_node_stack;
  XmlNode         *m_first;
  XmlNodeParser (const String &input_name) :
    MarkupParser (input_name), m_first (NULL)
  {}
  virtual
  ~XmlNodeParser()
  {}
  virtual void
  start_element (const String  &element_name,
                 ConstStrings  &attribute_names,
                 ConstStrings  &attribute_values,
                 Error         &error)
  {
    XmlNode *current = m_node_stack.size() ? m_node_stack[m_node_stack.size() - 1] : NULL;
    if (element_name.size() < 1 || !element_name[0]) /* paranoid checks */
      error.set (INVALID_ELEMENT, String() + "invalid element name: <" + escape_text (element_name) + "/>");
    int xline, xchar;
    get_position (&xline, &xchar);
    XmlNode *xnode = XmlNode::create_parent (element_name, xline, xchar);
    for (uint i = 0; i < attribute_names.size(); i++)
      xnode->set_attribute (attribute_names[i], attribute_values[i]);
    if (current)
      current->add_child (*xnode);
    else
      m_first = xnode;
    m_node_stack.push_back (xnode);
  }
  virtual void
  end_element (const String  &element_name,
               Error         &error)
  {
    m_node_stack.pop_back();
  }
  virtual void
  text (const String  &text,
        Error         &error)
  {
    XmlNode *current = m_node_stack.size() ? m_node_stack[m_node_stack.size() - 1] : NULL;
    int xline, xchar;
    get_position (&xline, &xchar);
    XmlNode *xnode = XmlNode::create_text (text, xline, xchar);
    if (current)
      current->add_child (*xnode);
    else
      m_first = xnode;
  }
public:
  static XmlNode*
  parse_xml (const String        &input_name,
             const char          *utf8data,
             ssize_t              utf8data_len,
             MarkupParser::Error &error,
             const String        &roottag)
  {
    const char *pseudoroot = roottag.size() ? roottag.c_str() : NULL;
    XmlNodeParser xnp (input_name);
    if (!error.code && pseudoroot)
      xnp.parse (String (String ("<") + pseudoroot + ">").c_str(), -1, &error);
    if (!error.code)
      xnp.parse (utf8data, utf8data_len, &error);
    if (!error.code && pseudoroot)
      xnp.parse (String (String ("</") + pseudoroot + ">").c_str(), -1, &error);
    if (!error.code)
      xnp.end_parse (&error);
    if (!error.code)
      {}
    return xnp.m_first;
  }
};

} // Anon

namespace Rapicorn {

XmlNode*
XmlNode::create_text (const String &utf8text,
                      uint          line,
                      uint          _char)
{
  return new XmlNodeText (utf8text, line, _char);
}

XmlNode*
XmlNode::create_parent (const String &element_name,
                        uint          line,
                        uint          _char)
{
  return new XmlNodeParent (element_name, line, _char);
}

XmlNode*
XmlNode::parse_xml (const String        &input_name,
                    const char          *utf8data,
                    ssize_t              utf8data_len,
                    MarkupParser::Error *error,
                    const String        &roottag)
{
  MarkupParser::Error perror;
  XmlNode *xnode = XmlNodeParser::parse_xml (input_name, utf8data, utf8data_len, perror, roottag);
  if (error)
    *error = perror;
  return xnode;
}

String
XmlNode::xml_string (uint64 indent)
{
  if (istext())
    return text();
#warning FIXME: XML-escape text
  String istr = string_multiply (" ", indent);
  String s;
  s += "<" + name();
  const StringVector keys = list_attributes();
  for (uint i = 0; i < keys.size(); i++)
    s += " " + keys[i] + "=\"" + get_attribute (keys[i]) + "\"";
#warning FIXME: XML-escape and quote attribute values
  ConstNodes &cl = children();
  if (cl.size())
    {
      s += ">";
      bool need_break = break_within();
      for (uint i = 0; i < cl.size(); i++)
        {
          if (need_break)
            s += "\n" + istr + "  ";
          s += cl[i]->xml_string (indent + 2);
          need_break = cl[i]->break_after();
        }
      if (break_within())
        s += "\n" + istr;
      s += "</" + name() + ">";
    }
  else
    s += "/>";
  return s;
}

} // Rapicorn
