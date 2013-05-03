// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include <rcore/testutils.hh>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <thread>

#if     __SIZEOF_POINTER__ == 8
namespace Rapicorn {
template<> struct Atomic<__int128> : Lib::Atomic<__int128> {
  Atomic<__int128> (__int128 i = 0) : Lib::Atomic<__int128> (i) {}
  using Lib::Atomic<__int128>::operator=;
};
} // Rapicorn
#endif

namespace {
using namespace Rapicorn;

// == constexpr ctors ==
// the following code checks constexpr initialization of types. these checks are run
// before main(), so we have to roll our own assertion and cannot register a real test.
#define QUICK_ASSERT(cond)      do {                                    \
    if (cond) ; else { printerr ("\n%s:%d: assertion failed: %s\n", __FILE__, __LINE__, #cond); abort(); } \
  } while (0)

// demo to check for non-constexpr ctor behavior
struct ComplexType { vector<int> v; int a; ComplexType (int a0) { v.push_back (a0); if (v.size()) a = a0; } };
static ptrdiff_t    read_complex_int     ();
static Init         complex_int_assert_0 ([]() { QUICK_ASSERT (read_complex_int() == 0); });    // first ctor, checks static mem (0)
static ComplexType  complex_int          (1337);                                                // second ctor, assigns non-0
static ptrdiff_t    read_complex_int     () { return complex_int.v.size() ? complex_int.a : 0; }
static Init         complex_int_assert_1 ([]() { QUICK_ASSERT (read_complex_int() == 1337); }); // third ctor, checks non-0
// check for constexpr ctor Atomic<int*>
static ptrdiff_t    read_atomic_ptr     ();
static Init         atomic_ptr_assert_0 ([]() { QUICK_ASSERT (read_atomic_ptr() == 17); });     // first ctor, check constexpr mem
static Atomic<int*> atomic_ptr          ((int*) 17);    // second ctor, constexpr affects static mem initialization
static ptrdiff_t    read_atomic_ptr     () { return ptrdiff_t (atomic_ptr.load()); }
static Init         atomic_ptr_assert_1 ([]() { QUICK_ASSERT (read_atomic_ptr() == 17); });     // third ctor, runs last
// check for constexpr ctor Atomic<ptrdiff_t>
static ptrdiff_t    read_atomic_pdt     ();
static Init         atomic_pdt_assert_0 ([]() { QUICK_ASSERT (read_atomic_pdt() == 879); });    // first ctor, check constexpr mem
static Atomic<int*> atomic_pdt          ((int*) 879);   // second ctor, constexpr affects static mem initialization
static ptrdiff_t    read_atomic_pdt     () { return ptrdiff_t (atomic_pdt.load()); }
static Init         atomic_pdt_assert_1 ([]() { QUICK_ASSERT (read_atomic_pdt() == 879); });    // third ctor, runs last

static void
test_constexpr_ctors ()
{
  QUICK_ASSERT (read_complex_int() == 1337);
  QUICK_ASSERT (read_atomic_ptr() == 17);
  QUICK_ASSERT (read_atomic_pdt() == 879);
}
REGISTER_TEST ("Threads/Constexpr Constructors", test_constexpr_ctors);

// == atomicity tests ==
template<typename V> static void
atomic_counter_func (Atomic<V> &ai, int niters, V d)
{
  if (d < 0)
    for (int i = 0; i < niters; i++)
      ai -= -d;
  else
    for (int i = 0; i < niters; i++)
      ai += d;
}

template<typename V> static void
test_atomic_counter (const int nthreads, int niters, V a, V b)
{
  assert (0 == (nthreads & 1));
  std::thread threads[nthreads];
  Atomic<V> atomic_counter = 0;
  for (int i = 0; i < nthreads; i++)
    {
      if (i & 1)
        threads[i] = std::thread (atomic_counter_func<V>, std::ref (atomic_counter), niters, a);
      else
        threads[i] = std::thread (atomic_counter_func<V>, std::ref (atomic_counter), niters, b);
      TASSERT (threads[i].joinable());
    }
  for (int i = 0; i < nthreads; i++)
    threads[i].join();
  const int64 result = nthreads / 2 * niters * a + nthreads / 2 * niters * b;
  const int64 atomic_result = V (atomic_counter);
  TCMP (atomic_result, ==, result);
}

struct ComplexStruct {
  String string;
  int    x;
  ComplexStruct (const String &s = "seven", int _x = 7) : string (s), x (_x) {}
};

static void
test_atomic ()
{
  test_atomic_counter<char>  (4, 3, +4, -3);
  test_atomic_counter<int8>  (6, 5, +2, -1);
  test_atomic_counter<uint8> (6, 5, +2, +1);
  test_atomic_counter<int>   (44, 25, +5, -2);
  test_atomic_counter<uint>  (44, 25, 1, 6);
  test_atomic_counter<int64> (52, 125, -50, +37);
  test_atomic_counter<int64> (52, 125, +42, +77);
#if     __SIZEOF_POINTER__ == 8
  test_atomic_counter<__int128> (68, 12500, +4200, +77000);
#endif
  Exclusive<ComplexStruct> excs;
  ComplexStruct copy = excs;    // atomic access
  assert (copy.string == "seven" && copy.x == 7);
  excs = { "three", 3 };        // atomic access
  copy = excs;                  // atomic access
  assert (copy.string == "three" && copy.x == 3);
}
REGISTER_TEST ("Threads/Atomic Operations", test_atomic);

// == do_once ==
/// [do_once-EXAMPLE]
static const char*
startup_message ()
{
  // A global variable that needs one time initialization
  static char text_variable[1024];

  // Initialize text_variable only once
  do_once
    {
      snprintf (text_variable, sizeof (text_variable), "Initialized at: %zu", size_t (timestamp_realtime()));
    }

  // Always returns the same text
  return text_variable;
}
/// [do_once-EXAMPLE]

static void
do_once_example_test()
{
  String first (startup_message());
  assert (first.size() > 5);
  String second (startup_message());
  TCMP (first, ==, second);
}
REGISTER_TEST ("Examples/do_once", do_once_example_test);

static Atomic<uint>    runonce_threadcount = 0;
static Mutex           runonce_mutex;
static Cond            runonce_cond;
static volatile size_t runonce_value = 0;

static void
runonce_thread (Atomic<uint> &runonce_counter)
{
  runonce_mutex.lock(); // syncronize
  runonce_mutex.unlock();
  do_once
    {
      runonce_mutex.lock();
      runonce_cond.broadcast();
      runonce_mutex.unlock();
      usleep (1); // sched_yield replacement to force contention
      ++runonce_counter;
      usleep (500); // sched_yield replacement to force contention
      runonce_value = 42;
    }
  TCMP (runonce_counter, ==, 1);
  TCMP (runonce_value, ==, 42);
  --runonce_threadcount;        // sinal thread end
  runonce_mutex.lock();
  runonce_cond.signal();
  runonce_mutex.unlock();
}

static void
test_runonce()
{
  const int count = 44;
  std::thread threads[count];
  Atomic<uint> runonce_counter = 0;
  runonce_threadcount.store (count);
  runonce_mutex.lock();
  for (int i = 0; i < count; i++)
    {
      threads[i] = std::thread (runonce_thread, std::ref (runonce_counter));
      TASSERT (threads[i].joinable());
    }
  TCMP (runonce_value, ==, 0);
  runonce_mutex.unlock(); // syncronized thread start
  runonce_mutex.lock();
  while (runonce_threadcount.load() > 0)
    {
      TOK();
      runonce_cond.wait (runonce_mutex);
    }
  runonce_mutex.unlock();
  for (int i = 0; i < count; i++)
    threads[i].join();
  TCMP (runonce_counter, ==, 1);
  TCMP (runonce_value, ==, 42);
}
REGISTER_TEST ("Threads/RunOnceTest", test_runonce);

// == basic mutex tests ==
static Mutex    static_mutex;
static Cond     static_cond;

static void
test_mutex (void)
{
  Mutex test_mutex;
  bool locked;
  // test stack mutex
  locked = test_mutex.try_lock();
  TASSERT (locked);
  locked = test_mutex.try_lock();
  TASSERT (!locked);
  test_mutex.unlock();
  // testing static_mutex
  locked = static_mutex.try_lock();
  TASSERT (locked);
  locked = static_mutex.try_lock();
  TASSERT (!locked);
  static_mutex.unlock();
  locked = static_mutex.try_lock();
  TASSERT (locked);
  static_mutex.unlock();
  // testing static_rec_mutex
  static Mutex static_rec_mutex (RECURSIVE_LOCK);
  locked = static_rec_mutex.try_lock();
  TASSERT (locked);
  static_rec_mutex.lock();
  locked = static_rec_mutex.try_lock();
  TASSERT (locked);
  static_rec_mutex.unlock();
  static_rec_mutex.unlock();
  static_rec_mutex.unlock();
  locked = static_rec_mutex.try_lock();
  TASSERT (locked);
  static_rec_mutex.unlock();
  // condition tests
  static_cond.signal();
  static_cond.broadcast();
  // testing function-static mutex
  static Mutex mutex;
  //static RecMutex rmutex;
  mutex.lock();
  //rmutex.lock();
  mutex.unlock();
  //rmutex.unlock();
}
REGISTER_TEST ("Threads/Basic Mutex", test_mutex);

// == simple spin lock test ==
static void
test_spin_lock_simple (void)
{
  Spinlock sp;
  bool l;
  l = sp.try_lock();
  TASSERT (l);
  l = sp.try_lock();
  TASSERT (!l);
  sp.unlock();
  l = sp.try_lock();
  TASSERT (l);
  l = sp.try_lock();
  TASSERT (!l);
  sp.unlock();
  sp.lock();
  l = sp.try_lock();
  TASSERT (!l);
  sp.unlock();
}
REGISTER_TEST ("Threads/C++SpinLock", test_spin_lock_simple);

// == Simple RWLock test ==
static RWLock static_read_write_lock;
static void
test_rwlock_simple (void)
{
  RWLock &rwl = static_read_write_lock;
  bool l;
  // try_rdlock + try_wrlock
  l = rwl.try_rdlock();
  TASSERT (l == true);
  l = rwl.try_wrlock();
  TASSERT (l == false);
  rwl.unlock();
  // rdlock + try_wrlock
  rwl.rdlock();
  l = rwl.try_wrlock();
  TASSERT (l == false);
  rwl.unlock();
  // try_wrlock
  l = rwl.try_wrlock();
  TASSERT (l == true);
  l = rwl.try_wrlock();
  TASSERT (l == false);
  rwl.unlock();
  // wrlock + try_wrlock
  rwl.wrlock();
  l = rwl.try_wrlock();
  TASSERT (l == false);
  rwl.unlock();
  // rdlock + try_wrlock
  rwl.rdlock();
  l = rwl.try_wrlock();
  TASSERT (l == false);
  rwl.unlock();
  // wrlock + try_rdlock
  rwl.wrlock();
  l = rwl.try_rdlock();
  TASSERT (l == false);
  rwl.unlock();
}
REGISTER_TEST ("Threads/C++RWLock", test_rwlock_simple);

// == ScopedLock test ==
template<class M> static bool
lockable (M &mutex)
{
  bool lockable = mutex.try_lock();
  if (lockable)
    mutex.unlock();
  return lockable;
}

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
      bool lockable1 = rec_mutex.try_lock();
      bool lockable2 = rec_mutex.try_lock();
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
    ScopedLock<Mutex> locker0 (mutex1, BALANCED_LOCK);
    TCMP (lockable (mutex1), ==, true);
    locker0.lock();
    TCMP (lockable (mutex1), ==, false);
    locker0.unlock();
    TCMP (lockable (mutex1), ==, true);
  }
  TCMP (lockable (mutex1), ==, true);
  {
    ScopedLock<Mutex> locker2 (mutex1, AUTOMATIC_LOCK);
    TCMP (lockable (mutex1), ==, false);
    locker2.unlock();
    TCMP (lockable (mutex1), ==, true);
    locker2.lock();
    TCMP (lockable (mutex1), ==, false);
  }
  TCMP (lockable (mutex1), ==, true);
  // test ScopedLock balancing unlock + lock
  mutex1.lock();
  {
    TCMP (lockable (mutex1), ==, false);
    ScopedLock<Mutex> locker (mutex1, BALANCED_LOCK);
    locker.unlock();
    TCMP (lockable (mutex1), ==, true);
  } // ~ScopedLock (BALANCED_LOCK) now does locker.lock()
  TCMP (lockable (mutex1), ==, false);
  {
    ScopedLock<Mutex> locker (mutex1, BALANCED_LOCK);
  } // ~ScopedLock (BALANCED_LOCK) now does nothing
  TCMP (lockable (mutex1), ==, false);
  mutex1.unlock();
  // test ScopedLock balancing lock + unlock
  {
    TCMP (lockable (mutex1), ==, true);
    ScopedLock<Mutex> locker (mutex1, BALANCED_LOCK);
    locker.lock();
    TCMP (lockable (mutex1), ==, false);
  } // ~ScopedLock (BALANCED_LOCK) now does locker.unlock()
  TCMP (lockable (mutex1), ==, true);
  {
    ScopedLock<Mutex> locker (mutex1, BALANCED_LOCK);
  } // ~ScopedLock (BALANCED_LOCK) now does nothing
  TCMP (lockable (mutex1), ==, true);
  // test ScopedLock with recursive mutex
  Mutex rmutex (RECURSIVE_LOCK);
  test_recursive_scoped_lock (rmutex, 999);
}
REGISTER_TEST ("Threads/Scoped Locks", test_scoped_locks);

