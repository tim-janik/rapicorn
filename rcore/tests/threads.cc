/* Rapicorn
 * Copyright (C) 2006 Tim Janik
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
#include <rcore/rapicorntests.h>
#include <stdlib.h>

namespace {
using namespace Rapicorn;

/* --- utilities --- */
RapicornThread*
rapicorn_thread_run (const gchar     *name,
                     RapicornThreadFunc func,
                     gpointer         user_data)
{
  g_return_val_if_fail (name && name[0], NULL);

  RapicornThread *thread = ThreadTable.thread_new (name);
  ThreadTable.thread_ref_sink (thread);
  if (ThreadTable.thread_start (thread, func, user_data))
    return thread;
  else
    {
      ThreadTable.thread_unref (thread);
      return NULL;
    }
}

/* --- atomicity tests --- */
static volatile guint atomic_count = 0;
static Mutex          atomic_mutex;
static Cond           atomic_cond;

static void
atomic_up_thread (gpointer data)
{
  volatile int *ip = (int*) data;
  for (guint i = 0; i < 25; i++)
    Atomic::add (ip, +3);
  atomic_mutex.lock();
  atomic_count -= 1;
  atomic_cond.signal();
  atomic_mutex.unlock();
  TASSERT (strcmp (ThreadTable.thread_name (ThreadTable.thread_self()), "AtomicTest") == 0);
}

static void
atomic_down_thread (gpointer data)
{
  volatile int *ip = (int*) data;
  for (guint i = 0; i < 25; i++)
    Atomic::add (ip, -4);
  atomic_mutex.lock();
  atomic_count -= 1; // FIXME: make this atomic
  atomic_cond.signal();
  atomic_mutex.unlock();
  TASSERT (strcmp (ThreadTable.thread_name (ThreadTable.thread_self()), "AtomicTest") == 0);
}

static void
test_atomic (void)
{
  TSTART ("AtomicThreading");
  int count = 44;
  RapicornThread *threads[count];
  volatile int atomic_counter = 0;
  atomic_count = count;
  for (int i = 0; i < count; i++)
    {
      threads[i] = rapicorn_thread_run ("AtomicTest", (i&1) ? atomic_up_thread : atomic_down_thread, (void*) &atomic_counter);
      TASSERT (threads[i]);
    }
  atomic_mutex.lock();
  while (atomic_count > 0)
    {
      TACK();
      atomic_cond.wait (atomic_mutex);
    }
  atomic_mutex.unlock();
  int result = count / 2 * 25 * +3 + count / 2 * 25 * -4;
  // g_printerr ("{ %d ?= %d }", atomic_counter, result);
  for (int i = 0; i < count; i++)
    ThreadTable.thread_unref (threads[i]);
  TASSERT (atomic_counter == result);
  TDONE ();
}

/* --- runonce tests --- */
static volatile uint runonce_threadcount = 0;
static Mutex         runonce_mutex;
static Cond          runonce_cond;

static volatile size_t runonce_value = 0;

static void
runonce_function (void *data)
{
  runonce_mutex.lock(); // syncronize
  runonce_mutex.unlock();
  volatile int *runonce_counter = (volatile int*) data;
  if (once_enter (&runonce_value))
    {
      runonce_mutex.lock();
      runonce_cond.broadcast();
      runonce_mutex.unlock();
      usleep (1); // sched_yield replacement to force contention
      Atomic::add (runonce_counter, 1);
      usleep (500); // sched_yield replacement to force contention
      once_leave (&runonce_value, size_t (42));
    }
  TASSERT (*runonce_counter == 1);
  TASSERT (runonce_value == 42);
  /* sinal thread end */
  Atomic::add (&runonce_threadcount, -1);
  runonce_mutex.lock();
  runonce_cond.signal();
  runonce_mutex.unlock();
}

static void
test_runonce (void)
{
  TSTART ("RunOnceTest");
  int count = 44;
  RapicornThread *threads[count];
  volatile int runonce_counter = 0;
  Atomic::set (&runonce_threadcount, count);
  runonce_mutex.lock();
  for (int i = 0; i < count; i++)
    {
      threads[i] = rapicorn_thread_run ("RunOnceTest", runonce_function, (void*) &runonce_counter);
      TASSERT (threads[i]);
    }
  TASSERT (runonce_value == 0);
  runonce_mutex.unlock(); // syncronized thread start
  runonce_mutex.lock();
  while (Atomic::get (&runonce_threadcount) > 0)
    {
      TACK();
      runonce_cond.wait (runonce_mutex);
    }
  runonce_mutex.unlock();
  for (int i = 0; i < count; i++)
    ThreadTable.thread_unref (threads[i]);
  TASSERT (runonce_counter == 1);
  TASSERT (runonce_value == 42);
  TDONE ();
}

