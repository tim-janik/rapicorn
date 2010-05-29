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
#ifndef __RAPICORN_THREAD_XX_HH__
#define __RAPICORN_THREAD_XX_HH__

#include <rcore/rapicornutils.hh>

namespace Rapicorn {

class Thread;

inline bool once_enter      (volatile size_t *value_location);
bool        once_enter_impl (volatile size_t *value_location);
void        once_leave      (volatile size_t *value_location,
                             size_t           initialization_value);

class Mutex {
  RapicornMutex mutex;
  friend class Cond;
  RAPICORN_PRIVATE_CLASS_COPY (Mutex);
public:
  explicit      Mutex   ();
  void          lock    ();
  void          unlock  ();
  bool          trylock (); // TRUE indicates success
  /*Des*/       ~Mutex  ();
};

class RecMutex {
  RapicornRecMutex rmutex;
  RAPICORN_PRIVATE_CLASS_COPY (RecMutex);
public:
  explicit      RecMutex  ();
  void          lock      ();
  void          unlock    ();
  bool          trylock   ();
  /*Des*/       ~RecMutex ();
};

class Cond {
  RapicornCond cond;
  RAPICORN_PRIVATE_CLASS_COPY (Cond);
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
/* atomic integers */
inline void    int_set  (volatile int  *iptr, int value)      { *iptr = value; write_barrier(); }
inline int     int_get  (volatile int  *iptr)                 { return __sync_fetch_and_add (iptr, 0); }
inline bool    int_cas  (volatile int  *iptr, int o, int n)   { return __sync_bool_compare_and_swap (iptr, o, n); }
inline int     int_add  (volatile int  *iptr, int diff)       { return __sync_fetch_and_add (iptr, diff); }
/* atomic unsigned integers */
inline void    uint_set (volatile uint *uptr, uint value)     { *uptr = value; write_barrier(); }
inline uint    uint_get (volatile uint *uptr)                 { return __sync_fetch_and_add (uptr, 0); }
inline bool    uint_cas (volatile uint *uptr, uint o, uint n) { return __sync_bool_compare_and_swap (uptr, o, n); }
inline uint    uint_add (volatile uint *uptr, uint diff)      { return __sync_fetch_and_add (uptr, diff); }
/* atomic size_t */
inline void    sizet_set (volatile size_t *sptr, size_t value)       { *sptr = value; write_barrier(); }
inline uint    sizet_get (volatile size_t *sptr)                     { return __sync_fetch_and_add (sptr, 0); }
inline bool    sizet_cas (volatile size_t *sptr, size_t o, size_t n) { return __sync_bool_compare_and_swap (sptr, o, n); }
inline uint    sizet_add (volatile size_t *sptr, size_t diff)        { return __sync_fetch_and_add (sptr, diff); }
/* atomic pointers */
template<class V>
inline void    ptr_set       (V* volatile *ptr_addr, V *n)      { *ptr_addr = n; write_barrier(); }
template<class V>
inline V*      ptr_get       (V* volatile *ptr_addr)            { return __sync_fetch_and_add (ptr_addr, 0); }
template<class V>
inline V*      ptr_get       (V* volatile const *ptr_addr)      { return __sync_fetch_and_add (ptr_addr, 0); }
template<class V>
inline bool    ptr_cas       (V* volatile *ptr_adr, V *o, V *n) { return __sync_bool_compare_and_swap (ptr_adr, o, n); }
} // Atomic

class OwnedMutex {
  RecMutex m_rec_mutex;
  Thread * volatile m_owner;
  uint     volatile m_count;
  RAPICORN_PRIVATE_CLASS_COPY (OwnedMutex);
public:
  explicit       OwnedMutex ();
  inline void    lock       ();
  inline bool    trylock    ();
  inline void    unlock     ();
  inline Thread* owner      ();
  inline bool    mine       ();
  /*Des*/       ~OwnedMutex ();
};

class Thread : public virtual BaseObject {
protected:
  explicit              Thread          (const String      &name);
  virtual void          run             () = 0;
  virtual               ~Thread         ();
public:
  void                  start           ();
  int                   pid             () const;
  String                name            () const;
  void                  queue_abort     ();
  void                  abort           ();
  bool                  aborted         ();
  void                  wakeup          ();
  bool                  running         ();
  void                  wait_for_exit   ();
  int                   last_affinity   () const;
  int                   affinity        (int cpu = -1);
  /* event loop */
  void                  exec_loop       ();
  void                  quit_loop       ();
  /* global methods */
  static void           emit_wakeups    (uint64             stamp);
  static Thread&        self            ();
  static int            online_cpus     ();
  /* Self thread */
  struct Self {
    static String       name            ();
    static void         name            (const String      &name);
    static bool         sleep           (long               max_useconds);
    static bool         aborted         ();
    static int          affinity        (int cpu = -1);
    static int          pid             ();
    static void         awake_after     (uint64             stamp);
    static void         set_wakeup      (RapicornThreadWakeup wakeup_func,
                                         void              *wakeup_data,
                                         void             (*destroy_data) (void*));
    static OwnedMutex&  owned_mutex     ();
    static void         yield           ();
    static void         exit            (void              *retval = NULL) RAPICORN_NORETURN;
  };
  /* DataListContainer API */
  template<typename Type> inline void set_data    (DataKey<Type> *key,
                                                   Type           data) { thread_lock(); data_list.set (key, data); thread_unlock(); }
  template<typename Type> inline Type get_data    (DataKey<Type> *key)  { thread_lock(); Type d = data_list.get (key); thread_unlock(); return d; }
  template<typename Type> inline Type swap_data   (DataKey<Type> *key)  { thread_lock(); Type d = data_list.swap (key); thread_unlock(); return d; }
  template<typename Type> inline Type swap_data   (DataKey<Type> *key,
                                                   Type           data) { thread_lock(); Type d = data_list.swap (key, data); thread_unlock(); return d; }
  template<typename Type> inline void delete_data (DataKey<Type> *key)  { thread_lock(); data_list.del (key); thread_unlock(); }
  /* implementaiton details */
private:
  DataList              data_list;
  RapicornThread       *bthread;
  OwnedMutex            m_omutex;
  int                   last_cpu;
  explicit              Thread          (RapicornThread      *thread);
  void                  thread_lock     ()                              { m_omutex.lock(); }
  bool                  thread_trylock  ()                              { return m_omutex.trylock(); }
  void                  thread_unlock   ()                              { m_omutex.unlock(); }
  RAPICORN_PRIVATE_CLASS_COPY (Thread);
protected:
  class ThreadWrapperInternal;
  static void threadxx_wrap   (RapicornThread *cthread);
  static void threadxx_delete (void         *cxxthread);
};

/**
 * The ScopedLock class can lock a mutex on construction, and will automatically
 * unlock on destruction when the scope is left.
 * So putting a ScopedLock object on the stack conveniently ensures that its mutex
 * will be automatically locked and properly unlocked when the function returns or
 * throws an exception. Objects to be used by a ScopedLock need to provide the
 * public methods lock() and unlock().
 */
template<class MUTEX>
class ScopedLock {
  MUTEX         &m_mutex;
  volatile uint  m_count;
  RAPICORN_PRIVATE_CLASS_COPY (ScopedLock);
public:
  inline     ~ScopedLock () { while (m_count) unlock(); }
  inline void lock       () { m_mutex.lock(); m_count++; }
  inline void unlock     () { RAPICORN_ASSERT (m_count > 0); m_count--; m_mutex.unlock(); }
  inline      ScopedLock (MUTEX &mutex, bool initlocked = true) :
    m_mutex (mutex), m_count (0)
  { if (initlocked) lock(); }
};

namespace Atomic {

template<typename T>
class RingBuffer {
  const uint    m_size;
  T            *m_buffer;
  volatile uint m_wmark, m_rmark;
  RAPICORN_PRIVATE_CLASS_COPY (RingBuffer);
public:
  explicit
  RingBuffer (uint bsize) :
    m_size (bsize + 1), m_wmark (0), m_rmark (bsize)
  {
    m_buffer = new T[m_size];
    Atomic::uint_set (&m_wmark, 0);
    Atomic::uint_set (&m_rmark, 0);
  }
  ~RingBuffer()
  {
    Atomic::uint_set ((volatile uint*) &m_size, 0);
    Atomic::uint_set (&m_rmark, 0);
    Atomic::uint_set (&m_wmark, 0);
    delete[] m_buffer;
  }
  uint
  n_writable()
  {
    const uint rm = Atomic::uint_get (&m_rmark);
    const uint wm = Atomic::uint_get (&m_wmark);
    uint space = (m_size - 1 + rm - wm) % m_size;
    return space;
  }
  uint
  write (uint     length,
         const T *data,
         bool     partial = true)
  {
    const uint orig_length = length;
    const uint rm = Atomic::uint_get (&m_rmark);
    uint wm = Atomic::uint_get (&m_wmark);
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
    /* the barrier ensures m_buffer writes are seen before the m_wmark update */
    Atomic::uint_set (&m_wmark, wm);
    return orig_length - length;
  }
  uint
  n_readable()
  {
    const uint wm = Atomic::uint_get (&m_wmark);
    const uint rm = Atomic::uint_get (&m_rmark);
    uint space = (m_size + wm - rm) % m_size;
    return space;
  }
  uint
  read (uint length,
        T   *data,
        bool partial = true)
  {
    const uint orig_length = length;
    /* need Atomic::read_barrier() here to ensure m_buffer writes are seen before m_wmark updates */
    const uint wm = Atomic::uint_get (&m_wmark); /* includes Atomic::read_barrier(); */
    uint rm = Atomic::uint_get (&m_rmark);
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
    Atomic::uint_set (&m_rmark, rm);
    return orig_length - length;
  }
};

} // Atomic

/* --- implementation --- */
inline void
OwnedMutex::lock ()
{
  m_rec_mutex.lock();
  Atomic::ptr_set (&m_owner, &Thread::self());
  m_count++;
}

inline bool
OwnedMutex::trylock ()
{
  if (m_rec_mutex.trylock())
    {
      Atomic::ptr_set (&m_owner, &Thread::self());
      m_count++;
      return true; /* TRUE indicates success */
    }
  else
    return false;
}

inline void
OwnedMutex::unlock ()
{
  if (--m_count == 0)
    Atomic::ptr_set (&m_owner, (Thread*) 0);
  m_rec_mutex.unlock();
}

inline Thread*
OwnedMutex::owner ()
{
  return Atomic::ptr_get (&m_owner);
}

inline bool
OwnedMutex::mine ()
{
  return Atomic::ptr_get (&m_owner) == &Thread::self();
}

inline bool
once_enter (volatile size_t *value_location)
{
  if (RAPICORN_LIKELY (Atomic::sizet_get (value_location) != 0))
    return false;
  else
    return once_enter_impl (value_location);
}

} // Rapicorn

#endif /* __RAPICORN_THREAD_XX_HH__ */

/* vim:set ts=8 sts=2 sw=2: */
