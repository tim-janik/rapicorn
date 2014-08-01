// CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0/
#include "aida.hh"
#include "aidaprops.hh"
#include "thread.hh"
#include "regex.hh"
#include "objects.hh"           // BaseObject*
#include "../configure.h"       // HAVE_SYS_EVENTFD_H

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
#include <poll.h>
#include <stddef.h>             // ptrdiff_t
#ifdef  HAVE_SYS_EVENTFD_H
#include <sys/eventfd.h>
#endif // HAVE_SYS_EVENTFD_H
#include <stdexcept>
#include <deque>
#include <unordered_map>

// == Auxillary macros ==
#ifndef __GNUC__
#define __PRETTY_FUNCTION__                     __func__
#endif
#define AIDA_CPP_PASTE2i(a,b)                   a ## b // indirection required to expand __LINE__ etc
#define AIDA_CPP_PASTE2(a,b)                    AIDA_CPP_PASTE2i (a,b)
#define ALIGN4(sz,unit)                         (sizeof (unit) * ((sz + sizeof (unit) - 1) / sizeof (unit)))

namespace Rapicorn {
/// The Aida namespace provides all IDL functionality exported to C++.
namespace Aida {

// == EnumValue ==
size_t
enum_value_count (const EnumValue *values)
{
  size_t n = 0;
  if (values)
    while (values[n].ident)
      n++;
  return n;
}

const EnumValue*
enum_value_find (const EnumValue *values, int64 value)
{
  if (values)
    for (size_t i = 0; values[i].ident; i++)
      if (values[i].value == value)
        return &values[i];
  return NULL;
}

const EnumValue*
enum_value_find (const EnumValue *values, const String &name)
{
  if (values)
    for (size_t i = 0; values[i].ident; i++)
      if (string_match_identifier_tail (values[i].ident, name))
        return &values[i];
  return NULL;
}

// == TypeKind ==
template<> const EnumValue*
enum_value_list<TypeKind> ()
{
  static const EnumValue values[] = {
    { UNTYPED,          "UNTYPED",              NULL, NULL },
    { VOID,             "VOID",                 NULL, NULL },
    { BOOL,             "BOOL",                 NULL, NULL },
    { INT32,            "INT32",                NULL, NULL },
    { INT64,            "INT64",                NULL, NULL },
    { FLOAT64,          "FLOAT64",              NULL, NULL },
    { STRING,           "STRING",               NULL, NULL },
    { ENUM,             "ENUM",                 NULL, NULL },
    { SEQUENCE,         "SEQUENCE",             NULL, NULL },
    { RECORD,           "RECORD",               NULL, NULL },
    { INSTANCE,         "INSTANCE",             NULL, NULL },
    { FUNC,             "FUNC",                 NULL, NULL },
    { TYPE_REFERENCE,   "TYPE_REFERENCE",       NULL, NULL },
    { LOCAL,            "LOCAL",                NULL, NULL },
    { REMOTE,           "REMOTE",               NULL, NULL },
    { ANY,              "ANY",                  NULL, NULL },
    { 0,                NULL,                   NULL, NULL }     // sentinel
  };
  return values;
}
template const EnumValue* enum_value_list<TypeKind> ();

const char*
type_kind_name (TypeKind type_kind)
{
  const EnumValue *ev = enum_value_find (enum_value_list<TypeKind>(), type_kind);
  return ev ? ev->ident : NULL;
}

// == SignalHandlerIdParts ==
union SignalHandlerIdParts {
  size_t   vsize;
  struct { // Bits
#if __SIZEOF_SIZE_T__ == 8
    uint   signal_handler_index : 24;
    uint   unused1 : 8;
    uint   unused2 : 16;
    uint   orbid_connection : 16;
#else // 4
    uint   signal_handler_index : 16;
    uint   orbid_connection : 16;
#endif
  };
  SignalHandlerIdParts (size_t handler_id) : vsize (handler_id) {}
  SignalHandlerIdParts (uint handler_index, uint connection_id) :
    signal_handler_index (handler_index),
#if __SIZEOF_SIZE_T__ == 8
    unused1 (0), unused2 (0),
#endif
    orbid_connection (connection_id)
  {}
};

// == FieldUnion ==
static_assert (sizeof (FieldUnion::smem) <= sizeof (FieldUnion::bytes), "sizeof FieldUnion::smem");
static_assert (sizeof (FieldBuffer) <= sizeof (FieldUnion), "sizeof FieldBuffer");

// === Utilities ===
void
assertion_error (const char *file, uint line, const char *expr)
{
  fatal_error (string_format ("%s:%u: assertion failed: %s", file, line, expr));
}

void
fatal_error (const String &msg)
{
  String s = msg;
  if (s.empty() || s[s.size() - 1] != '\n')
    s += "\n";
  fprintf (stderr, "Aida: error: %s", s.c_str());
  fflush (stderr);
  abort();
}

void
print_warning (const String &msg)
{
  String s = msg;
  if (s.empty() || s[s.size() - 1] != '\n')
    s += "\n";
  fprintf (stderr, "Aida: warning: %s", s.c_str());
  fflush (stderr);
}

// == ImplicitBase ==
ImplicitBase::~ImplicitBase()
{
  // this destructor implementation forces vtable emission
}

// == Any ==
RAPICORN_STATIC_ASSERT (sizeof (std::string) <= sizeof (Any)); // assert big enough Any impl

Any&
Any::operator= (const Any &clone)
{
  if (this == &clone)
    return *this;
  reset();
  type_kind_ = clone.type_kind_;
  switch (kind())
    {
    case STRING:        new (&u_.vstring()) String (clone.u_.vstring());  break;
    case ANY:           u_.vany = new Any (*clone.u_.vany);               break;
    case SEQUENCE:      u_.vanys = new AnyVector (*clone.u_.vanys);       break;
    case RECORD:        u_.vfields = new FieldVector (*clone.u_.vfields); break;
    case INSTANCE:      u_.shandle = new RemoteHandle (*clone.u_.shandle); break;
    case LOCAL:         u_.pholder = clone.u_.pholder ? clone.u_.pholder->clone() : NULL; break;
    default:            u_ = clone.u_;                                    break;
    }
  return *this;
}

void
Any::reset()
{
  switch (kind())
    {
    case STRING:        u_.vstring().~String();                 break;
    case ANY:           delete u_.vany;                         break;
    case SEQUENCE:      delete u_.vanys;                        break;
    case RECORD:        delete u_.vfields;                      break;
    case INSTANCE:      delete u_.shandle;                      break;
    case LOCAL:         delete u_.pholder;                      break;
    default: ;
    }
  type_kind_ = UNTYPED;
  u_.vuint64 = 0;
}

void
Any::rekind (TypeKind _kind)
{
  reset();
  type_kind_ = _kind;
  switch (_kind)
    {
    case STRING:   new (&u_.vstring()) String();                               break;
    case ANY:      u_.vany = new Any();                                        break;
    case SEQUENCE: u_.vanys = new AnyVector();                                 break;
    case RECORD:   u_.vfields = new FieldVector();                             break;
    case INSTANCE: u_.shandle = new RemoteHandle (RemoteHandle::_null_handle()); break;
    default:       break;
    }
}

template<class T> String any_field_name (const T          &);
template<>        String any_field_name (const Any        &any) { return ""; }
template<>        String any_field_name (const Any::Field &any) { return any.name; }

template<class AnyVector> static String
any_vector_to_string (const AnyVector &av)
{
  String s;
  for (auto const &any : av)
    {
      if (!s.empty())
        s += ", ";
      s += any.to_string (any_field_name (any));
    }
  s = s.empty() ? "[]" : "[ " + s + " ]";
  return s;
}

String
Any::to_string (const String &field_name) const
{
  String s = "{ ";
  s += "type=" + Rapicorn::string_to_cquote (type_kind_name (kind()));
  if (!field_name.empty())
    s += ", name=" + Rapicorn::string_to_cquote (field_name);
  switch (kind())
    {
    case BOOL:
    case ENUM:
    case INT32:
    case INT64:         s += string_format (", value=%d", u_.vint64);                           break;
    case FLOAT64:       s += string_format (", value=%.17g", u_.vdouble);                       break;
    case ANY:           s += ", value=" + u_.vany->to_string();                                 break;
    case STRING:        s += ", value=" + Rapicorn::string_to_cquote (u_.vstring());            break;
    case SEQUENCE:      if (u_.vanys) s += ", value=" + any_vector_to_string (*u_.vanys);       break;
    case RECORD:        if (u_.vfields) s += ", value=" + any_vector_to_string (*u_.vfields);   break;
    case INSTANCE:      s += string_format (", value=#%08x", u_.shandle->_orbid());             break;
    default:            ;
    case UNTYPED:       break;
    }
  s += " }";
  return s;
}

bool
Any::operator== (const Any &clone) const
{
  if (type_kind_ != clone.type_kind_)
    return false;
  switch (kind())
    {
    case UNTYPED:     break;
      // case UINT: // chain
    case BOOL: case ENUM: // chain
    case INT32:
    case INT64:       if (u_.vint64 != clone.u_.vint64) return false;                     break;
    case FLOAT64:     if (u_.vdouble != clone.u_.vdouble) return false;                   break;
    case STRING:      if (u_.vstring() != clone.u_.vstring()) return false;               break;
    case SEQUENCE:    if (*u_.vanys != *clone.u_.vanys) return false;                     break;
    case RECORD:      if (*u_.vfields != *clone.u_.vfields) return false;                 break;
    case INSTANCE:    if ((u_.shandle ? u_.shandle->_orbid() : 0) != (clone.u_.shandle ? clone.u_.shandle->_orbid() : 0)) return false; break;
    case ANY:         if (*u_.vany != *clone.u_.vany) return false;                       break;
    case LOCAL:
      if (u_.pholder)
        return u_.pholder->operator== (clone.u_.pholder);
      else
        return !clone.u_.pholder;
    default:
      fatal_error (String() + "Aida::Any:operator==: invalid type kind: " + type_kind_name (kind()));
    }
  return true;
}

bool
Any::operator!= (const Any &clone) const
{
  return !operator== (clone);
}

void
Any::swap (Any &other)
{
  constexpr size_t USIZE = sizeof (this->u_);
  uint64 buffer[(USIZE + 7) / 8];
  memcpy (buffer, &other.u_, USIZE);
  memcpy (&other.u_, &this->u_, USIZE);
  memcpy (&this->u_, buffer, USIZE);
  std::swap (type_kind_, other.type_kind_);
}

void
Any::hold (PlaceHolder *ph)
{
  ensure (LOCAL);
  u_.pholder = ph;
}

bool
Any::to_int (int64 &v, char b) const
{
  if (kind() != BOOL && kind() != INT32 && kind() != INT64)
    return false;
  bool s = 0;
  switch (b)
    {
    case 1:     s =  u_.vint64 >=         0 &&  u_.vint64 <= 1;        break;
    case 7:     s =  u_.vint64 >=      -128 &&  u_.vint64 <= 127;      break;
    case 8:     s =  u_.vint64 >=         0 &&  u_.vint64 <= 256;      break;
    case 47:    s = sizeof (LongIffy) == sizeof (int64); // chain
    case 31:    s |= u_.vint64 >=   INT_MIN &&  u_.vint64 <= INT_MAX;  break;
    case 48:    s = sizeof (ULongIffy) == sizeof (int64); // chain
    case 32:    s |= u_.vint64 >=         0 &&  u_.vint64 <= UINT_MAX; break;
    case 63:    s = 1; break;
    case 64:    s = 1; break;
    default:    s = 0; break;
    }
  if (s)
    v = u_.vint64;
  return s;
}

int64
Any::as_int () const
{
  switch (kind())
    {
    case BOOL: case INT32: case INT64:
    case ENUM:          return u_.vint64;
    case FLOAT64:       return u_.vdouble;
    case STRING:        return !u_.vstring().empty();
    case SEQUENCE:      return !u_.vanys->empty();
    case RECORD:        return !u_.vfields->empty();
    case INSTANCE:      return u_.shandle && u_.shandle->_orbid();
    default:            return 0;
    }
}

double
Any::as_float () const
{
  switch (kind())
    {
    case FLOAT64:       return u_.vdouble;
    default:            return as_int();
    }
}

std::string
Any::as_string() const
{
  switch (kind())
    {
    case BOOL: case ENUM:
    case INT32:
    case INT64:         return string_format ("%i", u_.vint64);
    case FLOAT64:       return string_format ("%.17g", u_.vdouble);
    case STRING:        return u_.vstring();
    default:            return "";
    }
}

bool
Any::operator>>= (EnumValue &v) const
{
  if (kind() != ENUM)
    return false;
  v = EnumValue (u_.vint64);
  return true;
}

bool
Any::operator>>= (double &v) const
{
  if (kind() != FLOAT64)
    return false;
  v = u_.vdouble;
  return true;
}

bool
Any::operator>>= (std::string &v) const
{
  if (kind() != STRING)
    return false;
  v = u_.vstring();
  return true;
}

bool
Any::operator>>= (const Any *&v) const
{
  if (kind() != ANY)
    return false;
  v = u_.vany;
  return true;
}

bool
Any::operator>>= (const AnyVector *&v) const
{
  if (kind() != SEQUENCE)
    return false;
  v = u_.vanys;
  return true;
}

bool
Any::operator>>= (const FieldVector *&v) const
{
  if (kind() != RECORD)
    return false;
  v = u_.vfields;
  return true;
}

bool
Any::operator>>= (RemoteHandle &v)
{
  if (kind() != INSTANCE)
    return false;
  v = u_.shandle ? *u_.shandle : RemoteHandle::_null_handle();
  return true;
}

void
Any::operator<<= (bool v)
{
  ensure (BOOL);
  u_.vint64 = v;
}

void
Any::operator<<= (int32 v)
{
  ensure (INT32);
  u_.vint64 = v;
}

void
Any::operator<<= (int64 v)
{
  ensure (INT64);
  u_.vint64 = v;
}

void
Any::operator<<= (uint64 v)
{
  // ensure (UINT);
  operator<<= (int64 (v));
}

void
Any::operator<<= (const EnumValue &v)
{
  ensure (ENUM);
  u_.vint64 = v.value;
}

void
Any::operator<<= (double v)
{
  ensure (FLOAT64);
  u_.vdouble = v;
}

void
Any::operator<<= (const String &v)
{
  ensure (STRING);
  u_.vstring().assign (v);
}

void
Any::operator<<= (const Any &v)
{
  ensure (ANY);
  if (u_.vany != &v)
    {
      Any *old = u_.vany;
      u_.vany = new Any (v);
      if (old)
        delete old;
    }
}

void
Any::operator<<= (const AnyVector &v)
{
  ensure (SEQUENCE);
  if (u_.vanys != &v)
    {
      AnyVector *old = u_.vanys;
      u_.vanys = new AnyVector (v);
      if (old)
        delete old;
    }
}

void
Any::operator<<= (const FieldVector &v)
{
  ensure (RECORD);
  if (u_.vfields != &v)
    {
      FieldVector *old = u_.vfields;
      u_.vfields = new FieldVector (v);
      if (old)
        delete old;
    }
}

void
Any::operator<<= (const RemoteHandle &v)
{
  ensure (INSTANCE);
  RemoteHandle *old = u_.shandle;
  u_.shandle = new RemoteHandle (v);
  if (old)
    delete old;
}

// == OrbObject ==
OrbObject::OrbObject (uint64 orbid) :
  orbid_ (orbid)
{}

OrbObject::~OrbObject()
{} // force vtable emission

class NullOrbObject : public virtual OrbObject {
public:
  explicit NullOrbObject() : OrbObject (0) {}
  virtual ~NullOrbObject()                 {}
};

// == RemoteHandle ==
static void (RemoteHandle::*pmf_cast_null_into) (const RemoteHandle&);

OrbObjectP
RemoteHandle::null_orb_object ()
{
  static OrbObjectP null_orbo = [] () {                 // use lambda to sneak in extra code
    pmf_cast_null_into = &RemoteHandle::cast_null_into; // export cast_null_into() for internal upgrades
    return std::make_shared<NullOrbObject> ();
  } ();                                                 // executes lambda atomically
  return null_orbo;
}

RemoteHandle::RemoteHandle (OrbObjectP orbo) :
  orbop_ (orbo ? orbo : null_orb_object())
{}

void
RemoteHandle::reset ()
{
  orbop_ = null_orb_object();
}

void
RemoteHandle::cast_null_into (const RemoteHandle &source)
{
  AIDA_ASSERT (_orbid() == 0);
  orbop_ = source.orbop_;
}

RemoteHandle::~RemoteHandle()
{}

struct OrbObjectRemoteHandle : RemoteHandle {
  OrbObjectRemoteHandle (OrbObjectP orbo) : RemoteHandle (orbo && orbo->orbid() ? orbo : null_orb_object()) {}
};

// == FieldBuffer ==
FieldBuffer::FieldBuffer (uint32 _ntypes) :
  buffermem (NULL)
{
  static_assert (sizeof (FieldBuffer) <= sizeof (FieldUnion), "sizeof FieldBuffer");
  // buffermem layout: [{n_types,nth}] [{type nibble} * n_types]... [field]...
  const uint _offs = 1 + (_ntypes + 7) / 8;
  buffermem = new FieldUnion[_offs + _ntypes];
  wmemset ((wchar_t*) buffermem, 0, sizeof (FieldUnion[_offs]) / sizeof (wchar_t));
  buffermem[0].capacity = _ntypes;
  buffermem[0].index = 0;
}

FieldBuffer::FieldBuffer (uint32    _ntypes,
                          FieldUnion *_bmem,
                          uint32    _bmemlen) :
  buffermem (_bmem)
{
  const uint32 _offs = 1 + (_ntypes + 7) / 8;
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
    fatal_error (string_format ("FieldBuffer(this=%p): capacity=%u size=%u", this, capacity(), size()));
}

void
FieldReader::check_request (int type)
{
  if (nth_ >= n_types())
    fatal_error (string_format ("FieldReader(this=%p): size=%u requested-index=%u", this, n_types(), nth_));
  if (get_type() != type)
    fatal_error (string_format ("FieldReader(this=%p): size=%u index=%u type=%s requested-type=%s",
                                this, n_types(), nth_,
                                FieldBuffer::type_name (get_type()).c_str(), FieldBuffer::type_name (type).c_str()));
}

uint64
FieldReader::debug_bits ()
{
  return fb_->upeek (nth_).vint64;
}

std::string
FieldBuffer::first_id_str() const
{
  uint64 fid = first_id();
  return string_format ("%016x", fid);
}

static std::string
strescape (const std::string &str)
{
  std::string buffer;
  for (std::string::const_iterator it = str.begin(); it != str.end(); it++)
    {
      uint8_t d = *it;
      if (d < 32 || d > 126 || d == '?')
        buffer += string_format ("\\%03o", d);
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
  return string_format ("<invalid:%d>", field_type);
}

std::string
FieldBuffer::to_string() const
{
  String s = string_format ("Aida::FieldBuffer(%p)={", this);
  s += string_format ("size=%u, capacity=%u", size(), capacity());
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
        case VOID:      s += string_format (", %s", tn); fbr.skip();                               break;
        case BOOL:      s += string_format (", %s: 0x%x", tn, fbr.pop_bool());                     break;
        case ENUM:      s += string_format (", %s: 0x%x", tn, fbr.pop_evalue());                   break;
        case INT32:     s += string_format (", %s: 0x%08x", tn, fbr.pop_int64());                  break;
        case INT64:     s += string_format (", %s: 0x%016x", tn, fbr.pop_int64());                 break;
        case FLOAT64:   s += string_format (", %s: %.17g", tn, fbr.pop_double());                  break;
        case STRING:    s += string_format (", %s: %s", tn, strescape (fbr.pop_string()).c_str()); break;
        case SEQUENCE:  s += string_format (", %s: %p", tn, &fbr.pop_seq());                       break;
        case RECORD:    s += string_format (", %s: %p", tn, &fbr.pop_rec());                       break;
        case INSTANCE:  s += string_format (", %s: %p", tn, (void*) fbr.debug_bits()); fbr.skip(); break;
        case ANY:       s += string_format (", %s: %p", tn, (void*) fbr.debug_bits()); fbr.skip(); break;
        default:        s += string_format (", %u: <unknown>", fbr.get_type()); fbr.skip();        break;
        }
    }
  s += '}';
  return s;
}

#if 0
FieldBuffer*
FieldBuffer::new_error (const String &msg,
                        const String &domain)
{
  FieldBuffer *fr = FieldBuffer::_new (3 + 2);
  fr->add_header1 (MSGID_ERROR, 0, 0, 0);
  fr->add_string (msg);
  fr->add_string (domain);
  return fr;
}
#endif

FieldBuffer*
FieldBuffer::new_result (MessageId m, uint rconnection, uint64 h, uint64 l, uint32 n)
{
  assert_return (msgid_is_result (m) && rconnection <= CONNECTION_MASK && rconnection, NULL);
  FieldBuffer *fr = FieldBuffer::_new (3 + n);
  fr->add_header1 (m, rconnection, h, l);
  return fr;
}

FieldBuffer*
FieldBuffer::renew_into_result (FieldBuffer *fb, MessageId m, uint rconnection, uint64 h, uint64 l, uint32 n)
{
  assert_return (msgid_is_result (m) && rconnection <= CONNECTION_MASK && rconnection, NULL);
  if (fb->capacity() < 3 + n)
    return FieldBuffer::new_result (m, rconnection, h, l, n);
  FieldBuffer *fr = fb;
  fr->reset();
  fr->add_header1 (m, rconnection, h, l);
  return fr;
}

FieldBuffer*
FieldBuffer::renew_into_result (FieldReader &fbr, MessageId m, uint rconnection, uint64 h, uint64 l, uint32 n)
{
  FieldBuffer *fb = const_cast<FieldBuffer*> (fbr.field_buffer());
  fbr.reset();
  return renew_into_result (fb, m, rconnection, h, l, n);
}

class OneChunkFieldBuffer : public FieldBuffer {
  virtual ~OneChunkFieldBuffer () { reset(); buffermem = NULL; }
  explicit OneChunkFieldBuffer (uint32    _ntypes,
                                FieldUnion *_bmem,
                                uint32    _bmemlen) :
    FieldBuffer (_ntypes, _bmem, _bmemlen)
  {}
public:
  static OneChunkFieldBuffer*
  _new (uint32 _ntypes)
  {
    const uint32 _offs = 1 + (_ntypes + 7) / 8;
    size_t bmemlen = sizeof (FieldUnion[_offs + _ntypes]);
    size_t objlen = ALIGN4 (sizeof (OneChunkFieldBuffer), int64);
    uint8_t *omem = (uint8_t*) operator new (objlen + bmemlen);
    FieldUnion *bmem = (FieldUnion*) (omem + objlen);
    return new (omem) OneChunkFieldBuffer (_ntypes, bmem, bmemlen);
  }
};

FieldBuffer*
FieldBuffer::_new (uint32 _ntypes)
{
  return OneChunkFieldBuffer::_new (_ntypes);
}

// == EventFd ==
EventFd::EventFd () :
  fds { -1, -1 }
{}

int
EventFd::open ()
{
  if (opened())
    return 0;
  long nflags;
#ifdef HAVE_SYS_EVENTFD_H
  do
    fds[0] = eventfd (0 /*initval*/, EFD_CLOEXEC | EFD_NONBLOCK);
  while (fds[0] < 0 && (errno == EAGAIN || errno == EINTR));
#else
  int err;
  do
    err = pipe2 (fds, O_CLOEXEC | O_NONBLOCK);
  while (err < 0 && (errno == EAGAIN || errno == EINTR));
  if (fds[1] >= 0)
    {
      nflags = fcntl (fds[1], F_GETFL, 0);
      assert (nflags & O_NONBLOCK);
      nflags = fcntl (fds[1], F_GETFD, 0);
      assert (nflags & FD_CLOEXEC);
    }
#endif
  if (fds[0] >= 0)
    {
      nflags = fcntl (fds[0], F_GETFL, 0);
      assert (nflags & O_NONBLOCK);
      nflags = fcntl (fds[0], F_GETFD, 0);
      assert (nflags & FD_CLOEXEC);
      return 0;
    }
  return -errno;
}

int
EventFd::inputfd () // fd for POLLIN
{
  return fds[0];
}

bool
EventFd::opened ()
{
  return inputfd() >= 0;
}

bool
EventFd::pollin ()
{
  struct pollfd pfd = { inputfd(), POLLIN, 0 };
  int presult;
  do
    presult = poll (&pfd, 1, -1);
  while (presult < 0 && (errno == EAGAIN || errno == EINTR));
  return pfd.revents != 0;
}

void
EventFd::wakeup()
{
  int err;
#ifdef HAVE_SYS_EVENTFD_H
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

void
EventFd::flush()
{
  int err;
#ifdef HAVE_SYS_EVENTFD_H
  eventfd_t bytes8;
  do
    err = eventfd_read (fds[0], &bytes8);
  while (err < 0 && errno == EINTR);
#else
  char buffer[512]; // 512 is POSIX pipe atomic read/write size
  do
    err = read (fds[0], buffer, sizeof (buffer));
  while (err == 512 || (err < 0 && errno == EINTR));
#endif
  // EAGAIN occours if no wakeups are pending
}

EventFd::~EventFd ()
{
#ifdef HAVE_SYS_EVENTFD_H
  close (fds[0]);
#else
  close (fds[0]);
  close (fds[1]);
#endif
  fds[0] = -1;
  fds[1] = -1;
}

// == lock-free, single-consumer queue ==
template<class Data> struct MpScQueueF {
  struct Node { Node *next; Data data; };
  MpScQueueF() :
    head_ (NULL), local_ (NULL)
  {}
  bool
  push (Data data)
  {
    Node *node = new Node;
    node->data = data;
    Node *last_head;
    do
      node->next = last_head = head_;
    while (!__sync_bool_compare_and_swap (&head_, node->next, node));
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
    if (AIDA_UNLIKELY (!local_))
      {
        Node *prev, *next, *node;
        do
          node = head_;
        while (!__sync_bool_compare_and_swap (&head_, node, NULL));
        for (prev = NULL; node; node = next)
          {
            next = node->next;
            node->next = prev;
            prev = node;
          }
        local_ = prev;
      }
    if (local_)
      {
        Node *node = local_;
        local_ = node->next;
        return node;
      }
    else
      return NULL;
  }
private:
  Node  *head_ __attribute__ ((aligned (64)));
  // we pad/align with CPU_CACHE_LINE_SIZE to avoid false sharing between pushing and popping threads
  Node  *local_ __attribute__ ((aligned (64)));
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
  {
    const int err = open();
    if (err != 0)
      fatal_error (string_format ("failed to create wakeup pipe: %s", strerror (-err)));
  }
};

// == TypeNameDB ==
class TypeNameDB {
  RWLock                                mutex_;
  std::vector<std::string>              vector_;
  std::unordered_map<std::string, uint> map_;
public:
  uint
  index (const std::string &type_name)
  {
    mutex_.rdlock();
    auto it = map_.find (type_name);
    uint result_type_index = it == map_.end() ? 0 : it->second;
    mutex_.unlock();
    if (result_type_index)
      return result_type_index;
    mutex_.wrlock();
    vector_.push_back (type_name);
    result_type_index = vector_.size();
    map_[type_name] = result_type_index;
    mutex_.unlock();
    assert (result_type_index < 65536); // see IdentifierParts.orbid_type_index
    return result_type_index;
  }
  std::string
  type_name (const uint type_index)
  {
    std::string result;
    if (type_index > 0)
      {
        mutex_.rdlock();
        if (type_index <= vector_.size())
          result = vector_[type_index - 1];
        mutex_.unlock();
      }
    return result;
  }
};
static TypeNameDB type_name_db;

// == ObjectMap ==
template<class Instance>
class ObjectMap {
public:
  typedef std::shared_ptr<Instance>     InstanceP;
private:
  struct Entry {
    OrbObjectP  orbop;
    InstanceP   instancep;
  };
  size_t                                start_id_, id_mask_;
  std::vector<Entry>                    entries_;
  std::unordered_map<Instance*, uint64> map_;
  std::vector<uint>                     free_list_;
  std::function<uint64 (InstanceP)>     orbid_hook_;
  class MappedObject : public virtual OrbObject {
    ObjectMap &omap_;
  public:
    explicit MappedObject (uint64 orbid, ObjectMap &omap) : OrbObject (orbid), omap_ (omap) {}
    virtual ~MappedObject ()                            { omap_.delete_orbid (orbid()); } // FIXME: might kill other, ABA problem
  };
  void          delete_orbid            (uint64            orbid);
  uint          next_index              ();
  template<class Num> static constexpr uint64_t
  hash_fnv64a (const Num *data, size_t length, uint64_t hash = 0xcbf29ce484222325)
  {
    return length ? hash_fnv64a (data + 1, length - 1, 0x100000001b3 * (hash ^ data[0])) : hash;
  }
public:
  explicit   ObjectMap          (size_t            start_id = 0) : start_id_ (start_id), id_mask_ (0xffffffffffffffff) {}
  /*dtor*/  ~ObjectMap          ()                 { assert (entries_.size() == 0); assert (map_.size() == 0); }
  OrbObjectP orbo_from_instance (InstanceP         instancep);
  InstanceP  instance_from_orbo (const OrbObjectP &orbo);
  OrbObjectP orbo_from_orbid    (uint64            orbid);
  void       assign_start_id    (uint64 start_id, uint64 mask = 0xffffffffffffffff);
  void       assign_orbid_hook  (const std::function<uint64 (InstanceP)> &orbid_hook);
};

template<class Instance> void
ObjectMap<Instance>::assign_start_id (uint64 start_id, uint64 id_mask)
{
  assert (entries_.size() == 0);
  assert ((start_id_ & id_mask) == start_id_);
  start_id_ = start_id;
  assert (id_mask > 0);
  id_mask_ = id_mask;
  assert (map_.size() == 0);
}

template<class Instance> void
ObjectMap<Instance>::assign_orbid_hook (const std::function<uint64 (InstanceP)> &orbid_hook)
{
  orbid_hook_ = orbid_hook;
}

template<class Instance> void
ObjectMap<Instance>::delete_orbid (uint64 orbid)
{
  assert ((orbid & id_mask_) >= start_id_);
  const uint64 index = (orbid & id_mask_) - start_id_;
  assert (index < entries_.size());
  Entry &e = entries_[index];
  assert (e.instancep != NULL);
  e.instancep = NULL;
  e.orbop = NULL;
  free_list_.push_back (index);
}

template<class Instance> uint
ObjectMap<Instance>::next_index ()
{
  uint idx;
  const size_t FREE_LENGTH = 5;
  if (free_list_.size() > FREE_LENGTH)
    {
      const size_t prandom = hash_fnv64a (free_list_.data(), free_list_.size());
      const size_t end = free_list_.size(), j = prandom % (end - 1);
      assert (j < end - 1); // use end-1 to avoid popping the last pushed slot
      idx = free_list_[j];
      free_list_[j] = free_list_[end - 1];
      free_list_.pop_back();
    }
  else
    {
      idx = entries_.size();
      entries_.resize (idx + 1);
    }
  return idx;
}

template<class Instance> OrbObjectP
ObjectMap<Instance>::orbo_from_instance (InstanceP instancep)
{
  OrbObjectP orbo;
  if (instancep)
    {
      uint64 orbid = map_[instancep.get()];
      if (AIDA_UNLIKELY (orbid == 0))
        {
          const uint64 index = next_index();
          uint64 masked_bits = 0;
          if (orbid_hook_)
            {
              masked_bits = orbid_hook_ (instancep);
              assert ((masked_bits & id_mask_) == 0);
            }
          orbid = (start_id_ + index) | masked_bits;
          orbo = std::make_shared<MappedObject> (orbid, *this);
          Entry e { orbo, instancep };
          entries_[index] = e;
          map_[instancep.get()] = orbid;
        }
      else
        orbo = entries_[(orbid & id_mask_) - start_id_].orbop;
    }
  return orbo;
}

template<class Instance> OrbObjectP
ObjectMap<Instance>::orbo_from_orbid (uint64 orbid)
{
  assert ((orbid & id_mask_) >= start_id_);
  const uint64 index = (orbid & id_mask_) - start_id_;
  if (index < entries_.size() && entries_[index].instancep) // check for deletion
    return entries_[index].orbop;
  return OrbObjectP();
}

template<class Instance> std::shared_ptr<Instance>
ObjectMap<Instance>::instance_from_orbo (const OrbObjectP &orbo)
{
  const uint64 orbid = orbo ? orbo->orbid() : 0;
  if ((orbid & id_mask_) >= start_id_)
    {
      const uint64 index = (orbid & id_mask_) - start_id_;
      if (index < entries_.size())
        return entries_[index].instancep;
    }
  return NULL;
}

// == BaseConnection ==
BaseConnection::BaseConnection (const std::string &feature_keys) :
  feature_keys_ (feature_keys), conid_ (0)
{
  AIDA_ASSERT (feature_keys_.size() && feature_keys_[0] == ':' && feature_keys_[feature_keys_.size()-1] == ':');
}

BaseConnection::~BaseConnection ()
{}

void
BaseConnection::assign_id (uint connection_id)
{
  assert_return (conid_ == 0);
  conid_ = connection_id;
}

/// Provide initial handle for remote connections.
void
BaseConnection::remote_origin (ImplicitBaseP)
{
  assertion_error (__FILE__, __LINE__, "not supported by this object type");
}

/** Retrieve initial handle after remote connection has been established.
 * The @a feature_key_list contains key=value pairs, where value is assumed to be "1" if
 * omitted and generally treated as a regular expression to match against connection
 * feature keys as registered with the ObjectBroker.
 */
RemoteHandle
BaseConnection::remote_origin (const vector<std::string> &feature_key_list)
{
  assertion_error (__FILE__, __LINE__, "not supported by this object type");
}

Any*
BaseConnection::any2remote (const Any &any)
{
  return new Any (any);
}

void
BaseConnection::any2local (Any &any)
{
  // any = any
}

// == ClientConnection ==
ClientConnection::ClientConnection (const std::string &feature_keys) :
  BaseConnection (feature_keys)
{}

ClientConnection::~ClientConnection ()
{}

// == ClientConnectionImpl ==
class ClientConnectionImpl : public ClientConnection {
  struct SignalHandler {
    uint64 hhi, hlo, cid;
    RemoteMember<RemoteHandle> remote;
    SignalEmitHandler *seh;
    void *data;
  };
  typedef std::set<uint64> UIntSet;
  pthread_spinlock_t            signal_spin_;
  TransportChannel              transport_channel_;     // messages arriving at client
  sem_t                         transport_sem_;         // signal incomming results
  std::deque<FieldBuffer*>      event_queue_;           // messages pending for client
  std::vector<SignalHandler*>   signal_handlers_;
  SignalHandler*                signal_lookup (size_t handler_id);
  UIntSet                       ehandler_set; // client event handler
  bool                          blocking_for_sem_;
public:
  ClientConnectionImpl (const std::string &feature_keys) :
    ClientConnection (feature_keys), blocking_for_sem_ (false)
  {
    signal_handlers_.push_back (NULL); // reserve 0 for NULL
    pthread_spin_init (&signal_spin_, 0 /* pshared */);
    sem_init (&transport_sem_, 0 /* unshared */, 0 /* init */);
    uint realid = ObjectBroker::register_connection (*this, 0xcccc);
    if (!realid)
      fatal_error ("Aida: failed to register ClientConnection");
    assign_id (realid);
  }
  ~ClientConnectionImpl ()
  {
    ObjectBroker::unregister_connection (*this);
    sem_destroy (&transport_sem_);
    pthread_spin_destroy (&signal_spin_);
  }
  virtual void
  send_msg (FieldBuffer *fb)
  {
    assert_return (fb);
    transport_channel_.send_msg (fb, !blocking_for_sem_);
    notify_for_result();
  }
  void                  notify_for_result ()            { if (blocking_for_sem_) sem_post (&transport_sem_); }
  void                  block_for_result  ()            { AIDA_ASSERT (blocking_for_sem_); sem_wait (&transport_sem_); }
  virtual int           notify_fd         ()            { return transport_channel_.inputfd(); }
  virtual bool          pending           ()            { return !event_queue_.empty() || transport_channel_.has_msg(); }
  virtual FieldBuffer*  call_remote       (FieldBuffer*);
  virtual FieldBuffer*  pop               ();
  virtual void          dispatch          ();
  virtual void          add_handle        (FieldBuffer &fb, const RemoteHandle &rhandle);
  virtual void          pop_handle        (FieldReader &fr, RemoteHandle &rhandle);
  virtual void          remote_origin     (ImplicitBaseP rorigin) { fatal ("assert not reached"); }
  virtual RemoteHandle  remote_origin     (const vector<std::string> &feature_key_list);
  virtual size_t        signal_connect    (uint64 hhi, uint64 hlo, const RemoteHandle &rhandle, SignalEmitHandler seh, void *data);
  virtual bool          signal_disconnect (size_t signal_handler_id);
  virtual std::string   type_name_from_handle (const RemoteHandle &rhandle);
};

FieldBuffer*
ClientConnectionImpl::pop ()
{
  if (event_queue_.empty())
    return transport_channel_.fetch_msg();
  FieldBuffer *fb = event_queue_.front();
  event_queue_.pop_front();
  return fb;
}

RemoteHandle
ClientConnectionImpl::remote_origin (const vector<std::string> &feature_key_list)
{
  RemoteMember<RemoteHandle> rorigin = RemoteHandle::_null_handle();
  const uint connection_id = ObjectBroker::connection_id_from_keys (feature_key_list);
  if (connection_id)
    {
      FieldBuffer *fb = FieldBuffer::_new (3);
      fb->add_header2 (MSGID_HELLO_REQUEST, connection_id, this->connection_id(), 0, 0);
      FieldBuffer *fr = this->call_remote (fb); // takes over fb
      FieldReader frr (*fr);
      frr.skip_header();
      pop_handle (frr, rorigin);
      delete fr;
    }
  return rorigin;
}

void
ClientConnectionImpl::add_handle (FieldBuffer &fb, const RemoteHandle &rhandle)
{
  fb.add_object (rhandle._orbid());
}

typedef std::map<uint64, OrbObjectP> ClientOrboMap;
static ClientOrboMap client_orbo_map;   // FIXME: integrate into client connection
static Mutex         client_orbo_mutex;

void
ClientConnectionImpl::pop_handle (FieldReader &fr, RemoteHandle &rhandle)
{
  const uint64 orbid = fr.pop_object();
  ScopedLock<Mutex> locker (client_orbo_mutex);
  OrbObjectP orbo = client_orbo_map[orbid];
  if (AIDA_UNLIKELY (!orbo))
    {
      struct ClientOrbObject : public OrbObject {
        ClientOrbObject (uint64 orbid) : OrbObject (orbid) {}
      };
      orbo = std::make_shared<ClientOrbObject> (orbid);
      client_orbo_map[orbid] = orbo;
    }
  (rhandle.*pmf_cast_null_into) (OrbObjectRemoteHandle (orbo));
}

void
ClientConnectionImpl::dispatch ()
{
  FieldBuffer *fb = pop();
  return_if (fb == NULL);
  FieldReader fbr (*fb);
  const MessageId msgid = MessageId (fbr.pop_int64());
  const uint64  idmask = msgid_mask (msgid);
  switch (idmask)
    {
    case MSGID_EMIT_TWOWAY:
    case MSGID_EMIT_ONEWAY:
      {
        fbr.skip(); // hashhigh
        fbr.skip(); // hashlow
        const size_t handler_id = fbr.pop_int64();
        SignalHandler *client_signal_handler = signal_lookup (handler_id);
        AIDA_ASSERT (client_signal_handler != NULL);
        FieldBuffer *fr = client_signal_handler->seh (fb, client_signal_handler->data);
        if (fr == fb)
          fb = NULL; // prevent deletion
        if (idmask == MSGID_EMIT_ONEWAY)
          AIDA_ASSERT (fr == NULL);
        else // MSGID_EMIT_TWOWAY
          {
            AIDA_ASSERT (fr && msgid_mask (fr->first_id()) == MSGID_EMIT_RESULT);
            ObjectBroker::post_msg (fr);
          }
      }
      break;
    case MSGID_DISCONNECT:
      {
        const uint64 hashhigh = fbr.pop_int64(), hashlow = fbr.pop_int64();
        const size_t handler_id = fbr.pop_int64();
        const bool deleted = true; // FIXME: currently broken
        if (!deleted)
          print_warning (string_format ("%s: invalid handler id (%016x) in message: (%016x, %016x%016x)",
                                        STRFUNC, handler_id, msgid, hashhigh, hashlow));
      }
      break;
    case MSGID_HELLO_REPLY:     // handled in call_remote
    case MSGID_CALL_RESULT:     // handled in call_remote
    case MSGID_CONNECT_RESULT:  // handled in call_remote
    default:
      print_warning (string_format ("%s: invalid message: %016x", STRFUNC, msgid));
      break;
    }
  if (AIDA_UNLIKELY (fb))
    delete fb;
}

FieldBuffer*
ClientConnectionImpl::call_remote (FieldBuffer *fb)
{
  AIDA_ASSERT (fb != NULL);
  // enqueue method call message
  const MessageId msgid = MessageId (fb->first_id());
  const bool needsresult = msgid_has_result (msgid);
  if (!needsresult)
    {
      ObjectBroker::post_msg (fb);
      return NULL;
    }
  const MessageId resultid = MessageId (msgid_mask (msgid_as_result (msgid)));
  blocking_for_sem_ = true; // results will notify semaphore
  ObjectBroker::post_msg (fb);
  FieldBuffer *fr;
  while (needsresult)
    {
      fr = transport_channel_.fetch_msg();
      while (AIDA_UNLIKELY (!fr))
        {
          block_for_result ();
          fr = transport_channel_.fetch_msg();
        }
      const uint64 retmask = msgid_mask (fr->first_id());
      if (retmask == resultid)
        break;
#if 0
      else if (msgid_is_error (retmask))
        {
          FieldReader fbr (*fr);
          fbr.skip_header();
          std::string msg = fbr.pop_string();
          std::string dom = fbr.pop_string();
          warning_printf ("%s: %s", dom.c_str(), msg.c_str());
          delete fr;
        }
#endif
      else if (retmask == MSGID_DISCONNECT || retmask == MSGID_EMIT_ONEWAY || retmask == MSGID_EMIT_TWOWAY)
        event_queue_.push_back (fr);
      else
        {
          FieldReader frr (*fb);
          const uint64 retid = frr.pop_int64(), rethh = frr.pop_int64(), rethl = frr.pop_int64();
          print_warning (string_format ("%s: invalid reply: (%016x, %016x%016x)", STRFUNC, retid, rethh, rethl));
        }
    }
  blocking_for_sem_ = false;
  return fr;
}

size_t
ClientConnectionImpl::signal_connect (uint64 hhi, uint64 hlo, const RemoteHandle &rhandle, SignalEmitHandler seh, void *data)
{
  assert_return (rhandle._orbid() > 0, 0);
  assert_return (hhi > 0, 0);   // FIXME: check for signal id
  assert_return (hlo > 0, 0);
  assert_return (seh != NULL, 0);
  SignalHandler *shandler = new SignalHandler;
  shandler->hhi = hhi;
  shandler->hlo = hlo;
  shandler->remote = rhandle;                   // emitting object
  shandler->cid = 0;
  shandler->seh = seh;
  shandler->data = data;
  pthread_spin_lock (&signal_spin_);
  const uint handler_index = signal_handlers_.size();
  signal_handlers_.push_back (shandler);
  pthread_spin_unlock (&signal_spin_);
  const size_t handler_id = SignalHandlerIdParts (handler_index, connection_id()).vsize;
  FieldBuffer &fb = *FieldBuffer::_new (3 + 1 + 2);
  const uint orbid_connection = ObjectBroker::connection_id_from_orbid (shandler->remote._orbid());
  fb.add_header2 (MSGID_CONNECT, orbid_connection, connection_id(), shandler->hhi, shandler->hlo);
  add_handle (fb, rhandle);                     // emitting object
  fb <<= handler_id;                            // handler connection request id
  fb <<= 0;                                     // disconnection request id
  FieldBuffer *connection_result = call_remote (&fb); // deletes fb
  assert_return (connection_result != NULL, 0);
  FieldReader frr (*connection_result);
  frr.skip_header();
  pthread_spin_lock (&signal_spin_);
  frr >>= shandler->cid;
  pthread_spin_unlock (&signal_spin_);
  delete connection_result;
  return handler_id;
}

bool
ClientConnectionImpl::signal_disconnect (size_t signal_handler_id)
{
  const SignalHandlerIdParts handler_id_parts (signal_handler_id);
  assert_return (handler_id_parts.orbid_connection == connection_id(), false);
  const uint handler_index = handler_id_parts.signal_handler_index;
  pthread_spin_lock (&signal_spin_);
  SignalHandler *shandler = handler_index < signal_handlers_.size() ? signal_handlers_[handler_index] : NULL;
  if (shandler)
    signal_handlers_[handler_index] = NULL;
  pthread_spin_unlock (&signal_spin_);
  return_if (!shandler, false);
  FieldBuffer &fb = *FieldBuffer::_new (3 + 1 + 2);
  const uint orbid_connection = ObjectBroker::connection_id_from_orbid (shandler->remote._orbid());
  fb.add_header2 (MSGID_CONNECT, orbid_connection, connection_id(), shandler->hhi, shandler->hlo);
  add_handle (fb, shandler->remote);            // emitting object
  fb <<= 0;                                     // handler connection request id
  fb <<= shandler->cid;                         // disconnection request id
  FieldBuffer *connection_result = call_remote (&fb); // deletes fb
  assert_return (connection_result != NULL, false);
  FieldReader frr (*connection_result);
  frr.skip_header();
  uint64 disconnection_success;
  frr >>= disconnection_success;
  delete connection_result;
  critical_unless (disconnection_success == true); // should always succeed due to the above guard; FIXME: possible race w/ ~Signal
  shandler->seh (NULL, shandler->data); // handler deletion hook
  delete shandler;
  return disconnection_success;
}

ClientConnectionImpl::SignalHandler*
ClientConnectionImpl::signal_lookup (size_t signal_handler_id)
{
  const SignalHandlerIdParts handler_id_parts (signal_handler_id);
  assert_return (handler_id_parts.orbid_connection == connection_id(), NULL);
  const uint handler_index = handler_id_parts.signal_handler_index;
  pthread_spin_lock (&signal_spin_);
  SignalHandler *shandler = handler_index < signal_handlers_.size() ? signal_handlers_[handler_index] : NULL;
  pthread_spin_unlock (&signal_spin_);
  return shandler;
}

std::string
ClientConnectionImpl::type_name_from_handle (const RemoteHandle &rhandle)
{
  const uint type_index = OrbObject::orbid_type_index (rhandle._orbid());
  return type_name_db.type_name (type_index);
}

// == ServerConnectionImpl ==
/// Transport and dispatch layer for messages sent between ClientConnection and ServerConnection.
class ServerConnectionImpl : public ServerConnection {
  TransportChannel         transport_channel_;  // messages arriving at server
  ObjectMap<ImplicitBase>  object_map_;         // map of all objects used remotely
  ImplicitBaseP            remote_origin_;
  std::unordered_map<size_t, EmitResultHandler> emit_result_map_;
  RAPICORN_CLASS_NON_COPYABLE (ServerConnectionImpl);
public:
  explicit              ServerConnectionImpl (const std::string &feature_keys);
  virtual              ~ServerConnectionImpl ()         { ObjectBroker::unregister_connection (*this); }
  virtual int           notify_fd      ()               { return transport_channel_.inputfd(); }
  virtual bool          pending        ()               { return transport_channel_.has_msg(); }
  virtual void          dispatch       ();
  virtual void          remote_origin  (ImplicitBaseP rorigin);
  virtual RemoteHandle  remote_origin  (const vector<std::string> &feature_key_list) { fatal ("assert not reached"); }
  virtual void          add_interface  (FieldBuffer &fb, ImplicitBaseP ibase);
  virtual ImplicitBaseP pop_interface  (FieldReader &fr);
  virtual void          send_msg   (FieldBuffer *fb)    { assert_return (fb); transport_channel_.send_msg (fb, true); }
  virtual void              emit_result_handler_add (size_t id, const EmitResultHandler &handler);
  virtual EmitResultHandler emit_result_handler_pop (size_t id);
  virtual ImplicitBaseP     interface_from_handle   (const RemoteHandle &rhandle);
  virtual void              cast_interface_handle   (RemoteHandle &rhandle, ImplicitBaseP ibase);
};

ServerConnectionImpl::ServerConnectionImpl (const std::string &feature_keys) :
  ServerConnection (feature_keys), remote_origin_ (NULL)
{
  const uint realid = ObjectBroker::register_connection (*this, 0xaaaa);
  if (!realid)
    fatal_error ("Aida: failed to register ServerConnection");
  assign_id (realid);
  const uint64 start_id = OrbObject::orbid_make (realid, // connection_id() must match connection_id_from_orbid()
                                                 0,      // type_name_db.index, see orbid_hook
                                                 1);     // counter = first object id
  object_map_.assign_start_id (start_id, OrbObject::orbid_make (0xffff, 0x0000, 0xffffffff));
  auto orbid_hook = [this] (ObjectMap<ImplicitBase>::InstanceP instance) {
    // hook is needed to determine the type_name index for newly created orbids
    // embedding the type_name into orbids is a small hack for Python to create the correct Handle type
    const uint64 type_index = type_name_db.index (instance->__aida_type_name__());
    return OrbObject::orbid_make (0, type_index, 0);
  };
  object_map_.assign_orbid_hook (orbid_hook);
}

void
ServerConnectionImpl::remote_origin (ImplicitBaseP rorigin)
{
  if (rorigin)
    AIDA_ASSERT (remote_origin_ == NULL);
  else // rorigin == NULL
    AIDA_ASSERT (remote_origin_ != NULL);
  remote_origin_ = rorigin;
}

void
ServerConnectionImpl::add_interface (FieldBuffer &fb, ImplicitBaseP ibase)
{
  OrbObjectP orbo = object_map_.orbo_from_instance (ibase);
  fb.add_object (orbo ? orbo->orbid() : 0);
}

ImplicitBaseP
ServerConnectionImpl::pop_interface (FieldReader &fr)
{
  const uint64 orbid = fr.pop_object();
  OrbObjectP orbo = object_map_.orbo_from_orbid (orbid);
  return object_map_.instance_from_orbo (orbo);
}

void
ServerConnectionImpl::dispatch ()
{
  FieldBuffer *fb = transport_channel_.fetch_msg();
  if (!fb)
    return;
  FieldReader fbr (*fb);
  const MessageId msgid = MessageId (fbr.pop_int64());
  const uint64  idmask = msgid_mask (msgid);
  switch (idmask)
    {
    case MSGID_HELLO_REQUEST:
      {
        const uint64 hashhigh = fbr.pop_int64(), hashlow = fbr.pop_int64();
        AIDA_ASSERT (hashhigh == 0 && hashlow == 0);
        AIDA_ASSERT (fbr.remaining() == 0);
        fbr.reset (*fb);
        ImplicitBaseP rorigin = remote_origin_;
        FieldBuffer *fr = FieldBuffer::renew_into_result (fbr, MSGID_HELLO_REPLY, ObjectBroker::receiver_connection_id (msgid), 0, 0, 1);
        add_interface (*fr, rorigin);
        if (AIDA_LIKELY (fr == fb))
          fb = NULL; // prevent deletion
        const uint64 resultmask = msgid_as_result (MessageId (idmask));
        AIDA_ASSERT (fr && msgid_mask (fr->first_id()) == resultmask);
        ObjectBroker::post_msg (fr);
      }
      break;
    case MSGID_CONNECT:
    case MSGID_TWOWAY_CALL:
    case MSGID_ONEWAY_CALL:
      {
        const uint64 hashhigh = fbr.pop_int64(), hashlow = fbr.pop_int64();
        const DispatchFunc server_method_implementation = find_method (hashhigh, hashlow);
        AIDA_ASSERT (server_method_implementation != NULL);
        fbr.reset (*fb);
        FieldBuffer *fr = server_method_implementation (fbr);
        if (AIDA_LIKELY (fr == fb))
          fb = NULL; // prevent deletion
        if (idmask == MSGID_ONEWAY_CALL)
          AIDA_ASSERT (fr == NULL);
        else // MSGID_TWOWAY_CALL
          {
            const uint64 resultmask = msgid_as_result (MessageId (idmask));
            AIDA_ASSERT (fr && msgid_mask (fr->first_id()) == resultmask);
            ObjectBroker::post_msg (fr);
          }
      }
      break;
    case MSGID_EMIT_RESULT:
      {
        fbr.skip(); // hashhigh
        fbr.skip(); // hashlow
        const uint64 emit_result_id = fbr.pop_int64();
        EmitResultHandler emit_result_handler = emit_result_handler_pop (emit_result_id);
        AIDA_ASSERT (emit_result_handler != NULL);
        emit_result_handler (fbr);
      }
      break;
    default:
      {
        const uint64 hashhigh = fbr.pop_int64(), hashlow = fbr.pop_int64();
        print_warning (string_format ("%s: invalid message: (%016x, %016x%016x)", STRFUNC, msgid, hashhigh, hashlow));
      }
      break;
    }
  if (AIDA_UNLIKELY (fb))
    delete fb;
}

ImplicitBaseP
ServerConnectionImpl::interface_from_handle (const RemoteHandle &rhandle)
{
  OrbObjectP orbo = object_map_.orbo_from_orbid (rhandle._orbid()); // FIXME: handle should store OrbObjectP directly
  return object_map_.instance_from_orbo (orbo);
}

void
ServerConnectionImpl::cast_interface_handle (RemoteHandle &rhandle, ImplicitBaseP ibase)
{
  OrbObjectP orbo = object_map_.orbo_from_instance (ibase);
  (rhandle.*pmf_cast_null_into) (OrbObjectRemoteHandle (orbo));
}

void
ServerConnectionImpl::emit_result_handler_add (size_t id, const EmitResultHandler &handler)
{
  AIDA_ASSERT (emit_result_map_.count (id) == 0);        // PARANOID
  emit_result_map_[id] = handler;
}

ServerConnectionImpl::EmitResultHandler
ServerConnectionImpl::emit_result_handler_pop (size_t id)
{
  auto it = emit_result_map_.find (id);
  if (AIDA_LIKELY (it != emit_result_map_.end()))
    {
      EmitResultHandler emit_result_handler = it->second;
      emit_result_map_.erase (it);
      return emit_result_handler;
    }
  else
    return EmitResultHandler();
}

// == ServerConnection ==
ServerConnection::ServerConnection (const std::string &feature_keys) :
  BaseConnection (feature_keys)
{}

ServerConnection::~ServerConnection()
{}

static inline bool
operator< (const TypeHash &a, const TypeHash &b)
{
  return AIDA_UNLIKELY (a.typehi == b.typehi) ? a.typelo < b.typelo : a.typehi < b.typehi;
}
struct HashTypeHash {
  inline size_t operator() (const TypeHash &t) const
  {
    return t.typehi ^ t.typelo;
  }
};

typedef std::unordered_map<TypeHash, DispatchFunc, HashTypeHash> DispatcherMap;
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
ServerConnection::find_method (uint64 hashhi, uint64 hashlo)
{
  TypeHash typehash (hashhi, hashlo);
#if 1 // avoid costly mutex locking
  if (AIDA_UNLIKELY (dispatcher_map_frozen == false))
    {
      ensure_dispatcher_map();
      dispatcher_map_frozen = true;
    }
  return (*dispatcher_map)[typehash]; // unknown hashes *shouldn't* happen, see assertion in caller
#else
  ensure_dispatcher_map();
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
  assert_return (dispatcher_map_frozen == false);
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
      perror (string_format ("%s:%u: Aida::ServerConnection::MethodRegistry::register_method: "
                             "duplicate hash registration (%016x%016x)",
                             __FILE__, __LINE__, mentry.hashhi, mentry.hashlo).c_str());
      abort();
    }
}

// == ObjectBroker ==
#define MAX_CONNECTIONS        7                                             // arbitrary limit that can be extended if needed
static Atomic<BaseConnection*> orb_connections[MAX_CONNECTIONS] = { NULL, }; // initialization needed to call consexpr ctor

uint
ObjectBroker::register_connection (BaseConnection &connection, uint suggested_id)
{
  for (size_t i = 0; i < MAX_CONNECTIONS; i++)
    {
      if (suggested_id + i == 0)
        suggested_id++;
      const uint64 idx = (suggested_id + i) % MAX_CONNECTIONS;
      if (orb_connections[idx] == NULL &&
          orb_connections[idx].cas (NULL, &connection))
        return suggested_id + i;
    }
  return 0;
}

void
ObjectBroker::unregister_connection (BaseConnection &connection)
{
  const uint64 conid = connection.connection_id();
  assert_return (conid > 0);
  const uint64 idx = conid % MAX_CONNECTIONS;
  assert_return (orb_connections[idx] == &connection);
  orb_connections[idx].store (NULL);
}

///< Connection lookup by id, used for message delivery.
BaseConnection*
ObjectBroker::connection_from_id (uint64 conid)
{
  const uint64 idx = conid % MAX_CONNECTIONS;
  return conid ? orb_connections[idx].load() : NULL;
}

uint
ObjectBroker::connection_id_from_signal_handler_id (size_t signal_handler_id)
{
  const SignalHandlerIdParts handler_id_parts (signal_handler_id);
  const size_t handler_index = handler_id_parts.signal_handler_index;
  return handler_index ? handler_id_parts.orbid_connection : 0; // FIXME
}

ServerConnection*
ObjectBroker::new_server_connection (const std::string &feature_keys)
{
  ServerConnectionImpl *simpl = new ServerConnectionImpl (feature_keys);
  return simpl;
}

ClientConnection*
ObjectBroker::new_client_connection (const std::string &feature_keys)
{
  ClientConnectionImpl *cimpl = new ClientConnectionImpl (feature_keys);
  return cimpl;
}

uint
ObjectBroker::connection_id_from_keys (const vector<std::string> &feature_key_list)
{ // feature_key_list is a list of key=regex_pattern pairs
  for (size_t idx = 0; idx < MAX_CONNECTIONS; idx++)
    {
      BaseConnection *bcon = orb_connections[idx];
      if (!bcon)
        continue;
      const String &feature_keys = bcon->feature_keys_;
      for (auto keyvalue : feature_key_list)
        {
          String key, value;
          const size_t eq = keyvalue.find ('=');
          if (eq != std::string::npos)
            {
              key = keyvalue.substr (0, eq);
              value = keyvalue.substr (eq + 1);
            }
          else
            {
              key = keyvalue;
              value = "1";
            }
          if (!Regex::match_simple (value, string_option_get (feature_keys, key), Regex::EXTENDED | Regex::CASELESS, Regex::MATCH_NORMAL))
            goto mismatch;
        }
      return bcon->connection_id(); // all of feature_key_list matched
    mismatch: ;
    }
  return 0;     // unmatched
}

void
ObjectBroker::post_msg (FieldBuffer *fb)
{
  assert_return (fb);
  const MessageId msgid = MessageId (fb->first_id());
  const uint connection_id = ObjectBroker::sender_connection_id (msgid);
  BaseConnection *bcon = connection_from_id (connection_id);
  if (!bcon)
    fatal_error (string_format ("Message ID without valid connection: %016x (connection_id=%u)", msgid, connection_id));
  const bool needsresult = msgid_has_result (msgid);
  const uint receiver_connection = ObjectBroker::receiver_connection_id (msgid);
  if (needsresult != (receiver_connection > 0)) // FIXME: move downwards
    fatal_error (string_format ("mismatch of result flag and receiver_connection: %016x", msgid));
  bcon->send_msg (fb);
}

} } // Rapicorn::Aida
