// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
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
  MainLoop *loop = MainLoop::_new();
  TASSERT (loop);
  ref_sink (loop);
  /* oneshot test */
  TASSERT (test_callback_touched == false);
  uint tcid = loop->exec_now (slot (test_callback));
  TASSERT (tcid > 0);
  TASSERT (test_callback_touched == false);
  loop->iterate_pending();
  TASSERT (test_callback_touched == true);
  bool tremove = loop->try_remove (tcid);
  TASSERT (tremove == false);
  test_callback_touched = false;
  /* keep-alive test */
  tcid = loop->exec_now (slot (keep_alive_callback));
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
  loop->exec_io_handler (slot (pipe_reader), pipe_fds[0], "r");
  loop->exec_io_handler (slot (pipe_writer), pipe_fds[1], "w");
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
  unref (loop);
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

class CheckSource : public virtual EventLoop::Source {
  enum {
    INITIALIZED = 1,
    PREPARED,
    CHECKED,
    DISPATCHED,
    DESTROYED,
    FINALIZED,
    DESTRUCTED
  };
  uint          m_state;
public:
  CheckSource () :
    m_state (0)
  {
    RAPICORN_ASSERT (m_state == 0);
    m_state = INITIALIZED;
    check_source_counter++;
  }
  virtual bool
  prepare (const EventLoop::State &state,
           int64 *timeout_usecs_p)
  {
    RAPICORN_ASSERT (m_state == INITIALIZED ||
                   m_state == PREPARED ||
                   m_state == CHECKED ||
                   m_state == DISPATCHED);
    m_state = PREPARED;
    if (quick_rand32() & 0xfeedf00d)
      *timeout_usecs_p = quick_rand32() % 5000;
    return quick_rand32() & 0x00400200a;
  }
  virtual bool
  check (const EventLoop::State &state)
  {
    RAPICORN_ASSERT (m_state == INITIALIZED ||
                   m_state == PREPARED);
    m_state = CHECKED;
    return quick_rand32() & 0xc0ffee;
  }
  virtual bool
  dispatch (const EventLoop::State &state)
  {
    RAPICORN_ASSERT (m_state == PREPARED ||
                   m_state == CHECKED);
    m_state = DISPATCHED;
    return (quick_rand32() % 131) != 0;
  }
  virtual void
  destroy ()
  {
    RAPICORN_ASSERT (m_state == INITIALIZED ||
                   m_state == PREPARED ||
                   m_state == CHECKED ||
                   m_state == DISPATCHED);
    m_state = DESTROYED;
    check_source_destroyed_counter++;
  }
  virtual void
  finalize ()
  {
    RAPICORN_ASSERT (m_state == DESTROYED);
    // RAPICORN_ASSERT (m_state == INITIALIZED || m_state == DESTROYED);
    EventLoop::Source::finalize();
    m_state = FINALIZED;
  }
  virtual
  ~CheckSource ()
  {
    RAPICORN_ASSERT (m_state == FINALIZED);
    m_state = DESTRUCTED;
    check_source_counter--;
  }
};

static CheckSource *check_sources[997] = { NULL, };

