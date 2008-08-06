/* Rapicorn Tests
 * Copyright (C) 2008 Tim Janik
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

static Store1*
create_store ()
{
  Store1 *s1 = Store1::create_memory_store (Type::lookup ("String"));
  Array row;
  row.push_head (AutoValue ("la"));
  s1->insert (0, row);
  row.push_tail (AutoValue ("le"));
  s1->insert (-1, row);
  row.push_tail (AutoValue ("lu"));
  s1->insert (-1, row);
  return s1;
}

extern "C" int
main (int   argc,
      char *argv[])
{
  /* initialize Rapicorn for X11 */
  Application::init_with_x11 (&argc, &argv, "FileView");
  /* initialization acquired global Rapicorn mutex */

  /* load GUI definition file, relative to argv[0] */
  Application::auto_load ("RapicornTest", "fileview.xml", argv[0]);

  /* create root item */
  Store1 *s1 = create_store();
  Window window = Application::create_window ("main-dialog",
                                              Args (""),
                                              Args ("ListModel=" + s1->model().object_url()));
  window.show();

  Application::execute_loops();

  return 0;
}

} // anon
