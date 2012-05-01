// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include <rcore/testutils.hh>

namespace {
using namespace Rapicorn;

static inline uint32
quick_rand32 (void)
{
  static uint32 accu = 2147483563;
  accu = 1664525 * accu + 1013904223;
  return accu;
}

static void
quick_timer_test()
{
  // test ctor, timing, dtor
  {
    QuickTimer tr (0);
    TASSERT (1);
    while (!tr.expired())
      ;
    TASSERT (1);
    while (tr.expired())
      ;
    TASSERT (1);
    while (!tr.expired())
      ;
    TASSERT (1);
  }
  // test real timing
  {
    uint64 i, accu = 0;
    QuickTimer qtimer (20 * 1000); // µseconds
    const uint64 big = ~uint64 (0), benchstart = timestamp_realtime();
    for (i = 0; i < big; i++)
      {
        // iterate over some time consuming stuff
        accu += quick_rand32();
        // poll quick_timer to avoid stalling the user
        if (qtimer.expired())
          break;
      }
    const uint64 benchdone = timestamp_realtime();
    TASSERT (accu > 0);
    TASSERT (i != big);
    if (Test::verbose())
      printout ("QuickTimer: loop performed %zu runs in %zuµs and accumulated: accu=%zu average=%zu\n",
                size_t (i), size_t (benchdone - benchstart), size_t (accu), size_t (accu / i));
  }
}
REGISTER_TEST ("QuickTimer/Test 20usec expiration", quick_timer_test);

} // Anon
