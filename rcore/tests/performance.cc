// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "configure.h"  // needed for g++-4.7 sleep_for/yield workaround
#include <rcore/testutils.hh>
#include <string.h>

namespace {
using namespace Rapicorn;

static const char *arg = NULL;
static uint        result;

static RAPICORN_NOINLINE void
count_whitespace_switch ()
{
  const char *c = arg;
  uint n = 0;
  while (RAPICORN_ISLIKELY (*c))
    {
      switch (*c)
        {
        case ' ': case '\t': case '\n': case '\r': case '\v': case '\f':
          n += 1;
        }
      c++;
    }
  result = n;
}

static RAPICORN_NOINLINE void
count_whitespace_strchr ()
{
  const char *c = arg;
  uint n = 0;
  while (RAPICORN_ISLIKELY (*c))
    {
      if (strchr (" \t\n\r\v\f", *c) != NULL) // memchr (" \t\n\r\v\f", *c, 6)
        n += 1;
      c++;
    }
  result = n;
}

static void
perf_skip_whitespace (void)
{
  ThisThread::yield(); // volountarily giveup time slice, so we last longer during the benchmark
  Test::Timer timer;
  arg = "Hello World,\nhere is text.\fAnd more \v and \t \t more.\n0\r1\n";
  result = 0;
  double switch_time = timer.benchmark (count_whitespace_switch);
  //uint64 hn = timer.n_runs();
  assert (result == 17);
  result = 0;
  double strchr_time = timer.benchmark (count_whitespace_strchr);
  //uint64 sn = timer.n_runs();
  assert (result == 17);
  TPASS ("count_whitespace benchmark # timing: switch=%fµs strchr=%fµs\n",
         switch_time * 1000000.0, strchr_time * 1000000.0);
  if (switch_time > strchr_time)
    TTODO ("count_whitespace benchmark: unexpected contest: switch=%gs strchr=%gs\n", switch_time, strchr_time);
}
REGISTER_TEST ("Performance/Whitespace Skipping", perf_skip_whitespace);

} // Anon