/* --- basic threading tests --- */
static void
plus1_thread (gpointer data)
{
  guint *tdata = (guint*) data;
  ThreadTable.thread_sleep (-1);
  *tdata += 1;
  while (!ThreadTable.thread_aborted ())
    ThreadTable.thread_sleep (-1);
}

static Mutex    static_mutex;
static RecMutex static_rec_mutex;
static Cond     static_cond;

static void
test_threads (void)
{
  static Mutex test_mutex;
  gboolean locked;
  TSTART ("Threading");
  /* test C mutex */
  locked = test_mutex.trylock();
  TASSERT (locked);
  locked = test_mutex.trylock();
  TASSERT (!locked);
  test_mutex.unlock();
  /* not initializing static_mutex */
  locked = static_mutex.trylock();
  TASSERT (locked);
  locked = static_mutex.trylock();
  TASSERT (!locked);
  static_mutex.unlock();
  locked = static_mutex.trylock();
  TASSERT (locked);
  static_mutex.unlock();
  /* not initializing static_rec_mutex */
  locked = static_rec_mutex.trylock();
  TASSERT (locked);
  static_rec_mutex.lock();
  locked = static_rec_mutex.trylock();
  TASSERT (locked);
  static_rec_mutex.unlock();
  static_rec_mutex.unlock();
  static_rec_mutex.unlock();
  locked = static_rec_mutex.trylock();
  TASSERT (locked);
  static_rec_mutex.unlock();
  /* not initializing static_cond */
  static_cond.signal();
  static_cond.broadcast();
  /* test C++ mutex */
  static Mutex mutex;
  static RecMutex rmutex;
  mutex.lock();
  rmutex.lock();
  mutex.unlock();
  rmutex.unlock();
  guint thread_data1 = 0;
  RapicornThread *thread1 = rapicorn_thread_run ("plus1", plus1_thread, &thread_data1);
  guint thread_data2 = 0;
  RapicornThread *thread2 = rapicorn_thread_run ("plus2", plus1_thread, &thread_data2);
  guint thread_data3 = 0;
  RapicornThread *thread3 = rapicorn_thread_run ("plus3", plus1_thread, &thread_data3);
  TASSERT (thread1 != NULL);
  TASSERT (thread2 != NULL);
  TASSERT (thread3 != NULL);
  TASSERT (thread_data1 == 0);
  TASSERT (thread_data2 == 0);
  TASSERT (thread_data3 == 0);
  TASSERT (ThreadTable.thread_get_running (thread1) == TRUE);
  TASSERT (ThreadTable.thread_get_running (thread2) == TRUE);
  TASSERT (ThreadTable.thread_get_running (thread3) == TRUE);
  ThreadTable.thread_wakeup (thread1);
  ThreadTable.thread_wakeup (thread2);
  ThreadTable.thread_wakeup (thread3);
  ThreadTable.thread_abort (thread1);
  ThreadTable.thread_abort (thread2);
  ThreadTable.thread_abort (thread3);
  TASSERT (thread_data1 > 0);
  TASSERT (thread_data2 > 0);
  TASSERT (thread_data3 > 0);
  ThreadTable.thread_unref (thread1);
  ThreadTable.thread_unref (thread2);
  ThreadTable.thread_unref (thread3);
  TDONE ();
}

/* --- C++ threading tests --- */
struct ThreadA : public virtual Rapicorn::Thread {
  int value;
  volatile int *counter;
  ThreadA (volatile int *counterp,
           int           v) :
    Thread ("ThreadA"),
    value (v), counter (counterp)
  {}
  virtual void
  run ()
  {
    TASSERT (this->name() == "ThreadA");
    TASSERT (this->name() == Thread::Self::name());
    for (int j = 0; j < 17905; j++)
      Atomic::add (counter, value);
  }
};

template<class M> static bool
lockable (M &mutex)
{
  bool lockable = mutex.trylock();
  if (lockable)
    mutex.unlock();
  return lockable;
}

