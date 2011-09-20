/* Rapicorn Examples
 * Copyright (C) 2006 Tim Janik
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
#include <rapicorn.hh>

namespace {
using namespace Rapicorn;

static void
add_button_row (Container area,
                uint      row)
{
  Item child = area.create_child ("button-row", Strings ("id=row#" + string_from_uint (row)));
  Container brow = Container::downcast (child);
  for (uint i = 0; i < 50; i++)
    {
      Strings args = Strings ("test-button-text=\"(" + string_from_uint (row) + "," + string_from_uint (i) + ")\"");
      child = brow.create_child ("test-button", args);
      ButtonArea button = child.component<ButtonArea> ("/Button");
      button.on_click (string_printf ("Item::print('Button-Click: (%d,%d)')", row, i));
    }
}

extern "C" int
main (int   argc,
      char *argv[])
{
  // initialize Rapicorn
  Application app = init_app (String ("Rapicorn/Examples/") + RAPICORN__FILE__, &argc, argv);

  // find and load GUI definitions relative to argv[0]
  app.auto_load ("RapicornExamples", "scroller.xml", argv[0]);

  // create main window
  Window window = app.create_window ("main-shell");

  // create button rows
  Container mshell = window;
  for (uint i = 0; i < 50; i++)
    add_button_row (mshell, i);

  // show main window
  window.show();

  // run event loops while windows are on screen
  return app.run_and_exit();
}

} // anon
