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
#include <rapicorn/birnettests.h>

#include "data.cc" // xml_data1

namespace {
using namespace Rapicorn;

static void
xml_tree_test (void)
{
  TSTART ("RapicornXmlNode");
  TOK();
  const char *input_file = "test-input";
  MarkupParser::Error error;
  XmlNode *xnode = XmlNode::parse_xml ("testdata", xml_data1, strlen (xml_data1), &error);
  TOK();
  if (xnode)
    ref_sink (xnode);
  TOK();
  if (error.code)
    Rapicorn::error ("%s:%d:%d:error.code=%d: %s", input_file, error.line_number, error.char_number, error.code, error.message.c_str());
  else
    TASSERT (xnode != NULL);
  /* check root */
  TASSERT (xnode->name() == "toplevel-tag");
  vector<XmlNode*>::const_iterator cit;
  uint testmask = 0;
  /* various checks */
  for (cit = xnode->children_begin(); cit != xnode->children_end(); cit++)
    if ((*cit)->name() == "child1")
      {
        /* check attribute parsing */
        XmlNode *child = *cit;
        TASSERT (child->has_attribute ("a") == true);
        TASSERT (child->has_attribute ("A") == false);
        TASSERT (child->get_attribute ("a") == "a");
        TASSERT (child->get_attribute ("b") == "1234b");
        TASSERT (child->get_attribute ("c") == "_<->^&+;");
        TASSERT (child->get_attribute ("d") == "");
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
            TASSERT (c != NULL);
            TASSERT (c->name() == children[i]);
          }
        testmask |= 2;
      }
    else if ((*cit)->name() == "child4")
      {
        /* check simple text */
        XmlNode *child = *cit;
        TASSERT (child->text() == "foo-blah");
        testmask |= 4;
      }
    else if ((*cit)->name() == "child5")
      {
        /* check text reconstruction */
        const XmlNode *child = *cit;
        TASSERT (child->text() == "foobarbaseldroehn!");
        // printout ("[node:line=%u:char=%u]", child->parsed_line(), child->parsed_char());
        testmask |= 8;
      }
  TASSERT (testmask == 15);
  /* check attribute order */
  const XmlNode *cnode = xnode->first_child ("orderedattribs");
  TASSERT (cnode != NULL);
  TASSERT (cnode->text() == "orderedattribs-text");
  const vector<String> oa = cnode->list_attributes();
  TASSERT (oa.size() == 6);
  TASSERT (oa[0] == "x1");
  TASSERT (oa[1] == "z");
  TASSERT (oa[2] == "U");
  TASSERT (oa[3] == "foofoo");
  TASSERT (oa[4] == "coffee");
  TASSERT (oa[5] == "last");
  /* cleanup */
  if (xnode)
    unref (xnode);
  TOK();
  TDONE();
}

} // anon

extern "C" int
main (int   argc,
      char *argv[])
{
  birnet_init_test (&argc, &argv);
  xml_tree_test();
  return 0;
}
