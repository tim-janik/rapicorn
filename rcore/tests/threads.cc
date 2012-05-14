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
#include <rcore/testutils.hh>
#include <stdlib.h>
#include <string.h>

namespace {
using namespace Rapicorn;

/* --- atomicity tests --- */
static volatile uint atomic_count = 0;
static Mutex         atomic_mutex;
static Cond          atomic_cond;

class Thread_AtomicUp : public Thread {
  void *data;
  virtual void
  run ()
  {
    TCMP (name(), ==, "AtomicTest");
    volatile int *ip = (int*) data;
    for (uint i = 0; i < 25; i++)
      Atomic::add (ip, +3);
    atomic_mutex.lock();
    atomic_count -= 1;
    atomic_cond.signal();
    atomic_mutex.unlock();
  }
public:
  Thread_AtomicUp (const String &name, void *udata) : Thread (name), data (udata) {}
};

class Thread_AtomicDown : public Thread {
  void *data;
  virtual void
  run ()
  {
    TCMP (name(), ==, "AtomicTest");
    volatile int *ip = (int*) data;
    for (uint i = 0; i < 25; i++)
      Atomic::add (ip, -4);
    atomic_mutex.lock();
    atomic_count -= 1; // FIXME: make this atomic
    atomic_cond.signal();
    atomic_mutex.unlock();
  }
public:
  Thread_AtomicDown (const String &name, void *udata) : Thread (name), data (udata) {}
};

static void
test_atomic (void)
{
  int count = 44;
  Thread *threads[count];
  volatile int atomic_counter = 0;
  atomic_count = count;
  for (int i = 0; i < count; i++)
    {
      if (i&1)
        threads[i] = new Thread_AtomicUp ("AtomicTest", (void*) &atomic_counter);
      else
        threads[i] = new Thread_AtomicDown ("AtomicTest", (void*) &atomic_counter);
      ref_sink (threads[i]);
      threads[i]->start();
      TASSERT (threads[i]);
    }
  atomic_mutex.lock();
  while (atomic_count > 0)
    {
      TOK();
      atomic_cond.wait (atomic_mutex);
    }
  atomic_mutex.unlock();
  int result = count / 2 * 25 * +3 + count / 2 * 25 * -4;
  // g_printerr ("{ %d ?= %d }", atomic_counter, result);
  for (int i = 0; i < count; i++)
    unref (threads[i]);
  TCMP (atomic_counter, ==, result);
}
REGISTER_TEST ("Threads/AtomicThreading", test_atomic);

/* --- runonce tests --- */
static volatile uint runonce_threadcount = 0;
static Mutex         runonce_mutex;
static Cond          runonce_cond;

static volatile size_t runonce_value = 0;

class Thread_RunOnce : public Thread {
  volatile void *data;
  virtual void
  run ()
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
    TCMP (*runonce_counter, ==, 1);
    TCMP (runonce_value, ==, 42);
    /* sinal thread end */
    Atomic::add (&runonce_threadcount, -1);
    runonce_mutex.lock();
    runonce_cond.signal();
    runonce_mutex.unlock();
  }
public:
  Thread_RunOnce (const String &name, volatile void *udata) : Thread (name), data (udata) {}
};

static void
test_runonce (void)
{
  int count = 44;
  Thread *threads[count];
  volatile int runonce_counter = 0;
  Atomic::set (&runonce_threadcount, count);
  runonce_mutex.lock();
  for (int i = 0; i < count; i++)
    {
      threads[i] = new Thread_RunOnce ("RunOnceTest", &runonce_counter);
      ref_sink (threads[i]);
      threads[i]->start();
      TASSERT (threads[i]);
    }
  TCMP (runonce_value, ==, 0);
  runonce_mutex.unlock(); // syncronized thread start
  runonce_mutex.lock();
  while (Atomic::get (&runonce_threadcount) > 0)
    {
      TOK();
      runonce_cond.wait (runonce_mutex);
    }
  runonce_mutex.unlock();
  for (int i = 0; i < count; i++)
    unref (threads[i]);
  TCMP (runonce_counter, ==, 1);
  TCMP (runonce_value, ==, 42);
}
REGISTER_TEST ("Threads/RunOnceTest", test_runonce);

/* --- basic threading tests --- */
class Thread_Plus1 : public Thread {
  void *data;
  virtual void
  run ()
  {
    uint *tdata = (uint*) data;
    Thread::Self::sleep (-1);
    *tdata += 1;
    while (!Thread::Self::aborted ())
      Thread::Self::sleep (-1);
  }
public:
  Thread_Plus1 (const String &name, void *udata) : Thread (name), data (udata) {}
};

