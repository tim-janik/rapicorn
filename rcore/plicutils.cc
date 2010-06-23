/* Plic Binding utilities
 * Copyright (C) 2010 Tim Janik
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
#include "plicutils.hh"
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <sched.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdexcept>
#include <map>
#include <set>
#if __GLIBC__ == 2 && __GLIBC_MINOR__ >= 10
#  include <sys/eventfd.h>
#endif

/* === Auxillary macros === */
#ifndef __GNUC__
#define __PRETTY_FUNCTION__                     __func__
#endif
#define PLIC_CPP_PASTE2i(a,b)                   a ## b // indirection required to expand __LINE__ etc
#define PLIC_CPP_PASTE2(a,b)                    PLIC_CPP_PASTE2i (a,b)
#define PLIC_STATIC_ASSERT_NAMED(expr,asname)   typedef struct { char asname[(expr) ? 1 : -1]; } PLIC_CPP_PASTE2 (Plic_StaticAssertion_LINE, __LINE__)
#define PLIC_STATIC_ASSERT(expr)                PLIC_STATIC_ASSERT_NAMED (expr, compile_time_assertion_failed)
#define ALIGN4(sz,unit)                         (sizeof (unit) * ((sz + sizeof (unit) - 1) / sizeof (unit)))
#define PLIC_THROW_IF_FAIL(expr)                do { if (expr) break; PLIC_THROW ("failed to assert (" + #expr + ")"); } while (0)
#define PLIC_THROW(msg)                         throw std::runtime_error (std::string() + __PRETTY_FUNCTION__ + ": " + msg)

