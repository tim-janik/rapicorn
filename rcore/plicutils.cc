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
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <semaphore.h>
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

void
FieldBuffer::check_internal ()
{
  if (nth() >= n_types())
    {
      String msg = string_printf ("FieldBuffer(this=%p): capacity=%u index=%u",
                                  this, n_types(), nth());
      throw std::out_of_range (msg);
    }
}

void
FieldReader::check_internal ()
{
  if (m_nth >= n_types())
    {
      String msg = string_printf ("FieldReader(this=%p): capacity=%u index=%u",
                                  this, n_types(), m_nth);
      throw std::out_of_range (msg);
    }
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
  const uint64 MSGID_ERROR = 0x8000000000000000ULL;
  fr->add_msgid (MSGID_ERROR, 0);
  fr->add_string (msg);
  fr->add_string (domain);
  return fr;
}

FieldBuffer*
FieldBuffer::new_result()
{
  FieldBuffer *fr = FieldBuffer::_new (2);
  const uint64 MSGID_RESULT_MASK = 0x9000000000000000ULL;
  fr->add_msgid (MSGID_RESULT_MASK, 0); // FIXME: needs original message
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

// === Connection ===
struct Hash128 {
  uint64 hashes[2];
  inline      Hash128 (uint64 h, uint64 l) { hashes[0] = h; hashes[1] = l; }
  inline bool operator< (const Hash128 &other) const
  {
    if (PLIC_UNLIKELY (hashes[0] == other.hashes[0]))
      return hashes[1] < other.hashes[1];
    else
      return hashes[0] < other.hashes[0];
  }
};
typedef std::map<Hash128, DispatchFunc> DispatcherMap;
static DispatcherMap                   dispatcher_map;
static pthread_mutex_t                 dispatcher_mutex = PTHREAD_MUTEX_INITIALIZER;

DispatchFunc
Connection::find_method (uint64 hashhi, uint64 hashlow)
{
  Hash128 hash128 (hashhi, hashlow);
  pthread_mutex_lock (&dispatcher_mutex);
  DispatchFunc dispatcher_func = dispatcher_map[hash128];
  pthread_mutex_unlock (&dispatcher_mutex);
  return dispatcher_func;
}

void
Connection::MethodRegistry::register_method (const MethodEntry &mentry)
{
  pthread_mutex_lock (&dispatcher_mutex);
  DispatcherMap::size_type size_before = dispatcher_map.size();
  Hash128 hash128 (mentry.hashhi, mentry.hashlow);
  dispatcher_map[hash128] = mentry.dispatcher;
  DispatcherMap::size_type size_after = dispatcher_map.size();
  pthread_mutex_unlock (&dispatcher_mutex);
  // simple hash collision check (sanity check, see below)
  if (PLIC_UNLIKELY (size_before == size_after))
    {
      errno = EKEYREJECTED;
      perror (string_printf ("%s:%u: Plic::Connection::MethodRegistry::register_method: "
                             "duplicate hash registration (%016llx%016llx)",
                             __FILE__, __LINE__, mentry.hashhi, mentry.hashlow).c_str());
      abort();
    }
  /* Two 64 bit ints are used for IPC message IDs. The upper four bits encode the
   * message type. The remaining 124 bits encode the method invocation identifier.
   * The 124 bit hash keeps collision probability below 10^-19 for up to 2 billion inputs.
   */
}

} // Plic