static Mutex    static_mutex;
static RecMutex static_rec_mutex;
static Cond     static_cond;

static void
test_threads (void)
{
  static Mutex test_mutex;
  bool locked;
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
  uint thread_data1 = 0;
  Thread *thread1 = new Thread_Plus1 ("plus1", &thread_data1);
  ref_sink (thread1);
  thread1->start();
  uint thread_data2 = 0;
  Thread *thread2 = new Thread_Plus1 ("plus2", &thread_data2);
  ref_sink (thread2);
  thread2->start();
  uint thread_data3 = 0;
  Thread *thread3 = new Thread_Plus1 ("plus3", &thread_data3);
  ref_sink (thread3);
  thread3->start();
  TCMP (thread1, !=, nullptr);
  TCMP (thread2, !=, nullptr);
  TCMP (thread3, !=, nullptr);
  TCMP (thread_data1, ==, 0);
  TCMP (thread_data2, ==, 0);
  TCMP (thread_data3, ==, 0);
  TCMP (thread1->running(), ==, TRUE);
  TCMP (thread2->running(), ==, TRUE);
  TCMP (thread3->running(), ==, TRUE);
  thread1->wakeup();
  thread2->wakeup();
  thread3->wakeup();
  thread1->abort();
  thread2->abort();
  thread3->abort();
  TCMP (thread_data1, >, 0);
  TCMP (thread_data2, >, 0);
  TCMP (thread_data3, >, 0);
  unref (thread1);
  unref (thread2);
  unref (thread3);
}
REGISTER_TEST ("Threads/Threading", test_threads);

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
    TCMP (this->name(), ==, "ThreadA");
    TCMP (this->name(), ==, Thread::Self::name());
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
  TCMP (nullptr, !=, &Thread::self());
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
  TCMP (atomic_counter, ==, 0);
  for (int i = 0; i < count; i++)
    threads[i]->start();
  for (int i = 0; i < count; i++)
    {
      threads[i]->wait_for_exit();
      unref (threads[i]);
    }
  TCMP (atomic_counter, ==, result);
  TDONE ();

  TSTART ("Threads/C++OwnedMutex");
  static OwnedMutex static_omutex;
  TCMP (static_omutex.mine(), ==, false);
  static_omutex.lock();
  TCMP (static_omutex.mine(), ==, true);
  static_omutex.unlock();
  TCMP (static_omutex.mine(), ==, false);
  static_omutex.lock();
  TCMP (static_omutex.mine(), ==, true);
  static_omutex.lock();
  TCMP (static_omutex.mine(), ==, true);
  static_omutex.unlock();
  TCMP (static_omutex.mine(), ==, true);
  static_omutex.unlock();
  TCMP (static_omutex.mine(), ==, false);
  TCMP (nullptr, !=, &Thread::self());
  OwnedMutex omutex;
  TCMP (omutex.owner(), ==, nullptr);
  TCMP (omutex.mine(), ==, false);
  omutex.lock();
  TCMP (omutex.owner(), ==, &Thread::self());
  TCMP (omutex.mine(), ==, true);
  TCMP (lockable (omutex), ==, true);
  bool locked = omutex.trylock();
  TCMP (locked, ==, true);
  omutex.unlock();
  omutex.unlock();
  TCMP (omutex.owner(), ==, nullptr);
  TCMP (lockable (omutex), ==, true);
  TCMP (omutex.owner(), ==, nullptr);
  locked = omutex.trylock();
  TCMP (locked, ==, true);
  TCMP (omutex.owner(), ==, &Thread::self());
  TCMP (lockable (omutex), ==, true);
  omutex.unlock();
  TCMP (omutex.owner(), ==, nullptr);
}
REGISTER_TEST ("Threads/C++Threading", test_thread_cxx);

