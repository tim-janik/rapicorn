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
#include <pthread.h>
#include <stdexcept>
#include <map>
#include <set>

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
static void   printerr      (const char *format, ...) PLIC_PRINTF (1, 2);

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

/* === Coupler === */
struct Coupler::Priv {
  pthread_mutex_t           call_mutex;
  pthread_cond_t            call_cond;
  std::vector<FieldBuffer*> call_vector;
  std::vector<FieldBuffer*> call_queue;
  size_t                    call_index;
  pthread_mutex_t           result_mutex;
  pthread_cond_t            result_cond;
  std::vector<FieldBuffer*> result_vector;
  std::vector<FieldBuffer*> result_queue;
  size_t                    result_index;
  Priv() :
    call_index (0),
    result_index (0)
  {
    pthread_mutex_init (&call_mutex, NULL);
    pthread_cond_init (&call_cond, NULL);
    pthread_mutex_init (&result_mutex, NULL);
    pthread_cond_init (&result_cond, NULL);
  }
};

Coupler::Coupler () :
  priv (*new Priv()),
  reader (*(FieldBuffer*) NULL)
{}

bool
Coupler::send_call (FieldBuffer *fbcall) // deletes fbcall
{
  const int64 call_id = fbcall->first_id();
  const bool twoway = is_callid_twoway (call_id);
  pthread_mutex_lock (&priv.call_mutex);
  priv.call_vector.push_back (fbcall);
  const uint sz = priv.call_vector.size();
  pthread_cond_signal (&priv.call_cond);
  pthread_mutex_unlock (&priv.call_mutex);
  bool may_block = twoway ||    // minimize turn-around times for two-way calls
                   sz >= 509;   // want one-way batch processing below POSIX atomic buffer limit
  return may_block;             // should yield on single-cores
}

FieldBuffer*
Coupler::pop_call (bool advance)
{
  if (priv.call_index >= priv.call_queue.size())
    {
      if (priv.call_index)
        priv.call_queue.resize (0);
      priv.call_index = 0;
      pthread_mutex_lock (&priv.call_mutex);
      priv.call_vector.swap (priv.call_queue);
      pthread_mutex_unlock (&priv.call_mutex);
    }
  if (priv.call_index < priv.call_queue.size())
    {
      const size_t index = priv.call_index;
      if (advance)
        priv.call_index++;
      return priv.call_queue[index];
    }
  return NULL;
}

FieldBuffer*
Coupler::dispatch_call (const FieldBuffer &fbcall)
{
  const TypeHash &thash = fbcall.first_type_hash();
  DispatchFunc dispatcher_func = DispatcherRegistry::find_dispatcher (thash);
  if (PLIC_UNLIKELY (!dispatcher_func))
    return FieldBuffer::new_error ("unknown method hash: " + thash.to_string(), "PLIC");
  reader.reset (fbcall);
  return dispatcher_func (*this);
}

void
Coupler::push_return (FieldBuffer *rret)
{
  pthread_mutex_lock (&priv.result_mutex);
  priv.result_vector.push_back (rret);
  pthread_cond_signal (&priv.result_cond);
  pthread_mutex_unlock (&priv.result_mutex);
  sched_yield(); // allow fast return value handling on single core
}

FieldBuffer*
Coupler::receive_result (void)
{
  if (priv.result_index >= priv.result_queue.size())
    {
      if (priv.result_index)
        priv.result_queue.resize (0);
      else
        priv.result_queue.reserve (1);
      priv.result_index = 0;
      pthread_mutex_lock (&priv.result_mutex);
      while (priv.result_vector.size() == 0)
        pthread_cond_wait (&priv.result_cond, &priv.result_mutex);
      priv.result_vector.swap (priv.result_queue); // fetch result
      pthread_mutex_unlock (&priv.result_mutex);
    }
  // priv.result_index < priv.result_queue.size()
  return priv.result_queue[priv.result_index++];
}

bool
Coupler::dispatch ()
{
  const FieldBuffer *call = pop_call();
  if (!call)
    return false;
  const int64 call_id = call->first_id();
  const bool twoway = is_callid_twoway (call_id);
  FieldBuffer *fr = dispatch_call (*call);
  delete call;
  if (twoway)
    {
      if (!fr)
        fr = FieldBuffer::new_error (string_printf ("missing return from 0x%016llx", call_id), "PLIC");
      push_return (fr);
    }
  else if (fr && !twoway)
    {
      FieldBufferReader frr (*fr);
      const int64 ret_id = frr.pop_int64(); // first_id
      if (is_callid_error (ret_id))
        printerr ("PLIC: error during oneway call 0x%016llx: %s\n",
                  call_id, frr.pop_string().c_str()); // FIXME: logging?
      else if (is_callid_return (ret_id))
        printerr ("PLIC: unexpected return from oneway 0x%016llx\n", call_id); // FIXME: logging?
      else /* ? */
        printerr ("PLIC: invalid return garbage from 0x%016llx\n", call_id); // FIXME: logging?
      delete fr;
    }
  return true;
}

Coupler::~Coupler ()
{
  reader.reset();
  delete &priv;
}

/* === Dispatchers === */
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
  fr->add_int64 (Plic::callid_error); // proc_id
  fr->add_string (msg);
  fr->add_string (domain);
  return fr;
}

FieldBuffer*
FieldBuffer::new_return()
{
  FieldBuffer *fr = FieldBuffer::_new (2);
  fr->add_int64 (Plic::callid_return); // proc_id
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