static void
test_thread_cxx (void)
{
  TSTART ("C++Threading");
  TASSERT (NULL != &Thread::self());
  volatile int atomic_counter = 0;
  int result = 0;
  int count = 35;
  Rapicorn::Thread *threads[count];
  for (int i = 0; i < count; i++)
    {
      int v = rand();
      for (int j = 0; j < 17905; j++)
        result += v;
      threads[i] = new ThreadA (&atomic_counter, v);
      TASSERT (threads[i]);
      ref_sink (threads[i]);
    }
  TASSERT (atomic_counter == 0);
  for (int i = 0; i < count; i++)
    threads[i]->start();
  for (int i = 0; i < count; i++)
    {
      threads[i]->wait_for_exit();
      unref (threads[i]);
    }
  TASSERT (atomic_counter == result);
  TDONE ();

  TSTART ("C++OwnedMutex");
  static OwnedMutex static_omutex;
  TASSERT (static_omutex.mine() == false);
  static_omutex.lock();
  TASSERT (static_omutex.mine() == true);
  static_omutex.unlock();
  TASSERT (static_omutex.mine() == false);
  static_omutex.lock();
  TASSERT (static_omutex.mine() == true);
  static_omutex.lock();
  TASSERT (static_omutex.mine() == true);
  static_omutex.unlock();
  TASSERT (static_omutex.mine() == true);
  static_omutex.unlock();
  TASSERT (static_omutex.mine() == false);
  TASSERT (NULL != &Thread::self());
  OwnedMutex omutex;
  TASSERT (omutex.owner() == NULL);
  TASSERT (omutex.mine() == false);
  omutex.lock();
  TASSERT (omutex.owner() == &Thread::self());
  TASSERT (omutex.mine() == true);
  TASSERT (lockable (omutex) == true);
  bool locked = omutex.trylock();
  TASSERT (locked == true);
  omutex.unlock();
  omutex.unlock();
  TASSERT (omutex.owner() == NULL);
  TASSERT (lockable (omutex) == true);
  TASSERT (omutex.owner() == NULL);
  locked = omutex.trylock();
  TASSERT (locked == true);
  TASSERT (omutex.owner() == &Thread::self());
  TASSERT (lockable (omutex) == true);
  omutex.unlock();
  TASSERT (omutex.owner() == NULL);
  TDONE();
}

// simple spin lock test
static void
test_spin_lock_simple (void)
{
  TSTART ("C++SpinLock");
  {
    SpinLock sp;
    bool l;
    l = sp.trylock();
    TASSERT (l);
    l = sp.trylock();
    TASSERT (!l);
    sp.unlock();
    l = sp.trylock();
    TASSERT (l);
    l = sp.trylock();
    TASSERT (!l);
    sp.unlock();
    sp.lock();
    l = sp.trylock();
    TASSERT (!l);
    sp.unlock();
  }
  TDONE();
}

/* --- ScopedLock test --- */
template<typename XMutex> static void
test_recursive_scoped_lock (XMutex &rec_mutex, uint depth)
{
  ScopedLock<XMutex> locker (rec_mutex);
  if (depth > 1)
    test_recursive_scoped_lock (rec_mutex, depth - 1);
  else
    {
      locker.lock();
      locker.lock();
      locker.lock();
      bool lockable1 = rec_mutex.trylock();
      bool lockable2 = rec_mutex.trylock();
      TASSERT (lockable1 && lockable2);
      rec_mutex.unlock();
      rec_mutex.unlock();
      locker.unlock();
      locker.unlock();
      locker.unlock();
    }
}

static void
test_scoped_locks()
{
  TSTART ("Scoped Locks");
  Mutex mutex1;
  TASSERT (lockable (mutex1) == true);
  {
    ScopedLock<Mutex> locker1 (mutex1);
    TASSERT (lockable (mutex1) == false);
  }
  TASSERT (lockable (mutex1) == true);
  {
    ScopedLock<Mutex> locker0 (mutex1, false);
    TASSERT (lockable (mutex1) == true);
    locker0.lock();
    TASSERT (lockable (mutex1) == false);
    locker0.unlock();
    TASSERT (lockable (mutex1) == true);
  }
  TASSERT (lockable (mutex1) == true);
  {
    ScopedLock<Mutex> locker2 (mutex1, true);
    TASSERT (lockable (mutex1) == false);
    locker2.unlock();
    TASSERT (lockable (mutex1) == true);
    locker2.lock();
    TASSERT (lockable (mutex1) == false);
  }
  TASSERT (lockable (mutex1) == true);
  RecMutex rmutex;
  test_recursive_scoped_lock (rmutex, 999);
  OwnedMutex omutex;
  test_recursive_scoped_lock (omutex, 999);
  TDONE();
}

