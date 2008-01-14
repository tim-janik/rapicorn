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
#include <rapicorn-core/rapicorntests.h>
#include <rapicorn-core/rapicorn-core.hh>

namespace {
using namespace Rapicorn;

static void
test_types ()
{
  Type t1 ("RcTi;0003Foo;n;0008zonk=bar000eSHREK=Schreck!0009SomeNum=7;");
  assert (t1.storage() == Type::NUM);
  assert (t1.ident() == "Foo");
  assert (t1.label() == "Foo");
  assert (t1.aux_string ("SHREK") == "Schreck!");
  assert (t1.aux_num ("SomeNum") == 7);
  assert (t1.aux_float ("SomeNum") == 7.0);
  assert (t1.hints() == ":");
  Type t2 ("RcTi;0003Bar;n;0009label=BAR0008hints=rw000Dblurb=nothing;");
  assert (t2.storage() == Type::NUM);
  assert (t2.ident() == "Bar");
  assert (t2.label() == "BAR");
  assert (t2.blurb() == "nothing");
  assert (t2.hints() == ":rw:");
}

static void
test_value_num ()
{
  AnyValue v1 (Type::NUM);
  assert (v1.num() == false);
  v1 = true;
  assert (v1.num() == true);
  AnyValue v2 (v1);
  assert ((bool) v2 == true);
  v1.set ((int64) false);
  assert ((bool) v1 == false);
  v2 = v1;
  assert (v2.num() == false);

  v1 = 0;
  assert (v1.num() == 0);
  v1 = 1;
  assert (v1.num() == 1);
  v1.set ("-2");
  assert (v1.num() == -2.0);
  v1.set (9223372036854775807LL);
  assert (v1.num() == 9223372036854775807LL);
  v1 = false;
  assert (v1.num() == 0.0);
}

static void
test_value_float ()
{
  AnyValue v1 (Type::FLOAT);
  assert (v1.vfloat() == 0.0);
  v1 = 1.;
  assert (v1.vfloat() == 1.0);
  v1.set ("-1");
  assert (v1.vfloat() == -1.0);
  v1.set ((long double) 16.5e+6);
  assert (v1.vfloat() > 16000000.0);
  assert (v1.vfloat() < 17000000.0);
  v1 = 7;
  assert (v1.vfloat() == 7.0);
  v1 = false;
  assert (v1.vfloat() == 0.0);
}

static void
test_value_string ()
{
  AnyValue v1 (Type::STRING);
  assert (v1.string() == "");
  v1.set ("foo");
  assert (v1.string() == "foo");
  v1.set ((int64) 1);
  assert (v1.string() == "1");
  v1 = 0;
  assert (v1.string() == "0");
  v1 = "x";
  assert (v1.string() == "x");
}

static void
test_array ()
{
  Array a;
  // [0] == head
  a.push_head (AnyValue (Type::NUM, 0));
  assert (a[0].num() == 0);
  // [-1] == tail
  assert (a[-1].num() == 0);
  a.push_head (AnyValue (Type::NUM, 1));
  assert (a[0].num() == 1);
  assert (a[-1].num() == 0);
  assert (a.count() == 2);
  // out-of-range-int => string-key conversion:
  a[99] = 5;
  assert (a.key (2) == "99"); // count() was 2 before inserting
  assert (a[2].num() == 5);
  assert (a[-1].num() == 5);
  assert (5 == (int) a[-1]);
  assert (a.count() == 3);
  a.push_tail (AnyValue (Type::NUM, 2));
  assert (a[-1].num() == 2);
  assert (a.count() == 4);

  a.clear();
  a["ape"] = AnyValue (Type::STRING, "affe");
  a["beard"] = "bart";
  assert (a[-1].string() == "bart");
  a["anum"] = 5;
  assert (a.key (-3) == "ape");
  assert (a.pop_head().string() == "affe");
  assert (a.pop_head().string() == "bart");
  assert (a.key (-1) == "anum");
  assert (a.pop_head().num() == 5);
  assert (a.count() == 0);

  const char *board[][8] = {
    { "R","N","B","Q","K","B","N","R" },
    { "P","P","P","P","P","P","P","P" },
    { " "," "," "," "," "," "," "," " },
    { " "," "," "," "," "," "," "," " },
    { " "," "," "," "," "," "," "," " },
    { " "," "," "," "," "," "," "," " },
    { "p","p","p","p","p","p","p","p" },
    { "r","n","b","q","k","b","n","r" }
  };
  for (int64 i = 0; i < RAPICORN_ARRAY_SIZE (board); i++)
    a.push_tail (Array::FromArray<String> (board[i]));
  printout ("\nchess0:\n%s\n", a.to_string ("\n").c_str());

  a[3].array()[4] = a[1].array()[4];
  a[1].array()[4] = " ";

  printout ("\nchess1:\n%s\n", a.to_string ("\n").c_str());
}

} // Anon

int
main (int   argc,
      char *argv[])
{
  rapicorn_init_test (&argc, &argv);

  test_types();
  test_value_num();
  test_value_float();
  test_value_string();
  test_array();

  return 0;
}
