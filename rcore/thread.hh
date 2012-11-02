// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_THREAD_HH__
#define __RAPICORN_THREAD_HH__

#include <rcore/utilities.hh>
#include <rcore/threadlib.hh>
#include <thread>
#include <list>

namespace Rapicorn {

struct RECURSIVE_LOCK {} constexpr RECURSIVE_LOCK {}; ///< Flag for recursive Mutex initialization.

/**
 * The Mutex synchronization primitive is a thin wrapper around std::mutex.
 * This class supports static construction.
 */
class Mutex {
  pthread_mutex_t m_mutex;
public:
  constexpr Mutex       () : m_mutex (PTHREAD_MUTEX_INITIALIZER) {}
  constexpr Mutex       (struct RECURSIVE_LOCK) : m_mutex (PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP) {}
  void      lock        ()      { pthread_mutex_lock (&m_mutex); }
  void      unlock      ()      { pthread_mutex_unlock (&m_mutex); }
  bool      try_lock    ()      { return 0 == pthread_mutex_trylock (&m_mutex); }
  bool      debug_locked();
  typedef pthread_mutex_t* native_handle_type;
  native_handle_type native_handle() { return &m_mutex; }
  /*ctor*/  Mutex       (const Mutex&) = delete;
  Mutex&    operator=   (const Mutex&) = delete;
};

/**
 * The Spinlock uses low-latency busy spinning to acquire locks.
 * It is a thin wrapper around pthread_spin_lock().
 * This class supports static construction.
 */
class Spinlock {
  pthread_spinlock_t m_spinlock;
public:
  constexpr Spinlock    () : m_spinlock RAPICORN_SPINLOCK_INITIALIZER {}
  void      lock        ()      { pthread_spin_lock (&m_spinlock); }
  void      unlock      ()      { pthread_spin_unlock (&m_spinlock); }
  bool      try_lock    ()      { return 0 == pthread_spin_trylock (&m_spinlock); }
  typedef pthread_spinlock_t* native_handle_type;
  native_handle_type native_handle() { return &m_spinlock; }
  /*ctor*/  Spinlock    (const Spinlock&) = delete;
  Mutex&    operator=   (const Spinlock&) = delete;
};

/// Class keeping information per Thread.
struct ThreadInfo {
  /// @name Hazard Pointers
  typedef std::vector<void*> VoidPointers;
  void *volatile      hp[8];   ///< Hazard pointers variables, see: http://www.research.ibm.com/people/m/michael/ieeetpds-2004.pdf .
  static VoidPointers collect_hazards (); ///< Collect hazard pointers from all threads. Returns sorted vector of unique elements.
  static inline bool  lookup_pointer  (const std::vector<void*> &ptrs, void *arg); ///< Lookup pointers in a hazard pointer vector.
  /// @name Thread identification
  String                    ident       ();                             ///< Simple identifier for this thread, usually TID/PID.
  String                    name        ();                             ///< Get thread name.
  void                      name        (const String &newname);        ///< Change thread name.
  static inline ThreadInfo& self        ();  ///< Get ThreadInfo for the current thread, inlined, using fast thread local storage.
  /** @name Accessing custom data members
   * For further details, see DataListContainer.
   */
  template<typename T> inline T    get_data    (DataKey<T> *key)         { tdl(); T d = m_data_list.get (key); tdu(); return d; }
  template<typename T> inline void set_data    (DataKey<T> *key, T data) { tdl(); m_data_list.set (key, data); tdu(); }
  template<typename T> inline void delete_data (DataKey<T> *key)         { tdl(); m_data_list.del (key); tdu(); }
  template<typename T> inline T    swap_data   (DataKey<T> *key)         { tdl(); T d = m_data_list.swap (key); tdu(); return d; }
  template<typename T> inline T    swap_data   (DataKey<T> *key, T data) { tdl(); T d = m_data_list.swap (key, data); tdu(); return d; }
private:
  ThreadInfo        *volatile next;
  pthread_t                   pth_thread_id;
  char                        pad[RAPICORN_CACHE_LINE_ALIGNMENT - sizeof hp - sizeof next - sizeof pth_thread_id];
  String                      m_name;
  Mutex                       m_data_mutex;
  DataList                    m_data_list;
  static ThreadInfo __thread *self_cached;
  /*ctor*/              ThreadInfo      ();
  /*ctor*/              ThreadInfo      (const ThreadInfo&) = delete;
  /*dtor*/             ~ThreadInfo      ();
  ThreadInfo&           operator=       (const ThreadInfo&) = delete;
  static void           destroy_specific(void *vdata);
  void                  reset_specific  ();
  void                  setup_specific  ();
  static ThreadInfo*    create          ();
  void                  tdl             () { m_data_mutex.lock(); }
  void                  tdu             () { m_data_mutex.unlock(); }
};

struct AUTOMATIC_LOCK {} constexpr AUTOMATIC_LOCK {}; ///< Flag for automatic locking of a ScopedLock<Mutex>.
struct BALANCED_LOCK  {} constexpr BALANCED_LOCK  {}; ///< Flag for balancing unlock/lock in a ScopedLock<Mutex>.

/**
 * The ScopedLock is a scope based lock ownership wrapper.
 * Placing a ScopedLock object on the stack conveniently ensures that its mutex
 * will be automatically locked and properly unlocked when the scope is left,
 * the current function returns or throws an exception. Mutex obbjects to be used
 * by a ScopedLock need to provide the public methods lock() and unlock().
 * In AUTOMATIC_LOCK mode, the owned mutex is automatically locked upon construction
 * and unlocked upon destruction. Intermediate calls to unlock() and lock() on the
 * ScopedLock will be accounted for in the destructor.
 * In BALANCED_LOCK mode, the lock is not automatically acquired upon construction,
 * however the destructor will balance all intermediate unlock() and lock() calls.
 * So this mode can be used to manage ownership for an already locked mutex.
 */
template<class MUTEX>
class ScopedLock : protected NonCopyable {
  MUTEX         &m_mutex;
  volatile int   m_count;
public:
  inline     ~ScopedLock () { while (m_count < 0) lock(); while (m_count > 0) unlock(); }
  inline void lock       () { m_mutex.lock(); m_count++; }
  inline void unlock     () { m_count--; m_mutex.unlock(); }
  inline      ScopedLock (MUTEX &mutex, struct AUTOMATIC_LOCK = AUTOMATIC_LOCK) : m_mutex (mutex), m_count (0) { lock(); }
  inline      ScopedLock (MUTEX &mutex, struct BALANCED_LOCK) : m_mutex (mutex), m_count (0) {}
};

/**
 * The Cond synchronization primitive is a thin wrapper around pthread_cond_wait().
 * This class supports static construction.
 */
class Cond {
  pthread_cond_t m_cond;
  static struct timespec abstime (int64);
  /*ctor*/      Cond        (const Cond&) = delete;
  Cond&         operator=   (const Cond&) = delete;
public:
  constexpr     Cond        () : m_cond (PTHREAD_COND_INITIALIZER) {}
  /*dtor*/     ~Cond        ()  { pthread_cond_destroy (&m_cond); }
  void          signal      ()  { pthread_cond_signal (&m_cond); }
  void          broadcast   ()  { pthread_cond_broadcast (&m_cond); }
  void          wait        (Mutex &m)  { pthread_cond_wait (&m_cond, m.native_handle()); }
  void          wait_timed  (Mutex &m, int64 max_usecs)
  { struct timespec abs = abstime (max_usecs); pthread_cond_timedwait (&m_cond, m.native_handle(), &abs); }
  typedef pthread_cond_t* native_handle_type;
  native_handle_type native_handle() { return &m_cond; }
};

/// @namespace Rapicorn::ThisThread The Rapicorn::ThisThread namespace provides functions for the current thread of execution.
namespace ThisThread {

String  name            ();             ///< Get thread name.
int     online_cpus     ();             ///< Get the number of available CPUs.
int     affinity        ();             ///< Get the current CPU affinity.
void    affinity        (int cpu);      ///< Set the current CPU affinity.
int     thread_pid      ();             ///< Get the current threads's thread ID (TID). For further details, see gettid().
int     process_pid     ();             ///< Get the process ID (PID). For further details, see getpid().

#ifdef  DOXYGEN // parts reused from std::this_thread
/// Relinquish the processor to allow execution of other threads. For further details, see std::this_thread::yield().
void                                       yield       ();
/// Returns the pthread_t id for the current thread. For further details, see std::this_thread::get_id().
std::thread::id                            get_id      ();
/// Sleep for @a sleep_duration has been reached. For further details, see std::this_thread::sleep_for().
template<class Rep, class Period>     void sleep_for   (std::chrono::duration<Rep,Period> sleep_duration);
/// Sleep until @a sleep_time has been reached. For further details, see std::this_thread::sleep_until().
template<class Clock, class Duration> void sleep_until (const std::chrono::time_point<Clock,Duration> &sleep_time);
#else // !DOXYGEN
using namespace std::this_thread;
#endif // !DOXYGEN

} // ThisThread

#ifdef RAPICORN_CONVENIENCE

/** The @e do_once statement preceeds code blocks to ensure that a critical section is executed atomically and at most once.
 *  Example: @SNIPPET{rcore/tests/threads.cc, do_once-EXAMPLE}
 */
#define do_once                         RAPICORN_DO_ONCE

#endif  // RAPICORN_CONVENIENCE

/// Atomic types are race free integer and pointer types, similar to std::atomic.
/// All atomic types support load(), store(), cas() and additional type specific accessors.
template<typename T> class Atomic;

/// Atomic char type.
template<> struct Atomic<char> : Lib::Atomic<char> {
  Atomic<char> (char i = 0) : Lib::Atomic<char> (i) {}
  using Lib::Atomic<char>::operator=;
};

/// Atomic int8 type.
template<> struct Atomic<int8> : Lib::Atomic<int8> {
  Atomic<int8> (int8 i = 0) : Lib::Atomic<int8> (i) {}
  using Lib::Atomic<int8>::operator=;
};

/// Atomic uint8 type.
template<> struct Atomic<uint8> : Lib::Atomic<uint8> {
  Atomic<uint8> (uint8 i = 0) : Lib::Atomic<uint8> (i) {}
  using Lib::Atomic<uint8>::operator=;
};

/// Atomic int32 type.
template<> struct Atomic<int32> : Lib::Atomic<int32> {
  Atomic<int32> (int32 i = 0) : Lib::Atomic<int32> (i) {}
  using Lib::Atomic<int32>::operator=;
};

/// Atomic uint32 type.
template<> struct Atomic<uint32> : Lib::Atomic<uint32> {
  Atomic<uint32> (uint32 i = 0) : Lib::Atomic<uint32> (i) {}
  using Lib::Atomic<uint32>::operator=;
};

/// Atomic int64 type.
template<> struct Atomic<int64> : Lib::Atomic<int64> {
  Atomic<int64> (int64 i = 0) : Lib::Atomic<int64> (i) {}
  using Lib::Atomic<int64>::operator=;
};

/// Atomic uint64 type.
template<> struct Atomic<uint64> : Lib::Atomic<uint64> {
  Atomic<uint64> (uint64 i = 0) : Lib::Atomic<uint64> (i) {}
  using Lib::Atomic<uint64>::operator=;
};

/// Atomic pointer type.
template<typename V> class Atomic<V*> : protected Lib::Atomic<V*> {
  typedef Lib::Atomic<V*> A;
public:
  constexpr Atomic    (V *p = nullptr) : A (p) {}
  using A::store;
  using A::load;
  using A::cas;
  using A::operator=;
  V*       operator+= (ptrdiff_t d) volatile { return A::operator+= ((V*) d); }
  V*       operator-= (ptrdiff_t d) volatile { return A::operator-= ((V*) d); }
  operator V* () const volatile { return load(); }
  void     push_link (V*volatile *nextp, V *newv) { do { *nextp = load(); } while (!cas (*nextp, newv)); }
};

// == AsyncBlockingQueue ==
/**
 * This is a thread-safe asyncronous queue which blocks in pop() until data is provided through push().
 */
template<class Value>
class AsyncBlockingQueue {
  Mutex            m_mutex;
  Cond             m_cond;
  std::list<Value> m_list;
public:
  void  push (const Value &v);
  Value pop  ();
  void  swap (std::list<Value> &list);
};

// == Implementation Bits ==
template<class Value> void
AsyncBlockingQueue<Value>::push (const Value &v)
{
  ScopedLock<Mutex> sl (m_mutex);
  const bool notify = m_list.empty();
  m_list.push_back (v);
  if (RAPICORN_UNLIKELY (notify))
    m_cond.broadcast();
}

template<class Value> Value
AsyncBlockingQueue<Value>::pop ()
{
  ScopedLock<Mutex> sl (m_mutex);
  while (m_list.empty())
    m_cond.wait (m_mutex);
  Value v = m_list.front();
  m_list.pop_front();
  return v;
}

template<class Value> void
AsyncBlockingQueue<Value>::swap (std::list<Value> &list)
{
  ScopedLock<Mutex> sl (m_mutex);
  const bool notify = m_list.empty();
  m_list.swap (list);
  if (notify && !m_list.empty())
    m_cond.broadcast();
}

inline ThreadInfo&
ThreadInfo::self()
{
  if (RAPICORN_UNLIKELY (!self_cached))
    self_cached = create();
  return *self_cached;
}

inline bool
ThreadInfo::lookup_pointer (const std::vector<void*> &ptrs, void *arg)
{
  size_t n_elements = ptrs.size(), offs = 0;
  while (offs < n_elements)
    {
      size_t i = (offs + n_elements) >> 1;
      void *current = ptrs[i];
      if (arg == current)
        return true;    // match
      else if (arg < current)
        n_elements = i;
      else // (arg > current)
        offs = i + 1;
    }
  return false; // unmatched
}

} // Rapicorn

#endif // __RAPICORN_THREAD_HH__
