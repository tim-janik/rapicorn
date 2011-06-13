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
#include <rcore/testutils.hh>
#include <rapicorn-core.hh>

namespace {
using namespace Rapicorn;

static void
test_store1_string_row ()
{
  Type t1 = Type::lookup ("RapicornTest::SimpleString");
  assert (t1.istype());
  Store1 *s1 = Store1::create_memory_store ("models/mod1tst/string-row", t1, SELECTION_NONE);
  assert (s1 != NULL);
  /* assert model/store identity (for memory stores) */
  Model1 &m1 = s1->model();
  assert (s1 == m1.store());
  /* basic store assertions */
  assert (s1->count() == 0);
  /* insert first row */
  Array ra;
  ra.push_head (AutoValue ("first"));
  s1->insert (0, ra);
  assert (s1->count() == 1);
}

static void
test_store1_array ()
{
  /* create storage model with row type */
  Type t1 = Type::lookup ("RapicornTest::SimpleRecord");
  assert (t1.istype());
  Store1 *s1 = Store1::create_memory_store ("models/mod1tst/array", t1, SELECTION_NONE);
  assert (s1 != NULL);
  assert (s1->count() == 0);
  /* insert first row */
  Array row;
  row[-1] = "a C string";
  row[-1] = 0.5; // double
  row[-1] = 17; // integer
  s1->insert (0, row);
  assert (s1->count() == 1);
}

static Store1&
create_dummy_store1 ()
{
  Store1 &store = *Store1::create_memory_store ("models/mod1tst/dummy",
                                                Type::lookup ("RapicornTest::SimpleString"),
                                                SELECTION_NONE);
  for (uint i = 0; i < 4; i++)
    {
      Array row;
      for (uint j = 0; j < 4; j++)
        row[j] = string_printf ("%u-%u", i + 1, j + 1);
      store.insert (-1, row);
    }
  return store;
}

static String
stringify_model (Model1 &model)
{
  String s = "[\n";
  for (uint i = 0; i < model.count(); i++)
    {
      Array row = model.get (i);
      s += "  (";
      for (uint j = 0; j < row.count(); j++)
        s += string_printf ("%s,", row[j].string().c_str());
      s += "),\n";
    }
  s += "]";
  return s;
}

static void
test_store1_row_ops ()
{
  Store1 &store = create_dummy_store1();
  Model1 &model = store.model();
  Array row;

  // store operations
  row[0] = "Newly_appended_last_row";
  store.insert (-1, row);
  row[0] = "Newly_prepended_first_row";
  store.insert (0, row);

  row = model.get (1);
  row[4] = "Extra_text_added_to_row_1";
  store.update (1, row);

  row = model.get (2);
  store.remove (2);
  row.clear();
  row[0] = "Replacement_for_removed_row_2";
  store.insert (2, row);

  // test verification
  String e = "[\n  (Newly_prepended_first_row,),\n  (1-1,1-2,1-3,1-4,Extra_text_added_to_row_1,),\n"
             "  (Replacement_for_removed_row_2,),\n  (3-1,3-2,3-3,3-4,),\n  (4-1,4-2,4-3,4-4,),\n"
             "  (Newly_appended_last_row,),\n]";
  String s = stringify_model (model);
  // printout ("\nMODEL: %s\n", s.c_str());
  assert (e == s);

  // cleanup
  unref (store);
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
  rapicorn_init_test (&argc, argv);

  // first, load required type package
  Type::register_package_file (Path::vpath_find ("testtypes.tpg"));

  Test::add ("/Store/string-row", test_store1_string_row);
  Test::add ("/Store/array-row", test_store1_array);
  Test::add ("/Store/row-ops", test_store1_row_ops);
  Test::add ("/Store/row-test", test_store1_row_test);

  return Test::run();
}
