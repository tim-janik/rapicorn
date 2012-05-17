// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
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
  int mycpu = Thread::Self::affinity();
  mycpu = MAX (0, mycpu);
  // fixate the CPU we're running on
  Thread::Self::affinity (mycpu);
  // request CPU for server thread
  StringVector iargs;
  iargs.push_back (string_printf ("cpu-affinity=%d", mycpu));
  // init test application
  init_core_test (argv[0], &argc, argv, iargs);
  ApplicationH app = init_app (argv[0], &argc, argv, iargs);
  const int clockid = CLOCK_REALTIME; // CLOCK_MONOTONIC
  double calls = 0, slowest = 0, fastest = 9e+9;
  for (uint j = 0; j < 29; j++)
    {
      app.test_counter_set (0);
      struct timespec ts0, ts1;
      const int count = 7000;
      clock_gettime (clockid, &ts0);
      for (int i = 0; i < count; i++)
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
