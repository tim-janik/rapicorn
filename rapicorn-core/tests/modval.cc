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

template<typename CharArray> static String
string_from_chars (CharArray &ca)
{
  return String (ca, sizeof (ca));
}

static void
test_type_info ()
{
  String ts;
  // fail type extraction
  Type t0 = Type::try_lookup (":invalid:::type^^^Definition");
  assert (t0.istype() == false);
  // ensure standard types
  Type s1 = Type::try_lookup ("String");
  assert (s1.storage() == STRING);
  Type s2 = Type::try_lookup ("Num");
  assert (s2.storage() == NUM);
  Type s3 = Type::try_lookup ("Real");
  assert (s3.storage() == REAL);
  // type extraction by reference
  Type t1 = Type::lookup ("RapicornTest::NumWithFooAsLabel");
  assert (t1.istype());
  assert (t1.name() == "NumWithFooAsLabel");
  assert (t1.storage() == NUM);
  assert (t1.aux_string ("label") == "Foo");
  assert (t1.hints() == ":");
  // type extraction by pointer (need to check it didn't fail)
  Type t2 = Type::lookup ("RapicornTest::RealWithBlurbBlurb");
  assert (t2.istype());
  assert (t2.name() == "RealWithBlurbBlurb");
  assert (t2.storage() == REAL);
  assert (t2.aux_string ("blurb") == "Real Blurb");
  // check that type memory used above is still valid
  assert (t1.name() + "-postfix" == String ("NumWithFooAsLabel-postfix"));
  assert (t2.name() == String ("RealWithBlurbBlurb"));
}

static void
test_type_api ()
{
  { // Num
    Type t = Type::lookup ("RapicornTest::ExtendNumWithAux");
    assert (t.istype());
    assert (t.storage() == NUM);
    assert (t.storage_name() == String ("NUM"));
    assert (t.name() == t.ident());
    assert (t.ident() == "ExtendNumWithAux");
    assert (t.label() == "Extended Num");
    assert (t.blurb() == "This Num demonstrates extensive auxillary data use");
    assert (t.aux_num ("default") == 33);
    assert (t.aux_real ("step") == -5);
    assert (t.hints().find (":extra-option:") != String().npos);
  }
  { // Real
    Type t = Type::lookup ("RapicornTest::RealWithBlurbBlurb");
    assert (t.istype());
    assert (t.storage() == REAL);
    assert (t.storage_name() == String ("REAL"));
    assert (t.name() == t.ident());
    assert (t.ident() == "RealWithBlurbBlurb");
    assert (t.label() == "Real Label");
    assert (t.blurb() == "Real Blurb");
    assert (t.aux_real ("default") == 97.97);
    assert (t.hints() == ":");
  }
  { // String
    Type t = Type::lookup ("RapicornTest::ExtendedString");
    assert (t.istype());
    assert (t.storage() == STRING);
    assert (t.storage_name () == String ("STRING"));
    assert (t.name() == t.ident());
    assert (t.ident() == "ExtendedString");
    assert (t.label() == "Extended String");
    assert (t.blurb() == "Demonstrate full string specification");
    assert (t.aux_string ("default") == "Default-String-Value");
    assert (t.hints().find (":ro:") != String().npos);
  }
  { // Array
    Type t = Type::lookup ("Array");
    assert (t.istype());
    assert (t.storage() == ARRAY);
    assert (t.storage_name () == String ("ARRAY"));
    assert (t.name() == t.ident());
    assert (t.ident() == "Array");
    assert (t.label() == "Array");
    assert (t.blurb() == "");
    assert (t.aux_string ("default") == "");
    assert (t.hints() == ":");
  }
  { // Choice
    Type t = Type::lookup ("RapicornTest::Enum1");
    assert (t.istype());
    assert (t.storage() == CHOICE);
    assert (t.storage_name () == String ("CHOICE"));
    assert (t.name() == t.ident());
    assert (t.ident() == "Enum1");
    assert (t.label() == "Enum1");
    assert (t.blurb() == "");
    assert (t.aux_string ("default") == "");
    assert (t.hints() == ":");
  }
  { // Sequence
    Type t = Type::lookup ("RapicornTest::SimpleSequence");
    assert (t.istype());
    assert (t.storage() == SEQUENCE);
    assert (t.storage_name () == String ("SEQUENCE"));
    assert (t.name() == t.ident());
    assert (t.ident() == "SimpleSequence");
    assert (t.label() == "SimpleSequence");
    assert (t.blurb() == "");
    assert (t.aux_string ("default") == "");
    assert (t.hints() == ":");
  }
}

