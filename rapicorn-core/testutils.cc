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
      printout ("%s: ", testnames[i].c_str());
      testfuncs[i] (testdatas[i]);
      printout ("DONE.\n");
    }
  return 0;
}

} // Test
} // Rapicorn
