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

extern "C" int
main (int   argc,
      char *argv[])
{
  /* initialize rapicorn */
  app.init_with_x11 (&argc, &argv, "TextTest");

  /* parse GUI description */
  app.auto_load ("RapicornTest", "texttest.xml", argv[0]);

  /* create main window */
  WindowBase &window = *app.create_window ("main-shell");

  /* show and process */
  window.show();
  app.execute_loops();

  return 0;
}

} // anon