/* --- C++ atomicity tests --- */
static void
test_thread_atomic_cxx (void)
{
  TSTART ("C++AtomicThreading");
  /* integer functions */
  volatile int ai, r;
  Atomic::set (&ai, 17);
  TASSERT (ai == 17);
  r = Atomic::get (&ai);
  TASSERT (r == 17);
  Atomic::add (&ai, 9);
  r = Atomic::get (&ai);
  TASSERT (r == 26);
  Atomic::set (&ai, -1147483648);
  TASSERT (ai == -1147483648);
  r = Atomic::get (&ai);
  TASSERT (r == -1147483648);
  Atomic::add (&ai, 9);
  r = Atomic::get (&ai);
  TASSERT (r == -1147483639);
  Atomic::add (&ai, -20);
  r = Atomic::get (&ai);
  TASSERT (r == -1147483659);
  r = Atomic::cas (&ai, 17, 19);
  TASSERT (r == false);
  r = Atomic::get (&ai);
  TASSERT (r == -1147483659);
  r = Atomic::cas (&ai, -1147483659, 19);
  TASSERT (r == true);
  r = Atomic::get (&ai);
  TASSERT (r == 19);
  r = Atomic::add (&ai, 1);
  TASSERT (r == 19);
  r = Atomic::get (&ai);
  TASSERT (r == 20);
  r = Atomic::add (&ai, -20);
  TASSERT (r == 20);
  r = Atomic::get (&ai);
  TASSERT (r == 0);
  /* pointer functions */
  void * volatile ap, * volatile p;
  Atomic::ptr_set (&ap, (void*) 119);
  TASSERT (ap == (void*) 119);
  p = Atomic::ptr_get (&ap);
  TASSERT (p == (void*) 119);
  r = Atomic::ptr_cas (&ap, (void*) 17, (void*) -42);
  TASSERT (r == false);
  p = Atomic::ptr_get (&ap);
  TASSERT (p == (void*) 119);
  r = Atomic::ptr_cas (&ap, (void*) 119, (void*) 4294967279U);
  TASSERT (r == true);
  p = Atomic::ptr_get (&ap);
  TASSERT (p == (void*) 4294967279U);
  TDONE ();
}

/* --- thread_yield --- */
static inline void
handle_contention ()
{
  /* we're waiting for our contention counterpart if we got here:
   * - sched_yield(3posix) will immediately give up the CPU and let another
   *   task run. but if the contention counterpart is running on another
   *   CPU this will lead to scheduler trashing on our CPU. and if other
   *   bacground tasks are running, they could get all our CPU time,
   *   because sched_yield() effectively discards the current time slice.
   * - busy spinning is useful if the contention counterpart runs on a
   *   different CPU, as long as the loop doesn't involve syncronization
   *   primitives which cause IO bus trashing ("lock" prefix in x86 asm).
   * - usleep(3posix) is a way to give up the CPU without discarding our
   *   time slices and avoids scheduler or bus trashing. allthough it is
   *   not the perfect or optimum syncronization/timing primitive, it
   *   avoids most ill effects and still allows for a sufficient number
   *   of task switches.
   */
  usleep (500); // 1usec is the minimum value to cause an effect
}

