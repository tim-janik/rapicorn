// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "debugtools.hh"
#include "regex.hh"
#include "markup.hh"
#include <stdio.h>
#include <syslog.h>
#include <errno.h>

namespace Rapicorn {

TestStream::TestStream ()
{}

TestStream::~TestStream ()
{}

static String
value_escape (const String &text)
{
  /* escape XML attribute values for test dumps */
  const char *c = &*text.begin();
  size_t l = text.size();
  String str;
  for (size_t i = 0; i < l; i++)
    switch (c[i])
      {
      case '<':
        str += "&lt;";
        break;
      case '&':
        str += "&amp;";
        break;
      case '"':
        str += "&quot;";
        break;
      default:
        str.append (c + i, 1);
        break;
      }
  return str;
}

class TestStreamImpl : public TestStream {
  vector<String> node_stack;
  vector<String> node_matches;
  vector<String> node_unmatches;
  String         indent;
  String         dat;
  bool           node_open, nodematch;
  uint           ignore_count;
  bool // true == ignore node
  check_ignore_node (const String &name)
  {
    for (uint i = 0; i < node_unmatches.size(); i++)
      if (Regex::match_simple (node_unmatches[i], name,
                               Regex::EXTENDED | Regex::ANCHORED,
                               Regex::MATCH_NORMAL))
        return true;
    return false;
  }
  bool // true == show node
  check_match_node (const String &name)
  {
    for (uint i = 0; i < node_matches.size(); i++)
      if (Regex::match_simple (node_matches[i], name,
                               Regex::EXTENDED | Regex::ANCHORED,
                               Regex::MATCH_NORMAL))
        return true;
    return node_matches.size() < 1;
  }
  virtual void
  ddump (Kind          kind,
         const String &name,
         const String &val)
  {
    bool should_ignore;
    switch (kind)
      {
      case TEXT:
        assert_return (name == "");
        close_node();
        if (!ignore_count || nodematch)
          dat += value_escape (val) + "\n";
        break;
      case NODE:
        assert_return (val == "");
        close_node();
        node_stack.push_back (name);
        if (!ignore_count)
          should_ignore = check_ignore_node (name);
        if (ignore_count || should_ignore)
          ignore_count++;
        nodematch = check_match_node (name);
        if (!ignore_count || nodematch)
          dat += indent + "<" + name + "\n";
        node_open = true;
        push_indent();
        break;
      case POPNODE:
        close_node();
        pop_indent();
        if (!ignore_count || nodematch)
          dat += indent + "</" + node_stack.back() + ">\n";
        if (ignore_count)
          ignore_count--;
        node_stack.pop_back();
        nodematch = node_stack.size() < 1 || check_match_node (node_stack.back());
        break;
      case VALUE:
        if (!ignore_count || nodematch)
          {
            if (node_open)
              dat += indent + name + "=\"" + value_escape (val) + "\"\n";
            else
              dat += indent + "<ATTRIBUTE " + name + "=\"" + value_escape (val) + "\"/>\n";
          }
        break;
      case INTERN:
        close_node();
        if (!ignore_count || nodematch)
          dat += indent + "<INTERN " + name + "=\"" + value_escape (val) + "\"/>\n";
        break;
      case INDENT:
        if (!ignore_count || nodematch)
          indent += "  ";
        break;
      case POPINDENT:
        if (!ignore_count || nodematch)
          indent = indent.substr (0, indent.size() - 2);
        break;
      }
  }
  void
  close_node ()
  {
    if (!node_open)
      return;
    if (!ignore_count || nodematch)
      dat += indent.substr (0, indent.size() - 2) + ">\n";
    node_open = false;
  }
  virtual String
  string ()
  {
    return dat;
  }
  virtual void
  filter_matched_nodes (const String &matchpattern)
  {
    if (node_unmatches.size() < 1)
      ignore_count++; // ignore nodes by default now
    node_matches.push_back (matchpattern);
  }
  virtual void
  filter_unmatched_nodes (const String &matchpattern)
  {
    node_unmatches.push_back (matchpattern);
  }
public:
  TestStreamImpl() : node_open (false), nodematch (true), ignore_count (0)
  {}
};

TestStream*
TestStream::create_test_stream ()
{
  return new TestStreamImpl();
}

} // Rapicorn
