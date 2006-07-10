/* Tests
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
//#define TEST_VERBOSE
#include <rapicorn/rapicorn.hh>
#include <birnet/birnettests.h>
#include <rapicorn/testitem.hh>

namespace {
using namespace Rapicorn;

static bool test_item_fatal_asserts = true;

static void
assertion_ok (const String &assertion)
{
  TPRINT ("%s\n", assertion.c_str());
}

static void
assertions_passed ()
{
  TOK();
}

static void
run_and_test (const char *test_name)
{
  TSTART (test_name);
  Handle<Item> ihandle = Factory::create_item (test_name);
  TOK();
  AutoLocker locker (ihandle);
  Item &item = ihandle.get();
  Root &root = item.interface<Root&>();
  TOK();
  TestItem *titem = item.interface<TestItem*>();
  if (titem)
    {
      TOK();
      titem->sig_assertion_ok += slot (assertion_ok);
      titem->sig_assertions_passed += slot (assertions_passed);
      titem->fatal_asserts (test_item_fatal_asserts);
    }
  TOK();
  root.run_async();
  TOK();
  locker.unlock();
  TOK();
  sleep (3);
  warning ("EEEEEEEEEEK: have to abort! **FIXME!!!**"); _exit (0);
  TOK();
  TDONE();
}

extern "C" int
main (int   argc,
      char *argv[])
{
  birnet_init_test (&argc, &argv);

  for (int i = 0; i < argc; i++)
    if (String (argv[i]) == "--non-fatal")
      test_item_fatal_asserts = false;

  /* initialize rapicorn */
  rapicorn_init_with_gtk_thread (&argc, &argv, NULL);

  /* parse GUI description */
  Factory::must_parse_file ("testitems.xml", "TEST-ITEM", Path::dirname (argv[0]), Path::join (Path::dirname (argv[0]), ".."));

  /* create/run tests */
  run_and_test ("test-alignment");

  return 0;
}

} // Anon