static void
test_event_loop_sources()
{
  const uint max_runs = 999;
  MainLoop *loop = MainLoop::_new();
  TASSERT (loop);
  ref_sink (loop);
  loop->iterate_pending();
  /* source state checks */
  TASSERT (check_source_counter == 0);
  const uint nsrc = quick_rand32() % (1 + ARRAY_SIZE (check_sources));
  for (uint i = 0; i < nsrc; i++)
    {
      check_sources[i] = new CheckSource();
      ref (check_sources[i]);
      loop->add (check_sources[i], quick_rand32());
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
  loop->kill_sources();
  TCMP (check_source_destroyed_counter, ==, nsrc); /* checks execution of enough destroy() handlers */
  TASSERT (check_source_counter == nsrc);
  for (uint i = 0; i < nsrc; i++)
    unref (check_sources[i]);
  TASSERT (check_source_counter == 0);
  unref (loop);
}
REGISTER_TEST ("Loops/Test Event Sources", test_event_loop_sources);

static bool round_robin_increment (uint *loc) { (*loc)++; return true; }
static uint round_robin_1 = 0, round_robin_2 = 0;
static void
test_loop_round_robin (void)
{
  const uint rungroup = 977;
  MainLoop *loop = MainLoop::_new();
  TASSERT (loop);
  ref_sink (loop);
  for (uint i = 0; i < 77; i++)
    loop->iterate (false);
  /* We're roughly checking round-robin execution behaviour, by checking if
   * two concurrently running handlers are both executed. If one starves,
   * we'll catch that.
   */
  TASSERT (round_robin_1 == 0 && round_robin_2 == 0);
  loop->exec_now (slot (round_robin_increment, &round_robin_1));
  loop->exec_now (slot (round_robin_increment, &round_robin_2));
  /* We make an educated guess at loop iterations needed for two handlers
   * to execute >= rungroup times. No correlation is guaranteed here, but
   * we guess that any count in significant excess of 2 * rungroup should
   * suffice.
   */
  const uint rungroup_for_two = 2 * rungroup + 2;
  for (uint i = 0; i < rungroup_for_two; i++)
    loop->iterate (false);
  TASSERT (round_robin_1 >= rungroup && round_robin_2 >= rungroup);
  loop->kill_sources();
  // we should be able to repeat the check
  loop->exec_background (slot (round_robin_increment, &round_robin_1));
  loop->exec_background (slot (round_robin_increment, &round_robin_2));
  round_robin_1 = round_robin_2 = 0;
  TASSERT (round_robin_1 == 0 && round_robin_2 == 0);
  for (uint i = 0; i < rungroup_for_two; i++)
    loop->iterate (false);
  TASSERT (round_robin_1 >= rungroup && round_robin_2 >= rungroup);
  loop->kill_sources();
  // cross-check, intentionally cause starvation of one handler
  loop->exec_background (slot (round_robin_increment, &round_robin_1));
  loop->exec_now (slot (round_robin_increment, &round_robin_2));
  round_robin_1 = round_robin_2 = 0;
  TASSERT (round_robin_1 == 0 && round_robin_2 == 0);
  for (uint i = 0; i < rungroup_for_two; i++)
    loop->iterate (false);
  TASSERT (round_robin_1 < rungroup && round_robin_2 >= rungroup);
  loop->kill_sources();
  // check round-robin for loops
  EventLoop *dummy1 = loop->new_slave();
  EventLoop *slave = loop->new_slave();
  EventLoop *dummy2 = loop->new_slave();
  ref_sink (slave);
  ref_sink (dummy1);
  ref_sink (dummy2);
  round_robin_1 = round_robin_2 = 0;
  TASSERT (round_robin_1 == 0 && round_robin_2 == 0);
  loop->exec_background (slot (round_robin_increment, &round_robin_1));
  slave->exec_normal (slot (round_robin_increment, &round_robin_2));
  for (uint i = 0; i < rungroup_for_two; i++)
    loop->iterate (false);
  TASSERT (round_robin_1 >= rungroup && round_robin_2 >= rungroup);
  loop->kill_sources();
  slave->kill_sources();
  unref (loop);
  unref (slave);
  unref (dummy1);
  unref (dummy2);
}
REGISTER_TEST ("Loops/Test Round Robin Looping", test_loop_round_robin);

static String loop_breadcrumbs = "";
static MainLoop *breadcrumb_loop = NULL;
static void handler_d();
static void handler_a() { loop_breadcrumbs += "a"; TASSERT (loop_breadcrumbs == "a"); }
static void handler_b()
{
  loop_breadcrumbs += "b"; TASSERT (loop_breadcrumbs == "ab");
  breadcrumb_loop->exec_now (slot (handler_d));
}
static void handler_c() { loop_breadcrumbs += "c"; TASSERT (loop_breadcrumbs == "abDc"); }
static void handler_d() { loop_breadcrumbs += "D"; TASSERT (loop_breadcrumbs == "abD"); }
static void
test_loop_priorities (void)
{
  TASSERT (!breadcrumb_loop);
  breadcrumb_loop = MainLoop::_new();
  ref_sink (breadcrumb_loop);
  for (uint i = 0; i < 7; i++)
    breadcrumb_loop->iterate (false);
  breadcrumb_loop->exec_normal (slot (handler_a));
  breadcrumb_loop->exec_normal (slot (handler_b));
  breadcrumb_loop->exec_normal (slot (handler_c));
  breadcrumb_loop->iterate_pending();
  TASSERT (loop_breadcrumbs == "abDc");
  breadcrumb_loop->kill_sources();
  unref (breadcrumb_loop);
  breadcrumb_loop = NULL;
}
REGISTER_TEST ("Loops/Test Loop Priorities", test_loop_priorities);

} // anon
