// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
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
 * This class is a thin wrapper around pthread_mutex_lock() and related functions.
 * This class supports static construction.
 */
class Mutex {
  pthread_mutex_t mutex_;
public:
  constexpr Mutex       () : mutex_ (PTHREAD_MUTEX_INITIALIZER) {}
  constexpr Mutex       (struct RECURSIVE_LOCK) : mutex_ (PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP) {}
  void      lock        ()      { pthread_mutex_lock (&mutex_); }
  void      unlock      ()      { pthread_mutex_unlock (&mutex_); }
  bool      try_lock    ()      { return 0 == pthread_mutex_trylock (&mutex_); }
  bool      debug_locked();
  typedef pthread_mutex_t* native_handle_type;
  native_handle_type native_handle() { return &mutex_; }
  /*ctor*/  Mutex       (const Mutex&) = delete;
  Mutex&    operator=   (const Mutex&) = delete;
};

/**
 * The Spinlock uses low-latency busy spinning to acquire locks.
 * This class is a thin wrapper around pthread_spin_lock() and related functions.
 * This class supports static construction.
 */
class Spinlock {
  pthread_spinlock_t spinlock_;
public:
  constexpr Spinlock    () : spinlock_ RAPICORN_SPINLOCK_INITIALIZER {}
  void      lock        ()      { pthread_spin_lock (&spinlock_); }
  void      unlock      ()      { pthread_spin_unlock (&spinlock_); }
  bool      try_lock    ()      { return 0 == pthread_spin_trylock (&spinlock_); }
  typedef pthread_spinlock_t* native_handle_type;
  native_handle_type native_handle() { return &spinlock_; }
  /*ctor*/  Spinlock    (const Spinlock&) = delete;
  Mutex&    operator=   (const Spinlock&) = delete;
};

/**
 * The RWLock allows multiple readers to simultaneously access a critical code section or one writer.
 * This class is a thin wrapper around pthread_rwlock_rdlock() and related functions.
 * This class supports static construction.
 */
