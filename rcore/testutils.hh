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
#ifndef __RAPICORN_TESTUTILS_HH__
#define __RAPICORN_TESTUTILS_HH__

#include <core/rapicorn-core.hh>
#include <core/rapicorntests.h>

namespace Rapicorn {
namespace Test {

#undef TASSERT
#define TASSERT TCHECK

/* test maintenance */
int     run             (void);
bool    verbose         (void);
bool    quick           (void);
bool    slow            (void);
bool    thorough        (void);
bool    perf            (void);


void    add_internal    (const String &testname,
                         void        (*test_func) (void*),
                         void         *data);
void    add             (const String &funcname,
                         void (*test_func) (void));
template<typename D>
void    add             (const String &testname,
                         void        (*test_func) (D*),
                         D            *data)
{
  add_internal (testname, (void(*)(void*)) test_func, (void*) data);
}

/* random numbers */
char    rand_bit                (void);
int32   rand_int                (void);
int32   rand_int_range          (int32          begin,
                                 int32          end);
double  test_rand_double        (void);
double  test_rand_double_range  (double          range_start,
                                 double          range_end);


} // Test
} // Rapicorn

#endif /* __RAPICORN_TESTUTILS_HH__ */
