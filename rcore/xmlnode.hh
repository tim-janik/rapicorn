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

#include <rcore/markup.hh>

namespace Rapicorn {

class XmlNode : public virtual BaseObject {
  String                m_name; // element name
  XmlNode              *m_parent;
  StringVector          m_attribute_names;
  StringVector          m_attribute_values;
  String                m_file;
  uint                  m_line, m_char;
protected:
  explicit              XmlNode         (const String&, uint, uint, const String&);
  uint64                flags           () const;
  void                  flags           (uint64 flags);
  virtual              ~XmlNode         ();
  static void           set_parent      (XmlNode *c, XmlNode *p);
public:
  typedef const vector<XmlNode*>     ConstNodes;
  typedef ConstNodes::const_iterator ConstChildIter;
  String                name            () const                { return m_name; }
  XmlNode*              parent          () const                { return m_parent; }
  const StringVector&   list_attributes () const                { return m_attribute_names; }
  const StringVector&   list_values     () const                { return m_attribute_values; }
  bool                  set_attribute   (const String   &name,
                                         const String   &value,
                                         bool            replace = true);
  String                get_attribute   (const String   &name,
                                         bool            case_insensitive = false) const;
  bool                  has_attribute   (const String   &name,
                                         bool            case_insensitive = false) const;
  bool                  del_attribute   (const String   &name);
  String                parsed_file     () const                { return m_file; }
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
  void                  steal_children  (XmlNode        &parent);
  /* hints */
  void                  break_after     (bool            newline_after_tag);
  bool                  break_after     () const;
  void                  break_within    (bool            newlines_around_chidlren);
  bool                  break_within    () const;
  String                xml_string      (uint64          indent = 0, bool include_outer = true) const;
  /* nodes */
  static XmlNode*       create_text     (const String   &utf8text,
                                         uint            line,
                                         uint            _char,
                                         const String   &file);
  static XmlNode*       create_parent   (const String   &element_name,
                                         uint            line,
                                         uint            _char,
                                         const String   &file);
  /* IO */
  static XmlNode*       parse_xml       (const String   &input_name,
                                         const char     *utf8data,
                                         ssize_t         utf8data_len,
                                         MarkupParser::Error *error,
                                         const String   &roottag = "");
  static String         xml_escape      (const String   &input);
};

} // Rapicorn

#endif  /* __RAPICORN_XML_HH__ */
