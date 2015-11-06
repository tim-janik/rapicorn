// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include <rcore/testutils.hh>
#include <errno.h>
#include <unistd.h>

namespace {
using namespace Rapicorn;

// === test_loop_basics ===
static bool test_callback_touched = false;
static void
test_callback()
{
  test_callback_touched = true;
}

static bool
keep_alive_callback()
{
  return true;
}

static bool
pipe_writer (PollFD &pfd)
{
  uint8 buffer[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17 };
  int err;
  do
    err = write (pfd.fd, buffer, sizeof (buffer));
  while (err < 0 && (errno == EINTR || errno == EAGAIN));
  /* we assert error-free IO operation, because loop source prepare/poll/check
   * must make sure the file descriptor really is valid and writable
   */
  TASSERT (err == sizeof (buffer));
  return true;
}

static int pipe_reader_seen = 0;

static bool
pipe_reader (PollFD &pfd)
{
  int counter = 1;
  while (counter <= 17)
    {
      uint8 data;
      int err;
      do
        err = read (pfd.fd, &data, 1);
      while (err < 0 && (errno == EINTR || errno == EAGAIN));
      /* we assert error-free IO operation, because loop source prepare/poll/check
       * must make sure the file descriptor really is valid and readable
       */
      TASSERT (err == 1);
      TASSERT (counter == data);
      counter++;
    }
  pipe_reader_seen++;
  if (pipe_reader_seen % 997 == 0)
    TOK();
  return true;
}

static void
test_loop_basics()
{
  const uint max_runs = 1999;
  /* basal loop tests */
  MainLoopP loop = MainLoop::create();
  TASSERT (loop);
  /* oneshot test */
  TASSERT (test_callback_touched == false);
  uint tcid = loop->exec_callback (test_callback, EventLoop::PRIORITY_NEXT);
  TASSERT (tcid > 0);
  TASSERT (test_callback_touched == false);
  loop->iterate_pending();
  TASSERT (test_callback_touched == true);
  bool tremove = loop->try_remove (tcid);
  TASSERT (tremove == false);
  test_callback_touched = false;
  /* keep-alive test */
  tcid = loop->exec_callback (keep_alive_callback, EventLoop::PRIORITY_NEXT);
  for (uint counter = 0; counter < max_runs; counter++)
    if (!loop->iterate (false))
      break;
  tremove = loop->try_remove (tcid);
  TASSERT (tremove == true);
  loop->iterate_pending();
  tremove = loop->try_remove (tcid);
  TASSERT (tremove == false);
  /* loop + pipe */
  int pipe_fds[2];
  int err = pipe (pipe_fds);
  TASSERT (err == 0);
  loop->iterate_pending();
  loop->exec_io_handler (pipe_reader, pipe_fds[0], "r");
  loop->exec_io_handler (pipe_writer, pipe_fds[1], "w");
  TASSERT (pipe_reader_seen == 0);
  while (pipe_reader_seen < 4999)
    loop->iterate (true);
  TCMP (pipe_reader_seen, ==, 4999);
  err = close (pipe_fds[1]);    // pipe becomes unusable
  TASSERT (err == 0);
  loop->iterate_pending();      // PollFDSource prepare/check must auto-close fds here
  err = close (pipe_fds[0]);
  TASSERT (err == -1);          // fd should have already been auto-closed by PollFDSource
  loop->iterate_pending();
}
REGISTER_TEST ("Loops/Test Basics", test_loop_basics);

// === test_event_loop_sources ===
static uint         check_source_counter = 0;
static uint         check_source_destroyed_counter = 0;

inline uint32
quick_rand32 (void)
{
  static uint32 accu = 2147483563;
  accu = 1664525 * accu + 1013904223;
  return accu;
}

class CheckSource;
typedef std::shared_ptr<CheckSource> CheckSourceP;

class CheckSource : public virtual EventSource {
  enum {
    INITIALIZED = 1,
    PREPARED,
    CHECKED,
    DISPATCHED,
    DESTROYED,
    DESTRUCTED
  };
  uint          state_;
  CheckSource () :
    state_ (0)
  {
    RAPICORN_ASSERT (state_ == 0);
    state_ = INITIALIZED;
    check_source_counter++;
  }
  virtual bool
  prepare (const LoopState &state, int64 *timeout_usecs_p)
  {
    RAPICORN_ASSERT (state_ == INITIALIZED ||
                   state_ == PREPARED ||
                   state_ == CHECKED ||
                   state_ == DISPATCHED);
    state_ = PREPARED;
    if (quick_rand32() & 0xfeedf00d)
      *timeout_usecs_p = quick_rand32() % 5000;
    return quick_rand32() & 0x00400200a;
  }
  virtual bool
  check (const LoopState &state)
  {
    RAPICORN_ASSERT (state_ == INITIALIZED ||
                   state_ == PREPARED);
    state_ = CHECKED;
    return quick_rand32() & 0xc0ffee;
  }
  virtual bool
  dispatch (const LoopState &state)
  {
    RAPICORN_ASSERT (state_ == PREPARED ||
                   state_ == CHECKED);
    state_ = DISPATCHED;
    return (quick_rand32() % 131) != 0;
  }
  virtual void
  destroy ()
  {
    RAPICORN_ASSERT (state_ == INITIALIZED ||
                   state_ == PREPARED ||
                   state_ == CHECKED ||
                   state_ == DISPATCHED);
    state_ = DESTROYED;
    check_source_destroyed_counter++;
  }
  virtual
  ~CheckSource ()
  {
    RAPICORN_ASSERT (state_ == DESTROYED);
    state_ = DESTRUCTED;
    check_source_counter--;
  }
  friend class FriendAllocator<CheckSource>;
public:
  static CheckSourceP create ()
  { return FriendAllocator<CheckSource>::make_shared(); }
};

static CheckSourceP check_sources[997] = { NULL, };

static void
test_event_loop_sources()
{
  const uint max_runs = 999;
  MainLoopP loop = MainLoop::create();
  TASSERT (loop);
  loop->iterate_pending();
  /* source state checks */
  TASSERT (check_source_counter == 0);
  const uint nsrc = quick_rand32() % (1 + ARRAY_SIZE (check_sources));
  for (uint i = 0; i < nsrc; i++)
    {
      check_sources[i] = CheckSource::create();
      loop->add (check_sources[i], 1 + quick_rand32() % 999);
    }
  TASSERT (check_source_counter == nsrc);
  TASSERT (check_source_destroyed_counter == 0);
  for (uint counter = 0; counter < max_runs; counter++)
    {
      if (!loop->iterate (false))
        break;
      if (counter % 347 == 0)
        TOK();
    }
  TASSERT (check_source_counter == nsrc);
  loop->destroy_loop();
  TCMP (check_source_destroyed_counter, ==, nsrc); /* checks execution of enough destroy() handlers */
  TASSERT (check_source_counter == nsrc);
  for (uint i = 0; i < nsrc; i++)
    check_sources[i] = NULL;
  TASSERT (check_source_counter == 0);
}
REGISTER_TEST ("Loops/Test Event Sources", test_event_loop_sources);

static bool round_robin_increment (uint *loc) { (*loc)++; return true; }
static uint round_robin_1 = 0, round_robin_2 = 0, round_robin_3 = 0;
static void
test_loop_round_robin (void)
{
  uint *const round_robin_1p = &round_robin_1, *const round_robin_2p = &round_robin_2, *const round_robin_3p = &round_robin_3;
  const EventLoop::BoolSlot increment_round_robin_1 = [round_robin_1p] () { return round_robin_increment (round_robin_1p); };
  const EventLoop::BoolSlot increment_round_robin_2 = [round_robin_2p] () { return round_robin_increment (round_robin_2p); };
  const EventLoop::BoolSlot increment_round_robin_3 = [round_robin_3p] () { return round_robin_increment (round_robin_3p); };
  const uint rungroup = 977;
  MainLoopP loop = MainLoop::create();
  TASSERT (loop);
  for (uint i = 0; i < 77; i++)
    loop->iterate (false);
  uint id1, id2, id3;
  /* We're roughly checking round-robin execution behaviour, by checking if
   * some concurrently running handlers are all executed. If one starves,
   * we'll catch that.
   */
  TASSERT (round_robin_1 == 0 && round_robin_2 == 0 && round_robin_3 == 0);
  id1 = loop->exec_callback (increment_round_robin_1, EventLoop::PRIORITY_NEXT);
  id2 = loop->exec_callback (increment_round_robin_2, EventLoop::PRIORITY_NEXT);
  id3 = loop->exec_callback (increment_round_robin_3, EventLoop::PRIORITY_NEXT);
  /* We make an educated guess at loop iterations needed for two handlers
   * to execute >= rungroup times. No correlation is guaranteed here, but
   * we guess that any count in significant excess of n_handlers * rungroup
   * should suffice.
   */
  const uint rungroup_for_all = 3 * rungroup + 3;
  for (uint i = 0; i < rungroup_for_all; i++)
    loop->iterate (false);
  TASSERT (round_robin_1 >= rungroup);
  TASSERT (round_robin_2 >= rungroup);
  TASSERT (round_robin_3 >= rungroup);
  loop->remove (id1);
  loop->remove (id2);
  loop->remove (id3);
  // we should be able to repeat the check
  id1 = loop->exec_idle (increment_round_robin_1);
  id2 = loop->exec_idle (increment_round_robin_2);
  id3 = loop->exec_idle (increment_round_robin_3);
  round_robin_1 = round_robin_2 = round_robin_3 = 0;
  TASSERT (round_robin_1 == 0 && round_robin_2 == 0 && round_robin_3 == 0);
  for (uint i = 0; i < rungroup_for_all; i++)
    loop->iterate (false);
  TASSERT (round_robin_1 >= rungroup && round_robin_2 >= rungroup && round_robin_3 >= rungroup);
  loop->remove (id1);
  loop->remove (id2);
  loop->remove (id3);
  // cross-check, intentionally cause starvation of one handler
  id1 = loop->exec_idle (increment_round_robin_1);
  id2 = loop->exec_callback (increment_round_robin_2, EventLoop::PRIORITY_NEXT);
  round_robin_1 = round_robin_2 = 0;
  TASSERT (round_robin_1 == 0 && round_robin_2 == 0);
  const uint rungroup_for_two = 2 * rungroup + 2;
  for (uint i = 0; i < rungroup_for_two; i++)
    loop->iterate (false);
  TASSERT (round_robin_1 < rungroup && round_robin_2 >= rungroup);
  loop->remove (id1);
  loop->remove (id2);
  // check round-robin for loops
  EventLoopP slave = loop->create_slave();
  round_robin_1 = round_robin_2 = 0;
  TASSERT (round_robin_1 == 0 && round_robin_2 == 0);
  id1 = loop->exec_callback (increment_round_robin_1);
  id2 = slave->exec_idle (increment_round_robin_2);
  for (uint i = 0; i < rungroup_for_two; i++)
    loop->iterate (false);
  TASSERT (round_robin_1 >= rungroup && round_robin_2 >= rungroup);
  // cleanup
  loop->remove (id1);
  slave->remove (id2);
  slave->destroy_loop();
  // check round-robin starvation between loops due to PRIORITY_ASCENT and higher
  slave = loop->create_slave();
  round_robin_1 = round_robin_2 = 0;
  TASSERT (round_robin_1 == 0 && round_robin_2 == 0);
  id1 = loop->exec_callback (increment_round_robin_1);
  id2 = slave->exec_now (increment_round_robin_2); // PRIORITY_NOW starves other sources *and* loops
  for (uint i = 0; i < rungroup_for_two; i++)
    loop->iterate (false);
  TASSERT (round_robin_1 < rungroup && round_robin_2 >= rungroup);
  // cleanup
  loop->remove (id1);
  slave->remove (id2);
  slave->destroy_loop();
  // MainLoop::destroy_loop also cleans up all slaves and their sources
  loop->destroy_loop();
}
REGISTER_TEST ("Loops/Test Round Robin Looping", test_loop_round_robin);

static String loop_breadcrumbs = "";
static MainLoopP breadcrumb_loop = NULL;
static void handler_d();
static void handler_a() { loop_breadcrumbs += "a"; TASSERT (loop_breadcrumbs == "a"); }
static void handler_b()
{
  loop_breadcrumbs += "b"; TASSERT (loop_breadcrumbs == "ab");
  breadcrumb_loop->exec_callback (handler_d, EventLoop::PRIORITY_NEXT);
}
static void handler_c() { loop_breadcrumbs += "c"; TASSERT (loop_breadcrumbs == "abDc"); }
static void handler_d() { loop_breadcrumbs += "D"; TASSERT (loop_breadcrumbs == "abD"); }
static void
test_loop_priorities (void)
{
  TASSERT (!breadcrumb_loop);
  breadcrumb_loop = MainLoop::create();
  for (uint i = 0; i < 7; i++)
    breadcrumb_loop->iterate (false);
  breadcrumb_loop->exec_callback (handler_a);
  breadcrumb_loop->exec_callback (handler_b);
  breadcrumb_loop->exec_callback (handler_c);
  breadcrumb_loop->iterate_pending();
  TASSERT (loop_breadcrumbs == "abDc");
  breadcrumb_loop->destroy_loop();
  breadcrumb_loop = NULL;
}
REGISTER_TEST ("Loops/Test Loop Priorities", test_loop_priorities);

static void
test_outwards_indexing()
{
  String r;
  // outwards_index left-bias
  r = "";
  for (size_t max = 0, i = 0; i < max; i++)
    r += string_format (" %u", outwards_index (i, max, false));
  TCMP (r, ==, "");
  r = "";
  for (size_t max = 1, i = 0; i < max; i++)
    r += string_format (" %u", outwards_index (i, max, false));
  TCMP (r, ==, " 0");
  r = "";
  for (size_t max = 2, i = 0; i < max; i++)
    r += string_format (" %u", outwards_index (i, max, false));
  TCMP (r, ==, " 1 0");
  r = "";
  for (size_t max = 3, i = 0; i < max; i++)
    r += string_format (" %u", outwards_index (i, max, false));
  TCMP (r, ==, " 1 0 2");
  r = "";
  for (size_t max = 4, i = 0; i < max; i++)
    r += string_format (" %u", outwards_index (i, max, false));
  TCMP (r, ==, " 2 1 3 0");
  r = "";
  for (size_t max = 5, i = 0; i < max; i++)
    r += string_format (" %u", outwards_index (i, max, false));
  TCMP (r, ==, " 2 1 3 0 4");
  r = "";
  for (size_t max = 6, i = 0; i < max; i++)
    r += string_format (" %u", outwards_index (i, max, false));
  TCMP (r, ==, " 3 2 4 1 5 0");
  r = "";
  for (size_t max = 7, i = 0; i < max; i++)
    r += string_format (" %u", outwards_index (i, max, false));
  TCMP (r, ==, " 3 2 4 1 5 0 6");
  r = "";
  for (size_t max = 8, i = 0; i < max; i++)
    r += string_format (" %u", outwards_index (i, max, false));
  TCMP (r, ==, " 4 3 5 2 6 1 7 0");
  r = "";
  for (size_t max = 9, i = 0; i < max; i++)
    r += string_format (" %u", outwards_index (i, max, false));
  TCMP (r, ==, " 4 3 5 2 6 1 7 0 8");
  // outwards_index right-bias
  r = "";
  for (size_t max = 0, i = 0; i < max; i++)
    r += string_format (" %u", outwards_index (i, max, true));
  TCMP (r, ==, "");
  r = "";
  for (size_t max = 1, i = 0; i < max; i++)
    r += string_format (" %u", outwards_index (i, max, true));
  TCMP (r, ==, " 0");
  r = "";
  for (size_t max = 2, i = 0; i < max; i++)
    r += string_format (" %u", outwards_index (i, max, true));
  TCMP (r, ==, " 1 0");
  r = "";
  for (size_t max = 3, i = 0; i < max; i++)
    r += string_format (" %u", outwards_index (i, max, true));
  TCMP (r, ==, " 1 2 0");
  r = "";
  for (size_t max = 4, i = 0; i < max; i++)
    r += string_format (" %u", outwards_index (i, max, true));
  TCMP (r, ==, " 2 3 1 0");
  r = "";
  for (size_t max = 5, i = 0; i < max; i++)
    r += string_format (" %u", outwards_index (i, max, true));
  TCMP (r, ==, " 2 3 1 4 0");
  r = "";
  for (size_t max = 6, i = 0; i < max; i++)
    r += string_format (" %u", outwards_index (i, max, true));
  TCMP (r, ==, " 3 4 2 5 1 0");
  r = "";
  for (size_t max = 7, i = 0; i < max; i++)
    r += string_format (" %u", outwards_index (i, max, true));
  TCMP (r, ==, " 3 4 2 5 1 6 0");
  r = "";
  for (size_t max = 8, i = 0; i < max; i++)
    r += string_format (" %u", outwards_index (i, max, true));
  TCMP (r, ==, " 4 5 3 6 2 7 1 0");
  r = "";
  for (size_t max = 9, i = 0; i < max; i++)
    r += string_format (" %u", outwards_index (i, max, true));
  TCMP (r, ==, " 4 5 3 6 2 7 1 8 0");
}
REGISTER_TEST ("Loops/Outwards Indexing", test_outwards_indexing);

} // anon
