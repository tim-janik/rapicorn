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
#include <rapicorn/rapicorn.hh>

namespace {
using namespace Birnet;
using namespace Rapicorn;

static void
text_markup_test()
{
  TSTART ("Label-Markup");
  Item &label = Factory::create_item ("Label");
  TOK();
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
  label.set_property ("markup_text", test_markup);
  TOK();
  String str = label.get_property ("markup_text");
  const char *markup_result = str.c_str();
  // g_printerr ("MARKUP: %s\n", markup_result);
  TASSERT (strstr (markup_result, "<I"));
  TASSERT (strstr (markup_result, "italic<"));
  TASSERT (strstr (markup_result, "<B"));
  TASSERT (strstr (markup_result, "bold<"));
  TASSERT (strstr (markup_result, "<U"));
  TASSERT (strstr (markup_result, "underline<"));
  /* try re-parsing */
  label.set_property ("markup_text", markup_result);
  TOK();
  str = label.get_property ("markup_text");
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
  Application::init_with_x11 (&argc, &argv, "MarkupTest"); // FIXME: should work offscreen
  
  text_markup_test();
  return 0;
}

} // anon