class RWLock {
  pthread_rwlock_t rwlock_;
  char             initialized_;
  void             real_init ();
  inline void      fixinit   () { if (RAPICORN_UNLIKELY (!atomic_load (&initialized_))) real_init(); }
public:
  constexpr RWLock      () : rwlock_ (), initialized_ (0) {}
  void      rdlock      ()      { fixinit(); while (pthread_rwlock_rdlock (&rwlock_) == EAGAIN); }
  void      wrlock      ()      { fixinit(); pthread_rwlock_wrlock (&rwlock_); }
  void      unlock      ()      { fixinit(); pthread_rwlock_unlock (&rwlock_); }
  bool      try_rdlock  ()      { fixinit(); return 0 == pthread_rwlock_tryrdlock (&rwlock_); }
  bool      try_wrlock  ()      { fixinit(); return 0 == pthread_rwlock_trywrlock (&rwlock_); }
  typedef pthread_rwlock_t* native_handle_type;
  native_handle_type native_handle() { return &rwlock_; }
  /*dtor*/ ~RWLock      ()      { fixinit(); pthread_rwlock_destroy (&rwlock_); }
  /*ctor*/  RWLock      (const RWLock&) = delete;
  Mutex&    operator=   (const RWLock&) = delete;
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
  template<typename T> inline T    get_data    (DataKey<T> *key)         { tdl(); T d = data_list_.get (key); tdu(); return d; }
  template<typename T> inline void set_data    (DataKey<T> *key, T data) { tdl(); data_list_.set (key, data); tdu(); }
  template<typename T> inline void delete_data (DataKey<T> *key)         { tdl(); data_list_.del (key); tdu(); }
  template<typename T> inline T    swap_data   (DataKey<T> *key)         { tdl(); T d = data_list_.swap (key); tdu(); return d; }
  template<typename T> inline T    swap_data   (DataKey<T> *key, T data) { tdl(); T d = data_list_.swap (key, data); tdu(); return d; }
private:
  std::atomic<ThreadInfo*>    next;
  std::atomic<pthread_t>      pth_thread_id;
  char                        pad[RAPICORN_CACHE_LINE_ALIGNMENT - sizeof hp - sizeof next - sizeof pth_thread_id];
  String                      name_;
  Mutex                       data_mutex_;
  DataList                    data_list_;
  static ThreadInfo __thread *self_cached;
  /*ctor*/              ThreadInfo      ();
  /*ctor*/              ThreadInfo      (const ThreadInfo&) = delete;
  /*dtor*/             ~ThreadInfo      ();
  ThreadInfo&           operator=       (const ThreadInfo&) = delete;
  static void           destroy_specific(void *vdata);
  void                  reset_specific  ();
  void                  setup_specific  ();
  static ThreadInfo*    create          ();
  void                  tdl             () { data_mutex_.lock(); }
  void                  tdu             () { data_mutex_.unlock(); }
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
class ScopedLock {
  MUTEX         &mutex_;
  volatile int   count_;
  RAPICORN_CLASS_NON_COPYABLE (ScopedLock);
public:
  inline     ~ScopedLock () { while (count_ < 0) lock(); while (count_ > 0) unlock(); }
  inline void lock       () { mutex_.lock(); count_++; }
  inline void unlock     () { count_--; mutex_.unlock(); }
  inline      ScopedLock (MUTEX &mutex, struct AUTOMATIC_LOCK = AUTOMATIC_LOCK) : mutex_ (mutex), count_ (0) { lock(); }
  inline      ScopedLock (MUTEX &mutex, struct BALANCED_LOCK) : mutex_ (mutex), count_ (0) {}
};

/**
 * The Cond synchronization primitive is a thin wrapper around pthread_cond_wait().
 * This class supports static construction.
 */
class Cond {
  pthread_cond_t cond_;
  static struct timespec abstime (int64);
  /*ctor*/      Cond        (const Cond&) = delete;
  Cond&         operator=   (const Cond&) = delete;
public:
  constexpr     Cond        () : cond_ (PTHREAD_COND_INITIALIZER) {}
  /*dtor*/     ~Cond        ()  { pthread_cond_destroy (&cond_); }
  void          signal      ()  { pthread_cond_signal (&cond_); }
  void          broadcast   ()  { pthread_cond_broadcast (&cond_); }
  void          wait        (Mutex &m)  { pthread_cond_wait (&cond_, m.native_handle()); }
  void          wait_timed  (Mutex &m, int64 max_usecs)
  { struct timespec abs = abstime (max_usecs); pthread_cond_timedwait (&cond_, m.native_handle(), &abs); }
  typedef pthread_cond_t* native_handle_type;
  native_handle_type native_handle() { return &cond_; }
};

/// The ThisThread namespace provides functions for the current thread of execution.
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
 *  Example: @snippet tests/t203/more-basics-threads.cc do_once-EXAMPLE
 */
#define do_once                         RAPICORN_DO_ONCE

#endif  // RAPICORN_CONVENIENCE

/// Exclusive<> is a type wrapper that provides non-racy atomic access to a copyable @a Type.
template<class Type>
class Exclusive {
  Mutex     mutex_;
  Type     *data_;
  uint64    mem_[(sizeof (Type) + 7) / 8];
  void      setup_L   (const Type &data)        { if (data_) return; data_ = new (mem_) Type (data); }
  void      replace_L (const Type &data)        { if (!data_) setup_L (data); else *data_ = data; }
public:
  constexpr Exclusive () : mutex_(), data_ (0)  {}
  void      operator= (const Type &data)        { ScopedLock<Mutex> locker (mutex_); replace_L (data); }
  operator  Type      ()                        { ScopedLock<Mutex> locker (mutex_); if (!data_) setup_L (Type()); return *data_; }
  /*dtor*/ ~Exclusive ()                        { ScopedLock<Mutex> locker (mutex_); if (data_) data_->~Type(); }
};

// == AsyncBlockingQueue ==
/**
 * This is a thread-safe asyncronous queue which blocks in pop() until data is provided through push().
 */
template<class Value>
class AsyncBlockingQueue {
  Mutex            mutex_;
  Cond             cond_;
  std::list<Value> list_;
public:
  void  push    (const Value &v);
  Value pop     ();
  bool  pending ();
  void  swap    (std::list<Value> &list);
};

// == AsyncNotifyingQueue ==
/**
 * This is a thread-safe asyncronous queue which returns 0 from pop() until data is provided through push().
 */
template<class Value>
class AsyncNotifyingQueue {
  Mutex                 mutex_;
  std::function<void()> notifier_;
  std::list<Value>      list_;
public:
  void  push     (const Value &v);
  Value pop      (Value fallback = 0);
  bool  pending  ();
  void  swap     (std::list<Value> &list);
  void  notifier (const std::function<void()> &notifier);
};

// == AsyncRingBuffer ==
/**
 * This is a thread-safe lock-free ring buffer of fixed size.
 * This ring buffer is a single-producer, single-consumer ring buffer that uses atomic
 * non-blocking operations to synchronize between reader and writer. Calls to read()
 * will return 0 until data is provided through write(). Only a single single reader
 * and a single writer must access the ring buffer at any time.
 * The amount of writable elements is limited by the maximum size of the ring buffer,
 * as specified during construction. If the elements to be written exceed the available
 * space, partial writes are possible, depending on how write() was called.
 * Reading happens analogously, partial reads occur if the number of readable elements
 * passed into read() exceed the available number of elements in the ring buffer.
 */
template<typename T>
class AsyncRingBuffer {
  const uint    size_;
  volatile uint wmark_, rmark_;
  T            *buffer_;
  RAPICORN_CLASS_NON_COPYABLE (AsyncRingBuffer);
public:
  explicit      AsyncRingBuffer (uint buffer_size);     ///< Construct ring buffer with the maximum available buffer size.
  /*dtor*/     ~AsyncRingBuffer ();                     ///< Deletes all resources, no asynchronous access must occour at this point.
  uint          n_readable      () const;               ///< Number elements that can currently be read from ring buffer.
  uint          n_writable      () const;               ///< Number of elements that can currently be written to ring buffer.
  uint          read            (uint length, T *data, bool partial = true);       ///< Read (possibly partial) data from ring buffer.
  uint          write           (uint length, const T *data, bool partial = true); ///< Write (possibly partial) data to ring buffer.
};

// == Implementation Bits ==
template<typename T>
AsyncRingBuffer<T>::AsyncRingBuffer (uint buffer_size) :
  size_ (buffer_size + 1), wmark_ (0), rmark_ (0), buffer_ (new T[size_])
{}

template<typename T>
AsyncRingBuffer<T>::~AsyncRingBuffer()
{
  T *old = buffer_;
  buffer_ = NULL;
  rmark_ = 0;
  wmark_ = 0;
  delete[] old;
  *const_cast<uint*> (&size_) = 0;
}

template<typename T> uint
AsyncRingBuffer<T>::n_writable() const
{
  const uint rm = atomic_load (&rmark_);
  const uint wm = atomic_load (&wmark_);
  const uint space = (size_ - 1 + rm - wm) % size_;
  return space;
}

template<typename T> uint
AsyncRingBuffer<T>::write (uint length, const T *data, bool partial)
{
  const uint orig_length = length;
  const uint rm = atomic_load (&rmark_);
  uint wm = atomic_load (&wmark_);
  uint space = (size_ - 1 + rm - wm) % size_;
  if (!partial && length > space)
    return 0;
  while (length)
    {
      if (rm <= wm)
        space = size_ - wm + (rm == 0 ? -1 : 0);
      else
        space = rm - wm -1;
      if (!space)
        break;
      space = MIN (space, length);
      std::copy (data, &data[space], &buffer_[wm]);
      wm = (wm + space) % size_;
      data += space;
      length -= space;
    }
  RAPICORN_SFENCE; // wmb ensures buffer_ writes are made visible before the wmark_ update
  atomic_store (&wmark_, wm);
  return orig_length - length;
}

template<typename T> uint
AsyncRingBuffer<T>::n_readable() const
{
  const uint wm = atomic_load (&wmark_);
  const uint rm = atomic_load (&rmark_);
  const uint space = (size_ + wm - rm) % size_;
  return space;
}

template<typename T> uint
AsyncRingBuffer<T>::read (uint length, T *data, bool partial)
{
  const uint orig_length = length;
  RAPICORN_LFENCE; // rmb ensures buffer_ contents are seen before wmark_ updates
  const uint wm = atomic_load (&wmark_);
  uint rm = atomic_load (&rmark_);
  uint space = (size_ + wm - rm) % size_;
  if (!partial && length > space)
    return 0;
  while (length)
    {
      if (wm < rm)
        space = size_ - rm;
      else
        space = wm - rm;
      if (!space)
        break;
      space = MIN (space, length);
      std::copy (&buffer_[rm], &buffer_[rm + space], data);
      rm = (rm + space) % size_;
      data += space;
      length -= space;
    }
  atomic_store (&rmark_, rm);
  return orig_length - length;
}

template<class Value> void
AsyncBlockingQueue<Value>::push (const Value &v)
{
  ScopedLock<Mutex> sl (mutex_);
  const bool notify = list_.empty();
  list_.push_back (v);
  if (RAPICORN_UNLIKELY (notify))
    cond_.broadcast();
}

template<class Value> Value
AsyncBlockingQueue<Value>::pop ()
{
  ScopedLock<Mutex> sl (mutex_);
  while (list_.empty())
    cond_.wait (mutex_);
  Value v = list_.front();
  list_.pop_front();
  return v;
}

template<class Value> bool
AsyncBlockingQueue<Value>::pending()
{
  ScopedLock<Mutex> sl (mutex_);
  return !list_.empty();
}

template<class Value> void
AsyncBlockingQueue<Value>::swap (std::list<Value> &list)
{
  ScopedLock<Mutex> sl (mutex_);
  const bool notify = list_.empty();
  list_.swap (list);
  if (notify && !list_.empty())
    cond_.broadcast();
}

template<class Value> void
AsyncNotifyingQueue<Value>::push (const Value &v)
{
  ScopedLock<Mutex> sl (mutex_);
  const bool notify = list_.empty();
  list_.push_back (v);
  if (RAPICORN_UNLIKELY (notify) && notifier_)
    notifier_();
}

template<class Value> Value
AsyncNotifyingQueue<Value>::pop (Value fallback)
{
  ScopedLock<Mutex> sl (mutex_);
  if (RAPICORN_UNLIKELY (list_.empty()))
    return fallback;
  Value v = list_.front();
  list_.pop_front();
  return v;
}

template<class Value> bool
AsyncNotifyingQueue<Value>::pending()
{
  ScopedLock<Mutex> sl (mutex_);
  return !list_.empty();
}

template<class Value> void
AsyncNotifyingQueue<Value>::swap (std::list<Value> &list)
{
  ScopedLock<Mutex> sl (mutex_);
  const bool notify = list_.empty();
  list_.swap (list);
  if (notify && !list_.empty() && notifier_)
    notifier_();
}

template<class Value> void
AsyncNotifyingQueue<Value>::notifier (const std::function<void()> &notifier)
{
  ScopedLock<Mutex> sl (mutex_);
  notifier_ = notifier;
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
