// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include <rapicorn-core.hh>
using namespace Rapicorn;
#include <ui/clientapi.hh>      // generated client API
#include <rcore/testutils.hh>
#include <stdexcept>

int
main (int   argc,
      char *argv[])
{
  // find out which CPU we run on
  int mycpu = ThisThread::affinity();
  mycpu = MAX (0, mycpu);
  // fixate the CPU we're running on
  ThisThread::affinity (mycpu);
  // request CPU for server thread
  StringVector iargs;
  iargs.push_back (string_format ("cpu-affinity=%d", mycpu));
  // init application
  ApplicationH app = init_app (argv[0], &argc, argv, iargs);
  double calls = 0, slowest = 0, fastest = 9e+9;
  for (uint j = 0; j < 97; j++)
    {
      app.test_counter_set (0);
      const int count = 7000;
      const uint64 ts0 = timestamp_benchmark();
      for (int i = 0; i < count; i++)
        app.test_counter_inc_fetch ();
      const uint64 ts1 = timestamp_benchmark();
      assert (app.test_counter_get() == count);
      double t0 = ts0 / 1000000000.;
      double t1 = ts1 / 1000000000.;
      double call1 = (t1 - t0) / count;
      slowest = MAX (slowest, call1 * 1000000.);
      fastest = MIN (fastest, call1 * 1000000.);
      double this_calls = 1 / call1;
      calls = MAX (calls, this_calls);
    }
  double err = (slowest - fastest) / slowest;
  printout ("  BENCH    Aida: %g calls/s; fastest: %.2fus; slowest: %.2fus; err: %.2f%%\n",
            calls, fastest, slowest, err * 100);
  app.shutdown();
  return 0;
}