// == C++ atomicity tests ==
static void
test_thread_atomic_cxx (void)
{
  /* integer functions */
  Atomic<int> ai;
  int r;
  ai.store (17);
  TCMP (ai, ==, 17);
  r = ai;
  TCMP (r, ==, 17);
  ai += 9;
  r = ai.load();
  TCMP (r, ==, 26);
  ai = -1147483648;
  TCMP (ai, ==, -1147483648);
  r = ai.load();
  TCMP (r, ==, -1147483648);
  ai += 9;
  r = ai;
  TCMP (r, ==, -1147483639);
  ai += -20;
  r = ai.load();
  TCMP (r, ==, -1147483659);
  r = ai.cas (17, 19);
  TCMP (r, ==, false);
  r = ai.load();
  TCMP (r, ==, -1147483659);
  r = ai.cas (-1147483659, 19);
  TCMP (r, ==, true);
  r = ai.load();
  TCMP (r, ==, 19);
  r = ai++;
  TCMP (r, ==, 19); TCMP (ai, ==, 20);
  r = ai--;
  TCMP (r, ==, 20); TCMP (ai, ==, 19);
  r = ++ai;
  TCMP (r, ==, 20); TCMP (ai, ==, 20);
  r = --ai;
  TCMP (r, ==, 19); TCMP (ai, ==, 19);
  r = (ai += 1);
  TCMP (r, ==, 20);
  r = ai.load();
  TCMP (r, ==, 20);
  r = (ai -= 20);
  TCMP (r, ==, 0);
  r = ai.load();
  TCMP (r, ==, 0);
  /* pointer functions */
  Atomic<void*> ap;
  void *p;
  ap = (void*) 119;
  TCMP (ap, ==, (void*) 119);
  p = ap;
  TCMP (p, ==, (void*) 119);
  r = ap.cas ((void*) 17, (void*) -42);
  TCMP (r, ==, false);
  p = ap.load();
  TCMP (p, ==, (void*) 119);
  r = ap.cas ((void*) 119, (void*) 4294967279U);
  TCMP (r, ==, true);
  p = ap;
  TCMP (p, ==, (void*) 4294967279U);
}
REGISTER_TEST ("Threads/C++AtomicThreading", test_thread_atomic_cxx);

