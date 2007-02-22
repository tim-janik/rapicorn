/* Tests
 * Copyright (C) 2005 Tim Janik
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
#include <birnet/birnettests.h>
#include <rapicorn/rapicorn.hh>
#include <errno.h>

namespace {
using namespace Rapicorn;

inline uint32
quick_rand32 (void)
{
  static uint32 accu = 2147483563;
  accu = 1664525 * accu + 1013904223;
  return accu;
}

/* --- basic loop tests --- */
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
  BIRNET_ASSERT (err == sizeof (buffer));
  return true;
}

static uint pipe_reader_seen = 0;

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
      BIRNET_ASSERT (err == 1);
      BIRNET_ASSERT (counter == data);
      counter++;
    }
  pipe_reader_seen++;
  if (pipe_reader_seen % 997 == 0)
    TICK();
  return true;
}

static void
basic_loop_test()
{
  const uint max_runs = 9999;
  TSTART ("loop");
  /* basal loop tests */
  MainLoop *loop = MainLoop::create();
  TASSERT (loop);
  ref_sink (loop);
  while (loop->pending())
    loop->iteration (false);
  /* oneshot test */
  TASSERT (test_callback_touched == false);
  uint tcid = loop->exec_now (slot (test_callback));
  TASSERT (test_callback_touched == false);
  while (loop->pending())
    loop->iteration (false);
  TASSERT (test_callback_touched == true);
  bool tremove = loop->try_remove (tcid);
  TASSERT (tremove == false);
  test_callback_touched = false;
  /* keep-alive test */
  tcid = loop->exec_now (slot (keep_alive_callback));
  for (uint counter = 0; counter < max_runs; counter++)
    if (loop->pending())
      loop->iteration (false);
    else
      break;
  tremove = loop->try_remove (tcid);
  TASSERT (tremove == true);
  while (loop->pending())
    loop->iteration (false);
  tremove = loop->try_remove (tcid);
  TASSERT (tremove == false);
  /* loop + pipe */
  int pipe_fds[2];
  int err = pipe (pipe_fds);
  TASSERT (err == 0);
  while (loop->pending())
    loop->iteration (false);
  loop->exec_io_handler (slot (pipe_reader), pipe_fds[0], "r");
  loop->exec_io_handler (slot (pipe_writer), pipe_fds[1], "w");
  TASSERT (pipe_reader_seen == 0);
  while (pipe_reader_seen < 49999)
    {
      if (loop->pending())
        loop->iteration (false);
      else
        loop->iteration (true);
    }
  TASSERT_CMP (pipe_reader_seen, ==, 49999);
  err = close (pipe_fds[1]);
  TASSERT (err == 0);
  while (loop->pending())
    loop->iteration (false);
  err = close (pipe_fds[0]);
  TASSERT (err == -1); /* should have been auto-closed by PollFDSource */
  unref (loop);
  TDONE ();
}

/* --- loop state tests --- */
static uint         check_source_counter = 0;
static uint         check_source_destroyed_counter = 0;

class CheckSource : public virtual MainLoop::Source {
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
    BIRNET_ASSERT (m_state == 0);
    m_state = INITIALIZED;
    check_source_counter++;
  }
  virtual bool
  prepare (uint64 current_time_usecs,
           int64 *timeout_usecs_p)
  {
    BIRNET_ASSERT (m_state == INITIALIZED ||
                   m_state == PREPARED ||
                   m_state == CHECKED ||
                   m_state == DISPATCHED);
    m_state = PREPARED;
    if (quick_rand32() & 0xfeedf00d)
      *timeout_usecs_p = quick_rand32() % 5000;
    return quick_rand32() & 0x00400200a;
  }
  virtual bool
  check (uint64 current_time_usecs)
  {
    BIRNET_ASSERT (m_state == INITIALIZED ||
                   m_state == PREPARED);
    m_state = CHECKED;
    return quick_rand32() & 0xc0ffee;
  }
  virtual bool
  dispatch ()
  {
    BIRNET_ASSERT (m_state == PREPARED ||
                   m_state == CHECKED);
    m_state = DISPATCHED;
    return (quick_rand32() % 131) != 0;
  }
  virtual void
  destroy ()
  {
    BIRNET_ASSERT (m_state == INITIALIZED ||
                   m_state == PREPARED ||
                   m_state == CHECKED ||
                   m_state == DISPATCHED);
    m_state = DESTROYED;
    check_source_destroyed_counter++;
  }
  virtual void
  finalize ()
  {
    BIRNET_ASSERT (m_state == DESTROYED);
    // BIRNET_ASSERT (m_state == INITIALIZED || m_state == DESTROYED);
    MainLoop::Source::finalize();
    m_state = FINALIZED;
  }
  virtual
  ~CheckSource ()
  {
    BIRNET_ASSERT (m_state == FINALIZED);
    m_state = DESTRUCTED;
    check_source_counter--;
  }
};

