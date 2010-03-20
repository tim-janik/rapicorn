/* Rapicorn Examples
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
#include <ui/rapicorn.hh>

namespace {
using namespace Rapicorn;

static bool
custom_commands (Window             &window,
                 const String       &command,
                 const StringVector &args)
{
  if (command == "testdump")
    {
      Root &root = window.root();
      TestStream *tstream = TestStream::create_test_stream();
      // tstream->filter_matched_nodes ("Button");
      root.get_test_dump (*tstream);
      printout ("%s", tstream->string().c_str());
      delete tstream;
    }
  else
    printout ("%s(): custom command: %s(%s) (window: %s)\n", __func__,
              command.c_str(), string_join (",", args).c_str(), window.root().name().c_str());
  return true;
}

#include "../rcore/tests/testpixs.c" // alpha_rle alpha_raw rgb_rle rgb_raw

extern "C" int
main (int   argc,
      char *argv[])
{
  /* initialize Rapicorn and its gtk backend */
  Application::init_with_x11 (&argc, &argv, "TourTest");
  /* initialization acquired global Rapicorn mutex */

  /* register builtin images */
  Application::pixstream ("testimage-alpha-rle", alpha_rle);
  Application::pixstream ("testimage-alpha-raw", alpha_raw);
  Application::pixstream ("testimage-rgb-rle", rgb_rle);
  Application::pixstream ("testimage-rgb-raw", rgb_raw);

  /* load GUI definition file, relative to argv[0] */
  Application::auto_load ("RapicornTest", "tour.xml", argv[0]);

  /* create root item */
  Window window = Application::create_window ("tour-dialog");
  window.commands += custom_commands;

  window.show();

  Application::execute_loops();

  return 0;
}

} // anon
