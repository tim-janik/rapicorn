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
//#define TEST_VERBOSE
#include <rapicorn/birnettests.h>
#include <rapicorn/rapicorn.hh>

namespace {
using namespace Rapicorn;

class EvalTest {
  Evaluator ev;
  String
  eval (const String &s)
  {
    return ev.expand_expression (s);
  }
public:
  EvalTest()
  {
  }
  void
  test_evaluator (void)
  {
    TSTART ("simple expressions");
    TASSERT (eval ("") == "");
    TASSERT (eval ("foo") == "foo");
    TASSERT (eval ("$$") == "$");
    TDONE();

    TSTART ("variable expansion");
    TASSERT (eval ("$zero") == "");
    ev.set ("zero=0");
    TASSERT (eval ("$zero") == "0");
    ev.set ("zero=");
    TASSERT (eval ("$zero") == "");
    ev.set ("zero=0");
    TASSERT (eval ("$zero") == "0");
    TASSERT (eval ("$one") == "");
    ev.set ("one=1");
    TASSERT (eval ("$zero") == "0");
    TASSERT (eval ("$one") == "1");
    TASSERT (eval ("zero") == "zero");
    TASSERT (eval ("before^$one") == "before^1");
    TASSERT (eval ("$one^after") == "1^after");
    TASSERT (eval ("before^$one^after") == "before^1^after");
    TDONE();

    TSTART ("function evaluation");
    TASSERT (eval ("$(not 0)") == "1");
    TASSERT (eval ("$(not,1)") == "0");
    TASSERT (eval ("$(bool,ABC)") == "1");
    TASSERT (eval ("before$(not 0)") == "before1");
    TASSERT (eval ("$(not 0)after") == "1after");
    TASSERT (eval ("before$(not 0)after") == "before1after");
    TDONE();
  }
  void
  test_standard_env()
  {
    TSTART ("standard variables");
    TASSERT (eval ("$(streq ,)") == "1");
    TASSERT (eval ("$(streq text,)") == "0");
    TASSERT (eval ("$(streq $one,)") == "0");
    TASSERT (eval ("$(streq $one,1)") == "1");
    TASSERT (eval ("$(streq $ONE,1)") == "0");
    ev.set ("ONE=1");
    TASSERT (eval ("$(streq $ONE,1)") == "1");
    TASSERT (eval ("$(streq $one,$ONE)") == "1");
    TASSERT (eval ("$(streq $one,$zero)") == "0");
    TASSERT (eval ("$(streq $UNSET_variable,)") == "1");
    TASSERT (eval ("$(streq $EMPTY_variable,)") == "1");
    ev.set ("EMPTY_variable=");
    TASSERT (eval ("$(streq $EMPTY_variable,)") == "1");
    /* variables that are non-empty by default */
    TASSERT (eval ("$(streq $RANDOM,)") == "0");
    TASSERT (eval ("$(streq $RAPICORN_VERSION,)") == "0");
    TASSERT (eval ("$(streq $RAPICORN_ARCHITECTURE,)") == "0");
    TDONE();
  }
  void
  test_functions()
  {
    TSTART ("count");
    TASSERT (eval ("$(count)") == "0");
    TASSERT (eval ("$(count )") == "0");
    TASSERT (eval ("$(count,)") == "1");
    TASSERT (eval ("$(count ,)") == "1");
    TASSERT (eval ("$(count a)") == "1");
    TASSERT (eval ("$(count a,)") == "2");
    TASSERT (eval ("$(count a,b)") == "2");
    TASSERT (eval ("$(count ,b,)") == "2");
    TASSERT (eval ("$(count a,b,)") == "3");
    TASSERT (eval ("$(count a,b,c)") == "3");
    TASSERT (eval ("$(count a,b,c,)") == "4");
    TDONE();
    TSTART ("bool");
    TASSERT (eval ("$(bool 0)") == "0");
    TASSERT (eval ("$(bool 1)") == "1");
    TASSERT (eval ("$(bool 2)") == "1");
    TASSERT (eval ("$(bool,0)") == "0");
    TASSERT (eval ("$(bool,1)") == "1");
    TASSERT (eval ("$(bool,2)") == "1");
    TASSERT (eval ("$(bool n)") == "0");
    TASSERT (eval ("$(bool y)") == "1");
    TASSERT (eval ("$(bool 00)") == "0");
    TASSERT (eval ("$(bool 01)") == "1");
    TASSERT (eval ("$(bool 05)") == "1");
    TASSERT (eval ("$(bool 09)") == "1");
    TASSERT (eval ("$(bool On)") == "1");
    TASSERT (eval ("$(bool Off)") == "0");
    TASSERT (eval ("$(bool 0b)") == "0");
    TASSERT (eval ("$(bool 0x0b)") == "1");
    TASSERT (eval ("$(bool true)") == "1");
    TASSERT (eval ("$(bool false)") == "0");
    TDONE();
    TSTART ("not");
    TASSERT (eval ("$(not 0)") == "1");
    TASSERT (eval ("$(not 1)") == "0");
    TASSERT (eval ("$(not)") == "1");
    TASSERT (eval ("$(not,yes)") == "0");
    TDONE();
    g_print ("println: "); /* special "check" */
    String retval = eval ("$(println,[ok])");
    TSTART ("println");
    TASSERT (retval == "");
    TDONE();
  }
  void
  test_bool_logic()
  {
    TSTART ("xor");
    TASSERT (eval ("$(xor)")         == "0");
    TASSERT (eval ("$(xor       0)") == "0");
    TASSERT (eval ("$(xor       1)") == "1");
    TASSERT (eval ("$(xor    0, 0)") == "0");
    TASSERT (eval ("$(xor    0, 1)") == "1");
    TASSERT (eval ("$(xor    1, 0)") == "1");
    TASSERT (eval ("$(xor    1, 1)") == "0");
    TASSERT (eval ("$(xor 0, 0, 0)") == "0");
    TASSERT (eval ("$(xor 0, 0, 1)") == "1");
    TASSERT (eval ("$(xor 0, 1, 0)") == "1");
    TASSERT (eval ("$(xor 0, 1, 1)") == "0");
    TASSERT (eval ("$(xor 1, 0, 0)") == "1");
    TASSERT (eval ("$(xor 1, 0, 1)") == "0");
    TASSERT (eval ("$(xor 1, 1, 0)") == "0");
    TASSERT (eval ("$(xor 1, 1, 1)") == "1");
    TDONE();
    TSTART ("or");
    TASSERT (eval ("$(or)")         == "0");
    TASSERT (eval ("$(or       0)") == "0");
    TASSERT (eval ("$(or       1)") == "1");
    TASSERT (eval ("$(or    0, 0)") == "0");
    TASSERT (eval ("$(or    0, 1)") == "1");
    TASSERT (eval ("$(or    1, 0)") == "1");
    TASSERT (eval ("$(or    1, 1)") == "1");
    TASSERT (eval ("$(or 0, 0, 0)") == "0");
    TASSERT (eval ("$(or 0, 0, 1)") == "1");
    TASSERT (eval ("$(or 0, 1, 0)") == "1");
    TASSERT (eval ("$(or 0, 1, 1)") == "1");
    TASSERT (eval ("$(or 1, 0, 0)") == "1");
    TASSERT (eval ("$(or 1, 0, 1)") == "1");
    TASSERT (eval ("$(or 1, 1, 0)") == "1");
    TASSERT (eval ("$(or 1, 1, 1)") == "1");
    TDONE();
    TSTART ("and");
    TASSERT (eval ("$(and)")         == "1");
    TASSERT (eval ("$(and       0)") == "0");
    TASSERT (eval ("$(and       1)") == "1");
    TASSERT (eval ("$(and    0, 0)") == "0");
    TASSERT (eval ("$(and    0, 1)") == "0");
    TASSERT (eval ("$(and    1, 0)") == "0");
    TASSERT (eval ("$(and    1, 1)") == "1");
    TASSERT (eval ("$(and 0, 0, 0)") == "0");
    TASSERT (eval ("$(and 0, 0, 1)") == "0");
    TASSERT (eval ("$(and 0, 1, 0)") == "0");
    TASSERT (eval ("$(and 0, 1, 1)") == "0");
    TASSERT (eval ("$(and 1, 0, 0)") == "0");
    TASSERT (eval ("$(and 1, 0, 1)") == "0");
    TASSERT (eval ("$(and 1, 1, 0)") == "0");
    TASSERT (eval ("$(and 1, 1, 1)") == "1");
    TDONE();
    TSTART ("xnor");
    TASSERT (eval ("$(xnor)")         == "1");
    TASSERT (eval ("$(xnor       0)") == "1");
    TASSERT (eval ("$(xnor       1)") == "0");
    TASSERT (eval ("$(xnor    0, 0)") == "1");
    TASSERT (eval ("$(xnor    0, 1)") == "0");
    TASSERT (eval ("$(xnor    1, 0)") == "0");
    TASSERT (eval ("$(xnor    1, 1)") == "1");
    TASSERT (eval ("$(xnor 0, 0, 0)") == "1");
    TASSERT (eval ("$(xnor 0, 0, 1)") == "0");
    TASSERT (eval ("$(xnor 0, 1, 0)") == "0");
    TASSERT (eval ("$(xnor 0, 1, 1)") == "1");
    TASSERT (eval ("$(xnor 1, 0, 0)") == "0");
    TASSERT (eval ("$(xnor 1, 0, 1)") == "1");
    TASSERT (eval ("$(xnor 1, 1, 0)") == "1");
    TASSERT (eval ("$(xnor 1, 1, 1)") == "0");
    TDONE();
    TSTART ("nor");
    TASSERT (eval ("$(nor)")         == "1");
    TASSERT (eval ("$(nor       0)") == "1");
    TASSERT (eval ("$(nor       1)") == "0");
    TASSERT (eval ("$(nor    0, 0)") == "1");
    TASSERT (eval ("$(nor    0, 1)") == "0");
    TASSERT (eval ("$(nor    1, 0)") == "0");
    TASSERT (eval ("$(nor    1, 1)") == "0");
    TASSERT (eval ("$(nor 0, 0, 0)") == "1");
    TASSERT (eval ("$(nor 0, 0, 1)") == "0");
    TASSERT (eval ("$(nor 0, 1, 0)") == "0");
    TASSERT (eval ("$(nor 0, 1, 1)") == "0");
    TASSERT (eval ("$(nor 1, 0, 0)") == "0");
    TASSERT (eval ("$(nor 1, 0, 1)") == "0");
    TASSERT (eval ("$(nor 1, 1, 0)") == "0");
    TASSERT (eval ("$(nor 1, 1, 1)") == "0");
    TDONE();
    TSTART ("nand");
    TASSERT (eval ("$(nand)")         == "0");
    TASSERT (eval ("$(nand       0)") == "1");
    TASSERT (eval ("$(nand       1)") == "0");
    TASSERT (eval ("$(nand    0, 0)") == "1");
    TASSERT (eval ("$(nand    0, 1)") == "1");
    TASSERT (eval ("$(nand    1, 0)") == "1");
    TASSERT (eval ("$(nand    1, 1)") == "0");
    TASSERT (eval ("$(nand 0, 0, 0)") == "1");
    TASSERT (eval ("$(nand 0, 0, 1)") == "1");
    TASSERT (eval ("$(nand 0, 1, 0)") == "1");
    TASSERT (eval ("$(nand 0, 1, 1)") == "1");
    TASSERT (eval ("$(nand 1, 0, 0)") == "1");
    TASSERT (eval ("$(nand 1, 0, 1)") == "1");
    TASSERT (eval ("$(nand 1, 1, 0)") == "1");
    TASSERT (eval ("$(nand 1, 1, 1)") == "0");
    TDONE();
  }
  void
  test_float_cmp()
  {
    TSTART ("lt");
    TASSERT (eval ("$(lt 4, 4)")    == "0");
    TASSERT (eval ("$(lt 4, 4, 4)") == "0");
    TASSERT (eval ("$(lt 4, 7)")    == "1");
    TASSERT (eval ("$(lt 4, 4, 7)") == "0");
    TASSERT (eval ("$(lt 4, 7, 7)") == "0");
    TASSERT (eval ("$(lt 4, 7, 8)") == "1");
    TASSERT (eval ("$(lt 8, 7, 4)") == "0");
    TASSERT (eval ("$(lt 7, 7, 4)") == "0");
    TASSERT (eval ("$(lt 7, 4, 4)") == "0");
    TASSERT (eval ("$(lt 7, 4)")    == "0");
    TDONE();
    TSTART ("le");
    TASSERT (eval ("$(le 4, 4)")    == "1");
    TASSERT (eval ("$(le 4, 4, 4)") == "1");
    TASSERT (eval ("$(le 4, 7)")    == "1");
    TASSERT (eval ("$(le 4, 4, 7)") == "1");
    TASSERT (eval ("$(le 4, 7, 7)") == "1");
    TASSERT (eval ("$(le 4, 7, 8)") == "1");
    TASSERT (eval ("$(le 8, 7, 4)") == "0");
    TASSERT (eval ("$(le 7, 7, 4)") == "0");
    TASSERT (eval ("$(le 7, 4, 4)") == "0");
    TASSERT (eval ("$(le 7, 4)")    == "0");
    TDONE();
    TSTART ("eq");
    TASSERT (eval ("$(eq 4, 4)")    == "1");
    TASSERT (eval ("$(eq 4, 4, 4)") == "1");
    TASSERT (eval ("$(eq 4, 7)")    == "0");
    TASSERT (eval ("$(eq 4, 4, 7)") == "0");
    TASSERT (eval ("$(eq 4, 7, 7)") == "0");
    TASSERT (eval ("$(eq 4, 7, 8)") == "0");
    TASSERT (eval ("$(eq 8, 7, 4)") == "0");
    TASSERT (eval ("$(eq 7, 7, 4)") == "0");
    TASSERT (eval ("$(eq 7, 4, 4)") == "0");
    TASSERT (eval ("$(eq 7, 4)")    == "0");
    TDONE();
    TSTART ("ge");
    TASSERT (eval ("$(ge 4, 4)")    == "1");
    TASSERT (eval ("$(ge 4, 4, 4)") == "1");
    TASSERT (eval ("$(ge 4, 7)")    == "0");
    TASSERT (eval ("$(ge 4, 4, 7)") == "0");
    TASSERT (eval ("$(ge 4, 7, 7)") == "0");
    TASSERT (eval ("$(ge 4, 7, 8)") == "0");
    TASSERT (eval ("$(ge 8, 7, 4)") == "1");
    TASSERT (eval ("$(ge 7, 7, 4)") == "1");
    TASSERT (eval ("$(ge 7, 4, 4)") == "1");
    TASSERT (eval ("$(ge 7, 4)")    == "1");
    TDONE();
    TSTART ("gt");
    TASSERT (eval ("$(gt 4, 4)")    == "0");
    TASSERT (eval ("$(gt 4, 4, 4)") == "0");
    TASSERT (eval ("$(gt 4, 7)")    == "0");
    TASSERT (eval ("$(gt 4, 4, 7)") == "0");
    TASSERT (eval ("$(gt 4, 7, 7)") == "0");
    TASSERT (eval ("$(gt 4, 7, 8)") == "0");
    TASSERT (eval ("$(gt 8, 7, 4)") == "1");
    TASSERT (eval ("$(gt 7, 7, 4)") == "0");
    TASSERT (eval ("$(gt 7, 4, 4)") == "0");
    TASSERT (eval ("$(gt 7, 4)")    == "1");
    TDONE();
    TSTART ("ne");
    TASSERT (eval ("$(ne 4, 4)")    == "0");
    TASSERT (eval ("$(ne 4, 4, 4)") == "0");
    TASSERT (eval ("$(ne 4, 7)")    == "1");
    TASSERT (eval ("$(ne 4, 4, 7)") == "0");
    TASSERT (eval ("$(ne 4, 7, 7)") == "0");
    TASSERT (eval ("$(ne 4, 7, 8)") == "1");
    TASSERT (eval ("$(ne 8, 7, 4)") == "1");
    TASSERT (eval ("$(ne 7, 7, 4)") == "0");
    TASSERT (eval ("$(ne 7, 4, 4)") == "0");
    TASSERT (eval ("$(ne 7, 4)")    == "1");
    TDONE();
  }
  void
  test_str_cmp()
  {
    TSTART ("strlt");
    TASSERT (eval ("$(strlt a,a)")   == "0");
    TASSERT (eval ("$(strlt a,a,a)") == "0");
    TASSERT (eval ("$(strlt a,b)")   == "1");
    TASSERT (eval ("$(strlt a,a,b)") == "0");
    TASSERT (eval ("$(strlt a,b,b)") == "0");
    TASSERT (eval ("$(strlt a,b,c)") == "1");
    TASSERT (eval ("$(strlt c,b,a)") == "0");
    TASSERT (eval ("$(strlt b,b,a)") == "0");
    TASSERT (eval ("$(strlt b,a,a)") == "0");
    TASSERT (eval ("$(strlt b,a)")   == "0");
    TDONE();
    TSTART ("strle");
    TASSERT (eval ("$(strle a,a)")   == "1");
    TASSERT (eval ("$(strle a,a,a)") == "1");
    TASSERT (eval ("$(strle a,b)")   == "1");
    TASSERT (eval ("$(strle a,a,b)") == "1");
    TASSERT (eval ("$(strle a,b,b)") == "1");
    TASSERT (eval ("$(strle a,b,c)") == "1");
    TASSERT (eval ("$(strle c,b,a)") == "0");
    TASSERT (eval ("$(strle b,b,a)") == "0");
    TASSERT (eval ("$(strle b,a,a)") == "0");
    TASSERT (eval ("$(strle b,a)")   == "0");
    TDONE();
    TSTART ("streq");
    TASSERT (eval ("$(streq a,a)")   == "1");
    TASSERT (eval ("$(streq a,a,a)") == "1");
    TASSERT (eval ("$(streq a,b)")   == "0");
    TASSERT (eval ("$(streq a,a,b)") == "0");
    TASSERT (eval ("$(streq a,b,b)") == "0");
    TASSERT (eval ("$(streq a,b,c)") == "0");
    TASSERT (eval ("$(streq c,b,a)") == "0");
    TASSERT (eval ("$(streq b,b,a)") == "0");
    TASSERT (eval ("$(streq b,a,a)") == "0");
    TASSERT (eval ("$(streq b,a)")   == "0");
    TDONE();
    TSTART ("strge");
    TASSERT (eval ("$(strge a,a)")   == "1");
    TASSERT (eval ("$(strge a,a,a)") == "1");
    TASSERT (eval ("$(strge a,b)")   == "0");
    TASSERT (eval ("$(strge a,a,b)") == "0");
    TASSERT (eval ("$(strge a,b,b)") == "0");
    TASSERT (eval ("$(strge a,b,c)") == "0");
    TASSERT (eval ("$(strge c,b,a)") == "1");
    TASSERT (eval ("$(strge b,b,a)") == "1");
    TASSERT (eval ("$(strge b,a,a)") == "1");
    TASSERT (eval ("$(strge b,a)")   == "1");
    TDONE();
    TSTART ("strgt");
    TASSERT (eval ("$(strgt a,a)")   == "0");
    TASSERT (eval ("$(strgt a,a,a)") == "0");
    TASSERT (eval ("$(strgt a,b)")   == "0");
    TASSERT (eval ("$(strgt a,a,b)") == "0");
    TASSERT (eval ("$(strgt a,b,b)") == "0");
    TASSERT (eval ("$(strgt a,b,c)") == "0");
    TASSERT (eval ("$(strgt c,b,a)") == "1");
    TASSERT (eval ("$(strgt b,b,a)") == "0");
    TASSERT (eval ("$(strgt b,a,a)") == "0");
    TASSERT (eval ("$(strgt b,a)")   == "1");
    TDONE();
    TSTART ("strne");
    TASSERT (eval ("$(strne a,a)")   == "0");
    TASSERT (eval ("$(strne a,a,a)") == "0");
    TASSERT (eval ("$(strne a,b)")   == "1");
    TASSERT (eval ("$(strne a,a,b)") == "0");
    TASSERT (eval ("$(strne a,b,b)") == "0");
    TASSERT (eval ("$(strne a,b,c)") == "1");
    TASSERT (eval ("$(strne c,b,a)") == "1");
    TASSERT (eval ("$(strne b,b,a)") == "0");
    TASSERT (eval ("$(strne b,a,a)") == "0");
    TASSERT (eval ("$(strne b,a)")   == "1");
    TDONE();
  }
};

extern "C" int
main (int   argc,
      char *argv[])
{
  rapicorn_init_test (&argc, &argv);

  /* initialize rapicorn */
  rapicorn_init_with_gtk_thread (&argc, &argv, NULL); // FIXME: should work without Gtk+

  /* run tests */
  EvalTest et;
  et.test_evaluator();
  et.test_functions();
  et.test_bool_logic();
  et.test_float_cmp();
  et.test_str_cmp();
  et.test_standard_env();

  return 0;
}

} // Anon