static CheckSource *check_sources[997] = { NULL, };

static void
more_loop_test2()
{
  const uint max_runs = 9999;
  TSTART ("loop-states");
  MainLoop *loop = MainLoop::create();
  TASSERT (loop);
  ref_sink (loop);
  while (loop->pending())
    loop->iteration (false);
  /* source state checks */
  TASSERT (check_source_counter == 0);
  const uint nsrc = quick_rand32() % (1 + ARRAY_SIZE (check_sources));
  for (uint i = 0; i < nsrc; i++)
    {
      check_sources[i] = new CheckSource();
      ref (check_sources[i]);
      loop->add_source (check_sources[i], quick_rand32());
    }
  TASSERT (check_source_counter == nsrc);
  TASSERT (check_source_destroyed_counter == 0);
  for (uint counter = 0; counter < max_runs; counter++)
    {
      if (loop->pending())
        loop->iteration (false);
      else
        break;
      if (counter % 347 == 0)
        TICK();
    }
  TASSERT (check_source_counter == nsrc);
  loop->quit();
  TASSERT_CMP (check_source_destroyed_counter, ==, nsrc); /* checks execution of enough destroy() handlers */
  TASSERT (check_source_counter == nsrc);
  for (uint i = 0; i < nsrc; i++)
    unref (check_sources[i]);
  TASSERT (check_source_counter == 0);
  unref (loop);
  TDONE ();
}

/* --- async loop tests --- */
static bool quit_source_destroyed = false;
class QuitSource : public virtual MainLoop::Source {
  uint      m_counter;
  MainLoop *m_loop;
public:
  QuitSource (uint      countdown,
              MainLoop *loop) :
    m_counter (countdown),
    m_loop (loop)
  {}
  virtual bool
  prepare (uint64 current_time_usecs,
           int64 *timeout_usecs_p)
  {
    return m_counter > 0;
  }
  virtual bool
  check (uint64 current_time_usecs)
  {
    return m_counter > 0;
  }
  virtual bool
  dispatch ()
  {
    if (m_counter)
      {
        m_counter--;
        if (!m_counter && m_loop)
          m_loop->quit();
      }
    else
      ASSERT_NOT_REACHED();
    return true;
  }
  virtual void
  destroy ()
  {}
  virtual void
  finalize ()
  {}
  virtual
  ~QuitSource ()
  {
    quit_source_destroyed = true;
  }
};

static void
async_loop_test()
{
  const uint max_runs = 9999;
  TSTART ("async-loop");
  MainLoop *loop = MainLoop::create();
  TASSERT (loop);
  ref_sink (loop);
  loop->start();
  MainLoop::Source *source = new QuitSource (max_runs, loop);
  TASSERT (quit_source_destroyed == false);
  loop->add_source (source);
  while (!quit_source_destroyed)
    Thread::Self::sleep (25 * 1000);
  TASSERT (quit_source_destroyed == true);
  unref (loop);
  TDONE ();
}

/* --- affine --- */
static void
affine_test()
{
  TSTART ("affine");
  AffineIdentity id;
  Affine a;
  TASSERT (a == id);
  TASSERT (a.determinant() == 1);
  TASSERT (a.is_identity());
  AffineTranslate at (1, 1);
  TASSERT (at.point (3, 5) == Point (4, 6));
  TASSERT (at.ipoint (4, 6) == Point (3, 5));
  TDONE();
}

