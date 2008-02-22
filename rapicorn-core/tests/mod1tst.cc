/* Rapicorn
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
#include <rapicorn-core/testutils.hh>
#include <rapicorn-core/rapicorn-core.hh>

namespace {
using namespace Rapicorn;

static void
test_store1 ()
{
  /* create storage model with row type */
  Type t1 ("RcTi;000bDummyString;s;;");
  Store1 *s1 = Store1::create_memory_store (t1);
  assert (s1 != NULL);
  /* assert model/store identity (for memory stores) */
  Model1 &m1 = s1->model();
  assert (s1 == m1.store());
  /* basic store assertions */
  assert (s1->count() == 0);
  /* insert first row */
  Array ra;
  ra.push_head (AutoValue ("first"));
  s1->insert_row (0, ra);
  assert (s1->count() == 1);
}

} // Anon

int
main (int   argc,
      char *argv[])
{
  rapicorn_init_test (&argc, &argv);

  Test::add ("/Store/basics", test_store1);

  return Test::run();
}
