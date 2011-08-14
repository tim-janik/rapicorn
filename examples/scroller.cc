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
  Container brow = Container::_cast (child);
  for (uint i = 0; i < 50; i++)
    {
      Strings args = Strings ("test-button-text=\"(" + string_from_uint (row) + "," + string_from_uint (i) + ")\"");
      child = brow.create_child ("test-button", args);
      Item bchild = child.unique_component ("/Button");
      ButtonArea b = ButtonArea::_cast (bchild);
      b.on_click (string_printf ("Item::print('Button-Click: (%d,%d)')", row, i));
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

  // create main wind0w
  Wind0w wind0w = app.create_wind0w ("main-shell");

  // create button rows
  Container mshell = wind0w;
  for (uint i = 0; i < 50; i++)
    add_button_row (mshell, i);

  // show main window
  wind0w.show();

  // run event loops while wind0ws are on screen
  return app.run_and_exit();
}

} // anon