static void
test_value_num ()
{
  AnyValue v1 (NUM);
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
  AnyValue v1 (REAL);
  assert (v1.real() == 0.0);
  v1 = 1.;
  assert (v1.real() == 1.0);
  v1.set ("-1");
  assert (v1.real() == -1.0);
  v1.set ((long double) 16.5e+6);
  assert (v1.real() > 16000000.0);
  assert (v1.real() < 17000000.0);
  v1 = 7;
  assert (v1.real() == 7.0);
  v1 = false;
  assert (v1.real() == 0.0);
}

static void
test_value_string ()
{
  AnyValue v1 (STRING);
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

struct TestObject : public virtual ReferenceCountImpl {};

static void
test_array_auto_value ()
{
  Array a1, a2;
  TestObject *to = new TestObject();
  ref_sink (to);
  a1.push_head (AutoValue ("first")); // demand create AutoValue
  a1.push_head (String ("second")); // implicit AutoValue creation
  a1.push_head ("third"); // implicit AutoValue creation from C string
  // implicit AutoValue creations
  a1.push_head (0.1);
  a1.push_head (a2);
  a1.push_head (*to);
  unref (to);
}

static void
test_array ()
{
  Array a;
  // [0] == head
  a.push_head (AnyValue (NUM, 0));
  assert (a[0].num() == 0);
  // [-1] == tail
  assert (a[-1].num() == 0);
  a.push_head (AnyValue (NUM, 1));
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
  a.push_tail (AnyValue (NUM, 2));
  assert (a[-1].num() == 2);
  assert (a.count() == 4);

  a.clear();
  a["ape"] = AnyValue (STRING, "affe");
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
    a.push_tail (Array::FromCArray<String> (board[i]));
  if (Test::verbose())
    printout ("chess0:\n%s\n", a.to_string ("\n").c_str());

  a[3].array()[4] = a[1].array()[4];
  a[1].array()[4] = " ";

  if (Test::verbose())
    printout ("\nchess1:\n%s\n", a.to_string ("\n").c_str());
}

static void
variable_changed (Variable *self)
{
  if (Test::verbose())
    printout ("Variable::changed: %s\n", self->type().ident().c_str());
}

static void
test_variable ()
{
  Type somestring ("RapicornTest::SimpleString");
  Variable &v1 = *new Variable (somestring);
  ref_sink (v1);
  v1.sig_changed += slot (variable_changed, &v1);
  v1 = "foohoo";
  String as_string = v1;
  assert (as_string == "foohoo");
  v1 = 5;
  assert (v1.get<String>() == "5");
  v1.set (6.0);
  assert (v1.get<String>() == "6");
  bool b = v1;
  int i = v1;
  uint ui = v1;
  double d = v1;
  (void) (b + i + ui + d); // silence compiler
  unref (v1);
}


} // Anon

int
main (int   argc,
      char *argv[])
{
  rapicorn_init_test (&argc, &argv);

  // first, load required type package
  Type::register_package_file ("testtypes.tpg");

  Test::add ("/Type/type info", test_type_info);
  Test::add ("/Type/basics", test_type_api);
  Test::add ("/Value/num", test_value_num);
  Test::add ("/Value/float", test_value_float);
  Test::add ("/Value/string", test_value_string);
  Test::add ("/Array/AutoValue", test_array_auto_value);
  Test::add ("/Array/Chess", test_array);
  Test::add ("/Variable/notification", test_variable);

  return Test::run();
}
