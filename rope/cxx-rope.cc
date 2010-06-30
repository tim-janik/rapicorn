/* Rapicorn C++ Remote Object Programming Extension
 * Copyright (C) 2010 Tim Janik
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
#include "cxx-rope.hh"
using namespace Rapicorn;
#include "cxx-client.hh"        // generated client API
#include <ui/rope.hh>
#include <stdexcept>

namespace { // Anonymous

static Application
cxxrope_init_dispatcher (const String         &appname,
                         const vector<String> &cmdline_args)
{
  int cpu = Thread::Self::affinity (1);
  uint64 app_id = rope_thread_start (appname, cmdline_args, cpu);
  if (app_id == 0)
    throw std::runtime_error ("failed to initialize rapicorn thread");
  // printout ("APPURL: 0x%016llx\n", app_id);
  /* minor hack to create the initial smart handler for an Application */
  Plic::FieldBuffer8 fb (4);
  fb.add_object (uint64 (app_id));
  Plic::FieldBufferReader fbr (fb);
  Application app (*rope_thread_coupler(), fbr);
  return app;
}

} // Anon

int
main (int   argc,
      char *argv[])
{
  const int clockid = CLOCK_REALTIME; // CLOCK_MONOTONIC
  RapicornInitValue ivalues[] = { { NULL } };
  rapicorn_init_core (&argc, &argv, NULL, ivalues);
  vector<String> cmdline_args;
  for (int i = 1; i < argc; i++)
    cmdline_args.push_back (argv[i]);
  Application app = cxxrope_init_dispatcher (argv[0], cmdline_args);
  double calls = 0, slowest = 0, fastest = 9e+9;
  for (uint j = 0; j < 29; j++)
    {
      app.test_counter_set (0);
      struct timespec ts0, ts1;
      const uint count = 7000;
      clock_gettime (clockid, &ts0);
      for (uint i = 0; i < count; i++)
        app.test_counter_inc_fetch ();
      clock_gettime (clockid, &ts1);
      assert (app.test_counter_get() == count);
      double t0 = ts0.tv_sec + ts0.tv_nsec / 1000000000.;
      double t1 = ts1.tv_sec + ts1.tv_nsec / 1000000000.;
      double call1 = (t1 - t0) / count;
      slowest = MAX (slowest, call1 * 1000000.);
      fastest = MIN (fastest, call1 * 1000000.);
      double this_calls = 1 / call1;
      calls = MAX (calls, this_calls);
    }
  double err = (slowest - fastest) / slowest;
  printout ("2way: best: %g calls/s; fastest: %.2fus; slowest: %.2fus; err: %.2f%%\n",
            calls, fastest, slowest, err * 100);
  return 0;
}

// generated client implementation
#define PLIC_COUPLER() (*rope_thread_coupler())
#include "cxx-client.cc"
