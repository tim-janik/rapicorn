// CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0/
#include "aida.hh"
#include "aidaprops.hh"

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <sched.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <semaphore.h>
#include <pthread.h>
#include <poll.h>
#include <sys/eventfd.h>        // defines EFD_SEMAPHORE
#include <stddef.h>             // ptrdiff_t
#include <stdexcept>
#include <deque>
#include <map>
#include <set>

// == Auxillary macros ==
#ifndef __GNUC__
#define __PRETTY_FUNCTION__                     __func__
#endif
#define AIDA_CPP_PASTE2i(a,b)                   a ## b // indirection required to expand __LINE__ etc
#define AIDA_CPP_PASTE2(a,b)                    AIDA_CPP_PASTE2i (a,b)
#define AIDA_STATIC_ASSERT_NAMED(expr,asname)   typedef struct { char asname[(expr) ? 1 : -1]; } AIDA_CPP_PASTE2 (Aida_StaticAssertion_LINE, __LINE__)
#define AIDA_STATIC_ASSERT(expr)                AIDA_STATIC_ASSERT_NAMED (expr, compile_time_assertion_failed)
#define ALIGN4(sz,unit)                         (sizeof (unit) * ((sz + sizeof (unit) - 1) / sizeof (unit)))
#define AIDA_THROW_IF_FAIL(expr)                do { if (AIDA_LIKELY (expr)) break; AIDA_THROW ("failed to assert (" + #expr + ")"); } while (0)
#define AIDA_THROW(msg)                         throw std::runtime_error (std::string() + __PRETTY_FUNCTION__ + ": " + msg)

