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
#include <map>
#include <set>

/* === Auxillary macros === */
#define PLIC_CPP_PASTE2i(a,b)                   a ## b // indirection required to expand __LINE__ etc
#define PLIC_CPP_PASTE2(a,b)                    PLIC_CPP_PASTE2i (a,b)
#define PLIC_STATIC_ASSERT_NAMED(expr,asname)   typedef struct { char asname[(expr) ? 1 : -1]; } PLIC_CPP_PASTE2 (Plic_StaticAssertion_LINE, __LINE__)
#define PLIC_STATIC_ASSERT(expr)                PLIC_STATIC_ASSERT_NAMED (expr, compile_time_assertion_failed)
#define ALIGN4(sz,unit)                         (sizeof (unit) * ((sz + sizeof (unit) - 1) / sizeof (unit)))

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

/* === SimpleProxy === */
const SimpleProxy &SimpleProxy::None = SimpleProxy (0);

SimpleProxy::SimpleProxy (uint64 rpc_id) :
  m_rpc_id (rpc_id)
{
  if (&SimpleProxy::None)
    assert (rpc_id != 0);
}

uint64
SimpleProxy::_rpc_id () const
{
  return m_rpc_id;
}

bool
SimpleProxy::_is_null () const
{
  return m_rpc_id == 0;
}

SimpleServer*
SimpleProxy::_iface () const
{
  return NULL; // FIXME
}

SimpleProxy::~SimpleProxy()
{}

SimpleProxy*
SimpleProxy::_rpc_id2obj (uint64 rpc_id)
{
  if (rpc_id == 0)
    return const_cast<SimpleProxy*> (&None);
  return NULL; // FIXME
}

/* === SimpleProxy === */
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

/* === Dispatchers === */
typedef std::map<TypeHash, DispatchFunc> TypeHashMap;
static TypeHashMap               dispatcher_type_map;
static pthread_mutex_t           dispatcher_call_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t            dispatcher_call_cond = PTHREAD_COND_INITIALIZER;
static std::vector<FieldBuffer*> dispatcher_call_vector, dispatcher_call_queue;
static size_t                    dispatcher_call_index = 0;
static pthread_mutex_t           dispatcher_return_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t            dispatcher_return_cond = PTHREAD_COND_INITIALIZER;
static std::vector<FieldBuffer*> dispatcher_return_vector, dispatcher_return_queue;
static size_t                    dispatcher_return_index = 0;

bool
DispatcherRegistry::push_call (FieldBuffer *call)
{
  const int64 call_id = call->first_id();
  const bool twoway = is_callid_twoway (call_id);
  pthread_mutex_lock (&dispatcher_call_mutex);
  dispatcher_call_vector.push_back (call);
  const uint sz = dispatcher_call_vector.size();
  pthread_cond_signal (&dispatcher_call_cond);
  pthread_mutex_unlock (&dispatcher_call_mutex);
  bool may_block = twoway ||    // minimize turn-around times for two-way calls
                   sz >= 509;   // want one-way batch processing below POSIX atomic buffer limit
  return may_block;             // should yield on single-cores
}

FieldBuffer*
DispatcherRegistry::pop_call (bool advance)
{
  if (dispatcher_call_index >= dispatcher_call_queue.size())
    {
      if (dispatcher_call_index)
        dispatcher_call_queue.resize (0);
      dispatcher_call_index = 0;
      pthread_mutex_lock (&dispatcher_call_mutex);
      dispatcher_call_vector.swap (dispatcher_call_queue);
      pthread_mutex_unlock (&dispatcher_call_mutex);
    }
  if (dispatcher_call_index < dispatcher_call_queue.size())
    {
      const size_t index = dispatcher_call_index;
      if (advance)
        dispatcher_call_index++;
      return dispatcher_call_queue[index];
    }
  return NULL;
}

void
DispatcherRegistry::push_return (FieldBuffer *rret)
{
  pthread_mutex_lock (&dispatcher_return_mutex);
  dispatcher_return_vector.push_back (rret);
  pthread_cond_signal (&dispatcher_return_cond);
  pthread_mutex_unlock (&dispatcher_return_mutex);
  sched_yield(); // allow fast return value handling on single core
}

FieldBuffer*
DispatcherRegistry::fetch_return (void)
{
  if (dispatcher_return_index >= dispatcher_return_queue.size())
    {
      if (dispatcher_return_index)
        dispatcher_return_queue.resize (0);
      else
        dispatcher_return_queue.reserve (1);
      dispatcher_return_index = 0;
      pthread_mutex_lock (&dispatcher_return_mutex);
      while (dispatcher_return_vector.size() == 0)
        pthread_cond_wait (&dispatcher_return_cond, &dispatcher_return_mutex);
      dispatcher_return_vector.swap (dispatcher_return_queue); // fetch result
      pthread_mutex_unlock (&dispatcher_return_mutex);
    }
  // dispatcher_return_index < dispatcher_return_queue.size()
  return dispatcher_return_queue[dispatcher_return_index++];
}

bool
DispatcherRegistry::check_dispatch ()
{
  return pop_call (false) != NULL;
}

bool
DispatcherRegistry::dispatch ()
{
  const FieldBuffer *call = pop_call();
  if (!call)
    return false;
  const int64 call_id = call->first_id();
  const bool twoway = Plic::is_callid_twoway (call_id);
  FieldBuffer *fr = Plic::DispatcherRegistry::dispatch_call (*call);
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
      if (Plic::is_callid_error (ret_id))
        printerr ("PLIC: error during oneway call 0x%016llx: %s\n",
                  call_id, frr.pop_string().c_str()); // FIXME: logging?
      else if (Plic::is_callid_return (ret_id))
        printerr ("PLIC: unexpected return from oneway 0x%016llx\n", call_id); // FIXME: logging?
      else /* ? */
        printerr ("PLIC: invalid return garbage from 0x%016llx\n", call_id); // FIXME: logging?
      delete fr;
    }
  return true;
}

void
DispatcherRegistry::register_dispatcher (const DispatcherEntry &dentry)
{
  dispatcher_type_map[TypeHash (dentry.hash_qwords)] = dentry.dispatcher;
}

FieldBuffer*
DispatcherRegistry::dispatch_call (const FieldBuffer &call)
{
  const TypeHash &thash = call.first_type_hash();
  DispatchFunc dispatcher = dispatcher_type_map[thash];
  if (PLIC_UNLIKELY (!dispatcher))
    return FieldBuffer::new_error ("unknown method hash: " + thash.to_string(), "PLIC");
  return dispatcher (call);
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