/* --- double<->int --- */
static void
double_int_test()
{
  TSTART ("dtoi32");
  TASSERT (_dtoi32_generic (0.0) == 0);
  TASSERT (_dtoi32_generic (+0.3) == +0);
  TASSERT (_dtoi32_generic (-0.3) == -0);
  TASSERT (_dtoi32_generic (+0.7) == +1);
  TASSERT (_dtoi32_generic (-0.7) == -1);
  TASSERT (_dtoi32_generic (+2147483646.3) == +2147483646);
  TASSERT (_dtoi32_generic (+2147483646.7) == +2147483647);
  TASSERT (_dtoi32_generic (-2147483646.3) == -2147483646);
  TASSERT (_dtoi32_generic (-2147483646.7) == -2147483647);
  TASSERT (_dtoi32_generic (-2147483647.3) == -2147483647);
  TASSERT (_dtoi32_generic (-2147483647.7) == -2147483648LL);
  TASSERT (dtoi32 (0.0) == 0);
  TASSERT (dtoi32 (+0.3) == +0);
  TASSERT (dtoi32 (-0.3) == -0);
  TASSERT (dtoi32 (+0.7) == +1);
  TASSERT (dtoi32 (-0.7) == -1);
  TASSERT (dtoi32 (+2147483646.3) == +2147483646);
  TASSERT (dtoi32 (+2147483646.7) == +2147483647);
  TASSERT (dtoi32 (-2147483646.3) == -2147483646);
  TASSERT (dtoi32 (-2147483646.7) == -2147483647);
  TASSERT (dtoi32 (-2147483647.3) == -2147483647);
  TASSERT (dtoi32 (-2147483647.7) == -2147483648LL);
  TDONE();
  TSTART ("dtoi64");
  TASSERT (_dtoi64_generic (0.0) == 0);
  TASSERT (_dtoi64_generic (+0.3) == +0);
  TASSERT (_dtoi64_generic (-0.3) == -0);
  TASSERT (_dtoi64_generic (+0.7) == +1);
  TASSERT (_dtoi64_generic (-0.7) == -1);
  TASSERT (_dtoi64_generic (+2147483646.3) == +2147483646);
  TASSERT (_dtoi64_generic (+2147483646.7) == +2147483647);
  TASSERT (_dtoi64_generic (-2147483646.3) == -2147483646);
  TASSERT (_dtoi64_generic (-2147483646.7) == -2147483647);
  TASSERT (_dtoi64_generic (-2147483647.3) == -2147483647);
  TASSERT (_dtoi64_generic (-2147483647.7) == -2147483648LL);
  TASSERT (_dtoi64_generic (+4294967297.3) == +4294967297LL);
  TASSERT (_dtoi64_generic (+4294967297.7) == +4294967298LL);
  TASSERT (_dtoi64_generic (-4294967297.3) == -4294967297LL);
  TASSERT (_dtoi64_generic (-4294967297.7) == -4294967298LL);
  TASSERT (_dtoi64_generic (+1125899906842624.3) == +1125899906842624LL);
  TASSERT (_dtoi64_generic (+1125899906842624.7) == +1125899906842625LL);
  TASSERT (_dtoi64_generic (-1125899906842624.3) == -1125899906842624LL);
  TASSERT (_dtoi64_generic (-1125899906842624.7) == -1125899906842625LL);
  TASSERT (dtoi64 (0.0) == 0);
  TASSERT (dtoi64 (+0.3) == +0);
  TASSERT (dtoi64 (-0.3) == -0);
  TASSERT (dtoi64 (+0.7) == +1);
  TASSERT (dtoi64 (-0.7) == -1);
  TASSERT (dtoi64 (+2147483646.3) == +2147483646);
  TASSERT (dtoi64 (+2147483646.7) == +2147483647);
  TASSERT (dtoi64 (-2147483646.3) == -2147483646);
  TASSERT (dtoi64 (-2147483646.7) == -2147483647);
  TASSERT (dtoi64 (-2147483647.3) == -2147483647);
  TASSERT (dtoi64 (-2147483647.7) == -2147483648LL);
  TASSERT (dtoi64 (+4294967297.3) == +4294967297LL);
  TASSERT (dtoi64 (+4294967297.7) == +4294967298LL);
  TASSERT (dtoi64 (-4294967297.3) == -4294967297LL);
  TASSERT (dtoi64 (-4294967297.7) == -4294967298LL);
  TASSERT (dtoi64 (+1125899906842624.3) == +1125899906842624LL);
  TASSERT (dtoi64 (+1125899906842624.7) == +1125899906842625LL);
  TASSERT (dtoi64 (-1125899906842624.3) == -1125899906842624LL);
  TASSERT (dtoi64 (-1125899906842624.7) == -1125899906842625LL);
  TDONE();
  TSTART ("iround");
  TASSERT (round (0.0) == 0.0);
  TASSERT (round (+0.3) == +0.0);
  TASSERT (round (-0.3) == -0.0);
  TASSERT (round (+0.7) == +1.0);
  TASSERT (round (-0.7) == -1.0);
  TASSERT (round (+4294967297.3) == +4294967297.0);
  TASSERT (round (+4294967297.7) == +4294967298.0);
  TASSERT (round (-4294967297.3) == -4294967297.0);
  TASSERT (round (-4294967297.7) == -4294967298.0);
  TASSERT (iround (0.0) == 0);
  TASSERT (iround (+0.3) == +0);
  TASSERT (iround (-0.3) == -0);
  TASSERT (iround (+0.7) == +1);
  TASSERT (iround (-0.7) == -1);
  TASSERT (iround (+4294967297.3) == +4294967297LL);
  TASSERT (iround (+4294967297.7) == +4294967298LL);
  TASSERT (iround (-4294967297.3) == -4294967297LL);
  TASSERT (iround (-4294967297.7) == -4294967298LL);
  TASSERT (iround (+1125899906842624.3) == +1125899906842624LL);
  TASSERT (iround (+1125899906842624.7) == +1125899906842625LL);
  TASSERT (iround (-1125899906842624.3) == -1125899906842624LL);
  TASSERT (iround (-1125899906842624.7) == -1125899906842625LL);
  TDONE();
  TSTART ("iceil");
  TASSERT (ceil (0.0) == 0.0);
  TASSERT (ceil (+0.3) == +1.0);
  TASSERT (ceil (-0.3) == -0.0);
  TASSERT (ceil (+0.7) == +1.0);
  TASSERT (ceil (-0.7) == -0.0);
  TASSERT (ceil (+4294967297.3) == +4294967298.0);
  TASSERT (ceil (+4294967297.7) == +4294967298.0);
  TASSERT (ceil (-4294967297.3) == -4294967297.0);
  TASSERT (ceil (-4294967297.7) == -4294967297.0);
  TASSERT (iceil (0.0) == 0);
  TASSERT (iceil (+0.3) == +1);
  TASSERT (iceil (-0.3) == -0);
  TASSERT (iceil (+0.7) == +1);
  TASSERT (iceil (-0.7) == -0);
  TASSERT (iceil (+4294967297.3) == +4294967298LL);
  TASSERT (iceil (+4294967297.7) == +4294967298LL);
  TASSERT (iceil (-4294967297.3) == -4294967297LL);
  TASSERT (iceil (-4294967297.7) == -4294967297LL);
  TASSERT (iceil (+1125899906842624.3) == +1125899906842625LL);
  TASSERT (iceil (+1125899906842624.7) == +1125899906842625LL);
  TASSERT (iceil (-1125899906842624.3) == -1125899906842624LL);
  TASSERT (iceil (-1125899906842624.7) == -1125899906842624LL);
  TDONE();
  TSTART ("ifloor");
  TASSERT (floor (0.0) == 0.0);
  TASSERT (floor (+0.3) == +0.0);
  TASSERT (floor (-0.3) == -1.0);
  TASSERT (floor (+0.7) == +0.0);
  TASSERT (floor (-0.7) == -1.0);
  TASSERT (floor (+4294967297.3) == +4294967297.0);
  TASSERT (floor (+4294967297.7) == +4294967297.0);
  TASSERT (floor (-4294967297.3) == -4294967298.0);
  TASSERT (floor (-4294967297.7) == -4294967298.0);
  TASSERT (ifloor (0.0) == 0);
  TASSERT (ifloor (+0.3) == +0);
  TASSERT (ifloor (-0.3) == -1);
  TASSERT (ifloor (+0.7) == +0);
  TASSERT (ifloor (-0.7) == -1);
  TASSERT (ifloor (+4294967297.3) == +4294967297LL);
  TASSERT (ifloor (+4294967297.7) == +4294967297LL);
  TASSERT (ifloor (-4294967297.3) == -4294967298LL);
  TASSERT (ifloor (-4294967297.7) == -4294967298LL);
  TASSERT (ifloor (+1125899906842624.3) == +1125899906842624LL);
  TASSERT (ifloor (+1125899906842624.7) == +1125899906842624LL);
  TASSERT (ifloor (-1125899906842624.3) == -1125899906842625LL);
  TASSERT (ifloor (-1125899906842624.7) == -1125899906842625LL);
  TDONE();
}