namespace Rapicorn {
/// The Aida namespace provides all IDL functionality exported to C++.
namespace Aida {

// == FieldUnion ==
AIDA_STATIC_ASSERT (sizeof (FieldUnion::smem) <= sizeof (FieldUnion::bytes));
AIDA_STATIC_ASSERT (sizeof (FieldUnion::bmem) <= 2 * sizeof (FieldUnion::bytes)); // FIXME

/* === Prototypes === */
static std::string string_cprintf (const char *format, ...) AIDA_PRINTF (1, 2);

// === Utilities ===
static std::string
string_vcprintf (const char *format, va_list vargs, bool force_newline = false)
{
  static locale_t volatile clocale = NULL;
  if (!clocale)
    {
      locale_t tmploc = newlocale (LC_ALL_MASK, "C", NULL);
      if (!__sync_bool_compare_and_swap (&clocale, NULL, tmploc))
        freelocale (tmploc);
    }
  locale_t olocale = uselocale (clocale);
  va_list pargs;
  char buffer[1024];
  buffer[0] = 0;
  va_copy (pargs, vargs);
  const int l = vsnprintf (buffer, sizeof (buffer), format, pargs);
  va_end (pargs);
  std::string string;
  if (l < 0)
    string = format; // error?
  else if (size_t (l) < sizeof (buffer))
    string = std::string (buffer, l);
  else
    {
      string.resize (l + 1);
      va_copy (pargs, vargs);
      const int j = vsnprintf (&string[0], string.size(), format, pargs);
      va_end (pargs);
      string.resize (std::min (l, std::max (j, 0)));
    }
  uselocale (olocale);
  if (force_newline && (string.empty() || string[string.size() - 1] != '\n'))
    string += "\n";
  return string;
}

static std::string
string_cprintf (const char *format, ...)
{
  va_list args;
  va_start (args, format);
  std::string s = string_vcprintf (format, args);
  va_end (args);
  return s;
}

void
error_vprintf (const char *format, va_list args)
{
  std::string s = string_vcprintf (format, args, true);
  fprintf (stderr, "Aida: error: %s", s.c_str());
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

void
warning_printf (const char *format, ...)
{
  va_list args;
  va_start (args, format);
  std::string s = string_vcprintf (format, args, true);
  va_end (args);
  fprintf (stderr, "Aida: warning: %s", s.c_str());
  fflush (stderr);
}

// == Any ==
Any::Any() :
  type_code (TypeMap::notype())
{
  memset (&u, 0, sizeof (u));
}

Any::Any (const Any &clone) :
  type_code (TypeMap::notype())
{
  this->operator= (clone);
}

Any&
Any::operator= (const Any &clone)
{
  if (this == &clone)
    return *this;
  reset();
  type_code = clone.type_code;
  switch (kind())
    {
    case STRING:        new (&u) String (*(String*) &clone.u);          break;
    case SEQUENCE:      new (&u) AnyVector (*(AnyVector*) &clone.u);    break;
    case RECORD:        new (&u) AnyVector (*(AnyVector*) &clone.u);    break;
    case ANY:           u.vany = new Any (*clone.u.vany);               break;
    default:            u = clone.u;                                    break;
    }
  return *this;
}

void
Any::reset()
{
  switch (kind())
    {
    case STRING:        ((String*) &u)->~String();              break;
    case SEQUENCE:      ((AnyVector*) &u)->~AnyVector();        break;
    case RECORD:        ((AnyVector*) &u)->~AnyVector();        break;
    case ANY:           delete u.vany;                          break;
    default: ;
    }
  type_code = TypeMap::notype();
  memset (&u, 0, sizeof (u));
}

void
Any::rekind (TypeKind _kind)
{
  reset();
  const char *type = NULL;
  switch (_kind)
    {
    case UNTYPED:     type = NULL;                                      break;
    case BOOL:        type = "bool";                                    break;
    case INT32:       type = "int32";                                   break;
      // case UINT32: type = "uint32";                                  break;
    case INT64:       type = "int64";                                   break;
    case FLOAT64:     type = "float64";                                 break;
    case ENUM:        type = "int";                                     break;
    case STRING:      type = "String";          new (&u) String();      break;
    case ANY:         type = "Any";             u.vany = new Any();     break;
    case SEQUENCE:    type = "Aida::AnySeq";    new (&u) AnyVector();   break;
    case RECORD:      type = "Aida::AnyRec";    new (&u) AnyVector();   break; // FIXME: mising details
    case INSTANCE:    type = "Aida::Instance";                          break; // FIXME: missing details
    default:
      error_printf ("Aida::Any:rekind: invalid type kind: %s", type_kind_name (_kind));
    }
  type_code = TypeMap::lookup (type);
  if (type_code.untyped() && type != NULL)
    error_printf ("Aida::Any:rekind: invalid type name: %s", type);
  if (kind() != _kind)
    error_printf ("Aida::Any:rekind: mismatch: %s -> %s (%u)", type_kind_name (_kind), type_kind_name (kind()), kind());
}

bool
Any::operator== (const Any &clone) const
{
  if (type_code != clone.type_code)
    return false;
  switch (kind())
    {
    case UNTYPED:     break;
      // case UINT: // chain
    case BOOL: case ENUM: // chain
    case INT32:
    case INT64:       if (u.vint64 != clone.u.vint64) return false;                     break;
    case FLOAT64:     if (u.vdouble != clone.u.vdouble) return false;                   break;
    case STRING:      if (*(String*) &u != *(String*) &clone.u) return false;           break;
    case SEQUENCE:    if (*(AnyVector*) &u != *(AnyVector*) &clone.u) return false;     break;
    case RECORD:      if (*(AnyVector*) &u != *(AnyVector*) &clone.u) return false;     break;
    case INSTANCE:    if (memcmp (&u, &clone.u, sizeof (u)) != 0) return false;         break; // FIXME
    case ANY:         if (*u.vany != *clone.u.vany) return false;                       break;
    default:
      error_printf ("Aida::Any:operator==: invalid type kind: %s", type_kind_name (kind()));
    }
  return true;
}

bool
Any::operator!= (const Any &clone) const
{
  return !operator== (clone);
}

Any::~Any ()
{
  reset();
}

void
Any::retype (const TypeCode &tc)
{
  if (type_code.untyped())
    reset();
  else
    {
      rekind (tc.kind());
      type_code = tc;
    }
}

void
Any::swap (Any &other)
{
  const size_t usize = sizeof (this->u);
  char *buffer[usize];
  memcpy (buffer, &other.u, usize);
  memcpy (&other.u, &this->u, usize);
  memcpy (&this->u, buffer, usize);
  type_code.swap (other.type_code);
}

bool
Any::to_int (int64_t &v, char b) const
{
  if (kind() != INT32 && kind() != INT64)
    return false;
  bool s = 0;
  switch (b)
    {
    case 1:     s =  u.vint64 >=         0 &&  u.vint64 <= 1;        break;
    case 7:     s =  u.vint64 >=      -128 &&  u.vint64 <= 127;      break;
    case 8:     s =  u.vint64 >=         0 &&  u.vint64 <= 256;      break;
    case 47:    s = sizeof (long) == sizeof (int64_t); // chain
    case 31:    s |= u.vint64 >=   INT_MIN &&  u.vint64 <= INT_MAX;  break;
    case 48:    s = sizeof (long) == sizeof (int64_t); // chain
    case 32:    s |= u.vint64 >=         0 &&  u.vint64 <= UINT_MAX; break;
    case 63:    s = 1; break;
    case 64:    s = 1; break;
    default:    s = 0; break;
    }
  if (s)
    v = u.vint64;
  return s;
}

int64_t
Any::as_int () const
{
  switch (kind())
    {
    case BOOL:          return u.vint64;
    case INT32:
    case INT64:         return u.vint64;
    case FLOAT64:       return u.vdouble;
    case ENUM:          return u.vint64;
    case STRING:        return !((String*) &u)->empty();
    default:            return 0;
    }
}

double
Any::as_float () const
{
  switch (kind())
    {
    case BOOL:          return u.vint64;
    case INT32:
    case INT64:         return u.vint64;
    case FLOAT64:       return u.vdouble;
    case ENUM:          return u.vint64;
    case STRING:        return !((String*) &u)->empty();
    default:            return 0;
    }
}

std::string
Any::as_string() const
{
  switch (kind())
    {
    case BOOL: case ENUM:
    case INT32:
    case INT64:         return string_cprintf ("%lli", u.vint64);
    case FLOAT64:       return string_cprintf ("%.17g", u.vdouble);
    case STRING:        return *(String*) &u;
    default:            return "";
    }
}

bool
Any::operator>>= (int64_t &v) const
{
  const bool r = to_int (v, 63);
  return r;
}

bool
Any::operator>>= (double &v) const
{
  if (kind() != FLOAT64)
    return false;
  v = u.vdouble;
  return true;
}

bool
Any::operator>>= (std::string &v) const
{
  if (kind() != STRING)
    return false;
  v = *(String*) &u;
  return true;
}

bool
Any::operator>>= (const Any *&v) const
{
  if (kind() != ANY)
    return false;
  v = u.vany;
  return true;
}

void
Any::operator<<= (uint64_t v)
{
  // ensure (UINT);
  operator<<= (int64_t (v));
}

void
Any::operator<<= (int64_t v)
{
  if (kind() == BOOL && v >= 0 && v <= 1)
    {
      u.vint64 = v;
      return;
    }
  ensure (INT64);
  u.vint64 = v;
}

void
Any::operator<<= (double v)
{
  ensure (FLOAT64);
  u.vdouble = v;
}

void
Any::operator<<= (const String &v)
{
  ensure (STRING);
  ((String*) &u)->assign (v);
}

void
Any::operator<<= (const Any &v)
{
  ensure (ANY);
  if (u.vany != &v)
    {
      Any *old = u.vany;
      u.vany = new Any (v);
      if (old)
        delete old;
    }
}

void
Any::resize (size_t n)
{
  ensure (SEQUENCE);
  ((AnyVector*) &u)->resize (n);
}

/* === SmartHandle === */
SmartHandle::SmartHandle (uint64_t ipcid) :
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

uint64_t
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

/* === FieldBuffer === */
FieldBuffer::FieldBuffer (uint _ntypes) :
  buffermem (NULL)
{
  AIDA_STATIC_ASSERT (sizeof (FieldBuffer) <= sizeof (FieldUnion));
  // buffermem layout: [{n_types,nth}] [{type nibble} * n_types]... [field]...
  const uint _offs = 1 + (_ntypes + 7) / 8;
  buffermem = new FieldUnion[_offs + _ntypes];
  wmemset ((wchar_t*) buffermem, 0, sizeof (FieldUnion[_offs]) / sizeof (wchar_t));
  buffermem[0].capacity = _ntypes;
  buffermem[0].index = 0;
}

FieldBuffer::FieldBuffer (uint32_t    _ntypes,
                          FieldUnion *_bmem,
                          uint32_t    _bmemlen) :
  buffermem (_bmem)
{
  const uint32_t _offs = 1 + (_ntypes + 7) / 8;
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
      String msg = string_cprintf ("FieldBuffer(this=%p): capacity=%u size=%u",
                                   this, capacity(), size());
      throw std::out_of_range (msg);
    }
}

void
FieldReader::check_request (int type)
{
  if (m_nth >= n_types())
    {
      String msg = string_cprintf ("FieldReader(this=%p): size=%u requested-index=%u",
                                   this, n_types(), m_nth);
      throw std::out_of_range (msg);
    }
  if (get_type() != type)
    {
      String msg = string_cprintf ("FieldReader(this=%p): size=%u index=%u type=%s requested-type=%s",
                                   this, n_types(), m_nth,
                                  FieldBuffer::type_name (get_type()).c_str(), FieldBuffer::type_name (type).c_str());
      throw std::invalid_argument (msg);
    }
}

std::string
FieldBuffer::first_id_str() const
{
  uint64_t fid = first_id();
  return string_cprintf ("%016llx", fid);
}

static std::string
strescape (const std::string &str)
{
  std::string buffer;
  for (std::string::const_iterator it = str.begin(); it != str.end(); it++)
    {
      uint8_t d = *it;
      if (d < 32 || d > 126 || d == '?')
        buffer += string_cprintf ("\\%03o", d);
      else if (d == '\\')
        buffer += "\\\\";
      else if (d == '"')
        buffer += "\\\"";
      else
        buffer += d;
    }
  return buffer;
}

std::string
FieldBuffer::type_name (int field_type)
{
  const char *tkn = type_kind_name (TypeKind (field_type));
  if (tkn)
    return tkn;
  return string_cprintf ("<invalid:%d>", field_type);
}

std::string
FieldBuffer::to_string() const
{
  String s = string_cprintf ("Aida::FieldBuffer(%p)={", this);
  s += string_cprintf ("size=%u, capacity=%u", size(), capacity());
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
        case VOID:      s += string_cprintf (", %s", tn); fbr.skip();                               break;
        case BOOL: case ENUM:
        case INT32:
        case INT64:     s += string_cprintf (", %s: 0x%llx", tn, fbr.pop_int64());                  break;
        case FLOAT64:   s += string_cprintf (", %s: %.17g", tn, fbr.pop_double());                  break;
        case STRING:    s += string_cprintf (", %s: %s", tn, strescape (fbr.pop_string()).c_str()); break;
        case SEQUENCE:  s += string_cprintf (", %s: %p", tn, &fbr.pop_seq());                       break;
        case RECORD:    s += string_cprintf (", %s: %p", tn, &fbr.pop_rec());                       break;
        case INSTANCE:  s += string_cprintf (", %s: %p", tn, (void*) fbr.pop_object());             break;
        case ANY:       s += string_cprintf (", %s: %p", tn, &fbr.pop_any());                       break;
        default:        s += string_cprintf (", %u: <unknown>", fbr.get_type()); fbr.skip();        break;
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
  const uint64_t MSGID_ERROR = 0x8000000000000000ULL;
  fr->add_msgid (MSGID_ERROR, 0);
  fr->add_string (msg);
  fr->add_string (domain);
  return fr;
}

FieldBuffer*
FieldBuffer::new_result (uint32_t n)
{
  FieldBuffer *fr = FieldBuffer::_new (2 + n);
  const uint64_t MSGID_RESULT_MASK = 0x9000000000000000ULL;
  fr->add_msgid (MSGID_RESULT_MASK, 0); // FIXME: needs original message
  return fr;
}

class OneChunkFieldBuffer : public FieldBuffer {
  virtual ~OneChunkFieldBuffer () { reset(); buffermem = NULL; }
  explicit OneChunkFieldBuffer (uint32_t    _ntypes,
                                FieldUnion *_bmem,
                                uint32_t    _bmemlen) :
    FieldBuffer (_ntypes, _bmem, _bmemlen)
  {}
public:
  static OneChunkFieldBuffer*
  _new (uint32_t _ntypes)
  {
    const uint32_t _offs = 1 + (_ntypes + 7) / 8;
    size_t bmemlen = sizeof (FieldUnion[_offs + _ntypes]);
    size_t objlen = ALIGN4 (sizeof (OneChunkFieldBuffer), int64_t);
    uint8_t *omem = (uint8_t*) operator new (objlen + bmemlen);
    FieldUnion *bmem = (FieldUnion*) (omem + objlen);
    return new (omem) OneChunkFieldBuffer (_ntypes, bmem, bmemlen);
  }
};

FieldBuffer*
FieldBuffer::_new (uint32_t _ntypes)
{
  return OneChunkFieldBuffer::_new (_ntypes);
}

// == EventFd ==
class EventFd {
  void     operator= (const EventFd&); // no assignments
  explicit EventFd   (const EventFd&); // no copying
  int      efd;
public:
  int inputfd() const { return efd; }
  ~EventFd()
  {
    close (efd);
    efd = -1;
  }
  EventFd() :
    efd (-1)
  {
    do
      efd = eventfd (0 /*initval*/, 0 /*flags*/);
    while (efd < 0 && (errno == EAGAIN || errno == EINTR));
    if (efd >= 0)
      {
        int err;
        long nflags = fcntl (efd, F_GETFL, 0);
        nflags |= O_NONBLOCK;
        do
          err = fcntl (efd, F_SETFL, nflags);
        while (err < 0 && (errno == EINTR || errno == EAGAIN));
      }
    if (efd < 0)
      error_printf ("failed to open eventfd: %s", strerror (errno));
  }
  void
  wakeup()
  {
    int err;
    do
      err = eventfd_write (efd, 1);
    while (err < 0 && errno == EINTR);
    // EAGAIN occours if too many wakeups are pending
  }
  bool
  pollin ()
  {
    struct pollfd pfd = { efd, POLLIN, 0 };
    int presult;
    do
      presult = poll (&pfd, 1, -1);
    while (presult < 0 && (errno == EAGAIN || errno == EINTR));
    return pfd.revents != 0;
  }
  void
  flush () // clear pending wakeups
  {
    eventfd_t bytes8;
    int err;
    do
      err = eventfd_read (efd, &bytes8);
    while (err < 0 && errno == EINTR);
    // EAGAIN occours if no wakeups are pending
  }
};

// == lock-free, single-consumer queue ==
template<class Data> struct MpScQueueF {
  struct Node { Node *next; Data data; };
  MpScQueueF() :
    m_head (NULL), m_local (NULL)
  {}
  bool
  push (Data data)
  {
    Node *node = new Node;
    node->data = data;
    Node *last_head;
    do
      node->next = last_head = m_head;
    while (!__sync_bool_compare_and_swap (&m_head, node->next, node));
    return last_head == NULL; // was empty
  }
  Data
  pop()
  {
    Node *node = pop_node();
    if (node)
      {
        Data d = node->data;
        delete node;
        return d;
      }
    else
      return Data();
  }
protected:
  Node*
  pop_node()
  {
    if (AIDA_UNLIKELY (!m_local))
      {
        Node *prev, *next, *node;
        do
          node = m_head;
        while (!__sync_bool_compare_and_swap (&m_head, node, NULL));
        for (prev = NULL; node; node = next)
          {
            next = node->next;
            node->next = prev;
            prev = node;
          }
        m_local = prev;
      }
    if (m_local)
      {
        Node *node = m_local;
        m_local = node->next;
        return node;
      }
    else
      return NULL;
  }
private:
  Node  *m_head __attribute__ ((aligned (64)));
  // we pad/align with CPU_CACHE_LINE_SIZE to avoid false sharing between pushing and popping threads
  Node  *m_local __attribute__ ((aligned (64)));
} __attribute__ ((aligned (64)));


// == TransportChannel ==
class TransportChannel : public EventFd { // Channel for cross-thread FieldBuffer IO
  MpScQueueF<FieldBuffer*> msg_queue;
  FieldBuffer             *last_fb;
  enum Op { PEEK, POP, POP_BLOCKED };
  FieldBuffer*
  get_msg (const Op op)
  {
    if (!last_fb)
      do
        {
          // fetch new messages
          last_fb = msg_queue.pop();
          if (!last_fb)
            {
              flush();                          // flush stale wakeups, to allow blocking until an empty => full transition
              last_fb = msg_queue.pop();        // retry, to ensure we've not just discarded a real wakeup
            }
          if (last_fb)
            break;
          // no messages available
          if (op == POP_BLOCKED)
            pollin();
        }
      while (op == POP_BLOCKED);
    FieldBuffer *fb = last_fb;
    if (op != PEEK) // advance
      last_fb = NULL;
    return fb; // may be NULL
  }
public:
  void
  send_msg (FieldBuffer *fb, // takes fb ownership
            bool         may_wakeup)
  {
    const bool was_empty = msg_queue.push (fb);
    if (may_wakeup && was_empty)
      wakeup();                                 // wakeups are needed to catch empty => full transition
  }
  FieldBuffer*  fetch_msg()     { return get_msg (POP); }
  bool          has_msg()       { return get_msg (PEEK); }
  FieldBuffer*  pop_msg()       { return get_msg (POP_BLOCKED); }
  ~TransportChannel ()
  {}
  TransportChannel () :
    last_fb (NULL)
  {}
};

// == ConnectionTransport ==
class ConnectionTransport /// Transport layer for messages sent between ClientConnection and ServerConnection.
{
  TransportChannel         server_queue; // messages sent to server
  TransportChannel         client_queue; // messages sent to client
  std::deque<FieldBuffer*> client_events; // messages pending for client
  sem_t                    result_sem;
  int                      ref_count;
  void     operator=               (const ConnectionTransport&); // no assignments
  explicit ConnectionTransport     (const ConnectionTransport&); // no copying
public:
  ConnectionTransport () :
    ref_count (1)
  {
    pthread_spin_init (&ehandler_spin, 0 /* pshared */);
    sem_init (&result_sem, 0 /* unshared */, 0 /* init */);
  }
  ConnectionTransport*
  ref()
  {
    int last_ref = __sync_fetch_and_add (&ref_count, 1);
    assert (last_ref > 0);
    return this;
  }
  void
  unref()
  {
    int last_ref = __sync_fetch_and_add (&ref_count, -1);
    assert (last_ref > 0);
    if (last_ref == 1)
      delete this;
  }
  virtual
  ~ConnectionTransport ()
  {
    assert (ref_count == 0);
    sem_destroy (&result_sem);
    pthread_spin_destroy (&ehandler_spin);
  }
  void  block_for_result        () { sem_wait (&result_sem); }
  void  notify_for_result       () { sem_post (&result_sem); }
public: // server
  void          server_send_event (FieldBuffer *fb) { return client_queue.send_msg (fb, true); } // FIXME: check type
  int           server_notify_fd  ()    { return server_queue.inputfd(); }
  bool          server_pending    ()    { return server_queue.has_msg(); }
  void          server_dispatch   ();
public: // client
  int          client_notify_fd   ()    { return client_queue.inputfd(); }
  bool         client_pending     ()    { return !client_events.empty() || client_queue.has_msg(); }
  void         client_dispatch    ();
  FieldBuffer* client_call_remote (FieldBuffer *fb);
  // client event handler
  typedef std::set<uint64_t> UIntSet;
  pthread_spinlock_t        ehandler_spin;
  UIntSet                   ehandler_set;
  uint64_t
  client_register_event_handler (ClientConnection::EventHandler *evh)
  {
    pthread_spin_lock (&ehandler_spin);
    const std::pair<UIntSet::iterator,bool> ipair = ehandler_set.insert (ptrdiff_t (evh));
    pthread_spin_unlock (&ehandler_spin);
    return ipair.second ? ptrdiff_t (evh) : 0; // unique insertion
  }
  ClientConnection::EventHandler*
  client_find_event_handler (uint64_t handler_id)
  {
    pthread_spin_lock (&ehandler_spin);
    UIntSet::iterator iter = ehandler_set.find (handler_id);
    pthread_spin_unlock (&ehandler_spin);
    if (iter == ehandler_set.end())
      return NULL; // unknown handler_id
    ClientConnection::EventHandler *evh = (ClientConnection::EventHandler*) ptrdiff_t (handler_id);
    return evh;
  }
  bool
  client_delete_event_handler (uint64_t handler_id)
  {
    pthread_spin_lock (&ehandler_spin);
    size_t nerased = ehandler_set.erase (handler_id);
    pthread_spin_unlock (&ehandler_spin);
    return nerased > 0; // deletion successful?
  }
};

FieldBuffer*
ConnectionTransport::client_call_remote (FieldBuffer *fb)
{
  AIDA_THROW_IF_FAIL (fb != NULL);
  // enqueue method call message
  const Aida::MessageId msgid = Aida::MessageId (fb->first_id());
  server_queue.send_msg (fb, true);
  // wait for result
  const bool needsresult = Aida::msgid_has_result (msgid);
  if (needsresult)
    block_for_result ();
  while (needsresult)
    {
      FieldBuffer *fr = client_queue.pop_msg();
      Aida::MessageId retid = Aida::MessageId (fr->first_id());
      if (Aida::msgid_is_error (retid))
        {
          FieldReader fbr (*fr);
          fbr.skip_msgid();
          std::string msg = fbr.pop_string();
          std::string dom = fbr.pop_string();
          warning_printf ("%s: %s", dom.c_str(), msg.c_str());
          delete fr;
        }
      else if (Aida::msgid_is_result (retid))
        return fr;
      else
        client_events.push_back (fr);
    }
  return NULL;
}

void
ConnectionTransport::server_dispatch ()
{
  FieldBuffer *fb = server_queue.fetch_msg();
  if (!fb)
    return;
  FieldReader fbr (*fb);
  const MessageId msgid = MessageId (fbr.pop_int64());
  const uint64_t  hashlow = fbr.pop_int64();
  const bool needsresult = msgid_has_result (msgid);
  struct Wrapper : ServerConnection { using ServerConnection::find_method; };
  const DispatchFunc method = Wrapper::find_method (msgid, hashlow);
  FieldBuffer *fr = NULL;
  if (method)
    {
      fr = method (fbr);
      const MessageId retid = MessageId (fr ? fr->first_id() : 0);
      if (fr && (!needsresult || !msgid_is_result (retid)))
        {
          warning_printf ("bogus result from method (%016lx%016llx): id=%016lx", msgid, hashlow, retid);
          delete fr;
          fr = NULL;
        }
    }
  else
    warning_printf ("unknown message (%016lx%016llx)", msgid, hashlow);
  delete fb;
  if (needsresult)
    {
      client_queue.send_msg (fr ? fr : FieldBuffer::new_result(), false);
      notify_for_result();
    }
}

void
ConnectionTransport::client_dispatch ()
{
  FieldBuffer *fb;
  if (!client_events.empty())
    {
      fb = client_events.front();
      client_events.pop_front();
    }
  else
    fb = client_queue.fetch_msg();
  if (!fb)
    return;
  FieldReader fbr (*fb);
  const MessageId msgid = MessageId (fbr.pop_int64());
  const uint64_t  hashlow = fbr.pop_int64();
  if (msgid_is_event (msgid))
    {
      const uint64_t handler_id = fbr.pop_int64();
      ClientConnection::EventHandler *evh = client_find_event_handler (handler_id);
      if (evh)
        {
          FieldBuffer *fr = evh->handle_event (*fb);
          if (fr)
            {
              FieldReader frr (*fr);
              const MessageId retid = MessageId (frr.pop_int64());
              frr.skip(); // msgid low
              if (msgid_is_error (retid))
                {
                  std::string msg = frr.pop_string(), domain = frr.pop_string();
                  if (domain.size())
                    domain += ": ";
                  msg = domain + msg;
                  warning_printf ("%s", msg.c_str());
                }
              delete fr;
            }
        }
      else
        warning_printf ("invalid signal handler id in %s: %llu", "event", handler_id);
    }
  else if (msgid_is_discon (msgid))
    {
      const uint64_t handler_id = fbr.pop_int64();
      const bool deleted = true; // FIXME: currently broken : connection.delete_event_handler (handler_id);
      if (!deleted)
        warning_printf ("invalid signal handler id in %s: %llu", "disconnect", handler_id);
    }
  else
    warning_printf ("unknown message (%016lx%016llx)", msgid, hashlow);
  delete fb;
}

// == ClientConnection ==
ClientConnection::ClientConnection (ServerConnection &server_connection) :
  m_transport (server_connection.m_transport->ref())
{}

ClientConnection::ClientConnection() :
  m_transport (NULL)
{}

ClientConnection::ClientConnection (const ClientConnection &clone) :
  m_transport (clone.m_transport ? clone.m_transport->ref() : NULL)
{}

void
ClientConnection::operator= (const ClientConnection &clone)
{
  ConnectionTransport *old = m_transport;
  m_transport = clone.m_transport ? clone.m_transport->ref() : NULL;
  if (old)
    old->unref();
}

ClientConnection::~ClientConnection ()
{
  if (m_transport)
    m_transport->unref();
  m_transport = NULL;
}

bool
ClientConnection::is_null () const              { return m_transport == NULL; }
int
ClientConnection::notify_fd ()                  { return m_transport->client_notify_fd(); }
bool
ClientConnection::pending ()                    { return m_transport->client_pending(); }
void
ClientConnection::dispatch ()                   { return m_transport->client_dispatch(); }
FieldBuffer*
ClientConnection::call_remote (FieldBuffer *fb) { return m_transport->client_call_remote (fb); }
uint64_t
ClientConnection::register_event_handler (EventHandler *evh) { return m_transport->client_register_event_handler (evh); }
ClientConnection::EventHandler*
ClientConnection::find_event_handler (uint64_t h_id)     { return m_transport->client_find_event_handler (h_id); }
bool
ClientConnection::delete_event_handler (uint64_t h_id)   { return m_transport->client_delete_event_handler (h_id); }

ClientConnection::EventHandler::~EventHandler()
{}

// == ServerConnection ==
ServerConnection::ServerConnection (ConnectionTransport &transport) :
  m_transport (transport.ref())
{}

ServerConnection::ServerConnection () :
  m_transport (NULL)
{}

ServerConnection::ServerConnection (const ServerConnection &clone) :
  m_transport (clone.m_transport ? clone.m_transport->ref() : NULL)
{}

void
ServerConnection::operator= (const ServerConnection &clone)
{
  ConnectionTransport *old = m_transport;
  m_transport = clone.m_transport ? clone.m_transport->ref() : NULL;
  if (old)
    old->unref();
}

ServerConnection::~ServerConnection ()
{
  if (m_transport)
    m_transport->unref();
  m_transport = NULL;
}

bool
ServerConnection::is_null () const              { return m_transport == NULL; }
void
ServerConnection::send_event (FieldBuffer *fb) { return m_transport->server_send_event (fb); }
int
ServerConnection::notify_fd () { return m_transport->server_notify_fd(); }
bool
ServerConnection::pending () { return m_transport->server_pending(); }
void
ServerConnection::dispatch () { return m_transport->server_dispatch(); }

ServerConnection
ServerConnection::create_threaded ()
{
  ConnectionTransport *t = new ConnectionTransport(); // ref_count=1
  ServerConnection scon (*t);
  t->unref();
  return scon;
}

// == MethodRegistry ==
static inline bool
operator< (const TypeHash &a, const TypeHash &b)
{
  return AIDA_UNLIKELY (a.typehi == b.typehi) ? a.typelo < b.typelo : a.typehi < b.typehi;
}
typedef std::map<TypeHash, DispatchFunc> DispatcherMap;
static DispatcherMap                    *dispatcher_map = NULL;
static pthread_mutex_t                   dispatcher_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool                              dispatcher_map_frozen = false;

static inline void
ensure_dispatcher_map()
{
  if (AIDA_UNLIKELY (dispatcher_map == NULL))
    {
      pthread_mutex_lock (&dispatcher_mutex);
      if (!dispatcher_map)
        dispatcher_map = new DispatcherMap();
      pthread_mutex_unlock (&dispatcher_mutex);
    }
}

DispatchFunc
ServerConnection::find_method (uint64_t hashhi, uint64_t hashlo)
{
  TypeHash typehash (hashhi, hashlo);
  ensure_dispatcher_map();
#if 1 // avoid costly mutex locking
  if (AIDA_UNLIKELY (dispatcher_map_frozen == false))
    dispatcher_map_frozen = true;
  return (*dispatcher_map)[typehash];
#else
  pthread_mutex_lock (&dispatcher_mutex);
  DispatchFunc dispatcher_func = (*dispatcher_map)[typehash];
  pthread_mutex_unlock (&dispatcher_mutex);
  return dispatcher_func;
#endif
}

void
ServerConnection::MethodRegistry::register_method (const MethodEntry &mentry)
{
  ensure_dispatcher_map();
  AIDA_THROW_IF_FAIL (dispatcher_map_frozen == false);
  pthread_mutex_lock (&dispatcher_mutex);
  DispatcherMap::size_type size_before = dispatcher_map->size();
  TypeHash typehash (mentry.hashhi, mentry.hashlo);
  (*dispatcher_map)[typehash] = mentry.dispatcher;
  DispatcherMap::size_type size_after = dispatcher_map->size();
  pthread_mutex_unlock (&dispatcher_mutex);
  // simple hash collision check (sanity check, see below)
  if (AIDA_UNLIKELY (size_before == size_after))
    {
      errno = EKEYREJECTED;
      perror (string_cprintf ("%s:%u: Aida::ServerConnection::MethodRegistry::register_method: "
                              "duplicate hash registration (%016llx%016llx)",
                              __FILE__, __LINE__, mentry.hashhi, mentry.hashlo).c_str());
      abort();
    }
}

} } // Rapicorn::Aida

#include "aidamap.cc"
