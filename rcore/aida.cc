// CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0/
#include "aida.hh"
#include "aidaprops.hh"
#include "thread.hh"
#include "regex.hh"

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
#include <sys/eventfd.h>        // defines EFD_SEMAPHORE
#include <stddef.h>             // ptrdiff_t
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
  error_printf ("%s:%u: assertion failed: %s", file, line, expr);
}

void
error_vprintf (const char *format, va_list args)
{
  std::string s = string_vprintf (format, args);
  if (s.empty() || s[s.size() - 1] != '\n')
    s += "\n";
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
  std::string s = string_vprintf (format, args);
  va_end (args);
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
RAPICORN_STATIC_ASSERT (sizeof (TypeCode) <= 2 * sizeof (void*));
RAPICORN_STATIC_ASSERT (sizeof (Any) <= sizeof (TypeCode) + sizeof (void*));

Any&
Any::operator= (const Any &clone)
{
  if (this == &clone)
    return *this;
  reset();
  type_code_ = clone.type_code_;
  switch (kind())
    {
    case STRING:        new (&u_.vstring()) String (clone.u_.vstring());  break;
    case ANY:           u_.vany = new Any (*clone.u_.vany);               break;
    case SEQUENCE:      u_.vanys = new AnyVector (*clone.u_.vanys);       break;
    case RECORD:        u_.vfields = new FieldVector (*clone.u_.vfields); break;
    case INSTANCE:      u_.shandle = new SmartHandle (*clone.u_.shandle); break;
    default:            u_ = clone.u_;                                    break;
    }
  return *this;
}

void
Any::reset()
{
  switch (kind())
    {
    case STRING:        u_.vstring().~String();                  break;
    case ANY:           delete u_.vany;                          break;
    case SEQUENCE:      delete u_.vanys;                         break;
    case RECORD:        delete u_.vfields;                       break;
    case INSTANCE:      delete u_.shandle;                       break;
    default: ;
    }
  type_code_ = TypeMap::notype();
  u_.vuint64 = 0;
}

void
Any::retype (const TypeCode &tc)
{
  reset();
  switch (tc.kind())
    {
    case STRING:   new (&u_.vstring()) String();                               break;
    case ANY:      u_.vany = new Any();                                        break;
    case SEQUENCE: u_.vanys = new AnyVector();                                 break;
    case RECORD:   u_.vfields = new FieldVector();                             break;
    case INSTANCE: u_.shandle = new SmartHandle (SmartHandle::_null_handle()); break;
    default:    break;
    }
  type_code_ = tc;
}

void
Any::rekind (TypeKind _kind)
{
  const char *name;
  switch (_kind)
    {
    case UNTYPED:
      reset();
      return;
    case BOOL:          name = "bool";                  break;
    case INT32:         name = "int32";                 break;
      // case UINT32:   name = "uint32";                break;
    case INT64:         name = "int64";                 break;
    case FLOAT64:       name = "float64";               break;
    case STRING:        name = "String";                break;
    case ANY:           name = "Any";                   break;
    case ENUM:          name = "Aida::DynamicEnum";     break;
    case SEQUENCE:      name = "Aida::DynamicSequence"; break;
    case RECORD:        name = "Aida::DynamicRecord";   break;
    case INSTANCE:
    default:
      error_printf ("Aida::Any:rekind: incomplete type: %s", type_kind_name (_kind));
    }
  TypeCode tc = TypeMap::lookup (name);
  if (tc.untyped())
    error_printf ("Aida::Any:rekind: unknown type: %s", name);
  retype (tc);
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
  s += "type=" + Rapicorn::string_to_cquote (type().name());
  if (!field_name.empty())
    s += ", name=" + Rapicorn::string_to_cquote (field_name);
  switch (kind())
    {
    case BOOL:
    case ENUM:
    case INT32:
    case INT64:         s += string_printf (", value=%lld", u_.vint64);                          break;
    case FLOAT64:       s += string_printf (", value=%.17g", u_.vdouble);                        break;
    case ANY:           s += ", value=" + u_.vany->to_string();                                  break;
    case STRING:        s += ", value=" + Rapicorn::string_to_cquote (u_.vstring());             break;
    case SEQUENCE:      if (u_.vanys) s += ", value=" + any_vector_to_string (*u_.vanys);         break;
    case RECORD:        if (u_.vfields) s += ", value=" + any_vector_to_string (*u_.vfields);     break;
    case INSTANCE:      s += string_printf (", value=#%08llx", u_.shandle->_orbid());            break;
    default:            ;
    case UNTYPED:       break;
    }
  s += " }";
  return s;
}

bool
Any::operator== (const Any &clone) const
{
  if (type_code_ != clone.type_code_)
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

void
Any::swap (Any &other)
{
  constexpr size_t USIZE = sizeof (this->u_);
  uint64_t buffer[(USIZE + 7) / 8];
  memcpy (buffer, &other.u_, USIZE);
  memcpy (&other.u_, &this->u_, USIZE);
  memcpy (&this->u_, buffer, USIZE);
  type_code_.swap (other.type_code_);
}

bool
Any::to_int (int64_t &v, char b) const
{
  if (kind() != BOOL && kind() != INT32 && kind() != INT64)
    return false;
  bool s = 0;
  switch (b)
    {
    case 1:     s =  u_.vint64 >=         0 &&  u_.vint64 <= 1;        break;
    case 7:     s =  u_.vint64 >=      -128 &&  u_.vint64 <= 127;      break;
    case 8:     s =  u_.vint64 >=         0 &&  u_.vint64 <= 256;      break;
    case 47:    s = sizeof (long) == sizeof (int64_t); // chain
    case 31:    s |= u_.vint64 >=   INT_MIN &&  u_.vint64 <= INT_MAX;  break;
    case 48:    s = sizeof (long) == sizeof (int64_t); // chain
    case 32:    s |= u_.vint64 >=         0 &&  u_.vint64 <= UINT_MAX; break;
    case 63:    s = 1; break;
    case 64:    s = 1; break;
    default:    s = 0; break;
    }
  if (s)
    v = u_.vint64;
  return s;
}

int64_t
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
    case INT64:         return string_printf ("%lli", u_.vint64);
    case FLOAT64:       return string_printf ("%.17g", u_.vdouble);
    case STRING:        return u_.vstring();
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
Any::operator>>= (SmartHandle &v)
{
  if (kind() != INSTANCE)
    return false;
  v = u_.shandle ? *u_.shandle : SmartHandle::_null_handle();
  return true;
}

void
Any::operator<<= (bool v)
{
  ensure (BOOL);
  u_.vint64 = v;
}

void
Any::operator<<= (int32_t v)
{
  ensure (INT32);
  u_.vint64 = v;
}

void
Any::operator<<= (int64_t v)
{
  ensure (INT64);
  u_.vint64 = v;
}

void
Any::operator<<= (uint64_t v)
{
  // ensure (UINT);
  operator<<= (int64_t (v));
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
Any::operator<<= (const SmartHandle &v)
{
  ensure (INSTANCE);
  SmartHandle *old = u_.shandle;
  u_.shandle = new SmartHandle (v);
  if (old)
    delete old;
}

void
Any::from_proto (const TypeCode type_code, FieldReader &pbr)
{
  Any &any = *this;
  switch (type_code.kind())
    {
    case BOOL:
      any <<= pbr.pop_bool();
      break;
    case INT32:
      any <<= pbr.pop_int64();
      break;
    case INT64:
      any <<= pbr.pop_int64();
      break;
    case FLOAT64:
      any <<= pbr.pop_double();
      break;
    case STRING:
      any <<= pbr.pop_string();
      break;
    case ENUM:
      any <<= EnumValue (pbr.pop_evalue());
      break;
    case ANY:
      any <<= pbr.pop_any();
      break;
    case RECORD: {
      const FieldBuffer &fb = pbr.pop_rec();
      FieldReader fbr (fb);
      const size_t field_count = type_code.field_count();
      assert_return (fbr.n_types() == field_count);
      Any::FieldVector fields;
      for (size_t i = 0; i < field_count; i++)
        {
          TypeCode ftc = type_code.field (i).resolve();
          Any fany (ftc);
          fany.from_proto (ftc, fbr);
          fields.push_back (Any::Field (ftc.name(), fany));
        }
      any.retype (type_code);
      any <<= fields;
    } break;
    case SEQUENCE: {
      const FieldBuffer &pb = pbr.pop_seq();
      FieldReader rbr (pb);
      assert_return (type_code.field_count() == 1);
      TypeCode ftc = type_code.field (0).resolve();
      const size_t len = rbr.remaining();
      Any::AnyVector anys;
      anys.resize (len);
      for (size_t k = 0; k < len; k++)
        {
          Any fany (ftc);
          fany.from_proto (ftc, rbr);
          anys.push_back (fany);
        }
      any.retype (type_code);
      any <<= anys;
    } break;
    case INSTANCE: {
      SmartMember<SmartHandle> sh;
      ObjectBroker::pop_handle (pbr, sh);
      any <<= sh;
    } break;
    case TYPE_REFERENCE: // ?
    default:
      critical ("%s: unknown type: %s", STRLOC(), type_code.kind_name().c_str());
      break;
    }
}

void
Any::to_proto (const TypeCode type_code, FieldBuffer &pb) const
{
  assert_return (pb.capacity() - pb.size() >= 1);
  const Any &any = *this;
  switch (type_code.kind())
    {
    case BOOL:
      pb.add_bool (any.as_int());
      break;
    case INT32: case INT64:
      pb.add_int64 (any.as_int());
      break;
    case FLOAT64:
      pb.add_double (any.as_float());
      break;
    case STRING:
      pb.add_string (any.as_string());
      break;
    case ENUM:
      pb.add_evalue (any.as_int());
      break;
    case ANY:
      pb.add_any (any.as_any());
      break;
    case RECORD: {
      const size_t field_count = type_code.field_count();
      FieldBuffer &rb = pb.add_rec (type_code.field_count());
      assert_return (rb.capacity() - rb.size() >= field_count);
      const Any::FieldVector *fields = NULL;
      any >>= fields;
      for (size_t i = 0; i < field_count; i++)
        {
          TypeCode ftc = type_code.field (i).resolve();
          const Any *pany = NULL;
          for (size_t j = 0; fields && j < fields->size(); j++)
            if ((*fields)[j].name == ftc.name())
              pany = &(*fields)[j];
          uint64_t amem[(sizeof (Any) + 7) / 8];
          const Any &fany = *(pany ? pany : new (amem) Any (ftc));      // stack Any constructor
          fany.to_proto (ftc, rb);
          if (!pany)
            fany.~Any();                                                // stack Any destructor
        }
    } break;
    case SEQUENCE: {
      assert_return (type_code.field_count() == 1);
      TypeCode ftc = type_code.field (0).resolve();
      const Any::AnyVector *anys = NULL;
      any >>= anys;
      const size_t len = anys ? anys->size() : 0;
      FieldBuffer &rb = pb.add_seq (len);
      for (size_t i = 0; i < len; i++)
        {
          const Any &fany = (*anys)[i];
          fany.to_proto (ftc, rb);
        }
    } break;
    case INSTANCE:
      pb.add_object (u_.shandle ? u_.shandle->_orbid() : 0);
      break;
    case TYPE_REFERENCE: // ?
    default:
      critical ("%s: unknown type: %s", STRLOC(), type_code.kind_name().c_str());
      break;
    }
}

// == OrbObject ==
OrbObject::OrbObject (uint64_t orbid) :
  orbid_ (orbid)
{}

// == OrbObjectImpl ==
struct OrbObjectImpl : public OrbObject {
  OrbObjectImpl (ptrdiff_t obid) : OrbObject (obid) {}
};

static const OrbObjectImpl aida_orb_object_null (0);

// == SmartHandle ==
static OrbObject* smart_handle_null_orb_object () { return &const_cast<OrbObjectImpl&> (aida_orb_object_null); }

SmartHandle::SmartHandle (OrbObject &orbo) :
  orbo_ (&orbo)
{
  assert (&orbo);
}

SmartHandle::SmartHandle() :
  orbo_ (smart_handle_null_orb_object())
{}

void
SmartHandle::reset ()
{
  if (orbo_ != smart_handle_null_orb_object())
    {
      orbo_ = smart_handle_null_orb_object();
    }
}

void
SmartHandle::assign (const SmartHandle &src)
{
  if (orbo_ == src.orbo_)
    return;
  if (NULL != *this)
    reset();
  orbo_ = src.orbo_;
}

SmartHandle::~SmartHandle()
{}

bool
SmartHandle::operator== (const SmartHandle &other) const noexcept
{
  if (orbo_ && other.orbo_)
    return orbo_->orbid() == other.orbo_->orbid();
  return orbo_ == other.orbo_;
}

bool
SmartHandle::operator!= (const SmartHandle &other) const noexcept
{
  return !operator== (other);
}

// == ObjectBroker ==
typedef std::map<ptrdiff_t, OrbObject*> OrboMap;
static OrboMap orbo_map;
static Mutex   orbo_mutex;

void
ObjectBroker::tie_handle (SmartHandle &sh, const uint64_t orbid)
{
  AIDA_ASSERT (NULL == sh);
  ScopedLock<Mutex> locker (orbo_mutex);
  OrbObject *orbo = orbo_map[orbid];
  if (AIDA_UNLIKELY (!orbo))
    orbo_map[orbid] = orbo = new OrbObjectImpl (orbid);
  sh.assign (SmartHandle (*orbo));
}

void
ObjectBroker::pop_handle (FieldReader &fr, SmartHandle &sh)
{
  tie_handle (sh, fr.pop_object());
}

uint
ObjectBroker::connection_id_from_signal_handler_id (size_t signal_handler_id)
{
  const SignalHandlerIdParts handler_id_parts (signal_handler_id);
  const uint handler_index = handler_id_parts.signal_handler_index;
  return handler_index ? handler_id_parts.orbid_connection : 0;
}

// == FieldBuffer ==
FieldBuffer::FieldBuffer (uint _ntypes) :
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
    error_printf ("FieldBuffer(this=%p): capacity=%u size=%u", this, capacity(), size());
}

void
FieldReader::check_request (int type)
{
  if (nth_ >= n_types())
    error_printf ("FieldReader(this=%p): size=%u requested-index=%u", this, n_types(), nth_);
  if (get_type() != type)
    error_printf ("FieldReader(this=%p): size=%u index=%u type=%s requested-type=%s",
                  this, n_types(), nth_,
                  FieldBuffer::type_name (get_type()).c_str(), FieldBuffer::type_name (type).c_str());
}

std::string
FieldBuffer::first_id_str() const
{
  uint64_t fid = first_id();
  return string_printf ("%016llx", fid);
}

static std::string
strescape (const std::string &str)
{
  std::string buffer;
  for (std::string::const_iterator it = str.begin(); it != str.end(); it++)
    {
      uint8_t d = *it;
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

std::string
FieldBuffer::type_name (int field_type)
{
  const char *tkn = type_kind_name (TypeKind (field_type));
  if (tkn)
    return tkn;
  return string_printf ("<invalid:%d>", field_type);
}

std::string
FieldBuffer::to_string() const
{
  String s = string_printf ("Aida::FieldBuffer(%p)={", this);
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
        case BOOL:      s += string_printf (", %s: 0x%llx", tn, fbr.pop_bool());                   break;
        case ENUM:      s += string_printf (", %s: 0x%llx", tn, fbr.pop_evalue());                 break;
        case INT32:     s += string_printf (", %s: 0x%08llx", tn, fbr.pop_int64());                break;
        case INT64:     s += string_printf (", %s: 0x%016llx", tn, fbr.pop_int64());               break;
        case FLOAT64:   s += string_printf (", %s: %.17g", tn, fbr.pop_double());                  break;
        case STRING:    s += string_printf (", %s: %s", tn, strescape (fbr.pop_string()).c_str()); break;
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
FieldBuffer::new_result (MessageId m, uint rconnection, uint64_t h, uint64_t l, uint32_t n)
{
  assert_return (msgid_is_result (m) && rconnection <= CONNECTION_MASK && rconnection, NULL);
  FieldBuffer *fr = FieldBuffer::_new (3 + n);
  fr->add_header1 (m, rconnection, h, l);
  return fr;
}

FieldBuffer*
ObjectBroker::renew_into_result (FieldBuffer *fb, MessageId m, uint rconnection, uint64_t h, uint64_t l, uint32_t n)
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
ObjectBroker::renew_into_result (FieldReader &fbr, MessageId m, uint rconnection, uint64_t h, uint64_t l, uint32_t n)
{
  FieldBuffer *fb = const_cast<FieldBuffer*> (fbr.field_buffer());
  fbr.reset();
  return renew_into_result (fb, m, rconnection, h, l, n);
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
  {}
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

// == BaseConnection ==
#define MAX_CONNECTIONS         8       ///< Arbitrary limit that can be extended if needed
static Atomic<BaseConnection*>  global_connections[MAX_CONNECTIONS] = { NULL, }; // initialization needed to call consexpr ctor

static void
register_connection (uint *indexp, BaseConnection *con)
{
  assert_return (*indexp == ~uint (0) && con);
  for (size_t i = 0; i < MAX_CONNECTIONS; i++)
    {
      *indexp = i;
      if (global_connections[i].cas (NULL, con))
        return;
    }
  *indexp = ~uint (0);
  error_printf ("MAX_CONNECTIONS limit reached");
}

static void
unregister_connection (uint *indexp)
{
  assert_return (*indexp < MAX_CONNECTIONS && global_connections[*indexp]);
  const uint index = *indexp;
  *indexp = ~uint (0);
  global_connections[index].store (NULL);
}

BaseConnection::BaseConnection (const std::string &feature_keys) :
  index_ (~uint (0)), feature_keys_ (feature_keys)
{
  AIDA_ASSERT (feature_keys_.size() && feature_keys_[0] == ':' && feature_keys_[feature_keys_.size()-1] == ':');
}

BaseConnection::~BaseConnection ()
{
  if (index_ < MAX_CONNECTIONS)
    Aida::unregister_connection (&index_);
}

void
BaseConnection::register_connection()
{
  assert_return (index_ > MAX_CONNECTIONS);
  Aida::register_connection (&index_, this);
}

void
BaseConnection::unregister_connection()
{
  assert_return (index_ < MAX_CONNECTIONS);
  Aida::unregister_connection (&index_);
}

uint
BaseConnection::connection_id () const
{
  return index_ < MAX_CONNECTIONS ? 1 + index_ : 0;
}

BaseConnection*
BaseConnection::connection_from_id (uint id)
{
  return id && id <= MAX_CONNECTIONS ? global_connections[id-1].load() : NULL;
}

/// Provide initial handle for remote connections.
void
BaseConnection::remote_origin (ImplicitBase*)
{
  assertion_error (__FILE__, __LINE__, "not supported by this object type");
}

/** Retrieve initial handle after remote connection has been established.
 * The @a feature_key_list contains key=value pairs, where value is assumed to be "1" if
 * omitted and generally treated as a regular expression to match against connection
 * feature keys as registered with the ObjectBroker.
 */
SmartHandle
BaseConnection::remote_origin (const vector<std::string> &feature_key_list)
{
  assertion_error (__FILE__, __LINE__, "not supported by this object type");
}

// == ClientConnection ==
ClientConnection::ClientConnection (const std::string &feature_keys) :
  BaseConnection (feature_keys)
{}

ClientConnection::~ClientConnection ()
{}

// == ClientConnectionImpl ==
class ClientConnectionImpl : public ClientConnection {
  struct SignalHandler { uint64_t hhi, hlo, oid, cid; SignalEmitHandler *seh; void *data; };
  typedef std::set<uint64_t> UIntSet;
  pthread_spinlock_t            signal_spin_;
  TransportChannel              transport_channel_;     // messages sent to client
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
    register_connection();
  }
  ~ClientConnectionImpl ()
  {
    unregister_connection();
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
  virtual SmartHandle   remote_origin     (const vector<std::string> &feature_key_list);
  virtual size_t        signal_connect    (uint64_t hhi, uint64_t hlo, uint64_t orbid, SignalEmitHandler seh, void *data);
  virtual bool          signal_disconnect (size_t signal_handler_id);
  virtual std::string   type_name_from_orbid (uint64_t orbid);
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

SmartHandle
ClientConnectionImpl::remote_origin (const vector<std::string> &feature_key_list)
{
  SmartMember<SmartHandle> rorigin = SmartHandle::_null_handle();
  const uint connection_id = ObjectBroker::connection_id_from_keys (feature_key_list);
  if (connection_id)
    {
      FieldBuffer *fb = FieldBuffer::_new (3);
      fb->add_header2 (MSGID_HELLO_REQUEST, connection_id, this->connection_id(), 0, 0);
      FieldBuffer *fr = this->call_remote (fb); // takes over fb
      FieldReader frr (*fr);
      frr.skip_header();
      ObjectBroker::pop_handle (frr, rorigin);
      delete fr;
    }
  return rorigin;
}

void
ClientConnectionImpl::dispatch ()
{
  FieldBuffer *fb = pop();
  return_if (fb == NULL);
  FieldReader fbr (*fb);
  const MessageId msgid = MessageId (fbr.pop_int64());
  const uint64_t  idmask = msgid_mask (msgid);
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
        const uint64_t hashhigh = fbr.pop_int64(), hashlow = fbr.pop_int64();
        const size_t handler_id = fbr.pop_int64();
        const bool deleted = true; // FIXME: currently broken
        if (!deleted)
          warning_printf ("%s: invalid handler id (%016zx) in message: (%016lx, %016llx%016llx)",
                          STRFUNC, handler_id, msgid, hashhigh, hashlow);
      }
      break;
    case MSGID_HELLO_REPLY:     // handled in call_remote
    case MSGID_CALL_RESULT:     // handled in call_remote
    case MSGID_CONNECT_RESULT:  // handled in call_remote
    default:
      warning_printf ("%s: invalid message: %016lx", STRFUNC, msgid);
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
      const uint64_t retmask = msgid_mask (fr->first_id());
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
          const uint64_t retid = frr.pop_int64(), rethh = frr.pop_int64(), rethl = frr.pop_int64();
          warning_printf ("%s: invalid reply: (%016llx, %016llx%016llx)", STRFUNC, retid, rethh, rethl);
        }
    }
  blocking_for_sem_ = false;
  return fr;
}

size_t
ClientConnectionImpl::signal_connect (uint64_t hhi, uint64_t hlo, uint64_t orbid, SignalEmitHandler seh, void *data)
{
  assert_return (orbid > 0, 0);
  assert_return (hhi > 0, 0);   // FIXME: check for signal id
  assert_return (hlo > 0, 0);
  assert_return (seh != NULL, 0);
  SignalHandler *shandler = new SignalHandler;
  shandler->hhi = hhi;
  shandler->hlo = hlo;
  shandler->oid = orbid;                        // emitting object
  shandler->cid = 0;
  shandler->seh = seh;
  shandler->data = data;
  pthread_spin_lock (&signal_spin_);
  const uint handler_index = signal_handlers_.size();
  signal_handlers_.push_back (shandler);
  pthread_spin_unlock (&signal_spin_);
  const size_t handler_id = SignalHandlerIdParts (handler_index, connection_id()).vsize;
  FieldBuffer &fb = *FieldBuffer::_new (3 + 1 + 2);
  const uint orbid_connection = ObjectBroker::connection_id_from_orbid (shandler->oid);
  fb.add_header2 (MSGID_CONNECT, orbid_connection, connection_id(), shandler->hhi, shandler->hlo);
  fb.add_object (shandler->oid);                // emitting object
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
  const uint orbid_connection = ObjectBroker::connection_id_from_orbid (shandler->oid);
  fb.add_header2 (MSGID_CONNECT, orbid_connection, connection_id(), shandler->hhi, shandler->hlo);
  fb.add_object (shandler->oid);                // emitting object
  fb <<= 0;                                     // handler connection request id
  fb <<= shandler->cid;                         // disconnection request id
  FieldBuffer *connection_result = call_remote (&fb); // deletes fb
  assert_return (connection_result != NULL, false);
  FieldReader frr (*connection_result);
  frr.skip_header();
  uint64_t disconnection_success;
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
ClientConnectionImpl::type_name_from_orbid (uint64_t orbid)
{
  const uint type_index = IdentifierParts (orbid).orbid_type_index;
  return type_name_db.type_name (type_index);
}

// == ServerConnectionImpl ==
/// Transport and dispatch layer for messages sent between ClientConnection and ServerConnection.
class ServerConnectionImpl : public ServerConnection {
  TransportChannel         transport_channel_;       // messages sent to server
  RAPICORN_CLASS_NON_COPYABLE (ServerConnectionImpl);
  std::unordered_map<ptrdiff_t,uint64_t> addr_map;
  std::vector<ptrdiff_t>                 addr_vector;
  std::unordered_map<size_t, EmitResultHandler> emit_result_map;
  ImplicitBase *remote_origin_;
public:
  explicit              ServerConnectionImpl (const std::string &feature_keys);
  virtual              ~ServerConnectionImpl ()         { unregister_connection(); }
  virtual int           notify_fd  ()                   { return transport_channel_.inputfd(); }
  virtual bool          pending    ()                   { return transport_channel_.has_msg(); }
  virtual void          dispatch   ();
  virtual ImplicitBase* remote_origin  () const         { return remote_origin_; }
  virtual void          remote_origin  (ImplicitBase *rorigin);
  virtual uint64_t      instance2orbid (ImplicitBase*);
  virtual ImplicitBase* orbid2instance (uint64_t);
  virtual void          send_msg   (FieldBuffer *fb)    { assert_return (fb); transport_channel_.send_msg (fb, true); }
  virtual void              emit_result_handler_add (size_t id, const EmitResultHandler &handler);
  virtual EmitResultHandler emit_result_handler_pop (size_t id);
};

ServerConnectionImpl::ServerConnectionImpl (const std::string &feature_keys) :
  ServerConnection (feature_keys), remote_origin_ (NULL)
{
  addr_map[0] = 0;                                      // lookiing up NULL yields uint64_t (0)
  addr_vector.push_back (0);                            // orbid uint64_t (0) yields NULL
  register_connection();
}

void
ServerConnectionImpl::remote_origin (ImplicitBase *rorigin)
{
  if (rorigin)
    AIDA_ASSERT (remote_origin_ == NULL);
  else // rorigin == NULL
    AIDA_ASSERT (remote_origin_ != NULL);
  remote_origin_ = rorigin;
}

uint64_t
ServerConnectionImpl::instance2orbid (ImplicitBase *instance)
{
  const ptrdiff_t addr = reinterpret_cast<ptrdiff_t> (instance);
  const auto it = addr_map.find (addr);
  if (AIDA_LIKELY (it != addr_map.end()))
    return (*it).second;
  const uint64_t orbid = IdentifierParts (IdentifierParts::ORBID(), connection_id(), // see connection_id_from_orbid
                                          addr_vector.size(), type_name_db.index (instance->__aida_type_name__())).vuint64;
  addr_vector.push_back (addr);
  addr_map[addr] = orbid;
  return orbid;
}

ImplicitBase*
ServerConnectionImpl::orbid2instance (uint64_t orbid)
{
  const uint32 index = IdentifierParts (orbid).orbid32; // see connection_id_from_orbid
  const ptrdiff_t addr = AIDA_LIKELY (index < addr_vector.size()) ? addr_vector[index] : 0;
  return reinterpret_cast<ImplicitBase*> (addr);
}

void
ServerConnectionImpl::dispatch ()
{
  FieldBuffer *fb = transport_channel_.fetch_msg();
  if (!fb)
    return;
  FieldReader fbr (*fb);
  const MessageId msgid = MessageId (fbr.pop_int64());
  const uint64_t  idmask = msgid_mask (msgid);
  switch (idmask)
    {
    case MSGID_HELLO_REQUEST:
      {
        const uint64_t hashhigh = fbr.pop_int64(), hashlow = fbr.pop_int64();
        AIDA_ASSERT (hashhigh == 0 && hashlow == 0);
        AIDA_ASSERT (fbr.remaining() == 0);
        fbr.reset (*fb);
        ImplicitBase *rorigin = this->remote_origin();
        FieldBuffer *fr = ObjectBroker::renew_into_result (fbr, MSGID_HELLO_REPLY, ObjectBroker::receiver_connection_id (msgid), 0, 0, 1);
        fr->add_object (this->instance2orbid (rorigin));
        if (AIDA_LIKELY (fr == fb))
          fb = NULL; // prevent deletion
        const uint64_t resultmask = msgid_as_result (MessageId (idmask));
        AIDA_ASSERT (fr && msgid_mask (fr->first_id()) == resultmask);
        ObjectBroker::post_msg (fr);
      }
      break;
    case MSGID_CONNECT:
    case MSGID_TWOWAY_CALL:
    case MSGID_ONEWAY_CALL:
      {
        const uint64_t hashhigh = fbr.pop_int64(), hashlow = fbr.pop_int64();
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
            const uint64_t resultmask = msgid_as_result (MessageId (idmask));
            AIDA_ASSERT (fr && msgid_mask (fr->first_id()) == resultmask);
            ObjectBroker::post_msg (fr);
          }
      }
      break;
    case MSGID_EMIT_RESULT:
      {
        fbr.skip(); // hashhigh
        fbr.skip(); // hashlow
        const uint64_t emit_result_id = fbr.pop_int64();
        EmitResultHandler emit_result_handler = emit_result_handler_pop (emit_result_id);
        AIDA_ASSERT (emit_result_handler != NULL);
        emit_result_handler (fbr);
      }
      break;
    default:
      {
        const uint64_t hashhigh = fbr.pop_int64(), hashlow = fbr.pop_int64();
        warning_printf ("%s: invalid message: (%016lx, %016llx%016llx)", STRFUNC, msgid, hashhigh, hashlow);
      }
      break;
    }
  if (AIDA_UNLIKELY (fb))
    delete fb;
}

void
ServerConnectionImpl::emit_result_handler_add (size_t id, const EmitResultHandler &handler)
{
  AIDA_ASSERT (emit_result_map.count (id) == 0);        // PARANOID
  emit_result_map[id] = handler;
}

ServerConnectionImpl::EmitResultHandler
ServerConnectionImpl::emit_result_handler_pop (size_t id)
{
  auto it = emit_result_map.find (id);
  if (AIDA_LIKELY (it != emit_result_map.end()))
    {
      EmitResultHandler emit_result_handler = it->second;
      emit_result_map.erase (it);
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
ServerConnection::find_method (uint64_t hashhi, uint64_t hashlo)
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
      perror (string_printf ("%s:%u: Aida::ServerConnection::MethodRegistry::register_method: "
                             "duplicate hash registration (%016llx%016llx)",
                             __FILE__, __LINE__, mentry.hashhi, mentry.hashlo).c_str());
      abort();
    }
}

// == ObjectBroker ==
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
  for (uint id = 1; id <= MAX_CONNECTIONS; id++)
    {
      BaseConnection *bcon = BaseConnection::connection_from_id (id);
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
      return id; // all of feature_key_list matched
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
  BaseConnection *bcon = BaseConnection::connection_from_id (connection_id);
  if (!bcon)
    error_printf ("Message ID without valid connection: %016lx (connection_id=%u)", msgid, connection_id);
  const bool needsresult = msgid_has_result (msgid);
  const uint receiver_connection = ObjectBroker::receiver_connection_id (msgid);
  if (needsresult != (receiver_connection > 0)) // FIXME: move downwards
    error_printf ("mismatch of result flag and receiver_connection: %016lx", msgid);
  bcon->send_msg (fb);
}

} } // Rapicorn::Aida

#include "aidamap.cc"
