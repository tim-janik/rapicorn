// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "plicutils.hh"

#include "plicutypes.cc" // includes TypeCode parser

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
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

// == Auxillary macros ==
#ifndef __GNUC__
#define __PRETTY_FUNCTION__                     __func__
#endif
#define PLIC_CPP_PASTE2i(a,b)                   a ## b // indirection required to expand __LINE__ etc
#define PLIC_CPP_PASTE2(a,b)                    PLIC_CPP_PASTE2i (a,b)
#define PLIC_STATIC_ASSERT_NAMED(expr,asname)   typedef struct { char asname[(expr) ? 1 : -1]; } PLIC_CPP_PASTE2 (Plic_StaticAssertion_LINE, __LINE__)
#define PLIC_STATIC_ASSERT(expr)                PLIC_STATIC_ASSERT_NAMED (expr, compile_time_assertion_failed)
#define ALIGN4(sz,unit)                         (sizeof (unit) * ((sz + sizeof (unit) - 1) / sizeof (unit)))
#define PLIC_THROW_IF_FAIL(expr)                do { if (PLIC_LIKELY (expr)) break; PLIC_THROW ("failed to assert (" + #expr + ")"); } while (0)
#define PLIC_THROW(msg)                         throw std::runtime_error (std::string() + __PRETTY_FUNCTION__ + ": " + msg)

/// @namespace Plic The Plic namespace provides all PLIC functionality exported to C++.
namespace Plic {

// == FieldUnion ==
PLIC_STATIC_ASSERT (sizeof (FieldUnion::smem) <= sizeof (FieldUnion::bytes));
PLIC_STATIC_ASSERT (sizeof (FieldUnion::bmem) <= 2 * sizeof (FieldUnion::bytes)); // FIXME

/* === Prototypes === */
static String string_printf (const char *format, ...) PLIC_PRINTF (1, 2);

// === Utilities ===
static String
string_printf (const char *format, ...)
{
  String str;
  va_list args;
  va_start (args, format);
  char buffer[1024 + 1];
  vsnprintf (buffer, 512, format, args);
  buffer[1024] = 0; // force termination
  va_end (args);
  return buffer;
}

void
error_vprintf (const char *format, va_list args)
{
  char buffer[1024 + 1];
  vsnprintf (buffer, 512, format, args);
  buffer[1024] = 0; // force termination
  const int l = strlen (buffer);
  const bool need_newline = l < 1 || buffer[l - 1] != '\n';
  fprintf (stderr, "PLIC::Connection: error: %s%s", buffer, need_newline ? "\n" : "");
  fflush (stderr);
  abort();
}

void
error_printf (const char *format, ...)
{
  va_list args;
  va_start (args, format);
  error_vprintf (format, args);
  va_end (args);
}

// == Any ==
void
Any::set_type_simple (TypeKind newkind)
{
  switch (kind())
    {
    case STRING:        ((String*) &u)->~String();      break;
    case ANY:           delete u.vany;                  break;
    default: ;
    }
  // kind = UNTYPED
}

Any::~Any ()
{
  set_type_simple (UNTYPED);
}

/* === SmartHandle === */
SmartHandle::SmartHandle (uint64 ipcid) :
  m_rpc_id (ipcid)
{
  assert (0 != ipcid);
}

SmartHandle::SmartHandle() :
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
  if (size() > capacity())
    {
      String msg = string_printf ("FieldBuffer(this=%p): capacity=%u size=%u",
                                  this, capacity(), size());
      throw std::out_of_range (msg);
    }
}

void
FieldReader::check_request (int type)
{
  if (m_nth >= n_types())
    {
      String msg = string_printf ("FieldReader(this=%p): size=%u requested-index=%u",
                                  this, n_types(), m_nth);
      throw std::out_of_range (msg);
    }
  if (get_type() != type)
    {
      String msg = string_printf ("FieldReader(this=%p): size=%u index=%u type=%s requested-type=%s",
                                  this, n_types(), m_nth,
                                  FieldBuffer::type_name (get_type()).c_str(), FieldBuffer::type_name (type).c_str());
      throw std::invalid_argument (msg);
    }
}

String
FieldBuffer::first_id_str() const
{
  uint64 fid = first_id();
  return string_printf ("%016llx", fid);
}

static String
strescape (const String &str)
{
  String buffer;
  for (String::const_iterator it = str.begin(); it != str.end(); it++)
    {
      uint8 d = *it;
      if (d < 32 || d > 126 || d == '?')
        buffer += string_printf ("\\%03o", d);
      else if (d == '\\')
        buffer += "\\\\";
      else if (d == '"')
        buffer += "\\\"";
      else
        buffer += d;
    }
  return buffer;
}

String
FieldBuffer::type_name (int field_type)
{
  const char *tkn = type_kind_name (TypeKind (field_type));
  if (tkn)
    return tkn;
  return string_printf ("<invalid:%d>", field_type);
}

