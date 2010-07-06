/* Rapicorn
 * Copyright (C) 2010 Tim Janik
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
#include <rcore/testutils.hh>
#include <rapicorn.hh>
using namespace Rapicorn;

static Application * volatile smart_app_p = NULL;

static void
test_server_smart_handle (void)
{
  Application *ab = &app;
  Plic::FieldBuffer8 fb (4);
  fb.add_object (uint64 ((BaseObject*) ab));
  Plic::Coupler &c = *rope_thread_coupler();
  c.reader.reset (fb);
  Application_SmartHandle sh (c, c.reader);
  Application &shab = *sh;
  assert (ab == &shab);
  assert (ab == sh.operator->());
}

int
main (int   argc,
      char *argv[])
{
  rapicorn_init_test (&argc, &argv);

  Test::add ("Server/Smart Handle", test_server_smart_handle);

  return Test::run();
}
