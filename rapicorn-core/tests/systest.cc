/* Rapicorn
 * Copyright (C) 2007 Tim Janik
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
#include <rapicorn-core/rapicorntests.h>
#include <poll.h>

namespace {
using namespace Rapicorn;

static void
test_poll()
{
  TSTART("POLL constants");
  TASSERT (RAPICORN_SYSVAL_POLLIN     == POLLIN);
  TASSERT (RAPICORN_SYSVAL_POLLPRI    == POLLPRI);
  TASSERT (RAPICORN_SYSVAL_POLLOUT    == POLLOUT);
  TASSERT (RAPICORN_SYSVAL_POLLRDNORM == POLLRDNORM);
  TASSERT (RAPICORN_SYSVAL_POLLRDBAND == POLLRDBAND);
  TASSERT (RAPICORN_SYSVAL_POLLWRNORM == POLLWRNORM);
  TASSERT (RAPICORN_SYSVAL_POLLWRBAND == POLLWRBAND);
  TASSERT (RAPICORN_SYSVAL_POLLERR    == POLLERR);
  TASSERT (RAPICORN_SYSVAL_POLLHUP    == POLLHUP);
  TASSERT (RAPICORN_SYSVAL_POLLNVAL   == POLLNVAL);
  TDONE();
}

} // Anon

int
main (int   argc,
      char *argv[])
{
  rapicorn_init_test (&argc, &argv);

  test_poll();

  return 0;
}

/* vim:set ts=8 sts=2 sw=2: */
