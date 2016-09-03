// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include <rcore/testutils.hh>
#include <string.h>

#include "tests/t201-rcore-basics-xmldata.cc" // xml_data1

namespace {
using namespace Rapicorn;

static void
xml_tree_test (void)
{
  const char *input_file = "test-input";
  MarkupParser::Error error;
  XmlNodeP xnode = XmlNode::parse_xml ("testdata", xml_data1, strlen (xml_data1), &error);
  if (error.code)
    fatal ("%s:%d:%d:error.code=%d: %s", input_file, error.line_number, error.char_number, error.code, error.message.c_str());
  else
    TCMP (xnode, !=, nullptr);
  /* check root */
  TCMP (xnode->name(), ==, "toplevel-tag");
  uint testmask = 0;
  /* various checks */
  for (auto cit = xnode->children_begin(); cit != xnode->children_end(); cit++)
    if ((*cit)->name() == "child1")
      {
        /* check attribute parsing */
        XmlNodeP child = *cit;
        TCMP (child->has_attribute ("a"), ==, true);
        TCMP (child->has_attribute ("A"), ==, false);
        TCMP (child->get_attribute ("a"), ==, "a");
        TCMP (child->get_attribute ("b"), ==, "1234b");
        TCMP (child->get_attribute ("c"), ==, "_<->^&+;");
        TCMP (child->get_attribute ("d"), ==, "");
        testmask |= 1;
      }
    else if ((*cit)->name() == "child2")
      {
        /* check nested children */
        XmlNodeP child = *cit;
        const char *children[] = { "child3", "x", "y", "z", "A", "B", "C" };
        for (uint i = 0; i < ARRAY_SIZE (children); i++)
          {
            const XmlNodeP c = child->find_child (children[i]);
            TCMP (c, !=, nullptr);
            TCMP (c->name(), ==, children[i]);
          }
        testmask |= 2;
      }
    else if ((*cit)->name() == "child4")
      {
        /* check simple text */
        XmlNodeP child = *cit;
        TCMP (child->text(), ==, "foo-blah");
        testmask |= 4;
      }
    else if ((*cit)->name() == "child5")
      {
        /* check text reconstruction */
        const XmlNodeP child = *cit;
        TCMP (child->text(), ==, "foobarbaseldroehn!");
        // printout ("[node:line=%u:char=%u]", child->parsed_line(), child->parsed_char());
        testmask |= 8;
      }
  TCMP (testmask, ==, 15);
  /* check attribute order */
  const XmlNodeP cnode = xnode->find_child ("orderedattribs");
  TCMP (cnode, !=, nullptr);
  TCMP (cnode->text(), ==, "orderedattribs-text");
  const vector<String> oa = cnode->list_attributes();
  TCMP (oa.size(), ==, 6);
  TCMP (oa[0], ==, "x1");
  TCMP (oa[1], ==, "z");
  TCMP (oa[2], ==, "U");
  TCMP (oa[3], ==, "foofoo");
  TCMP (oa[4], ==, "coffee");
  TCMP (oa[5], ==, "last");
}
REGISTER_TEST ("XML-Tests/Test XmlNode", xml_tree_test);

static const String expected_xmlarray =
  "<Array>\n"
  "  <row><int>0</int></row>\n"
  "  <row key=\"001\"><int>1</int></row>\n"
  "  <row><int>2</int></row>\n"
  "  <row key=\"007\"><float>7.7000000000000002</float></row>\n"
  "  <row key=\"string\">textSTRINGtext</row>\n"
  "  <row key=\"enum\">textENUMtext</row>\n"
  "  <row key=\"typename\">textTYPEtext</row>\n"
  "</Array>";

#if 0 // FIXME
static void
test_xml2array()
{
  String errstr;
  Array a = Array::from_xml (expected_xmlarray, __FILE__ ":expected_xmlarray", &errstr);
  if (errstr.size())
    fatal ("%s", errstr.c_str());
#warning FIXME: test full XML->Array features
  //printout ("XML:\n%s\n", a.to_xml()->xml_string().c_str());
  //assert (int (a["001"]) == 1);
  //assert (a["string"].string() == "textSTRINGtext");
}
REGISTER_TEST ("XML-Tests/Test XML->Array", test_xml2array);

static void
test_array2xml()
{
  Array a;
  a[0] = 0;
  a["001"] = 1;
  a[2] = 2;
  a["007"] = 7.7;
  a["string"] = "textSTRINGtext";
  AnyValue vc (ENUM);
  vc = "textENUMtext";
  a["enum"] = vc;
  AnyValue vt (TYPE_REFERENCE);
  vt = "textTYPEtext";
  a["typename"] = vt;
  XmlNode *xnode = a.to_xml();
  assert (xnode);
  ref_sink (xnode);
  String result = xnode->xml_string().c_str();
  assert (expected_xmlarray, ==, result);
  // printout ("XML:\n%s\n", result.c_str());
  unref (xnode);
}
REGISTER_TEST ("XML-Tests/Test Array->XML", test_array2xml);

static const char *xml_array = ""
  "<row> <col>1</col> <col>2</col> <col>3</col> <col>4</col> <col>5</col> </row>\n"
  "<row> Row Text </row>\n"
  "";

static void
test_xml_array()
{
  String inputname = String (__FILE__) + ":xml_array[]";
  String errstr;
  Array array = Array::from_xml (xml_array, inputname, &errstr);
  if (errstr.size())
    fatal ("%s", errstr.c_str());
}
REGISTER_TEST ("XML-Tests/Test XML Array", test_xml_array);

#endif

} // anon
