// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_XMLNODE_HH__
#define __RAPICORN_XMLNODE_HH__

#include <rcore/markup.hh>
#include <rcore/objects.hh>

namespace Rapicorn {

/** Simple XML tree representation.
 * @DISCOURAGED: Nonpublic API, data structure used internally.
 */
class XmlNode : public virtual ReferenceCountable, public virtual DataListContainer {
  String                name_; // element name
  XmlNode              *parent_;
  StringVector          attribute_names_;
  StringVector          attribute_values_;
  String                file_;
  uint                  line_, char_;
protected:
  explicit              XmlNode         (const String&, uint, uint, const String&);
  uint64                flags           () const;
  void                  flags           (uint64 flags);
  virtual              ~XmlNode         ();
  static void           set_parent      (XmlNode *c, XmlNode *p);
public:
  typedef const vector<XmlNode*>     ConstNodes;
  typedef ConstNodes::const_iterator ConstChildIter;
  String                name            () const                { return name_; }
  XmlNode*              parent          () const                { return parent_; }
  const StringVector&   list_attributes () const                { return attribute_names_; }
  const StringVector&   list_values     () const                { return attribute_values_; }
  bool                  set_attribute   (const String   &name,
                                         const String   &value,
                                         bool            replace = true);
  String                get_attribute   (const String   &name,
                                         bool            case_insensitive = false) const;
  bool                  has_attribute   (const String   &name,
                                         bool            case_insensitive = false) const;
  bool                  del_attribute   (const String   &name);
  String                parsed_file     () const                { return file_; }
  uint                  parsed_line     () const                { return line_; }
  uint                  parsed_char     () const                { return char_; }
  /* text node */
  virtual String        text            () const = 0;
  bool                  istext          () const                { return name_.size() == 0; }
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
  String                xml_string      (uint64          indent = 0, bool include_outer = true, uint64 recursion_depth = -1) const;
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

#endif  /* __RAPICORN_XMLNODE_HH__ */