// == Atomic Ring Buffer

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

typedef AsyncRingBuffer<int> IntRingBuffer;
class IntSequence {
  uint32 accu;
public:
  explicit      IntSequence() : accu (123456789) {}
  inline int32  gen_int    () { accu = 1664525 * accu + 1013904223; return accu; }
};
#define CONTENTION_PRINTF       while (0) printerr

struct RingBufferWriter : public IntSequence {
  IntRingBuffer *ring;
  uint64         total;
  uint           ring_buffer_test_length;
  RingBufferWriter (IntRingBuffer *rb,
                    uint           rbtl) :
    ring (rb), total (0), ring_buffer_test_length (rbtl)
  {
    ThreadInfo::self().name ("RingBufferWriter");
  }
  void
  run()
  {
    TINFO ("%s start.", ThisThread::name().c_str());
    for (uint l = 0; l < ring_buffer_test_length;)
      {
        uint k, n = Test::rand_int() % MIN (ring_buffer_test_length - l + 1, 65536 * 2);
        int buffer[n], *b = buffer;
        for (uint i = 0; i < n; i++)
          {
            b[i] = gen_int();
            total += b[i];
          }
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
    TINFO ("%s done (%lld).", ThisThread::name().c_str(), total);
  }
};

struct RingBufferReader : public IntSequence {
  IntRingBuffer *ring;
  uint64         total;
  uint           ring_buffer_test_length;
  RingBufferReader (IntRingBuffer *rb,
                    uint           rbtl) :
    ring (rb), total (0), ring_buffer_test_length (rbtl)
  {
    ThreadInfo::self().name ("RingBufferReader");
  }
  void
  run()
  {
    TINFO ("%s start.", ThisThread::name().c_str());
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
          {
            TCMP (b[i], ==, gen_int());
            total += b[i];
          }
        if (l / 499999 != (l + k) / 499999)
          TOK();
        l += k;
      }
    TINFO ("%s done (%lld).", ThisThread::name().c_str(), total);
  }
};

