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
#include <rapicorn/rapicorn.hh>

namespace {
using namespace Rapicorn;

static void
add_button_row (Container &area,
                uint       row)
{
  std::list<String> row_args;
  row_args.push_back ("id=row#" + string_from_uint (row));
  Container &brow = Factory::create_container ("button-row", row_args);
  area.add (brow);
  for (uint i = 0; i < 20; i++)
    {
      std::list<String> args;
      args.push_back ("test-button-text=(" + string_from_uint (row) + "," + string_from_uint (i) + ")");
      Item &tb = Factory::create_item ("test-button", args);
      brow.add (tb);
    }
}

extern "C" int
main (int   argc,
      char *argv[])
{
  /* initialize rapicorn */
  rapicorn_init_with_gtk_thread (&argc, &argv, "Scroller");

  /* parse GUI description */
  Factory::must_parse_file ("scroller.xml", "Scroller", Path::dirname (argv[0]));

  /* create main window */
  Window window = Factory::create_window ("main-shell");
  AutoLocker wl (window);
  Container &mshell = window.root().interface<Container>();
  /* get thread safe window handle */
  Root &root = mshell.interface<Root&>();

  /* create button rows */
  for (uint i = 0; i < 20; i++)
    add_button_row (mshell, i);

  /* show and process window */
  root.show();
  wl.unlock();             // un-protects item/root

  /* wait while the window runs asyncronously */
  sleep (10000);

  return 0;
}

} // anon