/* --- ring buffer --- */
typedef Atomic::RingBuffer<int> IntRingBuffer;
class IntSequence {
  uint32 accu;
public:
  explicit      IntSequence() : accu (123456789) {}
  inline int32  gen_int    () { accu = 1664525 * accu + 1013904223; return accu; }
};
#define CONTENTION_PRINTF       while (0) g_printerr
struct RingBufferWriter : public virtual Rapicorn::Thread, IntSequence {
  IntRingBuffer *ring;
  uint           ring_buffer_test_length;
  RingBufferWriter (IntRingBuffer *rb,
                    uint           rbtl) :
    Thread ("RingBufferWriter"),
    ring (rb), ring_buffer_test_length (rbtl)
  {}
  virtual void
  run ()
  {
    TPRINT ("%s start.", Thread::Self::name().c_str());
    for (uint l = 0; l < ring_buffer_test_length;)
      {
        uint k, n = g_random_int() % MIN (ring_buffer_test_length - l + 1, 65536 * 2);
        int buffer[n], *b = buffer;
        for (uint i = 0; i < n; i++)
          b[i] = gen_int();
        uint j = n;
        while (j)
          {
            k = ring->write (j, b);
            TCHECK (k <= j);
            j -= k;
            b += k;
            if (!k)     // waiting for reader thread
              handle_contention();
            CONTENTION_PRINTF (k ? "*" : "/");
          }
        if (l / 499999 != (l + n) / 499999)
          TICK();
        l += n;
      }
    TPRINT ("%s done.", Thread::Self::name().c_str());
  }
};
struct RingBufferReader : public virtual Rapicorn::Thread, IntSequence {
  IntRingBuffer *ring;
  uint           ring_buffer_test_length;
  RingBufferReader (IntRingBuffer *rb,
                    uint           rbtl) :
    Thread ("RingBufferReader"),
    ring (rb), ring_buffer_test_length (rbtl)
  {}
  virtual void
  run ()
  {
    TPRINT ("%s start.", Thread::Self::name().c_str());
    for (uint l = 0; l < ring_buffer_test_length;)
      {
        uint k, n = ring->n_readable();
        n = lrand48() % MIN (n + 1, 65536 * 2);
        int buffer[n], *b = buffer;
        if (rand() & 1)
          {
            k = ring->read (n, b, false);
            TCHECK (n == k);
            if (k)
              CONTENTION_PRINTF ("+");
          }
        else
          {
            k = ring->read (n, b, true);
            TCHECK (k <= n);
            if (!k)         // waiting for writer thread
              handle_contention();
            CONTENTION_PRINTF (k ? "+" : "\\");
          }
        for (uint i = 0; i < k; i++)
          TCHECK (b[i] == gen_int());
        if (l / 499999 != (l + k) / 499999)
          TACK();
        l += k;
      }
    TPRINT ("%s done.", Thread::Self::name().c_str());
  }
};

static void
test_ring_buffer ()
{
  static const gchar *testtext = "Ring Buffer test Text (47\xff)";
  uint n, ttl = strlen (testtext);
  TSTART ("RingBuffer");
  Atomic::RingBuffer<char> rb1 (ttl);
  TASSERT (rb1.n_writable() == ttl);
  n = rb1.write (ttl, testtext);
  TASSERT (n == ttl);
  TASSERT (rb1.n_writable() == 0);
  TASSERT (rb1.n_readable() == ttl);
  char buffer[8192];
  n = rb1.read (8192, buffer);
  TASSERT (n == ttl);
  TASSERT (rb1.n_readable() == 0);
  TASSERT (rb1.n_writable() == ttl);
  TASSERT (strncmp (buffer, testtext, n) == 0);
  TDONE();

  /* check lower end ring buffer sizes (high contention test) */
  for (uint step = 1; step < 8; step++)
    {
      uint ring_buffer_test_length = 17 * step + (rand() % 19);
      TSTART ("AsyncRingBuffer-%d-%d", step, ring_buffer_test_length);
      IntRingBuffer irb (step);
      RingBufferReader *rbr = new RingBufferReader (&irb, ring_buffer_test_length);
      ref_sink (rbr);
      RingBufferWriter *rbw = new RingBufferWriter (&irb, ring_buffer_test_length);
      ref_sink (rbw);
      TASSERT (rbr && rbw);
      rbr->start();
      rbw->start();
      rbw->wait_for_exit();
      rbr->wait_for_exit();
      TASSERT (rbr && rbw);
      unref (rbr);
      unref (rbw);
      TDONE();
    }

  /* check big ring buffer sizes */
  if (true)
    {
      TSTART ("AsyncRingBuffer-big");
      uint ring_buffer_test_length = 999999 * (init_settings().test_quick ? 1 : 20);
      IntRingBuffer irb (16384 + (lrand48() % 8192));
      RingBufferReader *rbr = new RingBufferReader (&irb, ring_buffer_test_length);
      ref_sink (rbr);
      RingBufferWriter *rbw = new RingBufferWriter (&irb, ring_buffer_test_length);
      ref_sink (rbw);
      TASSERT (rbr && rbw);
      rbr->start();
      rbw->start();
      rbw->wait_for_exit();
      rbr->wait_for_exit();
      TASSERT (rbr && rbw);
      unref (rbr);
      unref (rbw);
      TDONE();
    }
}