namespace Plic {

/* === Prototypes === */
static String string_printf (const char *format, ...) PLIC_PRINTF (1, 2);

/* === Utilities === */
static String // FIXME: support error format
string_printf (const char *format, ...)
{
  String str;
  va_list args;
  va_start (args, format);
  char buffer[512 + 1];
  vsnprintf (buffer, 512, format, args);
  va_end (args);
  buffer[512] = 0; // force termination
  return buffer;
}

static void
printerr (const char *format, ...)
{
  String str;
  va_list args;
  va_start (args, format);
  char buffer[512 + 1];
  vsnprintf (buffer, 512, format, args);
  va_end (args);
  buffer[512] = 0; // force termination
  size_t l = write (2, buffer, strlen (buffer));
  (void) l;
}

/* === EventFd === */
EventFd::EventFd ()
{
  fds[0] = -1;
  fds[1] = -1;
}

int
EventFd::open ()
{
  if (fds[0] >= 0)
    return 0;
  int err;
  long nflags;
#ifdef EFD_SEMAPHORE
  do
    fds[0] = eventfd (0 /*initval*/, 0 /*flags*/);
  while (fds[0] < 0 && (errno == EAGAIN || errno == EINTR));
#else
  do
    err = pipe (fds);
  while (err < 0 && (errno == EAGAIN || errno == EINTR));
  if (fds[1] >= 0)
    {
      nflags = fcntl (fds[1], F_GETFL, 0);
      nflags |= O_NONBLOCK;
      do
        err = fcntl (fds[1], F_SETFL, nflags);
      while (err < 0 && (errno == EINTR || errno == EAGAIN));
    }
#endif
  if (fds[0] >= 0)
    {
      nflags = fcntl (fds[0], F_GETFL, 0);
      nflags |= O_NONBLOCK;
      do
        err = fcntl (fds[0], F_SETFL, nflags);
      while (err < 0 && (errno == EINTR || errno == EAGAIN));
      return 0;
    }
  return -errno;
}

void
EventFd::wakeup () // wakeup polling end
{
  int err;
#ifdef EFD_SEMAPHORE
  do
    err = eventfd_write (fds[0], 1);
  while (err < 0 && errno == EINTR);
#else
  char w = 'w';
  do
    err = write (fds[1], &w, 1);
  while (err < 0 && errno == EINTR);
#endif
  // EAGAIN occours if too many wakeups are pending
}

int
EventFd::inputfd () // fd for POLLIN
{
  return fds[0];
}

void
EventFd::flush () // clear pending wakeups
{
  int err;
#ifdef EFD_SEMAPHORE
  eventfd_t bytes8;
  do
    err = eventfd_read (fds[0], &bytes8);
  while (err < 0 && errno == EINTR);
#else
  char buffer[512]; // 512 is posix pipe atomic read/write size
  do
    err = read (fds[0], buffer, sizeof (buffer));
  while (err < 0 && errno == EINTR);
#endif
  // EAGAIN occours if no wakeups are pending
}

EventFd::~EventFd ()
{
#ifdef EFD_SEMAPHORE
  close (fds[0]);
#else
  close (fds[0]);
  close (fds[1]);
#endif
}

/* === SmartHandle === */
struct SmartHandle0 : public SmartHandle {
  explicit SmartHandle0 () : SmartHandle () {}
};
const SmartHandle &SmartHandle::None = SmartHandle0();

SmartHandle::SmartHandle () :
  m_rpc_id (0)
{}

void
SmartHandle::_reset ()
{
  m_rpc_id = 0;
}

void
SmartHandle::_pop_rpc (Coupler           &c,
                       FieldBufferReader &fbr)
{
  _reset();
  m_rpc_id = c.pop_rpc_handle (fbr);
}

void*
SmartHandle::_cast_iface () const
{
  // unoptimized version of _void_iface()
  if (m_rpc_id & 3)
    PLIC_THROW ("invalid cast from rpc-id to class pointer");
  return (void*) m_rpc_id;
}

void
SmartHandle::_void_iface (void *rpc_id_ptr)
{
  uint64 rpcid = uint64 (rpc_id_ptr);
  if (rpcid & 3)
    PLIC_THROW ("invalid rpc-id assignment from unaligned class pointer");
  m_rpc_id = rpcid;
}

uint64
SmartHandle::_rpc_id () const
{
  return m_rpc_id;
}

bool
SmartHandle::_is_null () const
{
  return m_rpc_id == 0;
}

SmartHandle::~SmartHandle()
{}

SmartHandle*
SmartHandle::_rpc_id2obj (uint64 rpc_id)
{
  if (rpc_id == 0)
    return const_cast<SmartHandle*> (&None);
  return NULL; // FIXME
}

/* === EventDispatcher === */
EventDispatcher::~EventDispatcher ()
{}

/* === SimpleServer === */
static pthread_mutex_t         simple_server_mutex = PTHREAD_MUTEX_INITIALIZER;
static std::set<SimpleServer*> simple_server_set;

SimpleServer::SimpleServer ()
{
  pthread_mutex_lock (&simple_server_mutex);
  simple_server_set.insert (this);
  pthread_mutex_unlock (&simple_server_mutex);
}

SimpleServer::~SimpleServer ()
{
  pthread_mutex_lock (&simple_server_mutex);
  simple_server_set.erase (this);
  pthread_mutex_unlock (&simple_server_mutex);
}

uint64
SimpleServer::_rpc_id () const
{
  return uint64 (this);
}

SimpleServer*
SimpleServer::_rpc_id2obj (uint64 rpc_id)
{
  pthread_mutex_lock (&simple_server_mutex);
  std::set<SimpleServer*>::const_iterator it = simple_server_set.find ((SimpleServer*) rpc_id);
  SimpleServer *self = it == simple_server_set.end() ? NULL : *it;
  pthread_mutex_unlock (&simple_server_mutex);
  return self;
}

/* === TypeHash === */
String
TypeHash::to_string() const
{
  String s;
  for (uint i = 0; i < hash_size; i++)
    s += string_printf ("%016llx", qwords[i]);
  return s;
}

/* === Channel === */
struct Channel::Priv {
  pthread_spinlock_t        msg_spinlock;
  sem_t                     msg_sem;
  std::vector<FieldBuffer*> msg_vector;
  std::vector<FieldBuffer*> msg_queue;
  size_t                    msg_index;
  Priv() :
    msg_index (0)
  {
    pthread_spin_init (&msg_spinlock, 0 /* pshared */);
    sem_init (&msg_sem, 0 /* unshared */, 0 /* init */);
  }
  ~Priv()
  {
    sem_destroy (&msg_sem);
    pthread_spin_destroy (&msg_spinlock);
  }
};

Channel::Channel () :
  priv (*new Priv())
{}

bool
Channel::push_msg (FieldBuffer *fbmsg) // takes fbmsg ownership
{
  pthread_spin_lock (&priv.msg_spinlock);
  priv.msg_vector.push_back (fbmsg);
  const uint sz = priv.msg_vector.size();
  pthread_spin_unlock (&priv.msg_spinlock);
  sem_post (&priv.msg_sem);
  if (wakeup0.callable())
    wakeup0();
  bool may_block = sz >= 257;   // recommend sched_yield below POSIX atomic buffer limit
  return may_block;             // should yield on single-cores
}

FieldBuffer*
Channel::fetch_msg (bool advance,
                    bool block)
{
  while (true)
    {
      if (priv.msg_index >= priv.msg_queue.size())
        {
          if (priv.msg_index)
            priv.msg_queue.resize (0);
          priv.msg_index = 0;
          pthread_spin_lock (&priv.msg_spinlock);
          priv.msg_vector.swap (priv.msg_queue);
          pthread_spin_unlock (&priv.msg_spinlock);
        }
      if (priv.msg_index < priv.msg_queue.size())
        {
          const size_t index = priv.msg_index;
          if (advance)
            priv.msg_index++;
          return priv.msg_queue[index];
        }
      if (!block)
        return NULL;
      sem_wait (&priv.msg_sem);
    }
}

Channel::~Channel ()
{
  delete &priv;
}

/* === Coupler === */
struct Coupler::Priv {
  vector<EventDispatcher*> edispatchers;
  uint                     next_free;
  Priv() : next_free (0xffffffffULL) {}
  uint
  add_edispatcher (EventDispatcher *evd)
  {
    uint ix = next_free < edispatchers.size() ? ptrdiff_t (edispatchers[next_free]) : next_free;
    if (ix < edispatchers.size())
      { // this implements minor free-id shuffling
        edispatchers[next_free] = edispatchers[ix];
        edispatchers[ix] = evd;
        return ix;
      }
    edispatchers.push_back (evd);
    return edispatchers.size() - 1;
  }
  void
  remove_edispatcher (uint ix)
  {
    if (ix < edispatchers.size())
      {
        EventDispatcher *evd = edispatchers[ix];
        edispatchers[ix] = (EventDispatcher*) next_free;
        next_free = ix;
        if (evd)
          delete evd;
      }
  }
  bool
  isfree (uint ix)
  {
    for (uint jx = next_free; jx < edispatchers.size(); jx = ptrdiff_t (edispatchers[jx]))
      if (jx == ix)
        return true;
    return false;
  }
};

Coupler::Coupler () :
  priv (*new Coupler::Priv()),
  reader (*(FieldBuffer*) NULL)
{}

void
Coupler::dispatch ()
{
  const FieldBuffer *call = callc.pop_msg();
  if (!call)
    return;
  reader.reset (*call);
  FieldBuffer *fr = DispatcherRegistry::dispatch_call (*call, *this);
  reader.reset();
  delete call;
  if (fr)
    {
      push_result (fr);
      sched_yield(); // allow fast return value handling on single core
    }
}

Coupler::~Coupler ()
{
  reader.reset();
  delete &priv;
}

uint
Coupler::dispatcher_add (std::auto_ptr<EventDispatcher> evd)
{
  return 1 + priv.add_edispatcher (evd.release());
}

EventDispatcher*
Coupler::dispatcher_lookup (uint edispatcher_id)
{
  uint ix = edispatcher_id - 1;
  return ix < priv.edispatchers.size() ? priv.edispatchers[ix] : NULL;
}

void
Coupler::dispatcher_delete (uint edispatcher_id)
{
  uint ix = edispatcher_id - 1;
  if (ix < priv.edispatchers.size() && !priv.isfree (ix))
    priv.remove_edispatcher (ix);
  else
    printerr ("PLIC:Coupler::%s(): invalid id: %u", __func__, edispatcher_id);
}

/* === DispatchRegistry === */
typedef std::map<TypeHash, DispatchFunc> TypeHashMap;
static TypeHashMap              dispatcher_type_map;
static pthread_mutex_t          dispatcher_type_mutex = PTHREAD_MUTEX_INITIALIZER;

void
DispatcherRegistry::register_dispatcher (const DispatcherEntry &dentry)
{
  pthread_mutex_lock (&dispatcher_type_mutex);
  dispatcher_type_map[TypeHash (dentry.hash_qwords)] = dentry.dispatcher;
  pthread_mutex_unlock (&dispatcher_type_mutex);
}

DispatchFunc
DispatcherRegistry::find_dispatcher (const TypeHash &type_hash)
{
  pthread_mutex_lock (&dispatcher_type_mutex);
  DispatchFunc dispatcher_func = dispatcher_type_map[type_hash];
  pthread_mutex_unlock (&dispatcher_type_mutex);
  return dispatcher_func;
}

FieldBuffer*
DispatcherRegistry::dispatch_call (const FieldBuffer &fbcall,
                                   Coupler           &coupler)
{
  const TypeHash thash = fbcall.first_type_hash();
  const int64 call_id = thash.id (0);
  const bool needsresult = msgid_has_result (call_id);
  DispatchFunc dispatcher_func = DispatcherRegistry::find_dispatcher (thash);
  if (PLIC_UNLIKELY (!dispatcher_func))
    return FieldBuffer::new_error ("unknown method hash: " + thash.to_string(), "PLIC");
  FieldBuffer *fr = dispatcher_func (coupler);
  if (!fr)
    {
      if (needsresult)
        return FieldBuffer::new_error (string_printf ("missing return from 0x%016llx", call_id), "PLIC");
      else
        return NULL; // FieldBuffer::new_ok();
    }
  const int64 ret_id = fr->first_id();
  if (is_msgid_error (ret_id) ||
      (needsresult && is_msgid_result (ret_id)))
    return fr;
  if (!needsresult && is_msgid_ok (ret_id))
    {
      delete fr;
      return NULL; // FieldBuffer::new_ok();
    }
  delete fr; // bogus
  return FieldBuffer::new_error (string_printf ("bogus return from 0x%016llx: 0x%016llx", call_id, ret_id), "PLIC");
}

/* === FieldBuffer === */
FieldBuffer::FieldBuffer (uint _ntypes) :
  buffermem (NULL)
{
  PLIC_STATIC_ASSERT (sizeof (FieldBuffer) <= sizeof (FieldUnion));
  // buffermem layout: [{n_types,nth}] [{type nibble} * n_types]... [field]...
  const uint _offs = 1 + (_ntypes + 7) / 8;
  buffermem = new FieldUnion[_offs + _ntypes];
  wmemset ((wchar_t*) buffermem, 0, sizeof (FieldUnion[_offs]) / sizeof (wchar_t));
  buffermem[0].capacity = _ntypes;
  buffermem[0].index = 0;
}

FieldBuffer::FieldBuffer (uint        _ntypes,
                          FieldUnion *_bmem,
                          uint        _bmemlen) :
  buffermem (_bmem)
{
  const uint _offs = 1 + (_ntypes + 7) / 8;
  assert (_bmem && _bmemlen >= sizeof (FieldUnion[_offs + _ntypes]));
  wmemset ((wchar_t*) buffermem, 0, sizeof (FieldUnion[_offs]) / sizeof (wchar_t));
  buffermem[0].capacity = _ntypes;
  buffermem[0].index = 0;
}

FieldBuffer::~FieldBuffer()
{
  reset();
  if (buffermem)
    delete [] buffermem;
}

String
FieldBuffer::first_id_str() const
{
  uint64 fid = first_id();
  return string_printf ("%016llx", fid);
}

FieldBuffer*
FieldBuffer::new_error (const String &msg,
                        const String &domain)
{
  FieldBuffer *fr = FieldBuffer::_new (3);
  fr->add_int64 (Plic::msgid_error); // proc_id
  fr->add_string (msg);
  fr->add_string (domain);
  return fr;
}

FieldBuffer*
FieldBuffer::new_result()
{
  FieldBuffer *fr = FieldBuffer::_new (2);
  fr->add_int64 (Plic::msgid_result); // proc_id
  return fr;
}

FieldBuffer*
FieldBuffer::new_ok()
{
  FieldBuffer *fr = FieldBuffer::_new (2);
  fr->add_int64 (Plic::msgid_ok); // proc_id
  return fr;
}

class OneChunkFieldBuffer : public FieldBuffer {
  virtual ~OneChunkFieldBuffer () { reset(); buffermem = NULL; }
  explicit OneChunkFieldBuffer (uint        _ntypes,
                                FieldUnion *_bmem,
                                uint        _bmemlen) :
    FieldBuffer (_ntypes, _bmem, _bmemlen)
  {}
public:
  static OneChunkFieldBuffer*
  _new (uint _ntypes)
  {
    const uint _offs = 1 + (_ntypes + 7) / 8;
    size_t bmemlen = sizeof (FieldUnion[_offs + _ntypes]);
    size_t objlen = ALIGN4 (sizeof (OneChunkFieldBuffer), int64);
    uint8 *omem = new uint8[objlen + bmemlen];
    FieldUnion *bmem = (FieldUnion*) (omem + objlen);
    return new (omem) OneChunkFieldBuffer (_ntypes, bmem, bmemlen);
  }
};

FieldBuffer*
FieldBuffer::_new (uint _ntypes)
{
  return OneChunkFieldBuffer::_new (_ntypes);
}

} // Plic
