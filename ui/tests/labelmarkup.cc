// Licensed CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0
#include <rcore/testutils.hh>
#include <ui/uithread.hh>
#include <string.h>

#include "tests/t201-rcore-basics-xmldata.cc" // xml_data1

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
    RAPICORN_DIAG ("parsing error: %s", error.message.c_str());
  }
};

static void
test_markup_parser()
{
  TestRapicornMarkupParser *tmp = new TestRapicornMarkupParser ("-");
  TOK();
  MarkupParser::Error error;
  const char *input_file = "test-input";
  tmp->parse (xml_data1, strlen (xml_data1), &error);
  TOK();
  if (!error.code)
    tmp->end_parse (&error);
  TOK();
  if (error.code)
    fatal ("%s:%d:%d: %s (%d)", input_file, error.line_number, error.char_number, error.message.c_str(), error.code);
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
}
REGISTER_UITHREAD_TEST ("labelmarkup/Test Markup Parser", test_markup_parser);

static void
test_text_markup()
{
  WindowImplP window = shared_ptr_cast<WindowImpl> (Factory::create_ui_widget ("Window"));
  WidgetImplP label = Factory::create_ui_widget ("Label");
  window->add (*label); // Label needs Window context for text processing
  TOK();
  String test_markup =
    "Start "
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
    "<larger>larger</larger>";
  label->set_property ("markup_text", test_markup);
  TOK();
  /* Here, the Label widget has been created, but some updates might still be queued via exec_now,
   * we need those completed before running tests (this is *not* generally recommended, outside tests).
   */
  uithread_main_loop ()->iterate_pending();
  TOK();
  String str = label->get_property ("markup_text");
  const char *markup_result = str.c_str();
  // g_printerr ("MARKUP: %s\n", markup_resullt);
  TASSERT (strstr (markup_result, "<I"));
  TASSERT (strstr (markup_result, "italic<"));
  TASSERT (strstr (markup_result, "<B"));
  TASSERT (strstr (markup_result, "bold<"));
  TASSERT (strstr (markup_result, "<U"));
  TASSERT (strstr (markup_result, "underline<"));
  /* try re-parsing */
  label->set_property ("markup_text", markup_result);
  TOK();
  str = label->get_property ("markup_text");
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
}
REGISTER_UITHREAD_TEST ("labelmarkup/Test Text Markup", test_text_markup);

} // anon
