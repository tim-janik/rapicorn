/* Tests
 * Copyright (C) 2005 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
//#define TEST_VERBOSE
#include <birnet/birnettests.h>
#include <rapicorn/birnetmarkup.hh>

namespace {
using namespace Birnet;
using Birnet::uint;

struct TestMarkupParser : MarkupParser {
  virtual void start_element (const String  &element_name,
                              ConstStrings  &attribute_names,
                              ConstStrings  &attribute_values,
                              Error         &error)
  {
    //printf ("<%s>", element_name.c_str());
    printf ("<");
  }
  virtual void end_element   (const String  &element_name,
                              Error         &error)
  {
    // printf ("</%s>\n", element_name.c_str());
    printf (">");
  }
  virtual void text          (const String  &text,
                              Error         &error)
  {
    // printf ("TEXT: %s", text.c_str());
    printf (".");
  }
  virtual void pass_through (const String   &pass_through_text,
                             Error          &error)
  {
    // printf ("PASS: %s\n", pass_through_text.c_str());
    printf ("-");
  }
  virtual void error        (const Error    &error)
  {
    Birnet::warning ("parsing error: %s", error.message.c_str());
  }
};

extern const char *xml_data;

void
markup_parser_test()
{
  TestMarkupParser *tmp = new TestMarkupParser;
  MarkupParser::Error error;
  const char *input_file = "TestInput.xml";
  tmp->parse (xml_data, strlen (xml_data), &error);
  if (!error.code)
    tmp->end_parse (&error);
  if (error.code)
    Birnet::error ("%s:%d:%d: %s (%d)", input_file, error.line_number, error.char_number, error.message.c_str(), error.code);
  delete tmp;
  printf ("\n");
}

extern "C" int
main (int   argc,
      char *argv[])
{
  birnet_init_test (&argc, &argv);
  markup_parser_test();
  return 0;
}

const char *xml_data = /* random, but valid xml data */
"<?xml version='1.0' encoding='UTF-8'?>		<!--*-mode: xml;-*-->\n"
"<toplevel-tag\n"
"  xmlns:xfoo='http://beast.gtk.org/xmlns:xfoo' >\n"
"\n"
"  <!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!---->\n"
"  <!-- a real comment -->\n"
"  <xfoo:zibtupasti    inherit='xxx1' />\n"
"  <xfoo:ohanifurtasy  inherit='YbbmXXX1' />\n"
"  <xfoo:chacka        inherit='XyXyXyXy'\n"
"    prop:orientation='$(somestring)'\n"
"    prop:x-channels='$(other args,$n,0)' />\n"
"  <xfoo:ergostuff     inherit='Parent' >\n"
"  </xfoo:ergostuff>\n"
"\n"
"  <tag/><tag/><tag/><tag/><tag/><tag/><tag/><tag/><tag/><tag/><tag/><tag/>"
"  <!-- comment2 -->\n"
"  <xfoo:defineme inherit='SomeParent' variable='3' >\n"
"    <hbox spacing='3' >\n"
"      <hpaned height='120' position='240' >\n"
"	  <hscrollbar id='need-a-handle' size:vgroup='vg-size' />\n"
"      </hpaned>\n"
"      <vbox spacing='3' >\n"
"	<alignment size:vgroup='vg-size' />\n"
"	<vscrollbar id='another-handle' size:vgroup='vg-size' pack:expand='1' />\n"
"	<alignment size:vgroup='vg-size' />\n"
"      </vbox>\n"
"    </hbox>\n"
"  </xfoo:defineme>\n"
"  \n"
"  <!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!---->\n"
"  <nest>\n"
"    <nest>\n"
"      <nest>\n"
"        <nest>\n"
"          <nest>\n"
"            <nest>\n"
"              <nest>\n"
"                <nest>\n"
"                  <nest>\n"
"                    <nest>\n"
"                      <nest>\n"
"                        <nest>\n"
"                          <nest>\n"
"                            <nest>\n"
"                              <nest>\n"
"                              </nest>\n"
"                            </nest>\n"
"                          </nest>\n"
"                        </nest>\n"
"                      </nest>\n"
"                    </nest>\n"
"                  </nest>\n"
"                  <nest>\n"
"                    <nest>\n"
"                      <nest>\n"
"                        <nest>\n"
"                          <nest>\n"
"                            <nest>\n"
"                              <nest>\n"
"                              </nest>\n"
"                            </nest>\n"
"                          </nest>\n"
"                        </nest>\n"
"                      </nest>\n"
"                    </nest>\n"
"                  </nest>\n"
"                </nest>\n"
"              </nest>\n"
"            </nest>\n"
"          </nest>\n"
"        </nest>\n"
"      </nest>\n"
"    </nest>\n"
"  </nest>\n"
"  <!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!---->\n"
"\n"
"  \n"
"  \n"
"</toplevel-tag>\n";

} // anon
