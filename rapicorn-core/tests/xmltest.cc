/* Tests
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
//#define TEST_VERBOSE
#include <rapicorn-core/testutils.hh>

#include "data.cc" // xml_data1

namespace {
using namespace Rapicorn;

static void
xml_tree_test (void)
{
  const char *input_file = "test-input";
  MarkupParser::Error error;
  XmlNode *xnode = XmlNode::parse_xml ("testdata", xml_data1, strlen (xml_data1), &error);
  if (xnode)
    ref_sink (xnode);
  if (error.code)
    Rapicorn::error ("%s:%d:%d:error.code=%d: %s", input_file, error.line_number, error.char_number, error.code, error.message.c_str());
  else
    TCHECK (xnode != NULL);
  /* check root */
  TCHECK (xnode->name() == "toplevel-tag");
  vector<XmlNode*>::const_iterator cit;
  uint testmask = 0;
  /* various checks */
  for (cit = xnode->children_begin(); cit != xnode->children_end(); cit++)
    if ((*cit)->name() == "child1")
      {
        /* check attribute parsing */
        XmlNode *child = *cit;
        TCHECK (child->has_attribute ("a") == true);
        TCHECK (child->has_attribute ("A") == false);
        TCHECK (child->get_attribute ("a") == "a");
        TCHECK (child->get_attribute ("b") == "1234b");
        TCHECK (child->get_attribute ("c") == "_<->^&+;");
        TCHECK (child->get_attribute ("d") == "");
        testmask |= 1;
      }
    else if ((*cit)->name() == "child2")
      {
        /* check nested children */
        XmlNode *child = *cit;
        const char *children[] = { "child3", "x", "y", "z", "A", "B", "C" };
        for (uint i = 0; i < ARRAY_SIZE (children); i++)
          {
            const XmlNode *c = child->first_child (children[i]);
            TCHECK (c != NULL);
            TCHECK (c->name() == children[i]);
          }
        testmask |= 2;
      }
    else if ((*cit)->name() == "child4")
      {
        /* check simple text */
        XmlNode *child = *cit;
        TCHECK (child->text() == "foo-blah");
        testmask |= 4;
      }
    else if ((*cit)->name() == "child5")
      {
        /* check text reconstruction */
        const XmlNode *child = *cit;
        TCHECK (child->text() == "foobarbaseldroehn!");
        // printout ("[node:line=%u:char=%u]", child->parsed_line(), child->parsed_char());
        testmask |= 8;
      }
  TCHECK (testmask == 15);
  /* check attribute order */
  const XmlNode *cnode = xnode->first_child ("orderedattribs");
  TCHECK (cnode != NULL);
  TCHECK (cnode->text() == "orderedattribs-text");
  const vector<String> oa = cnode->list_attributes();
  TCHECK (oa.size() == 6);
  TCHECK (oa[0] == "x1");
  TCHECK (oa[1] == "z");
  TCHECK (oa[2] == "U");
  TCHECK (oa[3] == "foofoo");
  TCHECK (oa[4] == "coffee");
  TCHECK (oa[5] == "last");
  /* cleanup */
  if (xnode)
    unref (xnode);
}

static const char *xml_array = ""
  "<Rapicorn::Array>\n"
  "  <row> <col>1</col> <col>2</col> <col>3</col> <col>4</col> <col>5</col> </row>\n"
  "  <row> Row Text </row>\n"
  "</Rapicorn::Array>\n"
  "";

static void
test_xml_array()
{
  String inputname = String (__FILE__) + ":xml_array[]";
  MarkupParser::Error perror;
  XmlNode *xnode = XmlNode::parse_xml (inputname, xml_array, -1, &perror, "Model1");
  String err;
  if (perror.code)
    err = string_printf ("%s:%d:%d: %s", inputname.c_str(),
                         perror.line_number, perror.char_number, perror.message.c_str());
  if (xnode)
    {
      ref_sink (xnode);
      if (!err.size())
        err = ""; // use xnode
      unref (xnode);
    }
  if (err.size())
    error (err);
}

} // anon

extern "C" int
main (int   argc,
      char *argv[])
{
  rapicorn_init_test (&argc, &argv);

  Test::add ("Test XmlNode", xml_tree_test);
  Test::add ("Test XML Array", test_xml_array);

  return Test::run();
}
