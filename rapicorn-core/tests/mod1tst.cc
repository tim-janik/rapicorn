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
test_store1_string_row ()
{
  Type t1 = Type::lookup ("RapicornTest::SimpleString");
  assert (t1.istype());
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

static void
test_store1_array ()
{
  /* create storage model with row type */
  Type t1 = Type::lookup ("RapicornTest::SimpleRecord");
  assert (t1.istype());
  Store1 *s1 = Store1::create_memory_store (t1);
  assert (s1 != NULL);
  assert (s1->count() == 0);
  /* insert first row */
  Array row;
  row[-1] = "a C string";
  row[-1] = 0.5; // double
  row[-1] = 17; // integer
  s1->insert_row (0, row);
  assert (s1->count() == 1);
}

static void
test_store1_row_test ()
{
  /* creating a model 1D test:
   * 1: create array type with fixed field count
   * 2: create list store from type
   * 3: create row with random field values
   * 4: add row to list
   * 5: fetc row
   * 6: verify altered field values
   */
}

} // Anon

int
main (int   argc,
      char *argv[])
{
  rapicorn_init_test (&argc, &argv);

  // first, load required type package
  Type::register_package_file ("testtypes.tpg");

  Test::add ("/Store/string-row", test_store1_string_row);
  Test::add ("/Store/array-row", test_store1_array);
  Test::add ("/Store/row-test", test_store1_row_test);

  return Test::run();
}