static void
test_ring_buffer ()
{
  static const char *testtext = "Ring Buffer test Text (47\xff)";
  uint n, ttl = strlen (testtext);
  AsyncRingBuffer<char> rb1 (ttl);
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

  // check miniscule ring buffer sizes (high contention test)
  for (uint step = 1; step < 8; step++)
    {
      uint ring_buffer_test_length = 17 * step + (rand() % 19);
      TSTART ("Threads/AsyncRingBuffer-%d-%d", step, ring_buffer_test_length);
      IntRingBuffer irb (step);
      RingBufferReader rbr (&irb, ring_buffer_test_length);
      RingBufferWriter rbw (&irb, ring_buffer_test_length);
      TASSERT (rbr.total == 0 && rbr.total == rbw.total);
      std::thread r_thread = std::thread (&RingBufferReader::run, std::ref (rbr));
      std::thread w_thread = std::thread (&RingBufferWriter::run, std::ref (rbw));
      r_thread.join();
      w_thread.join();
      TASSERT (rbr.total != 0 && rbr.total == rbw.total);
      TDONE();
    }

  // check big ring buffer sizes
  TSTART ("Threads/AsyncRingBuffer-big");
  uint ring_buffer_test_length = 999999 * (Test::slow() ? 20 : 1);
  IntRingBuffer irb (16384 + (lrand48() % 8192));
  RingBufferReader rbr (&irb, ring_buffer_test_length);
  RingBufferWriter rbw (&irb, ring_buffer_test_length);
  TASSERT (rbr.total == 0 && rbr.total == rbw.total);
  std::thread r_thread = std::thread (&RingBufferReader::run, std::ref (rbr));
  std::thread w_thread = std::thread (&RingBufferWriter::run, std::ref (rbw));
  r_thread.join();
  w_thread.join();
  TASSERT (rbr.total != 0 && rbr.total == rbw.total);
}
REGISTER_TEST ("Threads/AsyncRingBuffer", test_ring_buffer);
REGISTER_SLOWTEST ("Threads/AsyncRingBuffer (slow)", test_ring_buffer);

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
    // not deleting this, due to stack allocation
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