/* --- hsv --- */
static void
color_test()
{
  /* assert that set_hsv(get_hsv(v))==v for all v */
  TSTART ("HSV-convert");
  const int step = init_settings().test_slow ? 1 : 10;
  for (uint r = 0; r < 256; r += step)
    {
      for (uint g = 0; g < 256; g += step)
        for (uint b = 0; b < 256; b += step)
          {
            Color c (r, g, b, 0xff);
            double hue, saturation, value;
            c.get_hsv (&hue, &saturation, &value);
            if (r > g && r > b)
              BIRNET_ASSERT (hue < 60 || hue > 300);
            else if (g > r && g > b)
              BIRNET_ASSERT (hue > 60 || hue < 180);
            else if (b > g && b > r)
              BIRNET_ASSERT (hue > 180 || hue < 300);
            Color d (0xff75c3a9);
            d.set_hsv (hue, saturation, value);
            if (c.red()   != d.red() ||
                c.green() != d.green() ||
                c.blue()  != d.blue() ||
                c.alpha() != d.alpha())
              error ("color difference after hsv: %s != %s (hue=%f saturation=%f value=%f)\n",
                     c.string().c_str(), d.string().c_str(), hue, saturation, value);
          }
      if (r % 5 == 0)
        TICK();
    }
  TDONE();
}

/* --- main --- */
extern "C" int
main (int   argc,
      char *argv[])
{
  birnet_init_test (&argc, &argv);

  basic_loop_test();
  more_loop_test2();
  async_loop_test();
  affine_test();
  double_int_test();
  color_test();
  return 0;
}

} // anon
