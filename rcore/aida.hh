// CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0/
#ifndef __RAPICORN_AIDA_HH__
#define __RAPICORN_AIDA_HH__

#include <rcore/cxxaux.hh>
#include <string>
#include <vector>
#include <memory>               // auto_ptr
#include <stdint.h>             // uint32_t
#include <stdarg.h>
#include <memory>               // shared_ptr
#include <set>
#include <map>

#ifndef _SHARED_PTR_H           // might require -std=c++0x
#include <tr1/memory>           // import shared_ptr if needed
namespace std {
using std::tr1::bad_weak_ptr;
using std::tr1::enable_shared_from_this;
using std::tr1::shared_ptr;
using std::tr1::weak_ptr;
}
#endif  // !_SHARED_PTR_H

namespace Rapicorn { namespace Aida {

// == Auxillary macros ==
#define AIDA_CPP_STRINGIFYi(s)  #s // indirection required to expand __LINE__ etc
#define AIDA_CPP_STRINGIFY(s)   AIDA_CPP_STRINGIFYi (s)
#if     __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)
#define AIDA_UNUSED             __attribute__ ((__unused__))
#define AIDA_DEPRECATED         __attribute__ ((__deprecated__))
#define AIDA_NORETURN           __attribute__ ((__noreturn__))
#define AIDA_PRINTF(fix, arx)   __attribute__ ((__format__ (__printf__, fix, arx)))
#define AIDA_BOOLi(expr)        __extension__ ({ bool _plic__bool; if (expr) _plic__bool = 1; else _plic__bool = 0; _plic__bool; })
#define AIDA_ISLIKELY(expr)     __builtin_expect (AIDA_BOOLi (expr), 1)
#define AIDA_UNLIKELY(expr)     __builtin_expect (AIDA_BOOLi (expr), 0)
#else   // !__GNUC__
#define AIDA_UNUSED
#define AIDA_DEPRECATED
#define AIDA_NORETURN
#define AIDA_PRINTF(fix, arx)
#define AIDA_ISLIKELY(expr)     expr
#define AIDA_UNLIKELY(expr)     expr
#endif
#define AIDA_LIKELY             AIDA_ISLIKELY

// == Standard Types ==
using std::vector;
typedef signed long long int   int64_t;  // libc's int64_t is a long on AMD64 which breaks printf
typedef unsigned long long int uint64_t; // libc's uint64_t is a long on AMD64 which breaks printf

// == TypeKind ==
/// Classification enum for the underlying kind of a TypeCode.
enum TypeKind {
  UNTYPED        = 0,   ///< Type indicator for unused Any instances.
  VOID           = 'v', ///< 'void' type.
  BOOL           = 'b', ///< Boolean type.
  INT32          = 'i', ///< Signed numeric type for 32bit.
  INT64          = 'l', ///< Signed numeric type for 64bit.
  FLOAT64        = 'd', ///< Floating point type of IEEE-754 Double precision.
  STRING         = 's', ///< String type for character sequence in UTF-8 encoding.
  ENUM           = 'E', ///< Enumeration type to represent choices.
  SEQUENCE       = 'Q', ///< Type to form sequences of other types.
  RECORD         = 'R', ///< Record type containing named field.
  INSTANCE       = 'C', ///< Interface instance type.
  FUNC           = 'F', ///< Type of methods or signals.
  TYPE_REFERENCE = 'T', ///< Type reference for record fields.
  ANY            = 'Y', ///< Generic type to hold any other type.
};
const char* type_kind_name (TypeKind type_kind); ///< Obtain TypeKind names as a string.

// == TypeCode ==
struct TypeCode /// Representation of type information to describe structured type compositions and for the Any class.
{
  typedef std::string String;
  TypeKind              kind            () const;               ///< Obtain the underlying primitive type kind.
  std::string           kind_name       () const;               ///< Obtain the name of kind().
  std::string           name            () const;               ///< Obtain the type name.
  size_t                aux_count       () const;               ///< Number of items of auxillary data.
  std::string           aux_data        (size_t index) const;   ///< Accessor for auxillary data as key=utf8data string.
  std::string           aux_value       (std::string key) const; ///< Accessor for auxillary data by key as utf8 string.
  std::string           hints           () const;               ///< Obtain "hints" aux_value(), enclosed in two ':'.
  size_t                enum_count      () const;               ///< Number of enum values for an enum type.
  std::vector<String>   enum_value      (size_t index) const;   ///< Obtain an enum value as: (ident,label,blurb)
  size_t                prerequisite_count () const;            ///< Number of interface prerequisites
  std::string           prerequisite    (size_t index) const;   ///< Obtain prerequisite type names for an interface type.
  size_t                field_count     () const;               ///< Number of fields in a record type.
  TypeCode              field           (size_t index) const;   ///< Obtain field type for a record or sequence type.
  std::string           origin          () const;               ///< Obtain the type origin for a TYPE_REFERENCE (fields).
  bool                  untyped         () const;               ///< Checks whether the TypeCode is undefined.
  std::string           pretty          (const std::string &indent = "") const; ///< Pretty print into a string.
  bool                  operator!=      (const TypeCode &o) const;
  bool                  operator==      (const TypeCode&) const;
  /*copy*/              TypeCode        (const TypeCode&);
  TypeCode&             operator=       (const TypeCode&);
  /*dtor*/             ~TypeCode        ();
  void                  swap            (TypeCode &other); ///< Swap the contents of @a this and @a other in constant time.
  class InternalType;  class MapHandle;
private: // implementation bits
  explicit              TypeCode        (MapHandle*, InternalType*);
  static TypeCode       notype          (MapHandle*);
  friend class TypeMap;
  MapHandle    *m_handle;
  InternalType *m_type;
};

class TypeMap /// A TypeMap serves as a repository and loader for IDL type information.
{
  TypeCode::MapHandle  *m_handle;     friend class TypeCode::MapHandle;
  explicit              TypeMap      (TypeCode::MapHandle*);
  static TypeMap        builtins     ();
public:
  /*copy*/              TypeMap      (const TypeMap&);
  TypeMap&              operator=    (const TypeMap&);
  /*dtor*/             ~TypeMap      ();
  size_t                type_count   () const;                  ///< Number of TypeCode classes in this TypeMap.
  const TypeCode        type         (size_t      index) const; ///< Obtain a TypeCode by index.
  int                   error_status ();                        ///< Obtain errno status from load().
  static TypeMap        load         (std::string file_name);   ///< Load a new TypeMap and register for global lookups.
  static TypeCode       lookup       (std::string name);        ///< Globally lookup a TypeCode by name.
  static TypeMap        load_local   (std::string file_name);   ///< Load a new TypeMap for local lookups only.
  TypeCode              lookup_local (std::string name) const;  ///< Lookup TypeCode within this TypeMap.
  static TypeCode       notype       ();
};

// == Any Type ==
class Any /// Generic value type that can hold values of all other types.
{
  typedef std::string String;
  TypeCode type_code;
  typedef std::vector<Any> AnyVector;
  union { int64_t vint64; uint64_t vuint64; double vdouble; Any *vany; uint64_t qmem[(sizeof (AnyVector) + 7) / 8];
    uint64_t smem[(sizeof (String) + 7) / 8]; uint8_t bytes[8]; } u;
  void    ensure  (TypeKind _kind) { if (AIDA_LIKELY (kind() == _kind)) return; rekind (_kind); }
  void    rekind  (TypeKind _kind);
  void    reset   ();
  bool    to_int  (int64_t &v, char b) const;
public:
  /*dtor*/ ~Any    ();
  explicit  Any    ();
  /*copy*/  Any    (const Any &clone);                   ///< Carry out a deep copy of @a clone into a new Any.
  Any& operator=   (const Any &clone);                   ///< Carry out a deep copy of @a clone into this Any.
  bool operator==  (const Any &clone) const;             ///< Check if Any is exactly equal to @a clone.
  bool operator!=  (const Any &clone) const;             ///< Check if Any is not equal to @a clone, see operator==().
  TypeCode  tyoe   () const { return type_code; }        ///< Obtain the full TypeCode for the contents of this Any.
  TypeKind  kind   () const { return type_code.kind(); } ///< Obtain the underlying primitive type kind.
  void      retype (const TypeCode &tc);                 ///< Force Any to assume type @a tc.
  void      swap   (Any            &other);              ///< Swap the contents of @a this and @a other in constant time.
  bool operator>>= (bool          &v) const { int64_t d; const bool r = to_int (d, 1); v = d; return r; }
  bool operator>>= (char          &v) const { int64_t d; const bool r = to_int (d, 7); v = d; return r; }
  bool operator>>= (unsigned char &v) const { int64_t d; const bool r = to_int (d, 8); v = d; return r; }
  bool operator>>= (int           &v) const { int64_t d; const bool r = to_int (d, 31); v = d; return r; }
  bool operator>>= (unsigned int  &v) const { int64_t d; const bool r = to_int (d, 32); v = d; return r; }
  bool operator>>= (long          &v) const { int64_t d; const bool r = to_int (d, 47); v = d; return r; }
  bool operator>>= (unsigned long &v) const { int64_t d; const bool r = to_int (d, 48); v = d; return r; }
  bool operator>>= (int64_t       &v) const; ///< Extract a 64bit integer if possible.
  bool operator>>= (uint64_t      &v) const { int64_t d; const bool r = to_int (d, 64); v = d; return r; }
  bool operator>>= (float         &v) const { double d; const bool r = operator>>= (d); v = d; return r; }
  bool operator>>= (double        &v) const; ///< Extract a floating point number as double if possible.
  bool operator>>= (const char   *&v) const { String s; const bool r = operator>>= (s); v = s.c_str(); return r; }
  bool operator>>= (std::string   &v) const; ///< Extract a std::string if possible.
  bool operator>>= (const Any    *&v) const; ///< Extract an Any if possible.
  const Any& as_any   () const { return kind() == ANY ? *u.vany : *this; } ///< Obtain contents as Any.
  double     as_float () const; ///< Obtain BOOL, INT*, or FLOAT* contents as double float.
  int64_t    as_int   () const; ///< Obtain BOOL, INT* or FLOAT* contents as integer (yields 1 for non-empty strings).
  String     as_string() const; ///< Obtain BOOL, INT*, FLOAT* or STRING contents as string.
  // >>= enum
  // >>= sequence
  // >>= record
  // >>= instance
  void operator<<= (bool           v) { operator<<= (int64_t (v)); }
  void operator<<= (char           v) { operator<<= (int64_t (v)); }
  void operator<<= (unsigned char  v) { operator<<= (int64_t (v)); }
  void operator<<= (int            v) { operator<<= (int64_t (v)); }
  void operator<<= (unsigned int   v) { operator<<= (int64_t (v)); }
  void operator<<= (long           v) { operator<<= (int64_t (v)); }
  void operator<<= (unsigned long  v) { operator<<= (int64_t (v)); }
  void operator<<= (uint64_t       v);
  void operator<<= (int64_t        v); ///< Store a 64bit integer.
  void operator<<= (float          v) { operator<<= (double (v)); }
  void operator<<= (double         v); ///< Store a double floating point number.
  void operator<<= (const char    *v) { operator<<= (std::string (v)); }
  void operator<<= (char          *v) { operator<<= (std::string (v)); }
  void operator<<= (const String  &v); ///< Store a std::string.
  void operator<<= (const Any     &v); ///< Store an Any,
  // <<= enum
  // <<= sequence
  // <<= record
  // <<= instance
  void resize (size_t n); ///< Resize Any to contain a sequence of length @a n.
};

// == Type Declarations ==
class ObjectBroker;
union FieldUnion;
class FieldBuffer;
class FieldReader;
typedef FieldBuffer* (*DispatchFunc) (FieldReader&);

// == Type Hash ==
struct TypeHash {
  uint64_t typehi, typelo;
  explicit    TypeHash   (uint64_t hi, uint64_t lo) : typehi (hi), typelo (lo) {}
  explicit    TypeHash   () : typehi (0), typelo (0) {}
  inline bool operator== (const TypeHash &z) const { return typehi == z.typehi && typelo == z.typelo; }
};
typedef std::vector<TypeHash> TypeHashList;

// == Utilities ==
template<class V> inline
bool    atomic_ptr_cas  (V* volatile *ptr_adr, V *o, V *n) { return __sync_bool_compare_and_swap (ptr_adr, o, n); }
void    error_printf    (const char *format, ...) AIDA_PRINTF (1, 2) AIDA_NORETURN;
void    error_vprintf   (const char *format, va_list args) AIDA_NORETURN;
void    warning_printf  (const char *format, ...) AIDA_PRINTF (1, 2);

// == Type Utilities ==
template<class Y> struct ValueType           { typedef Y T; };
template<class Y> struct ValueType<Y&>       { typedef Y T; };
template<class Y> struct ValueType<const Y&> { typedef Y T; };

// == Message IDs ==
enum MessageId {
  MSGID_NONE        = 0,
  MSGID_ONEWAY      = 0x2000000000000000ULL,      ///< One-way method call ID (void return).
  MSGID_TWOWAY      = 0x3000000000000000ULL,      ///< Two-way method call ID, returns result message.
  MSGID_DISCON      = 0x4000000000000000ULL,      ///< Signal handler disconnection ID.
  MSGID_SIGCON      = 0x5000000000000000ULL,      ///< Signal connection/disconnection request ID, returns result message.
  MSGID_EVENT       = 0x6000000000000000ULL,      ///< One-way signal event message ID.
  // MSGID_SIGNAL   = 0x7000000000000000ULL,      ///< Two-way signal message ID, returns result message.
};
inline bool msgid_has_result    (MessageId mid) { return (mid & 0x9000000000000000ULL) == 0x1000000000000000ULL; }
inline bool msgid_is_result     (MessageId mid) { return (mid & 0x9000000000000000ULL) == 0x9000000000000000ULL; }
inline bool msgid_is_error      (MessageId mid) { return (mid & 0xf000000000000000ULL) == 0x8000000000000000ULL; }
inline bool msgid_is_oneway     (MessageId mid) { return (mid & 0x7000000000000000ULL) == MSGID_ONEWAY; }
inline bool msgid_is_twoway     (MessageId mid) { return (mid & 0x7000000000000000ULL) == MSGID_TWOWAY; }
inline bool msgid_is_discon     (MessageId mid) { return (mid & 0x7000000000000000ULL) == MSGID_DISCON; }
inline bool msgid_is_sigcon     (MessageId mid) { return (mid & 0x7000000000000000ULL) == MSGID_SIGCON; }
inline bool msgid_is_event      (MessageId mid) { return (mid & 0x7000000000000000ULL) == MSGID_EVENT; }

// == OrbObject ==
/// Internal management structure for objects known to the ORB.
class OrbObject {
  ptrdiff_t     orbid_;
protected:
  explicit      OrbObject (ptrdiff_t orbid);
public:
  ptrdiff_t     orbid     ()            { return orbid_; }
};

// == SmartHandle ==
class SmartHandle {
  OrbObject     *orbo_;
  template<class Parent> struct NullSmartHandle : public Parent { TypeHashList cast_types () { return TypeHashList(); } };
  typedef NullSmartHandle<SmartHandle> NullHandle;
  friend class ObjectBroker;
protected:
  typedef bool (SmartHandle::*_UnspecifiedBool) () const; // non-numeric operator bool() result
  static inline _UnspecifiedBool _unspecified_bool_true ()      { return &Aida::SmartHandle::_is_null; }
  explicit                  SmartHandle (OrbObject&);
  explicit                  SmartHandle ();
public:
  uint64_t                  _orbid      () const { return orbo_->orbid(); }
  bool                      _is_null    () const { return !orbo_->orbid(); }
  virtual                  ~SmartHandle ();
  static NullHandle         _null_handle()       { return NullHandle(); }
};

// == ObjectBroker ==
class ObjectBroker {
public:
  static void pop_handle (FieldReader&, SmartHandle&);
  static void dup_handle (const ptrdiff_t[2], SmartHandle&);
};

// == FieldBuffer ==
class _FakeFieldBuffer { FieldUnion *u; virtual ~_FakeFieldBuffer() {}; };

union FieldUnion {
  int64_t      vint64;
  double       vdouble;
  Any         *vany;
  uint64_t     smem[(sizeof (std::string) + 7) / 8];      // String
  uint64_t     bmem[(sizeof (_FakeFieldBuffer) + 7) / 8]; // FieldBuffer
  uint8_t      bytes[8];                // FieldBuffer types
  struct { uint32_t capacity, index; }; // FieldBuffer.buffermem[0]
};

struct EnumValue { int64_t v; EnumValue (int64_t e) : v (e) {} };

class FieldBuffer { // buffer for marshalling procedure calls
  typedef std::string String;
  friend class FieldReader;
  void               check_internal ();
  inline FieldUnion& upeek (uint32_t n) const { return buffermem[offset() + n]; }
protected:
  FieldUnion        *buffermem;
  inline void        check ()      { if (AIDA_UNLIKELY (size() > capacity())) check_internal(); }
  inline uint32_t    offset () const { const uint32_t offs = 1 + (capacity() + 7) / 8; return offs; }
  inline TypeKind    type_at  (uint32_t n) const { return TypeKind (buffermem[1 + n/8].bytes[n%8]); }
  inline void        set_type (TypeKind ft)  { buffermem[1 + size()/8].bytes[size()%8] = ft; }
  inline uint32_t    capacity () const       { return buffermem[0].capacity; }
  inline uint32_t    size () const           { return buffermem[0].index; }
  inline FieldUnion& getu () const           { return buffermem[offset() + size()]; }
  inline FieldUnion& addu (TypeKind ft) { set_type (ft); FieldUnion &u = getu(); buffermem[0].index++; check(); return u; }
  inline FieldUnion& uat (uint32_t n) const { return n < size() ? upeek (n) : *(FieldUnion*) NULL; }
  explicit           FieldBuffer (uint32_t _ntypes);
  explicit           FieldBuffer (uint32_t, FieldUnion*, uint32_t);
public:
  virtual     ~FieldBuffer();
  inline uint64_t first_id () const { return buffermem && size() && type_at (0) == INT64 ? upeek (0).vint64 : 0; }
  inline void add_bool   (bool    vbool)   { FieldUnion &u = addu (BOOL); u.vint64 = vbool; }
  inline void add_int64  (int64_t vint64)  { FieldUnion &u = addu (INT64); u.vint64 = vint64; }
  inline void add_evalue (int64_t vint64)  { FieldUnion &u = addu (ENUM); u.vint64 = vint64; }
  inline void add_double (double vdouble)  { FieldUnion &u = addu (FLOAT64); u.vdouble = vdouble; }
  inline void add_string (const String &s) { FieldUnion &u = addu (STRING); new (&u) String (s); }
  inline void add_object (uint64_t objid)  { FieldUnion &u = addu (INSTANCE); u.vint64 = objid; }
  inline void add_any    (const Any &vany) { FieldUnion &u = addu (ANY); u.vany = new Any (vany); }
  inline void add_msgid  (uint64_t h, uint64_t l) { add_int64 (h); add_int64 (l); }
  inline FieldBuffer& add_rec (uint32_t nt) { FieldUnion &u = addu (RECORD); return *new (&u) FieldBuffer (nt); }
  inline FieldBuffer& add_seq (uint32_t nt) { FieldUnion &u = addu (SEQUENCE); return *new (&u) FieldBuffer (nt); }
  inline void         reset();
  String              first_id_str() const;
  String              to_string() const;
  static String       type_name (int field_type);
  static FieldBuffer* _new (uint32_t _ntypes); // Heap allocated FieldBuffer
  static FieldBuffer* new_error (const String &msg, const String &domain = "");
  static FieldBuffer* new_result (uint32_t n = 1);
  inline void operator<<= (size_t v)          { FieldUnion &u = addu (INT64); u.vint64 = v; }
  inline void operator<<= (uint64_t v)        { FieldUnion &u = addu (INT64); u.vint64 = v; }
  inline void operator<<= (int64_t  v)        { FieldUnion &u = addu (INT64); u.vint64 = v; }
  inline void operator<<= (uint32_t v)        { FieldUnion &u = addu (INT64); u.vint64 = v; }
  inline void operator<<= (int    v)          { FieldUnion &u = addu (INT64); u.vint64 = v; }
  inline void operator<<= (bool   v)          { FieldUnion &u = addu (BOOL); u.vint64 = v; }
  inline void operator<<= (double v)          { FieldUnion &u = addu (FLOAT64); u.vdouble = v; }
  inline void operator<<= (EnumValue e)       { FieldUnion &u = addu (ENUM); u.vint64 = e.v; }
  inline void operator<<= (const String &s)   { FieldUnion &u = addu (STRING); new (&u) String (s); }
  inline void operator<<= (Any    v)          { FieldUnion &u = addu (ANY); u.vany = new Any (v); }
  inline void operator<<= (const TypeHash &h) { *this <<= h.typehi; *this <<= h.typelo; }
};

class FieldBuffer8 : public FieldBuffer { // Stack contained buffer for up to 8 fields
  FieldUnion bmem[1 + 1 + 8];
public:
  virtual ~FieldBuffer8 () { reset(); buffermem = NULL; }
  inline   FieldBuffer8 (uint32_t ntypes = 8) : FieldBuffer (ntypes, bmem, sizeof (bmem)) {}
};

class FieldReader { // read field buffer contents
  typedef std::string String;
  const FieldBuffer *m_fb;
  uint32_t           m_nth;
  void               check_request (int type);
  inline void        request (int t) { if (AIDA_UNLIKELY (m_nth >= n_types() || get_type() != t)) check_request (t); }
  inline FieldUnion& fb_getu (int t) { request (t); return m_fb->upeek (m_nth); }
  inline FieldUnion& fb_popu (int t) { request (t); FieldUnion &u = m_fb->upeek (m_nth++); return u; }
public:
  explicit                 FieldReader (const FieldBuffer &fb) : m_fb (&fb), m_nth (0) {}
  inline void               reset      (const FieldBuffer &fb) { m_fb = &fb; m_nth = 0; }
  inline void               reset      () { m_fb = NULL; m_nth = 0; }
  inline uint32_t           remaining  () { return n_types() - m_nth; }
  inline void               skip       () { if (AIDA_UNLIKELY (m_nth >= n_types())) check_request (0); m_nth++; }
  inline void               skip_msgid () { skip(); skip(); }
  inline uint32_t           n_types    () { return m_fb->size(); }
  inline TypeKind           get_type   () { return m_fb->type_at (m_nth); }
  inline int64_t            get_bool   () { FieldUnion &u = fb_getu (BOOL); return u.vint64; }
  inline int64_t            get_int64  () { FieldUnion &u = fb_getu (INT64); return u.vint64; }
  inline int64_t            get_evalue () { FieldUnion &u = fb_getu (ENUM); return u.vint64; }
  inline double             get_double () { FieldUnion &u = fb_getu (FLOAT64); return u.vdouble; }
  inline const String&      get_string () { FieldUnion &u = fb_getu (STRING); return *(String*) &u; }
  inline uint64_t           get_object () { FieldUnion &u = fb_getu (INSTANCE); return u.vint64; }
  inline const Any&         get_any    () { FieldUnion &u = fb_getu (ANY); return *u.vany; }
  inline const FieldBuffer& get_rec    () { FieldUnion &u = fb_getu (RECORD); return *(FieldBuffer*) &u; }
  inline const FieldBuffer& get_seq    () { FieldUnion &u = fb_getu (SEQUENCE); return *(FieldBuffer*) &u; }
  inline int64_t            pop_bool   () { FieldUnion &u = fb_popu (BOOL); return u.vint64; }
  inline int64_t            pop_int64  () { FieldUnion &u = fb_popu (INT64); return u.vint64; }
  inline int64_t            pop_evalue () { FieldUnion &u = fb_popu (ENUM); return u.vint64; }
  inline double             pop_double () { FieldUnion &u = fb_popu (FLOAT64); return u.vdouble; }
  inline const String&      pop_string () { FieldUnion &u = fb_popu (STRING); return *(String*) &u; }
  inline uint64_t           pop_object () { FieldUnion &u = fb_popu (INSTANCE); return u.vint64; }
  inline const Any&         pop_any    () { FieldUnion &u = fb_popu (ANY); return *u.vany; }
  inline const FieldBuffer& pop_rec    () { FieldUnion &u = fb_popu (RECORD); return *(FieldBuffer*) &u; }
  inline const FieldBuffer& pop_seq    () { FieldUnion &u = fb_popu (SEQUENCE); return *(FieldBuffer*) &u; }
  inline void operator>>= (size_t &v)          { FieldUnion &u = fb_popu (INT64); v = u.vint64; }
  inline void operator>>= (uint64_t &v)        { FieldUnion &u = fb_popu (INT64); v = u.vint64; }
  inline void operator>>= (int64_t &v)         { FieldUnion &u = fb_popu (INT64); v = u.vint64; }
  inline void operator>>= (uint32_t &v)        { FieldUnion &u = fb_popu (INT64); v = u.vint64; }
  inline void operator>>= (int &v)             { FieldUnion &u = fb_popu (INT64); v = u.vint64; }
  inline void operator>>= (bool &v)            { FieldUnion &u = fb_popu (BOOL); v = u.vint64; }
  inline void operator>>= (double &v)          { FieldUnion &u = fb_popu (FLOAT64); v = u.vdouble; }
  inline void operator>>= (EnumValue &e)       { FieldUnion &u = fb_popu (ENUM); e.v = u.vint64; }
  inline void operator>>= (String &s)          { FieldUnion &u = fb_popu (STRING); s = *(String*) &u; }
  inline void operator>>= (Any &v)             { FieldUnion &u = fb_popu (ANY); v = *u.vany; }
  inline void operator>>= (TypeHash &h)        { *this >>= h.typehi; *this >>= h.typelo; }
  inline void operator>>= (std::vector<bool>::reference v) { bool b; *this >>= b; v = b; }
};

// == Connections ==
class ClientConnection;

/// Client and server connection interface.
class Connector {
public:
  virtual int          notify_fd   () = 0;      ///< Returns fd for POLLIN, to wake up on incomming events.
  virtual bool         pending     () = 0;      ///< Indicate whether any incoming events are pending that need to be dispatched.
  virtual FieldBuffer* pop         () = 0;      ///< Dispatch a single event if any is pending.
  virtual FieldBuffer* call_remote (FieldBuffer*) = 0; ///< Carry out a remote call syncronously, transfers memory.
  virtual void         ref         () = 0;
  virtual void         unref       () = 0;
};

/// Function typoe for internal signal handling.
typedef FieldBuffer* SignalEmitHandler (ClientConnection&, const FieldBuffer*, void*);

/// Connection context for IPC servers. @nosubgrouping
class ServerConnection {
  RAPICORN_CLASS_NON_COPYABLE (ServerConnection);
public: /// @name Construction
  static ServerConnection* create_threaded  ();
  virtual                 ~ServerConnection ();
protected:                 ServerConnection ();
public: /// @name API for remote calls
  virtual void       send_event (FieldBuffer*) = 0; ///< Send event to remote asyncronously, transfers memory.
  virtual int        notify_fd  () = 0; ///< Returns fd for POLLIN, to wake up on incomming events.
  virtual bool       pending    () = 0; ///< Indicate whether any incoming calls are pending that need to be dispatched.
  virtual void       dispatch   () = 0; ///< Dispatch a single call if any is pending.
  virtual Connector& connector  () = 0;
protected: /// @name Registry for IPC method lookups
  static DispatchFunc find_method (uint64_t hi, uint64_t lo); ///< Lookup method in registry.
public:
  struct MethodEntry       { uint64_t hashhi, hashlo; DispatchFunc dispatcher; };
  struct MethodRegistry    /// Registry structure for IPC method stubs.
  {
    template<class T, size_t S> MethodRegistry  (T (&static_const_entries)[S])
    { for (size_t i = 0; i < S; i++) register_method (static_const_entries[i]); }
  private: static void register_method  (const MethodEntry &mentry);
  };
};

/// Connection context for IPC clients. @nosubgrouping
class ClientConnection {
  RAPICORN_CLASS_NON_COPYABLE (ClientConnection);
public: /// @name API for remote calls
  FieldBuffer*  call_remote (FieldBuffer*); ///< Carry out a remote call syncronously, transfers memory.
  int           notify_fd   ();             ///< Returns fd for POLLIN, to wake up on incomming events.
  bool          pending     ();             ///< Indicate whether any incoming events are pending that need to be dispatched.
  void          dispatch    ();             ///< Dispatch a single event if any is pending.
public: /// @name Lifetime
  virtual      ~ClientConnection ();
  /*ctor*/      ClientConnection (Connector&);
public: /// @name API for event handler bookkeeping
  uint64_t             signal_connect    (uint64_t hhi, uint64_t hlo, uint64_t handle_id, SignalEmitHandler seh, void *data);
  bool                 signal_disconnect (uint64_t signal_handler_id);
  struct EventHandler                       /// Interface class used for client side signal emissions.
  {
    virtual             ~EventHandler ();
    virtual FieldBuffer* handle_event (Aida::FieldBuffer &event_fb) = 0; ///< Process an event and possibly return an error.
  };
  uint64_t      register_event_handler (EventHandler   *evh); ///< Register an event handler, transfers memory.
  EventHandler* find_event_handler     (uint64_t handler_id); ///< Find event handler by id.
  bool          delete_event_handler   (uint64_t handler_id); ///< Delete a registered event handler, returns success.
private: /// @name Internals
  Connector    *m_connector;
  struct SignalHandler { uint64_t hhi, hlo, oid, cid; SignalEmitHandler *seh; void *data; };
  std::map<uint64_t,SignalHandler*> signal_handler_map_;
  pthread_spinlock_t                signal_spin_;
  SignalHandler*                    signal_lookup (uint64_t handler_id);
  // client event handler
  typedef std::set<uint64_t> UIntSet;
  UIntSet                   ehandler_set;
};

// == inline implementations ==
inline void
FieldBuffer::reset()
{
  if (!buffermem)
    return;
  while (size() > 0)
    {
      buffermem[0].index--; // causes size()--
      switch (type_at (size()))
        {
        case STRING:    { FieldUnion &u = getu(); ((String*) &u)->~String(); }; break;
        case ANY:       { FieldUnion &u = getu(); delete u.vany; }; break;
        case SEQUENCE:
        case RECORD:    { FieldUnion &u = getu(); ((FieldBuffer*) &u)->~FieldBuffer(); }; break;
        default: ;
        }
    }
}

} } // Rapicorn::Aida

#endif // __RAPICORN_AIDA_HH__
