/* Rapicorn Examples
 * Copyright (C) 2006 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <rapicorn/rapicorn.hh>

namespace {
using namespace Rapicorn;

static void
add_button_row (Container &area)
{
  std::list<String> args;
  args.push_back ("Id=1");
  Handle<Container> rh = Factory::create_container ("button-row", args);
  AutoLocker rl (rh);
  area.add (rh.get());
  for (uint i = 0; i < 100; i++)
    {
      Handle<Item> bh = Factory::create_item ("test-button");
      AutoLocker bl (bh);
      rh.get().add (bh.get());
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
  Handle<Container> shandle = Factory::create_container ("main-shell");
  /* get thread safe window handle */
  AutoLocker sl (shandle); // auto-locks
  Container &shell = shandle.get();
  Root &root = shell.interface<Root&>();

  /* create button rows */
  for (uint i = 0; i < 100; i++)
    add_button_row (shell);

  /* show and process window */
  root.run_async();
  sl.unlock();             // un-protects item/root

  /* wait while the window runs asyncronously */
  sleep (10000);

  return 0;
}

} // anon
