/* Tests
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
#include <rcore/testutils.hh>
#include <rapicorn.hh>

namespace {
using namespace Rapicorn;

extern "C" int
main (int   argc,
      char *argv[])
{
  // initialize rapicorn
  Application_SmartHandle smApp = init_test_app (String ("Rapicorn/") + RAPICORN__FILE__, &argc, argv);
  ApplicationImpl &app = ApplicationImpl::the(); // FIXME: use Application_SmartHandle once C++ bindings are ready

  TSTART ("RapicornItems");
  /* parse standard GUI descriptions and create example item */
  Wind0wIface &wind0w = *app.create_wind0w ("Window");
  TOK();
  /* get thread safe wind0w handle */
  TOK();
  ItemImpl &item = wind0w.impl();
  TOK();
  WindowImpl &window = item.interface<WindowImpl&>();
  TASSERT (&window != NULL);
  TOK();
  TDONE();

  return 0;
}

} // Anon
