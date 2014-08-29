// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "xmlnode.hh"
#include "strings.hh"
#include <string.h>
#include <algorithm>

namespace Rapicorn {

XmlNode::XmlNode (const String &element_name,
                  uint          line,
                  uint          _char,
                  const String &file) :
  name_ (element_name),
  parent_ (NULL), file_ (file),
  line_ (line), char_ (_char)
{}

XmlNode::~XmlNode ()
{
  /* since parents own a reference on their children, parent_ must be NULL */
  assert (parent_ == NULL);
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
  vector<String>::const_iterator it = find_attribute (attribute_names_, name, false);
  if (it == attribute_names_.end())
    {
      attribute_names_.push_back (name);
      attribute_values_.push_back (value);
      return true;
    }
  else if (replace)
    {
      attribute_values_[it - attribute_names_.begin()] = value;
      return true;
    }
  else
    return false;
}

String
XmlNode::get_attribute (const String &name,
                        bool          case_insensitive) const
{
  vector<String>::const_iterator it = find_attribute (attribute_names_, name, case_insensitive);
  if (it != attribute_names_.end())
    return attribute_values_[it - attribute_names_.begin()];
  else
    return "";
}

bool
XmlNode::has_attribute (const String &name,
                        bool          case_insensitive) const
{
  vector<String>::const_iterator it = find_attribute (attribute_names_, name, case_insensitive);
  return it != attribute_names_.end();
}

bool
XmlNode::del_attribute (const String &name)
{
  vector<String>::const_iterator c_it = find_attribute (attribute_names_, name, false);
  if (c_it == attribute_names_.end())
    return false;
  const size_t nth = c_it - attribute_names_.begin();
  attribute_names_.erase (attribute_names_.begin() + nth);
  attribute_values_.erase (attribute_values_.begin() + nth);
  return true;
}

void
XmlNode::set_parent (XmlNode *c,
                     XmlNode *p)
{
  if (p)
    ref_sink (c);
  c->parent_ = p;
  if (!p)
    unref (c);
}

static DataKey<uint64> xml_node_flags_key;

uint64
XmlNode::flags () const
{
  const uint64 flagvalues = get_data (&xml_node_flags_key);
  return flagvalues;
}

void
XmlNode::flags (uint64 flagvalues)
{
  set_data (&xml_node_flags_key, flagvalues);
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
  flags (4 | flags());
}

bool
XmlNode::break_after () const
{
  return !!(4 & flags());
}

void
XmlNode::break_within (bool newlines_around_chidlren)
{
  flags (8 | flags());
}

bool
XmlNode::break_within () const
{
  return !!(8 & flags());
}

} // Rapicorn

namespace { // Anon
using namespace Rapicorn;

class XmlNodeText : public virtual XmlNode {
  String                text_;
  // XmlNodeText
  virtual String        text            () const         { return text_; }
  // XmlNodeParent
  virtual ConstNodes&   children        () const         { return *(ConstNodes*) NULL; }
  virtual bool          add_child       (XmlNode &child) { return false; }
  virtual bool          del_child       (XmlNode &child) { return false; }
public:
  XmlNodeText (const String &utf8text,
               uint          line,
               uint          _char,
               const String &file) :
    XmlNode ("", line, _char, file), text_ (utf8text)
  {}
};

class XmlNodeParent : public virtual XmlNode {
  vector<XmlNode*>      children_;
  // XmlNodeText
  virtual String
  text () const
  {
    String result;
    for (vector<XmlNode*>::const_iterator it = children_.begin(); it != children_.end(); it++)
      result.append ((*it)->text());
    return result;
  }
  /* XmlNodeParent */
  virtual ConstNodes&   children        () const        { return children_; }
  virtual bool
  add_child (XmlNode &child)
  {
    assert_return (child.parent() == NULL, false);
    children_.push_back (&child);
    set_parent (&child, this);
    return true;
  }
  virtual bool
  del_child (XmlNode &child)
  {
    /* walk backwards so removing the last child is O(1) */
    for (vector<XmlNode*>::reverse_iterator rit = children_.rbegin(); rit != children_.rend(); rit++)
      if (&child == *rit)
        {
          vector<XmlNode*>::iterator it = (++rit).base(); // see reverse_iterator.base() documentation
          assert (&child == *it);
          children_.erase (it);
          assert (child.parent() == this);
          set_parent (&child, NULL);
          return true;
        }
    return false;
  }
  ~XmlNodeParent()
  {
    while (children_.size())
      del_child (*children_[children_.size() - 1]);
  }
public:
  XmlNodeParent (const String &element_name,
                 uint          line,
                 uint          _char,
                 const String &file) :
    XmlNode (element_name, line, _char, file)
  {}
};

class XmlNodeParser : public Rapicorn::MarkupParser {
  vector<XmlNode*> node_stack_;
  XmlNode         *first_;
  XmlNodeParser (const String &input_name) :
    MarkupParser (input_name), first_ (NULL)
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
    XmlNode *current = node_stack_.size() ? node_stack_[node_stack_.size() - 1] : NULL;
    if (element_name.size() < 1 || !element_name[0]) /* paranoid checks */
      error.set (INVALID_ELEMENT, String() + "invalid element name: <" + escape_text (element_name) + "/>");
    int xline, xchar;
    get_position (&xline, &xchar);
    XmlNode *xnode = XmlNode::create_parent (element_name, xline, xchar, input_name());
    for (uint i = 0; i < attribute_names.size(); i++)
      xnode->set_attribute (attribute_names[i], attribute_values[i]);
    if (current)
      current->add_child (*xnode);
    else
      {
        if (first_)
          {
            error.set (INVALID_ELEMENT, String() + "multiple toplevel elements: "
                       "<" + escape_text (first_->name()) + "/> <" + escape_text (element_name) + "/>");
            unref (first_); // prevent leaks
            first_ = NULL;
          }
        first_ = xnode;
      }
    node_stack_.push_back (xnode);
  }
  virtual void
  end_element (const String  &element_name,
               Error         &error)
  {
    node_stack_.pop_back();
  }
  virtual void
  text (const String  &text,
        Error         &error)
  {
    XmlNode *current = node_stack_.size() ? node_stack_[node_stack_.size() - 1] : NULL;
    int xline, xchar;
    get_position (&xline, &xchar);
    XmlNode *xnode = XmlNode::create_text (text, xline, xchar, input_name());
    if (current)
      current->add_child (*xnode);
    else
      first_ = xnode;
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
    return xnp.first_;
  }
};

} // Anon