// simple spin lock test
static void
test_spin_lock_simple (void)
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
REGISTER_TEST ("Threads/C++SpinLock", test_spin_lock_simple);

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
  Mutex mutex1;
  TCMP (lockable (mutex1), ==, true);
  {
    ScopedLock<Mutex> locker1 (mutex1);
    TCMP (lockable (mutex1), ==, false);
  }
  TCMP (lockable (mutex1), ==, true);
  {
    ScopedLock<Mutex> locker0 (mutex1, BALANCED);
    TCMP (lockable (mutex1), ==, true);
    locker0.lock();
    TCMP (lockable (mutex1), ==, false);
    locker0.unlock();
    TCMP (lockable (mutex1), ==, true);
  }
  TCMP (lockable (mutex1), ==, true);
  {
    ScopedLock<Mutex> locker2 (mutex1, AUTOLOCK);
    TCMP (lockable (mutex1), ==, false);
    locker2.unlock();
    TCMP (lockable (mutex1), ==, true);
    locker2.lock();
    TCMP (lockable (mutex1), ==, false);
  }
  TCMP (lockable (mutex1), ==, true);
  RecMutex rmutex;
  test_recursive_scoped_lock (rmutex, 999);
  OwnedMutex omutex;
  test_recursive_scoped_lock (omutex, 999);
  // test ScopedLock balancing unlock + lock
  mutex1.lock();
  {
    TCMP (lockable (mutex1), ==, false);
    ScopedLock<Mutex> locker (mutex1, BALANCED);
    locker.unlock();
    TCMP (lockable (mutex1), ==, true);
  } // ~ScopedLock (BALANCED) now does locker.lock()
  TCMP (lockable (mutex1), ==, false);
  {
    ScopedLock<Mutex> locker (mutex1, BALANCED);
  } // ~ScopedLock (BALANCED) now does nothing
  TCMP (lockable (mutex1), ==, false);
  mutex1.unlock();
  // test ScopedLock balancing lock + unlock
  {
    TCMP (lockable (mutex1), ==, true);
    ScopedLock<Mutex> locker (mutex1, BALANCED);
    locker.lock();
    TCMP (lockable (mutex1), ==, false);
  } // ~ScopedLock (BALANCED) now does locker.unlock()
  TCMP (lockable (mutex1), ==, true);
  {
    ScopedLock<Mutex> locker (mutex1, BALANCED);
  } // ~ScopedLock (BALANCED) now does nothing
  TCMP (lockable (mutex1), ==, true);
}
REGISTER_TEST ("Threads/Scoped Locks", test_scoped_locks);

/* --- C++ atomicity tests --- */
static void
test_thread_atomic_cxx (void)
{
  /* integer functions */
  volatile int ai, r;
  Atomic::set (&ai, 17);
  TCMP (ai, ==, 17);
  r = Atomic::get (&ai);
  TCMP (r, ==, 17);
  Atomic::add (&ai, 9);
  r = Atomic::get (&ai);
  TCMP (r, ==, 26);
  Atomic::set (&ai, -1147483648);
  TCMP (ai, ==, -1147483648);
  r = Atomic::get (&ai);
  TCMP (r, ==, -1147483648);
  Atomic::add (&ai, 9);
  r = Atomic::get (&ai);
  TCMP (r, ==, -1147483639);
  Atomic::add (&ai, -20);
  r = Atomic::get (&ai);
  TCMP (r, ==, -1147483659);
  r = Atomic::cas (&ai, 17, 19);
  TCMP (r, ==, false);
  r = Atomic::get (&ai);
  TCMP (r, ==, -1147483659);
  r = Atomic::cas (&ai, -1147483659, 19);
  TCMP (r, ==, true);
  r = Atomic::get (&ai);
  TCMP (r, ==, 19);
  r = Atomic::add (&ai, 1);
  TCMP (r, ==, 19);
  r = Atomic::get (&ai);
  TCMP (r, ==, 20);
  r = Atomic::add (&ai, -20);
  TCMP (r, ==, 20);
  r = Atomic::get (&ai);
  TCMP (r, ==, 0);
  /* pointer functions */
  void * volatile ap, * volatile p;
  Atomic::ptr_set (&ap, (void*) 119);
  TCMP (ap, ==, (void*) 119);
  p = Atomic::ptr_get (&ap);
  TCMP (p, ==, (void*) 119);
  r = Atomic::ptr_cas (&ap, (void*) 17, (void*) -42);
  TCMP (r, ==, false);
  p = Atomic::ptr_get (&ap);
  TCMP (p, ==, (void*) 119);
  r = Atomic::ptr_cas (&ap, (void*) 119, (void*) 4294967279U);
  TCMP (r, ==, true);
  p = Atomic::ptr_get (&ap);
  TCMP (p, ==, (void*) 4294967279U);
}
REGISTER_TEST ("Threads/C++AtomicThreading", test_thread_atomic_cxx);

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
#define CONTENTION_PRINTF       while (0) printerr
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
    TINFO ("%s start.", Thread::Self::name().c_str());
    for (uint l = 0; l < ring_buffer_test_length;)
      {
        uint k, n = Test::rand_int() % MIN (ring_buffer_test_length - l + 1, 65536 * 2);
        int buffer[n], *b = buffer;
        for (uint i = 0; i < n; i++)
          b[i] = gen_int();
        uint j = n;
        while (j)
          {
            k = ring->write (j, b);
            TCMP (k, <=, j);
            j -= k;
            b += k;
            if (!k)     // waiting for reader thread
              handle_contention();
            CONTENTION_PRINTF (k ? "*" : "/");
          }
        if (l / 499999 != (l + n) / 499999)
          TOK();
        l += n;
      }
    TINFO ("%s done.", Thread::Self::name().c_str());
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
    TINFO ("%s start.", Thread::Self::name().c_str());
    for (uint l = 0; l < ring_buffer_test_length;)
      {
        uint k, n = ring->n_readable();
        n = lrand48() % MIN (n + 1, 65536 * 2);
        int buffer[n], *b = buffer;
        if (rand() & 1)
          {
            k = ring->read (n, b, false);
            TCMP (n, ==, k);
            if (k)
              CONTENTION_PRINTF ("+");
          }
        else
          {
            k = ring->read (n, b, true);
            TCMP (k, <=, n);
            if (!k)         // waiting for writer thread
              handle_contention();
            CONTENTION_PRINTF (k ? "+" : "\\");
          }
        for (uint i = 0; i < k; i++)
          TCMP (b[i], ==, gen_int());
        if (l / 499999 != (l + k) / 499999)
          TOK();
        l += k;
      }
    TINFO ("%s done.", Thread::Self::name().c_str());
  }
};