String
FieldBuffer::to_string() const
{
  String s = string_printf ("Plic::FieldBuffer(%p)={", this);
  s += string_printf ("size=%u, capacity=%u", size(), capacity());
  FieldReader fbr (*this);
  for (size_t i = 0; i < size(); i++)
    {
      const String tname = type_name (fbr.get_type());
      const char *tn = tname.c_str();
      switch (fbr.get_type())
        {
        case UNTYPED:
        case FUNC:
        case TYPE_REFERENCE:
        case VOID:      s += string_printf (", %s", tn); fbr.skip();                               break;
        case INT:       s += string_printf (", %s: 0x%llx", tn, fbr.pop_int64());                  break;
        case FLOAT:     s += string_printf (", %s: %.17g", tn, fbr.pop_double());                  break;
        case STRING:    s += string_printf (", %s: %s", tn, strescape (fbr.pop_string()).c_str()); break;
        case ENUM:      s += string_printf (", %s: 0x%llx", tn, fbr.pop_int64());                  break;
        case SEQUENCE:  s += string_printf (", %s: %p", tn, &fbr.pop_seq());                       break;
        case RECORD:    s += string_printf (", %s: %p", tn, &fbr.pop_rec());                       break;
        case INSTANCE:  s += string_printf (", %s: %p", tn, (void*) fbr.pop_object());             break;
        case ANY:       s += string_printf (", %s: %p", tn, &fbr.pop_any());                       break;
        default:        s += string_printf (", %u: <unknown>", fbr.get_type()); fbr.skip();        break;
        }
    }
  s += '}';
  return s;
}

FieldBuffer*
FieldBuffer::new_error (const String &msg,
                        const String &domain)
{
  FieldBuffer *fr = FieldBuffer::_new (2 + 2);
  const uint64 MSGID_ERROR = 0x8000000000000000ULL;
  fr->add_msgid (MSGID_ERROR, 0);
  fr->add_string (msg);
  fr->add_string (domain);
  return fr;
}

FieldBuffer*
FieldBuffer::new_result (uint n)
{
  FieldBuffer *fr = FieldBuffer::_new (2 + n);
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
static inline bool
operator< (const TypeHash &a, const TypeHash &b)
{
  return PLIC_UNLIKELY (a.typehi == b.typehi) ? a.typelo < b.typelo : a.typehi < b.typehi;
}
typedef std::map<TypeHash, DispatchFunc> DispatcherMap;
static DispatcherMap                     dispatcher_map;
static pthread_mutex_t                   dispatcher_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool                              dispatcher_map_locked = false;

DispatchFunc
Connection::find_method (uint64 hashhi, uint64 hashlo)
{
  TypeHash typehash (hashhi, hashlo);
#if 1 // avoid costly mutex locking
  if (PLIC_UNLIKELY (dispatcher_map_locked == false))
    dispatcher_map_locked = true;
  return dispatcher_map[typehash];
#else
  pthread_mutex_lock (&dispatcher_mutex);
  DispatchFunc dispatcher_func = dispatcher_map[typehash];
  pthread_mutex_unlock (&dispatcher_mutex);
  return dispatcher_func;
#endif
}

void
Connection::MethodRegistry::register_method (const MethodEntry &mentry)
{
  PLIC_THROW_IF_FAIL (dispatcher_map_locked == false);
  pthread_mutex_lock (&dispatcher_mutex);
  DispatcherMap::size_type size_before = dispatcher_map.size();
  TypeHash typehash (mentry.hashhi, mentry.hashlo);
  dispatcher_map[typehash] = mentry.dispatcher;
  DispatcherMap::size_type size_after = dispatcher_map.size();
  pthread_mutex_unlock (&dispatcher_mutex);
  // simple hash collision check (sanity check, see below)
  if (PLIC_UNLIKELY (size_before == size_after))
    {
      errno = EKEYREJECTED;
      perror (string_printf ("%s:%u: Plic::Connection::MethodRegistry::register_method: "
                             "duplicate hash registration (%016llx%016llx)",
                             __FILE__, __LINE__, mentry.hashhi, mentry.hashlo).c_str());
      abort();
    }
  /* Two 64 bit ints are used for IPC message IDs. The upper four bits encode the
   * message type. The remaining 124 bits encode the method invocation identifier.
   * The 124 bit hash keeps collision probability below 10^-19 for up to 2 billion inputs.
   */
}

bool
Connection::has_event ()
{
  return fetch_event (0) != NULL;
}

FieldBuffer*
Connection::pop_event (bool blocking)
{
  FieldBuffer *fb = fetch_event (1);
  if (blocking)
    while (!fb)
      {
        fetch_event (-1);
        fb = fetch_event (1);
      }
  return fb;
}

} // Plic
