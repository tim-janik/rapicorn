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
#ifndef __RAPICORN_XML_HH__
#define __RAPICORN_XML_HH__

#include <rapicorn/birnetmarkup.hh>

namespace Rapicorn {

class XmlNode : public virtual ReferenceCountImpl {
  String                m_name; // element name
  XmlNode              *m_parent;
  vector<String>        m_attribute_names;
  vector<String>        m_attribute_values;
  uint                  m_line, m_char;
protected:
  explicit              XmlNode         (const String &, uint, uint);
  virtual              ~XmlNode         ();
  static void           set_parent      (XmlNode *c, XmlNode *p);
public:
  typedef const vector<XmlNode*>     ConstNodes;
  typedef ConstNodes::const_iterator ConstChildIter;
  String                name            () const                { return m_name; }
  XmlNode*              parent          () const                { return m_parent; }
  const vector<String>  list_attributes () const                { return m_attribute_names; }
  bool                  set_attribute   (const String   &name,
                                         const String   &value,
                                         bool            replace = true);
  String                get_attribute   (const String   &name,
                                         bool            case_insensitive = false) const;
  bool                  has_attribute   (const String   &name,
                                         bool            case_insensitive = false) const;
  bool                  del_attribute   (const String   &name);
  uint                  parsed_line     () const                { return m_line; }
  uint                  parsed_char     () const                { return m_char; }
  /* text node */
  virtual String        text            () const = 0;
  bool                  istext          () const                { return m_name.size() == 0; }
  /* parent node */
  virtual ConstNodes&   children        () const = 0;
  ConstChildIter        children_begin  () const { return children().begin(); }
  ConstChildIter        children_end    () const { return children().end(); }
  const XmlNode*        first_child     (const String   &element_name) const;
  virtual bool          add_child       (XmlNode        &child) = 0;
  virtual bool          del_child       (XmlNode        &child) = 0;
  /* nodes */
  static XmlNode*       create_text     (const String   &utf8text,
                                         uint            line = 0,
                                         uint            _char = 0);
  static XmlNode*       create_parent   (const String   &element_name,
                                         uint            line = 0,
                                         uint            _char = 0);
  /* IO */
  static XmlNode*       parse_xml       (const String   &input_name,
                                         const char     *utf8data,
                                         ssize_t         utf8data_len,
                                         MarkupParser::Error *error,
                                         const String   &roottag = "");
};

} // Rapicorn

#endif  /* __RAPICORN_XML_HH__ */
