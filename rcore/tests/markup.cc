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
#include <rcore/testutils.hh>
#include <string.h>

#include "data.cc" // xml_data1

namespace {
using namespace Rapicorn;
using namespace Rapicorn;

struct TestRapicornMarkupParser : MarkupParser {
  String debug_string;
  TestRapicornMarkupParser (const String &input_name) :
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
    critical ("parsing error: %s", error.message.c_str());
  }
};

static void
rapicorn_markup_parser_test()
{
  TestRapicornMarkupParser *tmp = new TestRapicornMarkupParser ("-");
  MarkupParser::Error error;
  const char *input_file = "test-input";
  tmp->parse (xml_data1, strlen (xml_data1), &error);
  if (!error.code)
    tmp->end_parse (&error);
  if (error.code)
    fatal ("%s:%d:%d: %s (%d)", input_file, error.line_number, error.char_number, error.message.c_str(), error.code);
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
}
REGISTER_TEST ("Markup/RapicornMarkupParser", rapicorn_markup_parser_test);

static void
markup_string_escaping()
{
  TCMP ("<foo & bar>", ==, MarkupParser::printf_escaped ("<foo & bar>"));
  TCMP ("foo &amp; bar", ==, MarkupParser::printf_escaped ("foo %s bar", "&"));
  TCMP ("&lt;FROB/&gt;", ==, MarkupParser::printf_escaped ("%sFROB/%s", "<", ">"));
  TCMP ("|&apos;sqstring&apos; &amp; &quot;dqstring&quot;|", ==, MarkupParser::printf_escaped ("|%s|", "'sqstring' & \"dqstring\""));
}
REGISTER_TEST ("Markup/String Escaping", markup_string_escaping);

} // anon
