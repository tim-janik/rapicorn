// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include <rcore/testutils.hh>
#include <string.h>

#include "rcore-basics-xmldata.cc" // xml_data1

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
  TCMP ("<foo & bar>", ==, MarkupParser::escape_format_args ("<foo & bar>"));
  TCMP ("foo &amp; bar", ==, MarkupParser::escape_format_args ("foo %s bar", "&"));
  TCMP ("&lt;FROB/&gt;", ==, MarkupParser::escape_format_args ("%sFROB/%s", "<", ">"));
  TCMP ("|&apos;sqstring&apos; &amp; &quot;dqstring&quot;|", ==, MarkupParser::escape_format_args ("|%s|", "'sqstring' & \"dqstring\""));
}
REGISTER_TEST ("Markup/String Escaping", markup_string_escaping);

} // anon
