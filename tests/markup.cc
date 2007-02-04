/* Tests
 * Copyright (C) 2005 Tim Janik
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
#include <birnet/birnettests.h>
#include <rapicorn/birnetmarkup.hh>
#include <rapicorn/rapicorn.hh>

namespace {
using namespace Birnet;
using namespace Rapicorn;

struct TestBirnetMarkupParser : MarkupParser {
  String debug_string;
  TestBirnetMarkupParser (const String &input_name) :
    MarkupParser (input_name)
  {}
  virtual void start_element (const String  &element_name,
                              ConstStrings  &attribute_names,
                              ConstStrings  &attribute_values,
                              Error         &error)
  {
    //printf ("<%s>", element_name.c_str());
    debug_string += "<";
  }
  virtual void end_element   (const String  &element_name,
                              Error         &error)
  {
    // printf ("</%s>\n", element_name.c_str());
    debug_string += ">";
  }
  virtual void text          (const String  &text,
                              Error         &error)
  {
    // printf ("TEXT: %s", text.c_str());
    if (text.size())
      debug_string += ".";
  }
  virtual void pass_through (const String   &pass_through_text,
                             Error          &error)
  {
    // printf ("PASS: %s\n", pass_through_text.c_str());
    debug_string += "-";
  }
  virtual void error        (const Error    &error)
  {
    diag ("parsing error: %s", error.message.c_str());
  }
};

extern const char *xml_data;

static void
birnet_markup_parser_test()
{
  TSTART ("BirnetMarkupParser");
  TestBirnetMarkupParser *tmp = new TestBirnetMarkupParser ("-");
  TOK();
  MarkupParser::Error error;
  const char *input_file = "test-input";
  tmp->parse (xml_data, strlen (xml_data), &error);
  TOK();
  if (!error.code)
    tmp->end_parse (&error);
  TOK();
  if (error.code)
    Birnet::error ("%s:%d:%d: %s (%d)", input_file, error.line_number, error.char_number, error.message.c_str(), error.code);
  TOK();
  // g_printerr ("DEBUG_STRING: %s\n", tmp->debug_string.c_str());
  const char *dcode = tmp->debug_string.c_str();
  TASSERT (strstr (dcode, "----")); // comments
  TASSERT (strstr (dcode, "<><>")); // tags
  TASSERT (strstr (dcode, "."));    // text
  TASSERT (strstr (dcode, "<.>"));
  TASSERT (strstr (dcode, ".-."));
  TASSERT (strstr (dcode, "<.<.>.>"));
  TASSERT (strstr (dcode, ".>.<."));
  delete tmp;
  TOK();
  TDONE();
}

static void
text_markup_test()
{
  TSTART ("Label-Markup");
  Handle<Item> ihandle = Factory::create_item ("Label");
  TOK();
  AutoLocker locker (ihandle);
  TOK();
  Item &item = ihandle.get();
  TOK();
  String full_markup =
    "<PARA>Start "
    "<bold>bold</bold> "
    "<italic>italic</italic> "
    // "<oblique>oblique</oblique> "
    "<underline>underline</underline> "
    // "<condensed>condensed</condensed> "
    "<strike>strike</strike> "
    "text. "
    "<fg color='blue'>changecolor</fg> "
    "<font family='Serif'>changefont</font> "
    "<bg color='spEciaLCoLor184'>bgcolortest</bg> "
    "<fg color='spEciaLCoLor121'>fgcolortest</fg> "
    "<font family='SpEciALfoNT723'>changefont</font> "
    "<tt>teletype</tt> "
    "<smaller>smaller</smaller> "
    "<larger>larger</larger> "
    "</PARA>";
  item.set_property ("markup_text", full_markup);
  TOK();
  String str = item.get_property ("markup_text");
  const char *markup_result = str.c_str();
  // g_printerr ("MARKUP: %s\n", markup_result);
  TASSERT (strstr (markup_result, "<I"));
  TASSERT (strstr (markup_result, "italic<"));
  TASSERT (strstr (markup_result, "<B"));
  TASSERT (strstr (markup_result, "bold<"));
  TASSERT (strstr (markup_result, "<U"));
  TASSERT (strstr (markup_result, "underline<"));
  /* try re-parsing */
  item.set_property ("markup_text", markup_result);
  TOK();
  str = item.get_property ("markup_text");
  markup_result = str.c_str();
  TASSERT (strstr (markup_result, "<I"));
  TASSERT (strstr (markup_result, "italic<"));
  TASSERT (strstr (markup_result, "<U"));
  TASSERT (strstr (markup_result, "underline<"));
  TASSERT (strstr (markup_result, "strike<"));
  TASSERT (strstr (markup_result, "teletype<"));
  TASSERT (strstr (markup_result, "color='spEciaLCoLor184'"));
  TASSERT (strstr (markup_result, "color='spEciaLCoLor121'"));
  TASSERT (strstr (markup_result, "family='SpEciALfoNT723'"));
  TASSERT (strstr (markup_result, "smaller<"));
  TASSERT (strstr (markup_result, "larger<"));
  TDONE();
}

extern "C" int
main (int   argc,
      char *argv[])
{
  birnet_init_test (&argc, &argv);
  /* initialize rapicorn */
  rapicorn_init_with_gtk_thread (&argc, &argv, NULL); // FIXME: should work offscreen
  
  birnet_markup_parser_test();
  text_markup_test();
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
