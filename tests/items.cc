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
#include <birnet/birnettests.h>
#include <rapicorn/rapicorn.hh>

namespace {
using namespace Rapicorn;

extern "C" int
main (int   argc,
      char *argv[])
{
  birnet_init_test (&argc, &argv);

  /* initialize rapicorn */
  rapicorn_init_with_gtk_thread (&argc, &argv, NULL);

  TSTART ("RapicornItems");
  /* parse standard GUI descriptions and create example item */
  Handle<Item> shandle = Factory::create_item ("Root");
  TOK();
  /* get thread safe window handle */
  AutoLocker slocker (shandle); // auto-locks
  TOK();
  Item &item = shandle.get();
  TOK();
  Root &root = item.interface<Root&>();
  TASSERT (&root != NULL);
  slocker.unlock();             // un-protects item/root
  TOK();
  TDONE();

  return 0;
}

} // Anon
