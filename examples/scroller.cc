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
add_button_row (Container &area,
                uint       row)
{
  Container &brow = Factory::create_container ("button-row", Args ("id=row#" + string_from_uint (row)));
  area.add (brow);
  for (uint i = 0; i < 20; i++)
    {
      std::list<String> args;
      args.push_back ("test-button-text=\"(" + string_from_uint (row) + "," + string_from_uint (i) + ")\"");
      Item &tb = Factory::create_item ("test-button", args);
      ButtonArea *b = tb.interface<ButtonArea*>();
      b->on_click (string_printf ("button_%d_%d", row, i));
      brow.add (tb);
    }
}

extern "C" int
main (int   argc,
      char *argv[])
{
  /* initialize rapicorn */
  App.init_with_x11 (&argc, &argv, "Scroller");

  /* parse GUI description */
  App.auto_load ("RapicornTest", "scroller.xml", argv[0]);

  /* create main window */
  WinPtr window = App.create_winptr ("main-shell");
  Container &mshell = window.root().interface<Container>();

  /* create button rows */
  for (uint i = 0; i < 20; i++)
    add_button_row (mshell, i);

  /* show and process */
  window.show();
  App.execute_loops();

  return 0;
}

} // anon