/* --- --- */
static void
test_debug_channel ()
{
  TSTART ("DebugChannelFileAsync (countdown)");
  DebugChannel *dbg = DebugChannel::new_from_file_async ("/dev/stderr");
  ref_sink (dbg);
  TASSERT (dbg);
  dbg->printf ("9");
  usleep (100 * 1000);
  dbg->printf ("8");
  usleep (110 * 1000);
  dbg->printf ("7");
  usleep (120 * 1000);
  dbg->printf ("6");
  usleep (130 * 1000);
  dbg->printf ("5");
  usleep (140 * 1000);
  dbg->printf ("4");
  usleep (150 * 1000);
  dbg->printf ("3");
  usleep (160 * 1000);
  dbg->printf ("2");
  usleep (170 * 1000);
  dbg->printf ("1");
  usleep (180 * 1000);
  dbg->printf ("0");
  usleep (190 * 1000);
  unref (dbg);
  TICK();
  TDONE();
}

/* --- late deletable destruction --- */
static bool deletable_destructor = false;
struct MyDeletable : public virtual Deletable {
  virtual
  ~MyDeletable()
  {
    deletable_destructor = true;
  }
  void
  force_deletion_hooks()
  {
    invoke_deletion_hooks();
  }
};
struct MyDeletableHook : public Deletable::DeletionHook {
  Deletable *deletable;
  explicit     MyDeletableHook () :
    deletable (NULL)
  {}
  virtual void
  monitoring_deletable (Deletable &deletable_obj)
  {
    TASSERT (deletable == NULL);
    deletable = &deletable_obj;
  }
  virtual void
  dismiss_deletable ()
  {
    if (deletable)
      deletable = NULL;
  }
  virtual
  ~MyDeletableHook ()
  {
    // g_printerr ("~MyDeletableHook(): deletable=%p\n", deletable);
    if (deletable)
      deletable_remove_hook (deletable);
    deletable = NULL;
  }
};

static MyDeletable early_deletable __attribute__ ((init_priority (101)));
static MyDeletable late_deletable __attribute__ ((init_priority (65535)));

static void
test_deletable_destruction ()
{
  TSTART ("Deletable destruction");
  {
    MyDeletable test_deletable;
    TICK();
    MyDeletableHook dhook1;
    // g_printerr ("TestHook=%p\n", (Deletable::DeletionHook*) &dhook1);
    dhook1.deletable_add_hook (&test_deletable);
    TICK();
    dhook1.deletable_remove_hook (&test_deletable);
    dhook1.dismiss_deletable();
    TICK();
    MyDeletableHook dhook2;
    dhook2.deletable_add_hook (&test_deletable);
    test_deletable.force_deletion_hooks ();
    TICK();
    MyDeletableHook dhook3;
    dhook3.deletable_add_hook (&test_deletable);
    TICK();
    /* automatic deletion hook invocation */
    /* FIXME: deletable destructor is called first and doesn't auto-remove
     * - if deletion hooks were ring-linked, we could at least catch this case in ~DeletionHook
     */
  }
  MyDeletable *deletable2 = new MyDeletable;
  TASSERT (deletable2 != NULL);
  deletable_destructor = false;
  delete deletable2;
  TASSERT (deletable_destructor == true);
  TDONE();
  /* early_deletable and late_deletable are only tested at program end */
}

/* --- Mutextes before g_thread_init() --- */
static void
test_before_thread_init()
{
  /* check C++ mutex init + destruct before g_thread_init() */
  Mutex *mutex = new Mutex;
  RecMutex *rmutex = new RecMutex;
  Cond *cond = new Cond;
  delete mutex;
  delete rmutex;
  delete cond;
}

} // Anon

static guint constructur_attribute_test = 0;

static void RAPICORN_CONSTRUCTOR
constructur_attribute_test_initializer (void)
{
  constructur_attribute_test = 0x1237ABBA;
}

int
main (int   argc,
      char *argv[])
{
  if (constructur_attribute_test != 305638330)
    g_error ("%s: static constructors have not been called before main", G_STRFUNC);

  test_before_thread_init();

  rapicorn_init_test (&argc, &argv);

  test_threads();
  test_atomic();
  test_thread_cxx();
  test_spin_lock_simple();
  test_thread_atomic_cxx();
  test_scoped_locks();
  test_runonce();
  test_deletable_destruction();
  test_ring_buffer();
  test_debug_channel();

  return 0;
}

/* vim:set ts=8 sts=2 sw=2: */