namespace Rapicorn {

XmlNode*
XmlNode::create_text (const String &utf8text,
                      uint          line,
                      uint          _char,
                      const String &file)
{
  return new XmlNodeText (utf8text, line, _char, file);
}

XmlNode*
XmlNode::create_parent (const String &element_name,
                        uint          line,
                        uint          _char,
                        const String &file)
{
  return new XmlNodeParent (element_name, line, _char, file);
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

enum { SQ = 1, DQ = 2, AP = 4, BS = 8, LT = 16, GT = 32, ALL = SQ + DQ + AP + BS + LT + GT };

template<int WHAT> static inline String
escape_xml (const String &input)
{
  String d;
  d.reserve (input.size());
  for (String::const_iterator it = input.begin(); it != input.end(); it++)
    switch (*it)
      {
      default:   _default:                              d += *it;       break;
      case '"':  if (!(WHAT & DQ)) goto _default;       d += "&quot;";  break;
      case '&':  if (!(WHAT & AP)) goto _default;       d += "&amp;";   break;
      case '\'': if (!(WHAT & SQ)) goto _default;       d += "&apos;";  break;
      case '<':  if (!(WHAT & LT)) goto _default;       d += "&lt;";    break;
      case '>':  if (!(WHAT & GT)) goto _default;       d += "&gt;";    break;
      }
  return d;
}

String
XmlNode::xml_escape (const String &input)
{
  return escape_xml<ALL> (input);
}

String
XmlNode::xml_string (uint64 indent, bool include_outer, uint64 recursion_depth) const
{
  if (recursion_depth == 0)
    return "";
  if (istext())
    return xml_escape (text());
  const String istr = string_multiply (" ", indent);
  String s;
  if (include_outer)
    {
      s += "<" + xml_escape (name());
      const StringVector keys = list_attributes();
      for (uint i = 0; i < keys.size(); i++)
        s += " " + xml_escape (keys[i]) + "=\"" + escape_xml<ALL-SQ> (get_attribute (keys[i])) + "\"";
    }
  ConstNodes &cl = children();
  if (cl.size())
    {
      if (include_outer)
        s += ">";
      bool need_break = include_outer && break_within();
      for (size_t i = 0; i < cl.size(); i++)
        {
          if (need_break)
            s += "\n" + istr + "  ";
          s += cl[i]->xml_string (indent + 2, true, recursion_depth - 1);
          need_break = cl[i]->break_after();
        }
      if (include_outer && break_within())
        s += "\n" + istr;
      if (include_outer)
        s += "</" + xml_escape (name()) + ">";
    }
  else if (include_outer)
    s += "/>";
  return s;
}

} // Rapicorn
