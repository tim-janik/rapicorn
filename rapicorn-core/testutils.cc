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
#include "testutils.hh"

namespace Rapicorn {
namespace Test {

static vector<void(*)(void*)> testfuncs;
static vector<void*>          testdatas;
static vector<String>         testnames;

void
add_internal (const String &testname,
              void        (*test_func) (void*),
              void         *data)
{
  testnames.push_back (testname);
  testfuncs.push_back ((void(*)(void*)) test_func);
  testdatas.push_back ((void*) data);
}

void
add (const String &testname,
     void        (*test_func) (void))
{
  add (testname, (void(*)(void*)) test_func, (void*) 0);
}

int
run (void)
{
  for (uint i = 0; i < testfuncs.size(); i++)
    {
      if (verbose())
        printout ("Test: run: %s\n", testnames[i].c_str());
      else
        {
          String sc = string_printf ("%s:", testnames[i].c_str());
          String sleft = string_printf ("%-68s", sc.c_str());
          printout ("%70s ", sleft.c_str());
        }
      testfuncs[i] (testdatas[i]);
      if (verbose())
        printout ("Test: result: OK\n");
      else
        printout ("OK\n");
    }
  return 0;
}

bool
verbose (void)
{
  return init_settings().test_verbose;
}

bool
quick (void)
{
  return init_settings().test_quick;
}

bool
slow (void)
{
  return init_settings().test_slow;
}

bool
thorough (void)
{
  return init_settings().test_slow;
}

bool
perf (void)
{
  return init_settings().test_perf;
}


} // Test
} // Rapicorn