static void
test_ring_buffer ()
{
  static const char *testtext = "Ring Buffer test Text (47\xff)";
  uint n, ttl = strlen (testtext);
  Atomic::RingBuffer<char> rb1 (ttl);
  TCMP (rb1.n_writable(), ==, ttl);
  n = rb1.write (ttl, testtext);
  TCMP (n, ==, ttl);
  TCMP (rb1.n_writable(), ==, 0);
  TCMP (rb1.n_readable(), ==, ttl);
  char buffer[8192];
  n = rb1.read (8192, buffer);
  TCMP (n, ==, ttl);
  TCMP (rb1.n_readable(), ==, 0);
  TCMP (rb1.n_writable(), ==, ttl);
  TCMP (strncmp (buffer, testtext, n), ==, 0);
  TDONE();

  /* check lower end ring buffer sizes (high contention test) */
  for (uint step = 1; step < 8; step++)
    {
      uint ring_buffer_test_length = 17 * step + (rand() % 19);
      TSTART ("Threads/AsyncRingBuffer-%d-%d", step, ring_buffer_test_length);
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
  TSTART ("Threads/AsyncRingBuffer-big");
  uint ring_buffer_test_length = 999999 * (Test::slow() ? 20 : 1);
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
}
REGISTER_TEST ("Threads/RingBuffer", test_ring_buffer);
REGISTER_SLOWTEST ("Threads/RingBuffer (slow)", test_ring_buffer);

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
    TCMP (deletable, ==, nullptr);
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
  {
    MyDeletable test_deletable;
    TOK();
    MyDeletableHook dhook1;
    // g_printerr ("TestHook=%p\n", (Deletable::DeletionHook*) &dhook1);
    dhook1.deletable_add_hook (&test_deletable);
    TOK();
    dhook1.deletable_remove_hook (&test_deletable);
    dhook1.dismiss_deletable();
    TOK();
    MyDeletableHook dhook2;
    dhook2.deletable_add_hook (&test_deletable);
    test_deletable.force_deletion_hooks ();
    TOK();
    MyDeletableHook dhook3;
    dhook3.deletable_add_hook (&test_deletable);
    TOK();
    /* automatic deletion hook invocation */
    /* FIXME: deletable destructor is called first and doesn't auto-remove
     * - if deletion hooks were ring-linked, we could at least catch this case in ~DeletionHook
     */
  }
  MyDeletable *deletable2 = new MyDeletable;
  TCMP (deletable2, !=, nullptr);
  deletable_destructor = false;
  delete deletable2;
  TCMP (deletable_destructor, ==, true);
  /* early_deletable and late_deletable are only tested at program end */
}
REGISTER_TEST ("Threads/Deletable destruction", test_deletable_destruction);

} // Anon
