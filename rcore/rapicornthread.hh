// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_THREAD_XX_HH__
#define __RAPICORN_THREAD_XX_HH__

#include <rcore/utilities.hh>

namespace Rapicorn {

template<class Value> inline bool once_enter (volatile Value *value_location);
template<class Value> inline void once_leave (volatile Value *value_location,
                                              Value           initialization_value);
#define RAPICORN_ONCE_CONSTRUCT(pointer_variable)       \
  ({ typeof (pointer_variable) *___vp = &pointer_variable;      \
    while (once_enter (___vp)) { once_leave (___vp, new typeof (**___vp)); } *___vp; })

class Mutex : protected NonCopyable {
  RapicornMutex mutex;
  friend class Cond;
public:
  explicit      Mutex   ();
  void          lock    ();
  void          unlock  ();
  bool          trylock (); // TRUE indicates success
  /*Des*/       ~Mutex  ();
};

class Cond : protected NonCopyable {
  RapicornCond cond;
public:
  explicit      Cond          ();
  void          signal        ();
  void          broadcast     ();
  void          wait          (Mutex &m);
  void          wait_timed    (Mutex &m, int64 max_usecs);
  /*Des*/       ~Cond         ();
};

class SpinLock {
  volatile union {
    Mutex *fallback;                                    // union needs pointer alignment
    char   chars[RAPICORN_SIZEOF_PTHREADH_SPINLOCK];    // char may_alias any type
  } spinspace;
public:
  explicit SpinLock ();
  void     lock     ();
  void     unlock   ();
  bool     trylock  ();
  /*Des*/ ~SpinLock ();
};

namespace Atomic {
inline void    read_barrier  (void)                           { __sync_synchronize(); }
inline void    write_barrier (void)                           { __sync_synchronize(); }
inline void    full_barrier  (void)                           { __sync_synchronize(); }
/* atomic values */
template<class V> inline void value_set (volatile V *value_addr, V n)      { while (!__sync_bool_compare_and_swap (value_addr, *value_addr, n)); }
template<class V> inline V    value_get (volatile V *value_addr)           { return __sync_fetch_and_add (value_addr, 0); }
template<class V> inline bool value_cas (volatile V *value_addr, V o, V n) { return __sync_bool_compare_and_swap (value_addr, o, n); }
template<class V> inline V    value_add (volatile V *value_addr, V diff)   { return __sync_fetch_and_add (value_addr, diff); }
/* atomic numeric operations */
#define RAPICORN_ATOMIC_OPS(type) \
  inline void set (volatile type  *p, type v)         { return value_set (p, v); } \
  inline int  get (volatile type  *p)                 { return value_get (p); } \
  inline bool cas (volatile type  *p, type o, type n) { return value_cas (p, o, n); } \
  inline int  add (volatile type  *p, type d)         { return value_add (p, d); }
RAPICORN_ATOMIC_OPS (int);
RAPICORN_ATOMIC_OPS (uint);
RAPICORN_ATOMIC_OPS (int64);
RAPICORN_ATOMIC_OPS (uint64);
/* atomic pointer operations */
template<class V> inline void ptr_set (V* volatile *ptr_addr, V *n)      { while (!__sync_bool_compare_and_swap (ptr_addr, *ptr_addr, n)); }
template<class V> inline V*   ptr_get (V* volatile *ptr_addr)            { return __sync_fetch_and_add (ptr_addr, 0); }
template<class V> inline V*   ptr_get (V* volatile const *ptr_addr)      { return __sync_fetch_and_add (ptr_addr, 0); }
template<class V> inline bool ptr_cas (V* volatile *ptr_adr, V *o, V *n) { return __sync_bool_compare_and_swap (ptr_adr, o, n); }
// atomic stack, push-only
template<typename V> inline void stack_push        (V volatile &head, V &next, V newv) { stack_push<void*> ((void* volatile&) head, (void*&) next, (void*) newv); }
template<>           inline void stack_push<void*> (void* volatile &head, void* &next, void* vv) { do { next = head; } while (!Atomic::ptr_cas (&head, next, vv)); }
} // Atomic

namespace Thread {
  /* Self thread */
  struct Self {
    static bool         sleep           (long               max_useconds);
    static bool         aborted         ();
    static int          affinity        (int cpu = -1);
    static int          thread_pid      ();
    static void         yield           ();
    static void         exit            (void              *retval = NULL) RAPICORN_NORETURN;
  };
}

enum LockState { BALANCED = 0, AUTOLOCK = 1 };

/**
 * The ScopedLock class can lock a mutex on construction, and will automatically
 * unlock on destruction when the scope is left.
 * So putting a ScopedLock object on the stack conveniently ensures that its mutex
 * will be automatically locked and properly unlocked when the function returns or
 * throws an exception. Objects to be used by a ScopedLock need to provide the
 * public methods lock() and unlock().
 */
template<class MUTEX>
class ScopedLock : protected NonCopyable {
  MUTEX         &m_mutex;
  volatile int   m_count;
public:
  inline     ~ScopedLock () { while (m_count < 0) lock(); while (m_count > 0) unlock(); }
  inline void lock       () { m_mutex.lock(); m_count++; }
  inline void unlock     () { m_count--; m_mutex.unlock(); }
  inline      ScopedLock (MUTEX &mutex, LockState lockstate = AUTOLOCK) :
    m_mutex (mutex), m_count (0)
  { if (lockstate == AUTOLOCK) lock(); }
};

namespace Atomic {

template<typename T>
class RingBuffer : protected NonCopyable {
  const uint    m_size;
  T            *m_buffer;
  volatile uint m_wmark, m_rmark;
public:
  explicit
  RingBuffer (uint bsize) :
    m_size (bsize + 1), m_wmark (0), m_rmark (bsize)
  {
    m_buffer = new T[m_size];
    Atomic::set (&m_wmark, 0);
    Atomic::set (&m_rmark, 0);
  }
  ~RingBuffer()
  {
    // Atomic::set (&m_size, 0);
    Atomic::set (&m_rmark, 0);
    Atomic::set (&m_wmark, 0);
    delete[] m_buffer;
  }
  uint
  n_writable()
  {
    const uint rm = Atomic::get (&m_rmark);
    const uint wm = Atomic::get (&m_wmark);
    uint space = (m_size - 1 + rm - wm) % m_size;
    return space;
  }
  uint
  write (uint     length,
         const T *data,
         bool     partial = true)
  {
    const uint orig_length = length;
    const uint rm = Atomic::get (&m_rmark);
    uint wm = Atomic::get (&m_wmark);
    uint space = (m_size - 1 + rm - wm) % m_size;
    if (!partial && length > space)
      return 0;
    while (length)
      {
        if (rm <= wm)
          space = m_size - wm + (rm == 0 ? -1 : 0);
        else
          space = rm - wm -1;
        if (!space)
          break;
        space = MIN (space, length);
        std::copy (data, &data[space], &m_buffer[wm]);
        wm = (wm + space) % m_size;
        data += space;
        length -= space;
      }
    Atomic::write_barrier();
    /* the write barrier ensures m_buffer writes are made visible before the m_wmark update */
    Atomic::set (&m_wmark, wm);
    return orig_length - length;
  }
  uint
  n_readable()
  {
    const uint wm = Atomic::get (&m_wmark);
    const uint rm = Atomic::get (&m_rmark);
    uint space = (m_size + wm - rm) % m_size;
    return space;
  }
  uint
  read (uint length,
        T   *data,
        bool partial = true)
  {
    const uint orig_length = length;
    Atomic::read_barrier();
    /* the read barrier ensures m_buffer writes are seen before m_wmark updates */
    const uint wm = Atomic::get (&m_wmark);
    uint rm = Atomic::get (&m_rmark);
    uint space = (m_size + wm - rm) % m_size;
    if (!partial && length > space)
      return 0;
    while (length)
      {
        if (wm < rm)
          space = m_size - rm;
        else
          space = wm - rm;
        if (!space)
          break;
        space = MIN (space, length);
        std::copy (&m_buffer[rm], &m_buffer[rm + space], data);
        rm = (rm + space) % m_size;
        data += space;
        length -= space;
      }
    Atomic::set (&m_rmark, rm);
    return orig_length - length;
  }
};

} // Atomic

// == implementation ==
#ifdef RAPICORN_CONVENIENCE
#define ONCE_CONSTRUCT  RAPICORN_ONCE_CONSTRUCT
#endif // RAPICORN_CONVENIENCE

void once_list_enter  ();
bool once_list_bounce (volatile void *ptr);
bool once_list_leave  (volatile void *ptr);

template<class Value> inline bool
once_enter (volatile Value *value_location)
{
  if (RAPICORN_LIKELY (Atomic::value_get (value_location) != 0))
    return false;
  else
    {
      once_list_enter();
      const bool initialized = Atomic::value_get (value_location) != 0;
      const bool needs_init = once_list_bounce (initialized ? NULL : value_location);
      return needs_init;
    }
}

template<class Value> inline void
once_leave (volatile Value *value_location,
            Value           initialization_value)
{
  RAPICORN_RETURN_UNLESS (Atomic::value_get (value_location) == 0);
  RAPICORN_RETURN_UNLESS (initialization_value != 0);

  Atomic::value_set (value_location, initialization_value);
  const bool found_and_removed = once_list_leave (value_location);
  RAPICORN_RETURN_UNLESS (found_and_removed == true);
}

} // Rapicorn

#endif /* __RAPICORN_THREAD_XX_HH__ */

/* vim:set ts=8 sts=2 sw=2: */
