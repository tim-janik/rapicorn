// Licensed CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0
#ifndef __RAPICORN_AIDA_HH__
#define __RAPICORN_AIDA_HH__

#include <rcore/cxxaux.hh>
#include <string>
#include <vector>
#include <stdint.h>             // uint32_t
#include <stdarg.h>
#include <type_traits>
#include <future>
#include <set>
#include <map>

namespace Rapicorn { namespace Aida {

// == Auxillary macros ==
#define AIDA_CPP_STRINGIFYi(s)  #s // indirection required to expand __LINE__ etc
#define AIDA_CPP_STRINGIFY(s)   AIDA_CPP_STRINGIFYi (s)
#if     __GNUC__ >= 4
#define AIDA_UNUSED             __attribute__ ((__unused__))
#define AIDA_DEPRECATED         __attribute__ ((__deprecated__))
#define AIDA_NORETURN           __attribute__ ((__noreturn__))
#define AIDA_PRINTF(fix, arx)   __attribute__ ((__format__ (__printf__, fix, arx)))
#define AIDA_BOOLi(expr)        __extension__ ({ bool _plic__bool; if (expr) _plic__bool = 1; else _plic__bool = 0; _plic__bool; })
#define AIDA_ISLIKELY(expr)     __builtin_expect (AIDA_BOOLi (expr), 1)
#define AIDA_UNLIKELY(expr)     __builtin_expect (AIDA_BOOLi (expr), 0)
#define AIDA_ASSERT(expr)       do { if (__builtin_expect (!(expr), 0)) ::Rapicorn::Aida::assertion_error (__FILE__, __LINE__, #expr); } while (0)
#define AIDA_ASSERT_RETURN(expr,...) do { if (__builtin_expect (!!(expr), 1)) break; ::Rapicorn::Aida::assertion_error (__FILE__, __LINE__, #expr); return __VA_ARGS__; } while (0)
#else   // !__GNUC__
#define AIDA_UNUSED
#define AIDA_DEPRECATED
#define AIDA_NORETURN
#define AIDA_PRINTF(fix, arx)
#define AIDA_ISLIKELY(expr)     expr
#define AIDA_UNLIKELY(expr)     expr
#define AIDA_ASSERT(expr)       do { } while (0)
#endif
#define AIDA_LIKELY             AIDA_ISLIKELY

// == Type Imports ==
using std::vector;
using Rapicorn::int32;
using Rapicorn::uint32;
using Rapicorn::int64;
using Rapicorn::uint64;
typedef std::string String;

// == Forward Declarations ==
class RemoteHandle;
class OrbObject;
class ImplicitBase;
class ObjectBroker;
class BaseConnection;
class ClientConnection;
class ServerConnection;
union ProtoUnion;
class ProtoMsg;
class ProtoReader;
struct PropertyList;
class Property;
typedef std::shared_ptr<OrbObject>    OrbObjectP;
typedef std::shared_ptr<ImplicitBase> ImplicitBaseP;
typedef ProtoMsg* (*DispatchFunc) (ProtoReader&);

// == EnumValue ==
/// Aida info for enumeration values.
struct EnumValue {
  int64       value;
  const char *ident, *label, *blurb;
  constexpr EnumValue (int64 dflt = 0) : value (dflt), ident (0), label (0), blurb (0) {}
  constexpr EnumValue (int64 v, const char *vident, const char *vlabel, const char *vblurb) :
    value (v), ident (vident), label (vlabel), blurb (vblurb) {}
};
typedef std::vector<const EnumValue*> EnumValueVector;

// == Enum ==
/// Class for enum type introspection.
class EnumInfo {
  const char      *const name_;
  const EnumValue *values_;
  const uint32_t   n_values_;
  const bool       flags_;
  explicit         EnumInfo ();
  template<size_t N>
  explicit         EnumInfo (const char *nm, const EnumValue (&ev)[N], bool f) : name_ (nm), values_ (ev), n_values_ (N), flags_ (f) {}
public:
  String           name              () const;                          ///< Retrieve the enum type name for this Enum.
  const EnumValue* find_value        (const String &name) const;        ///< Find first enum value matching @a name.
  const EnumValue* find_value        (int64        value) const;        ///< Find first enum value equal to @a value.
  String           value_to_string   (int64        value) const;        ///< Create a string representing @a value.
  int64            value_from_string (const String &valuestring) const; ///< Reconstruct an enum value from @a valuestring.
  bool             flags_enum        () const;  ///< Whether enum values support bit combinations to form flags.
  size_t           n_values          () const;  ///< The number of enum values defined for this Enum.
  const EnumValue* values            () const;  ///< Get an enum value array with n_values() elements for this Enum.
  EnumValueVector  value_vector      () const;  ///< Retrieve the list of possible enum values as a std::vector<>.
  /// Template to be specialised by introspectable enums.
  template<typename>
  friend EnumInfo  enum_info         () { return EnumInfo (); }
};

template<typename EnumType> EnumType
enum_value_from_string (const String &valuestring)      ///< Type-safe variant of EnumInfo.value_from_string().
{ return (EnumType) Rapicorn::Aida::enum_info<EnumType>().value_from_string (valuestring); }
template<typename EnumType> String
enum_value_to_string (EnumType evalue)                  ///< Type-safe variant of EnumInfo.value_to_string().
{ return Rapicorn::Aida::enum_info<EnumType>().value_to_string (evalue); }

std::vector<const char*> split_aux_char_array (const char *char_array, size_t length); ///< Split @a char_array at '\\0'.

// == TypeKind ==
/// Classification enum for the underlying type.
enum TypeKind {
  UNTYPED        = 0,   ///< Type indicator for unused Any instances.
  VOID           = 'v', ///< 'void' type.
  BOOL           = 'b', ///< Boolean type.
  INT32          = 'i', ///< Signed numeric type for 32bit.
  INT64          = 'l', ///< Signed numeric type for 64bit.
  FLOAT64        = 'd', ///< Floating point type of IEEE-754 Double precision.
  STRING         = 's', ///< String type for character sequence in UTF-8 encoding.
  ENUM           = 'E', ///< Enumeration type to represent choices.
  SEQUENCE       = 'Q', ///< Type to form sequences of an other type.
  RECORD         = 'R', ///< Record type containing named fields.
  INSTANCE       = 'C', ///< Interface instance type.
  REMOTE         = 'r', ///< RemoteHandle type.
  LOCAL          = 'L', ///< Local object type.
  ANY            = 'Y', ///< Generic type to hold any other type.
};
template<> EnumInfo enum_info<TypeKind> ();

const char* type_kind_name (TypeKind type_kind); ///< Obtain TypeKind names as a string.

// == TypeHash ==
struct TypeHash {
  uint64 typehi, typelo;
  explicit    TypeHash   (uint64 hi, uint64 lo) : typehi (hi), typelo (lo) {}
  explicit    TypeHash   () : typehi (0), typelo (0) {}
  String      to_string  () const;
  inline bool operator== (const TypeHash &z) const { return typehi == z.typehi && typelo == z.typelo; }
  friend bool operator<  (const TypeHash &a, const TypeHash &b)
  {
    return AIDA_UNLIKELY (a.typehi == b.typehi) ? a.typelo < b.typelo : a.typehi < b.typehi;
  }
};
typedef std::vector<TypeHash> TypeHashList;

// == ImplicitBase ==
/// Abstract base interface that all IDL interfaces are implicitely derived from.
class ImplicitBase : public virtual std::enable_shared_from_this<ImplicitBase> {
protected:
  virtual                     ~ImplicitBase       () = 0; // abstract class
  virtual const PropertyList& __aida_properties__ () = 0; ///< Retrieve the list of properties for @a this instance.
  Property*                   __aida_lookup__     (const std::string &property_name);
  bool                        __aida_setter__     (const std::string &property_name, const std::string &value);
  std::string                 __aida_getter__     (const std::string &property_name);
public:
  virtual std::string        __aida_type_name__   () const = 0; ///< Retrieve the IDL type name of an instance.
  virtual TypeHashList       __aida_typelist__    () const = 0;
  std::shared_ptr
  <const ImplicitBase>       shared_from_this     () const { return std::enable_shared_from_this<ImplicitBase>::shared_from_this(); }
  ImplicitBaseP              shared_from_this     ()       { return std::enable_shared_from_this<ImplicitBase>::shared_from_this(); }
  ImplicitBaseP              shared_from_this     (std::nullptr_t);
};


// == Any Type ==
class Any /// Generic value type that can hold values of all other types.
{
  ///@cond
  template<class ANY> struct AnyField : ANY { // We must wrap Any::Field into a template, because "Any" is not yet fully defined.
    std::string name;
    AnyField () = default;
    template<class V> inline
    AnyField (const std::string &_name, V &&value) : ANY (::std::forward<V> (value)), name (_name) {}
  };
  struct PlaceHolder {
    virtual                      ~PlaceHolder() {}
    virtual PlaceHolder*          clone      () const = 0;
    virtual const std::type_info& type_info  () const = 0;
    virtual bool                  operator== (const PlaceHolder*) const = 0;
  };
  template<class T> struct Holder : PlaceHolder {
    explicit                      Holder    (const T &value) : value_ (value) {}
    virtual PlaceHolder*          clone     () const { return new Holder (value_); }
    virtual const std::type_info& type_info () const { return typeid (T); }
    virtual bool                  operator== (const PlaceHolder *rhs) const
    { const Holder *other = dynamic_cast<const Holder*> (rhs); return other ? eq (this, other) : false; }
    template<class Q = T> static typename std::enable_if<IsComparable<Q>::value, bool>::
    type eq (const Holder *a, const Holder *b) { return a->value_ == b->value_; }
    template<class Q = T> static typename std::enable_if<!IsComparable<Q>::value, bool>::
    type eq (const Holder *a, const Holder *b) { return false; }
    T value_;
  };
  template<class Ptr> Ptr
  cast () const
  {
    static_assert (::std::is_pointer<Ptr>::value, "Ptr is Object*");
    typedef typename ::std::remove_pointer<Ptr>::type T;
    if (kind() != LOCAL)
      return NULL;
    const std::type_info &tt = typeid (T);
    const std::type_info &ht = u_.pholder->type_info();
    if (tt != ht)
      return NULL;
    return &static_cast<Holder<T>*> (u_.pholder)->value_;
  }
  ///@endcond
public:
#ifndef DOXYGEN
  typedef AnyField<Any> Field;  // See DOXYGEN section for the "unwrapped" definition.
#else // DOXYGEN
  struct Field : Any    /// Any::Field is an Any with a std::string @a name attached.
  {
    String name;        ///< The @a name of this Any::Field, as used in e.g. #RECORD types.
    AnyField();         ///< Default initialize Any::Field.
    AnyField (const String &name, const Any &any);                   ///< Initialize Any::Field with @a name and an @a any value.
    template<class V> AnyField (const String &name, const V &value); ///< Initialize Any::Field with a @a value convertible to an Any.
  };
#endif // DOXYGEN
  typedef std::vector<Field> FieldVector; ///< Vector of fields (named Any structures) for use in #RECORD types.
  typedef std::vector<Any> AnyVector;     ///< Vector of Any structures for use in #SEQUENCE types.
protected:
  bool  plain_zero_type (TypeKind kind);
  template<class Rec> static void any_from_record (Any &any, const Rec &record);
private:
  TypeKind type_kind_;
  ///@cond
  union {
    uint64 vuint64; int64 vint64; double vdouble; Any *vany; AnyVector *vanys; FieldVector *vfields;
    ImplicitBaseP *ibase; RemoteHandle *rhandle; PlaceHolder *pholder;
    String&       vstring() { return *(String*) this; static_assert (sizeof (String) <= sizeof (*this), "union size"); }
    const String& vstring() const { return *(const String*) this; }
  } u_;
  ///@endcond
  void    hold    (PlaceHolder*);
  void    ensure  (TypeKind _kind) { if (AIDA_LIKELY (kind() == _kind)) return; rekind (_kind); }
  void    rekind  (TypeKind _kind);
  void    reset   ();
  bool    to_int  (int64 &v, char b) const;
public:
  /*dtor*/ ~Any    ();                                  ///< Any destructor.
  explicit  Any    ();                                  ///< Default initialize Any with no type.
  /// Initialize Any from a @a anany which is of Any or derived type.
  template<class V, REQUIRES< ::std::is_base_of< Any, typename std::remove_reference<V>::type >::value > = true> inline
  explicit  Any (V &&anany)     { this->operator= (::std::forward<V> (anany)); }
  /// Initialize Any and set its contents from @a value.
  template<class V, REQUIRES< !::std::is_base_of< Any, typename std::remove_reference<V>::type >::value > = true> inline
  explicit  Any    (V &&value)     { set (::std::forward<V> (value)); }
  /*copy*/  Any    (const Any &clone);                  ///< Copy constructor.
  /*move*/  Any    (Any &&other);                       ///< Move constructor.
  Any& operator=   (const Any &clone);                  ///< Set @a this Any to a copy of @a clone.
  Any& operator=   (Any &&other);                       ///< Move @a other into @a this Any.
  bool operator==  (const Any &clone) const;            ///< Check if Any is exactly equal to @a clone.
  bool operator!=  (const Any &clone) const;            ///< Check if Any is not equal to @a clone, see operator==().
  TypeKind  kind   () const { return type_kind_; }      ///< Obtain the type kind for the contents of this Any.
  void      swap   (Any           &other);              ///< Swap the contents of @a this and @a other in constant time.
  bool operator>>= (bool          &v) const { int64 d; const bool r = to_int (d, 1); v = d; return r; }
  bool operator>>= (char          &v) const { int64 d; const bool r = to_int (d, 7); v = d; return r; }
  bool operator>>= (unsigned char &v) const { int64 d; const bool r = to_int (d, 8); v = d; return r; }
  bool operator>>= (int32         &v) const { int64 d; const bool r = to_int (d, 31); v = d; return r; }
  bool operator>>= (uint32        &v) const { int64 d; const bool r = to_int (d, 32); v = d; return r; }
  bool operator>>= (LongIffy      &v) const { int64 d; const bool r = to_int (d, 47); v = d; return r; }
  bool operator>>= (ULongIffy     &v) const { int64 d; const bool r = to_int (d, 48); v = d; return r; }
  bool operator>>= (int64         &v) const { int64 d; const bool r = to_int (d, 63); v = d; return r; }
  bool operator>>= (uint64        &v) const { int64 d; const bool r = to_int (d, 64); v = d; return r; }
  bool operator>>= (float         &v) const { double d; const bool r = operator>>= (d); v = d; return r; }
  bool operator>>= (double        &v) const; ///< Extract a floating point number as double if possible.
  bool operator>>= (EnumValue          &v) const; ///< Extract the numeric representation of an EnumValue if possible.
  bool operator>>= (const char        *&v) const { String s; const bool r = operator>>= (s); v = s.c_str(); return r; }
  bool operator>>= (std::string        &v) const; ///< Extract a std::string if possible.
  bool operator>>= (const Any         *&v) const; ///< Extract an Any if possible.
  bool operator>>= (const AnyVector   *&v) const; ///< Extract an AnyVector if possible (sequence type).
  bool operator>>= (const FieldVector *&v) const; ///< Extract a FieldVector if possible (record type).
  bool operator>>= (RemoteHandle       &v);
private:
  template<class A, class B> using IsConvertible = ///< Avoid pointer->bool reduction for std::is_convertible<>.
    ::std::integral_constant<bool, ::std::is_convertible<A, B>::value && (!::std::is_pointer<A>::value || !IsBool<B>::value)>;
  template<class T>          using IsCStr                = ::std::is_same<const char*, typename ::std::decay<T>::type>;
  template<class T>          using IsLocalClass          =
    ::std::integral_constant<bool, (::std::is_class<T>::value && !DerivesString<T>::value &&
                                    !IsConvertible<T, ImplicitBase>::value && !IsConvertible<T, RemoteHandle>::value &&
                                    !IsConvertible<T, Any::FieldVector>::value && !IsConvertible<T, Any::AnyVector>::value &&
                                    !IsConvertible<T, Any>::value && !IsConvertible<T, Any::Field>::value)>;
  template<class T>          using IsLocalClassPtr       =
    ::std::integral_constant<bool, ::std::is_pointer<T>::value && IsLocalClass< typename std::remove_pointer<T>::type >::value>;
  template<class T>          using IsRemoteHandleDerived = ::std::integral_constant<bool,
                                                                                    (::std::is_base_of<RemoteHandle, T>::value &&
                                                                                     !::std::is_same<RemoteHandle,
                                                                                     typename ::std::decay<T>::type>::value)>;
  template<class T>          using IsImplicitBaseDerived = ::std::integral_constant<bool,
                                                                                    (::std::is_base_of<ImplicitBase, T>::value &&
                                                                                     !::std::is_same<ImplicitBase,
                                                                                     typename ::std::decay<T>::type>::value)>;
  bool               get_bool    () const;
  void               set_bool    (bool value);
  int64              get_int64   () const;
  void               set_int64   (int64 value);
  double             get_double  () const;
  void               set_double  (double value);
  std::string        get_string  () const;
  void               set_string  (const std::string &value);
  template<typename Enum>
  Enum               get_enum    () const               { return Enum (get_int64()); }
  template<typename Enum>
  void               set_enum    (Enum value)           { return set_int64 (value); }
  const AnyVector*   get_seq     () const;
  void               set_seq     (const AnyVector *seq);
  const FieldVector* get_rec     () const;
  void               set_rec     (const FieldVector *rec);
  ImplicitBase*      get_ibase   () const;
  void               set_ibase   (ImplicitBase *ibase);
  template<typename C>
  C*                 cast_ibase  () const               { return dynamic_cast<C*> (get_ibase()); }
  RemoteHandle       get_handle  () const;
  void               take_handle (RemoteHandle *handle);
  template<typename H>
  H                  cast_handle () const               { return H::down_cast (get_handle()); }
  template<typename Handle>
  void               set_handle  (Handle object)        { return take_handle (new Handle (object)); }
  const Any*         get_any     () const;
  void               set_any     (const Any *value);
public:
  // Type get() const;
  template<typename T, REQUIRES< IsBool<T>::value > = true>                            T    get () const { return get_bool(); }
  template<typename T, REQUIRES< IsInteger<T>::value > = true>                         T    get () const { return get_int64(); }
  template<typename T, REQUIRES< std::is_floating_point<T>::value > = true>            T    get () const { return get_double(); }
  template<typename T, REQUIRES< DerivesString<T>::value > = true>                     T    get () const { return get_string(); }
  template<typename T, REQUIRES< std::is_enum<T>::value > = true>                      T    get () const { return get_enum<T>(); }
  template<typename T, REQUIRES< IsConvertible<const AnyVector*, T>::value > = true>   T    get () const { return get_seq(); }
  template<typename T, REQUIRES< IsConvertible<const AnyVector, T>::value > = true>    T    get () const { return *get_seq(); }
  template<typename T, REQUIRES< IsConvertible<const FieldVector*, T>::value > = true> T    get () const { return get_rec(); }
  template<typename T, REQUIRES< IsConvertible<const FieldVector, T>::value > = true>  T    get () const { return *get_rec(); }
  template<typename T, REQUIRES< IsImplicitBaseDerived<T>::value > = true>             T&   get () const { return *cast_ibase<T>(); }
  template<typename T, REQUIRES< IsRemoteHandleDerived<T>::value > = true>             T    get () const { return cast_handle<T>(); }
  template<typename T, REQUIRES< IsConvertible<const Any, T>::value > = true>          T    get () const { return *get_any(); }
  template<typename T, REQUIRES< IsLocalClassPtr<T>::value > = true>                   T    get () const { return cast<T>(); }
  // void set (const Type&);
  template<typename T, REQUIRES< IsBool<T>::value > = true>                            void set (T v) { return set_bool (v); }
  template<typename T, REQUIRES< IsInteger<T>::value > = true>                         void set (T v) { return set_int64 (v); }
  template<typename T, REQUIRES< std::is_floating_point<T>::value > = true>            void set (T v) { return set_double (v); }
  template<typename T, REQUIRES< DerivesString<T>::value > = true>                     void set (T v) { return set_string (v); }
  template<typename T, REQUIRES< IsCStr<T>::value > = true>                            void set (T v) { return set_string (v); }
  template<typename T, REQUIRES< std::is_enum<T>::value > = true>                      void set (T v) { return set_enum<T> (v); }
  template<typename T, REQUIRES< std::is_same<AnyVector, T>::value > = true>           void set (const T &v) { return set_seq (&v); }
  template<typename T, REQUIRES< std::is_same<AnyVector, T>::value > = true>           void set (const T *v) { return set_seq (v); }
  template<typename T, REQUIRES< std::is_same<FieldVector, T>::value > = true>         void set (const T &v) { return set_rec (&v); }
  template<typename T, REQUIRES< std::is_same<FieldVector, T>::value > = true>         void set (const T *v) { return set_rec (v); }
  template<typename T, REQUIRES< IsImplicitBaseDerived<T>::value > = true>             void set (T &v) { return set_ibase (&v); }
  template<typename T, REQUIRES< IsRemoteHandleDerived<T>::value > = true>             void set (T v) { return set_handle<T> (v); }
  template<typename T, REQUIRES< std::is_base_of<Any, T>::value > = true>              void set (const T &v) { return set_any (&v); }
  template<typename T, REQUIRES< IsLocalClass<T>::value > = true>                      void set (const T &v) { hold (new Holder<T> (v)); }
  // convenience
  static Any                any_from_strings (const std::vector<std::string> &string_container);
  std::vector<std::string>  any_to_strings   () const;
  String     to_string (const String &field_name = "") const; ///< Retrieve string representation of Any for printouts.
  const Any& as_any   () const { return kind() == ANY ? *u_.vany : *this; } ///< Obtain contents as Any.
  double     as_float () const; ///< Obtain BOOL, INT*, or FLOAT* contents as double float.
  int64      as_int   () const; ///< Obtain BOOL, INT* or FLOAT* contents as integer (yields 1 for non-empty strings).
  String     as_string() const; ///< Obtain BOOL, INT*, FLOAT* or STRING contents as string.
  void operator<<= (bool           v);
  void operator<<= (char           v) { operator<<= (int32 (v)); }
  void operator<<= (unsigned char  v) { operator<<= (int32 (v)); }
  void operator<<= (int32          v);
  void operator<<= (uint32         v) { operator<<= (int64 (v)); }
  void operator<<= (LongIffy       v) { operator<<= (CastIffy (v)); }
  void operator<<= (ULongIffy      v) { operator<<= (UCastIffy (v)); }
  void operator<<= (int64          v); ///< Store a 64bit signed integer.
  void operator<<= (uint64         v); ///< Store a 64bit unsigned integer.
  void operator<<= (float          v) { operator<<= (double (v)); }
  void operator<<= (double         v); ///< Store a double floating point number.
  void operator<<= (const EnumValue   &v); ///< Store the numeric representation of an EnumValue.
  void operator<<= (const char        *v) { operator<<= (std::string (v)); }
  void operator<<= (char              *v) { operator<<= (std::string (v)); }
  void operator<<= (const String      &v); ///< Store a std::string.
  void operator<<= (const Any         &v); ///< Store an Any.
  void operator<<= (const AnyVector   &v); ///< Store a sequence of Any structures (sequence type).
  void operator<<= (const FieldVector &v); ///< Store a sequence of Any::Field structures (record type).
  void operator<<= (const RemoteHandle &v);
  template<class T, typename // SFINAE idiom for operator<<= to match unknown classes
           std::enable_if<(std::is_class<T>::value && !std::is_convertible<T, RemoteHandle>::value)>::type* = nullptr>
  void operator<<= (const T &v)         { hold (new Holder<T> (v)); }   ///< Store arbitrary type class in this Any, see any_cast<>().
  template<class T> friend T*
  any_cast (Any *const any)     ///< Cast Any* into @a T* if possible or return NULL.
  {
    return !any ? NULL : any->cast<T*>();
  }
  template<class T> friend const T*
  any_cast (const Any *any)     ///< Cast const Any* into @a const @a T* if possible or return NULL.
  {
    return !any ? NULL : any->cast<T*>();
  }
  template<class T> friend T
  any_cast (Any &any)           ///< Cast Any& into @a T if possible or throw a std::bad_cast exception.
  {
    const T *result = any.cast<typename std::remove_reference<T>::type*>();
    return result ? *result : throw std::bad_cast();
  }
  template<class T> friend T
  any_cast (const Any &any)     ///< Cast const Any& into @a const @a T if possible or throw a std::bad_cast exception.
  {
    const T *result = any.cast<typename std::add_const<typename std::remove_reference<T>::type>::type*>();
    return result ? *result : throw std::bad_cast();
  }
};


// === EventFd ===
class EventFd            /// Wakeup facility for IPC.
{
  int      fds[2];
  void     operator= (const EventFd&) = delete; // no assignments
  explicit EventFd   (const EventFd&) = delete; // no copying
public:
  explicit EventFd   ();
  int      open      (); ///< Opens the eventfd and returns -errno.
  bool     opened    (); ///< Indicates whether eventfd has been opened.
  void     wakeup    (); ///< Wakeup polling end.
  int      inputfd   (); ///< Returns the file descriptor for POLLIN.
  bool     pollin    (); ///< Checks whether events are pending.
  void     flush     (); ///< Clear pending wakeups.
  /*Des*/ ~EventFd   ();
};

// == Utilities ==
void assertion_error (const char *file, uint line, const char *expr) AIDA_NORETURN;
void fatal_error     (const char *file, uint line, const String &msg) AIDA_NORETURN;
void fatal_error     (const String &msg) AIDA_NORETURN;
void print_warning   (const String &msg);

// == Type Utilities ==
template<class Y> struct ValueType           { typedef Y T; };
template<class Y> struct ValueType<Y&>       { typedef Y T; };
template<class Y> struct ValueType<const Y&> { typedef Y T; };

// == Message IDs ==
enum MessageId {
  // none                   = 0x0000000000000000
  MSGID_CALL_ONEWAY         = 0x1000000000000000ULL, ///< One-way method call (void return).
  MSGID_EMIT_ONEWAY         = 0x2000000000000000ULL, ///< One-way signal emissions (void return).
  //MSGID_META_ONEWAY       = 0x3000000000000000ULL, ///< One-way method call (void return).
  MSGID_CONNECT             = 0x4000000000000000ULL, ///< Signal handler (dis-)connection, expects CONNECT_RESULT.
  MSGID_CALL_TWOWAY         = 0x5000000000000000ULL, ///< Two-way method call, expects CALL_RESULT.
  MSGID_EMIT_TWOWAY         = 0x6000000000000000ULL, ///< Two-way signal emissions, expects EMIT_RESULT.
  //MSGID_META_TWOWAY       = 0x7000000000000000ULL, ///< Two-way method call, expects META_REPLY.
  // meta_exception         = 0x8000000000000000
  MSGID_DISCONNECT          = 0xa000000000000000ULL, ///< Signal destroyed, disconnect all handlers.
  MSGID_CONNECT_RESULT      = 0xc000000000000000ULL, ///< Result message for CONNECT.
  MSGID_CALL_RESULT         = 0xd000000000000000ULL, ///< Result message for CALL_TWOWAY.
  MSGID_EMIT_RESULT         = 0xe000000000000000ULL, ///< Result message for EMIT_TWOWAY.
  //MSGID_META_REPLY        = 0xf000000000000000ULL, ///< Result message for MSGID_META_TWOWAY.
  // meta messages and results
  MSGID_META_HELLO          = 0x7100000000000000ULL, ///< Hello from client, expects WELCOME.
  MSGID_META_WELCOME        = 0xf100000000000000ULL, ///< Hello reply from server, contains remote_origin.
  MSGID_META_GARBAGE_SWEEP  = 0x7200000000000000ULL, ///< Garbage collection cycle, expects GARBAGE_REPORT.
  MSGID_META_GARBAGE_REPORT = 0xf200000000000000ULL, ///< Reports expired/retained references.
  MSGID_META_SEEN_GARBAGE   = 0x3300000000000000ULL, ///< Client indicates garbage collection may be useful.
};
/// Check if msgid is a reply for a two-way call (one of the _RESULT or _REPLY message ids).
inline constexpr bool msgid_is_result (MessageId msgid) { return (msgid & 0xc000000000000000ULL) == 0xc000000000000000ULL; }

/// Helper structure to pack MessageId, sender and receiver connection IDs.
union IdentifierParts {
  uint64        vuint64;
  struct { // MessageId bits
# if __BYTE_ORDER == __LITTLE_ENDIAN
    uint        sender_connection : 16, free16 : 16, destination_connection : 16, free8 : 8, message_id : 8;
# elif __BYTE_ORDER == __BIG_ENDIAN
    uint        message_id : 8, free8 : 8, destination_connection : 16, free16 : 16, sender_connection : 16;
# endif
  };
  static_assert (__BYTE_ORDER == __LITTLE_ENDIAN || __BYTE_ORDER == __BIG_ENDIAN, "__BYTE_ORDER unknown");
  constexpr IdentifierParts (uint64 vu64) : vuint64 (vu64) {}
  constexpr IdentifierParts (MessageId id, uint destination, uint sender) :
# if __BYTE_ORDER == __LITTLE_ENDIAN
    sender_connection (sender), free16 (0), destination_connection (destination), free8 (0), message_id (IdentifierParts (id).message_id)
# elif __BYTE_ORDER == __BIG_ENDIAN
    message_id (IdentifierParts (id).message_id), free8 (0), destination_connection (destination), free16 (0), sender_connection (sender)
# endif
  {}
};
constexpr uint64 CONNECTION_MASK = 0x0000ffff;

// == OrbObject ==
/// Internal management structure for remote objects.
class OrbObject {
  const uint64  orbid_;
protected:
  explicit                  OrbObject         (uint64 orbid);
  virtual                  ~OrbObject         ();
public:
  uint64                    orbid             () const       { return orbid_; }
  uint16                    connection        () const       { return orbid_connection (orbid_); }
  uint16                    type_index        () const       { return orbid_type_index (orbid_); }
  uint32                    counter           () const       { return orbid_counter (orbid_); }
  virtual ClientConnection* client_connection ();
  static uint16             orbid_connection  (uint64 orbid) { return orbid >> 48 /* & 0xffff */; }
  static uint16             orbid_type_index  (uint64 orbid) { return orbid >> 32 /* & 0xffff */; }
  static uint32             orbid_counter     (uint64 orbid) { return orbid /* & 0xffffffff */; }
  static uint64             orbid_make        (uint16 connection, uint16 type_index, uint32 counter)
  { return (uint64 (connection) << 48) | (uint64 (type_index) << 32) | counter; }
};

// == RemoteHandle ==
class RemoteHandle {
  OrbObjectP        orbop_;
  template<class Parent>
  struct NullRemoteHandleT : public Parent {
    TypeHashList __aida_typelist__ () const { return TypeHashList(); }
  };
  typedef NullRemoteHandleT<RemoteHandle> NullRemoteHandle;
  static OrbObjectP __aida_null_orb_object__ ();
protected:
  explicit          RemoteHandle             (OrbObjectP);
  explicit          RemoteHandle             () : orbop_ (__aida_null_orb_object__()) {}
  const OrbObjectP& __aida_orb_object__      () const   { return orbop_; }
  void              __aida_upgrade_from__    (const OrbObjectP&);
  void              __aida_upgrade_from__    (const RemoteHandle &rhandle) { __aida_upgrade_from__ (rhandle.__aida_orb_object__()); }
public:
  /*copy*/                RemoteHandle         (const RemoteHandle &y) : orbop_ (y.orbop_) {}
  virtual                ~RemoteHandle         ();
  ClientConnection*       __aida_connection__  () const { return orbop_->client_connection(); }
  uint64                  __aida_orbid__       () const { return orbop_->orbid(); }
  static NullRemoteHandle __aida_null_handle__ ()       { return NullRemoteHandle(); }
  // Determine if this RemoteHandle contains an object or null handle.
  explicit    operator bool () const noexcept               { return 0 != __aida_orbid__(); }
  bool        operator==    (std::nullptr_t) const noexcept { return 0 == __aida_orbid__(); }
  bool        operator!=    (std::nullptr_t) const noexcept { return 0 != __aida_orbid__(); }
  bool        operator==    (const RemoteHandle &rh) const noexcept { return __aida_orbid__() == rh.__aida_orbid__(); }
  bool        operator!=    (const RemoteHandle &rh) const noexcept { return !operator== (rh); }
  friend bool operator==    (std::nullptr_t nullp, const RemoteHandle &shd) noexcept { return shd == nullp; }
  friend bool operator!=    (std::nullptr_t nullp, const RemoteHandle &shd) noexcept { return shd != nullp; }
private:
  template<class TargetHandle> static typename
  std::enable_if<(std::is_base_of<RemoteHandle, TargetHandle>::value &&
                  !std::is_same<RemoteHandle, TargetHandle>::value), TargetHandle>::type
  __aida_reinterpret_down_cast__ (RemoteHandle smh)     ///< Reinterpret & dynamic cast, use discouraged.
  {
    TargetHandle target;
    target.__aida_upgrade_from__ (smh);                 // like reinterpret_cast<>
    return TargetHandle::down_cast (target);            // like dynamic_cast<>
  }
  friend  class ObjectBroker;
};

// == RemoteMember ==
template<class RemoteHandle>
class RemoteMember : public RemoteHandle {
public:
  inline   RemoteMember (const RemoteHandle &src) : RemoteHandle() { *this = src; }
  explicit RemoteMember () : RemoteHandle() {}
  void     operator=   (const RemoteHandle &src) { RemoteHandle::operator= (src); }
};

// == ObjectBroker ==
class ObjectBroker {
  static void   setup_connection_ctor_protocol    (const char *protocol);
  static void   verify_connection_construction    ();
  static void   construct_server_connection       (ServerConnection *&var);
  static void   construct_client_connection       (ClientConnection *&var);
  static uint   connection_id_from_protocol       (const std::string &protocol);
  static void   connection_handshake              (const std::string                 &endpoint,
                                                   std::function<BaseConnection*()>   aida_connection,
                                                   std::function<void (RemoteHandle)> origin_cast);
public:
  static uint              register_connection    (BaseConnection    &connection);
  static void              unregister_connection  (BaseConnection    &connection);
  static BaseConnection*   connection_from_id     (uint64             connection_id);
  static ClientConnection* get_client_connection  (ClientConnection *&var);
  static ServerConnection* get_server_connection  (ServerConnection *&var);
  static uint         connection_id_from_signal_handler_id (size_t signal_handler_id);
  static inline uint  connection_id_from_orbid  (uint64 orbid)        { return OrbObject::orbid_connection (orbid); }
  static inline uint  connection_id_from_handle (const RemoteHandle &sh) { return connection_id_from_orbid (sh.__aida_orbid__()); }
  static inline uint  destination_connection_id (uint64 msgid)        { return IdentifierParts (msgid).destination_connection; }
  static inline uint  sender_connection_id      (uint64 msgid)        { return IdentifierParts (msgid).sender_connection; }
  template<class C> static void bind            (const std::string &protocol, std::shared_ptr<C> object_ptr);
  template<class H> static H    connect         (const std::string &endpoint);
};

// == ProtoMsg ==
union ProtoUnion {
  int64        vint64;
  double       vdouble;
  Any         *vany;
  uint64       smem[(sizeof (std::string) + 7) / 8];    // String memory
  void        *pmem[2];                                 // equate sizeof (ProtoMsg)
  uint8        bytes[8];                                // ProtoMsg types
  struct { uint32 index, capacity; };                   // ProtoMsg.buffermem[0]
};

class ProtoMsg { // buffer for marshalling procedure calls
  friend class ProtoReader;
  void               check_internal ();
  inline ProtoUnion& upeek (uint32 n) const { return buffermem[offset() + n]; }
protected:
  ProtoUnion        *buffermem;
  inline void        check ()      { if (AIDA_UNLIKELY (size() > capacity())) check_internal(); }
  inline uint32      offset () const { const uint32 offs = 1 + (capacity() + 7) / 8; return offs; }
  inline TypeKind    type_at  (uint32 n) const { return TypeKind (buffermem[1 + n/8].bytes[n%8]); }
  inline void        set_type (TypeKind ft)  { buffermem[1 + size()/8].bytes[size()%8] = ft; }
  inline ProtoUnion& getu () const           { return buffermem[offset() + size()]; }
  inline ProtoUnion& addu (TypeKind ft) { set_type (ft); ProtoUnion &u = getu(); buffermem[0].index++; check(); return u; }
  inline ProtoUnion& uat (uint32 n) const { return AIDA_LIKELY (n < size()) ? upeek (n) : *(ProtoUnion*) NULL; }
  explicit           ProtoMsg (uint32 _ntypes);
  explicit           ProtoMsg (uint32, ProtoUnion*, uint32);
public:
  virtual      ~ProtoMsg ();
  inline uint32 size     () const          { return buffermem[0].index; }
  inline uint32 capacity () const          { return buffermem[0].capacity; }
  inline uint64 first_id () const          { return AIDA_LIKELY (buffermem && size() && type_at (0) == INT64) ? upeek (0).vint64 : 0; }
  inline void add_bool   (bool    vbool)   { ProtoUnion &u = addu (BOOL); u.vint64 = vbool; }
  inline void add_int64  (int64 vint64)    { ProtoUnion &u = addu (INT64); u.vint64 = vint64; }
  inline void add_evalue (int64 vint64)    { ProtoUnion &u = addu (ENUM); u.vint64 = vint64; }
  inline void add_double (double vdouble)  { ProtoUnion &u = addu (FLOAT64); u.vdouble = vdouble; }
  inline void add_string (const String &s) { ProtoUnion &u = addu (STRING); new (&u) String (s); }
  inline void add_orbid  (uint64 objid)    { ProtoUnion &u = addu (INSTANCE); u.vint64 = objid; }
  inline void add_any    (const Any &vany, BaseConnection &bcon);
  inline void add_header1 (MessageId m, uint64 h, uint64 l) { add_int64 (IdentifierParts (m).vuint64); add_int64 (h); add_int64 (l); }
  inline void add_header2 (MessageId m, uint64 h, uint64 l) { add_int64 (IdentifierParts (m).vuint64); add_int64 (h); add_int64 (l); }
  inline ProtoMsg& add_rec      (uint32 nt) { ProtoUnion &u = addu (RECORD); return *new (&u) ProtoMsg (nt); }
  inline ProtoMsg& add_seq      (uint32 nt) { ProtoUnion &u = addu (SEQUENCE); return *new (&u) ProtoMsg (nt); }
  inline void      reset        ();
  String           first_id_str () const;
  String           to_string    () const;
  static String    type_name    (int field_type);
  static ProtoMsg* _new         (uint32 _ntypes); // Heap allocated ProtoMsg
  // static ProtoMsg* new_error (const String &msg, const String &domain = "");
  static ProtoMsg* new_result        (MessageId m, uint64 h, uint64 l, uint32 n = 1);
  static ProtoMsg* renew_into_result (ProtoMsg *fb, MessageId m, uint64 h, uint64 l, uint32 n = 1);
  static ProtoMsg* renew_into_result (ProtoReader &fbr, MessageId m, uint64 h, uint64 l, uint32 n = 1);
  inline void operator<<= (uint32 v)          { ProtoUnion &u = addu (INT64); u.vint64 = v; }
  inline void operator<<= (ULongIffy v)       { ProtoUnion &u = addu (INT64); u.vint64 = v; }
  inline void operator<<= (uint64 v)          { ProtoUnion &u = addu (INT64); u.vint64 = v; }
  inline void operator<<= (int32 v)           { ProtoUnion &u = addu (INT64); u.vint64 = v; }
  inline void operator<<= (LongIffy v)        { ProtoUnion &u = addu (INT64); u.vint64 = v; }
  inline void operator<<= (int64 v)           { ProtoUnion &u = addu (INT64); u.vint64 = v; }
  inline void operator<<= (bool   v)          { ProtoUnion &u = addu (BOOL); u.vint64 = v; }
  inline void operator<<= (double v)          { ProtoUnion &u = addu (FLOAT64); u.vdouble = v; }
  inline void operator<<= (EnumValue e)       { ProtoUnion &u = addu (ENUM); u.vint64 = e.value; }
  inline void operator<<= (const String &s)   { ProtoUnion &u = addu (STRING); new (&u) String (s); }
  inline void operator<<= (const TypeHash &h) { *this <<= h.typehi; *this <<= h.typelo; }
  void        operator<<= (const RemoteHandle &rhandle);
  void        operator<<= (ImplicitBase *instance);
};

class ProtoMsg8 : public ProtoMsg { // Stack contained buffer for up to 8 fields
  ProtoUnion bmem[1 + 1 + 8];
public:
  virtual ~ProtoMsg8 () { reset(); buffermem = NULL; }
  inline   ProtoMsg8 (uint32 ntypes = 8) : ProtoMsg (ntypes, bmem, sizeof (bmem)) { AIDA_ASSERT (ntypes <= 8); }
};

class ProtoReader { // read ProtoMsg contents
  const ProtoMsg    *fb_;
  uint32             nth_;
  void               check_request (int type);
  ImplicitBaseP      pop_interface ();
  inline void        request (int t) { if (AIDA_UNLIKELY (nth_ >= n_types() || get_type() != t)) check_request (t); }
  inline ProtoUnion& fb_getu (int t) { request (t); return fb_->upeek (nth_); }
  inline ProtoUnion& fb_popu (int t) { request (t); ProtoUnion &u = fb_->upeek (nth_++); return u; }
public:
  explicit               ProtoReader (const ProtoMsg &fb) : fb_ (&fb), nth_ (0) {}
  uint64                 debug_bits  ();
  inline const ProtoMsg* proto_msg   () const { return fb_; }
  inline void            reset       (const ProtoMsg &fb) { fb_ = &fb; nth_ = 0; }
  inline void            reset       () { fb_ = NULL; nth_ = 0; }
  inline uint32          remaining   () { return n_types() - nth_; }
  inline void            skip        () { if (AIDA_UNLIKELY (nth_ >= n_types())) check_request (0); nth_++; }
  inline void            skip_header () { skip(); skip(); skip(); }
  inline uint32          n_types     () { return fb_->size(); }
  inline TypeKind        get_type    () { return fb_->type_at (nth_); }
  inline int64           get_bool    () { ProtoUnion &u = fb_getu (BOOL); return u.vint64; }
  inline int64           get_int64   () { ProtoUnion &u = fb_getu (INT64); return u.vint64; }
  inline int64           get_evalue  () { ProtoUnion &u = fb_getu (ENUM); return u.vint64; }
  inline double          get_double  () { ProtoUnion &u = fb_getu (FLOAT64); return u.vdouble; }
  inline const String&   get_string  () { ProtoUnion &u = fb_getu (STRING); return *(String*) &u; }
  inline const ProtoMsg& get_rec     () { ProtoUnion &u = fb_getu (RECORD); return *(ProtoMsg*) &u; }
  inline const ProtoMsg& get_seq     () { ProtoUnion &u = fb_getu (SEQUENCE); return *(ProtoMsg*) &u; }
  inline int64           pop_bool    () { ProtoUnion &u = fb_popu (BOOL); return u.vint64; }
  inline int64           pop_int64   () { ProtoUnion &u = fb_popu (INT64); return u.vint64; }
  inline int64           pop_evalue  () { ProtoUnion &u = fb_popu (ENUM); return u.vint64; }
  inline double          pop_double  () { ProtoUnion &u = fb_popu (FLOAT64); return u.vdouble; }
  inline const String&   pop_string  () { ProtoUnion &u = fb_popu (STRING); return *(String*) &u; }
  inline uint64          pop_orbid   () { ProtoUnion &u = fb_popu (INSTANCE); return u.vint64; }
  inline const Any&      pop_any     (BaseConnection &bcon);
  inline const ProtoMsg& pop_rec     () { ProtoUnion &u = fb_popu (RECORD); return *(ProtoMsg*) &u; }
  inline const ProtoMsg& pop_seq     () { ProtoUnion &u = fb_popu (SEQUENCE); return *(ProtoMsg*) &u; }
  inline void operator>>= (uint32 &v)          { ProtoUnion &u = fb_popu (INT64); v = u.vint64; }
  inline void operator>>= (ULongIffy &v)       { ProtoUnion &u = fb_popu (INT64); v = u.vint64; }
  inline void operator>>= (uint64 &v)          { ProtoUnion &u = fb_popu (INT64); v = u.vint64; }
  inline void operator>>= (int32 &v)           { ProtoUnion &u = fb_popu (INT64); v = u.vint64; }
  inline void operator>>= (LongIffy &v)        { ProtoUnion &u = fb_popu (INT64); v = u.vint64; }
  inline void operator>>= (int64 &v)           { ProtoUnion &u = fb_popu (INT64); v = u.vint64; }
  inline void operator>>= (bool &v)            { ProtoUnion &u = fb_popu (BOOL); v = u.vint64; }
  inline void operator>>= (double &v)          { ProtoUnion &u = fb_popu (FLOAT64); v = u.vdouble; }
  inline void operator>>= (EnumValue &e)       { ProtoUnion &u = fb_popu (ENUM); e.value = u.vint64; }
  inline void operator>>= (String &s)          { ProtoUnion &u = fb_popu (STRING); s = *(String*) &u; }
  inline void operator>>= (TypeHash &h)        { *this >>= h.typehi; *this >>= h.typelo; }
  inline void operator>>= (std::vector<bool>::reference v) { bool b; *this >>= b; v = b; }
  void        operator>>= (RemoteHandle &rhandle);
  template<class Target> std::shared_ptr<Target> pop_instance () { return std::dynamic_pointer_cast<Target> (pop_interface()); }
};

// == Connections ==
/// Base connection context for ORB message exchange.
class BaseConnection {
  const std::string protocol_;
  uint              conid_;
  BaseConnection   *peer_;
  friend  class ObjectBroker;
  RAPICORN_CLASS_NON_COPYABLE (BaseConnection);
protected:
  explicit               BaseConnection  (const std::string &protocol);
  virtual               ~BaseConnection  ();
  virtual void           remote_origin   (ImplicitBaseP rorigin) = 0;
  virtual void           receive_msg     (ProtoMsg*) = 0; ///< Accepts an incoming message, transfers memory.
  void                   post_peer_msg   (ProtoMsg*);     ///< Send message to peer, transfers memory.
  void                   assign_id       (uint connection_id);
  std::string            protocol        () const       { return protocol_; }
  void                   peer_connection (BaseConnection &peer);
public:
  BaseConnection&        peer_connection () const;
  uint                   connection_id   () const { return conid_; } ///< Get unique conneciton ID (returns 0 if unregistered).
  virtual int            notify_fd       () = 0; ///< Returns fd for POLLIN, to wake up on incomming events.
  virtual bool           pending         () = 0; ///< Indicate whether any incoming events are pending that need to be dispatched.
  virtual void           dispatch        () = 0; ///< Dispatch a single event if any is pending.
  virtual RemoteHandle   remote_origin   () = 0;
  virtual Any*           any2remote      (const Any&);
  virtual void           any2local       (Any&);
};

/// Function typoe for internal signal handling.
typedef ProtoMsg* SignalEmitHandler (const ProtoMsg*, void*);

/// Connection context for IPC servers. @nosubgrouping
class ServerConnection : public BaseConnection {
  RAPICORN_CLASS_NON_COPYABLE (ServerConnection);
protected:
  /*ctor*/                  ServerConnection      (const std::string &protocol);
  virtual                  ~ServerConnection      ();
  virtual void              cast_interface_handle (RemoteHandle &rhandle, ImplicitBaseP ibase) = 0;
public:
  typedef std::function<void (Rapicorn::Aida::ProtoReader&)> EmitResultHandler;
  void                      post_peer_msg           (ProtoMsg *pm)      { BaseConnection::post_peer_msg (pm); }
  virtual void              emit_result_handler_add (size_t id, const EmitResultHandler &handler) = 0;
  virtual void              add_interface           (ProtoMsg &fb, ImplicitBaseP ibase) = 0;
  virtual ImplicitBaseP     pop_interface           (ProtoReader &fr) = 0;
protected: /// @name Registry for IPC method lookups
  static DispatchFunc find_method (uint64 hi, uint64 lo); ///< Lookup method in registry.
public:
  struct MethodEntry       { uint64 hashhi, hashlo; DispatchFunc dispatcher; };
  struct MethodRegistry    /// Registry structure for IPC method stubs.
  {
    template<size_t S> MethodRegistry  (const MethodEntry (&static_const_entries)[S])
    { for (size_t i = 0; i < S; i++) register_method (static_const_entries[i]); }
  private: static void register_method  (const MethodEntry &mentry);
  };
};

/// Connection context for IPC clients. @nosubgrouping
class ClientConnection : public BaseConnection {
  RAPICORN_CLASS_NON_COPYABLE (ClientConnection);
protected:
  explicit              ClientConnection (const std::string &protocol);
  virtual              ~ClientConnection ();
public: /// @name API for remote calls.
  virtual ProtoMsg*         call_remote       (ProtoMsg*) = 0; ///< Carry out a remote call syncronously, transfers memory.
  virtual void              add_handle        (ProtoMsg &fb, const RemoteHandle &rhandle) = 0;
  virtual void              pop_handle        (ProtoReader &fr, RemoteHandle &rhandle) = 0;
public: /// @name API for signal event handlers.
  virtual size_t        signal_connect    (uint64 hhi, uint64 hlo, const RemoteHandle &rhandle, SignalEmitHandler seh, void *data) = 0;
  virtual bool          signal_disconnect (size_t signal_handler_id) = 0;
};

// == ProtoScpope ==
/// ProtoScpope keeps track of the ServerConnection and ClientConnection during RPC marshalling.
class ProtoScope {
  bool nested_;
public:
  /// Start/create an RPC scope for a connection pair within the current thread.
  explicit                 ProtoScope                (ClientConnection &client_connection);
  explicit                 ProtoScope                (ServerConnection &server_connection);
  /*dtor*/                ~ProtoScope                (); ///< Finish/destroy an RPC scope.
  ProtoMsg*                invoke                    (ProtoMsg *pm); ///< Carry out a remote call syncronously, transfers memory.
  static ClientConnection& current_client_connection (); ///< Access the client connection of the current thread-specific RPC scope.
  static ServerConnection& current_server_connection (); ///< Access the server connection of the current thread-specific RPC scope.
  RAPICORN_CLASS_NON_COPYABLE (ProtoScope);
};
struct ProtoScopeCall1Way : ProtoScope {
  ProtoScopeCall1Way (ProtoMsg &pm, const RemoteHandle &rhandle, uint64 hashi, uint64 hashlo);
};
struct ProtoScopeCall2Way : ProtoScope {
  ProtoScopeCall2Way (ProtoMsg &pm, const RemoteHandle &rhandle, uint64 hashi, uint64 hashlo);
};
struct ProtoScopeEmit1Way : ProtoScope {
  ProtoScopeEmit1Way (ProtoMsg &pm, ServerConnection &server_connection, uint64 hashi, uint64 hashlo);
};
struct ProtoScopeEmit2Way : ProtoScope {
  ProtoScopeEmit2Way (ProtoMsg &pm, ServerConnection &server_connection, uint64 hashi, uint64 hashlo);
};

// == inline implementations ==
inline
Any::Any() :
  type_kind_ (UNTYPED), u_ {0}
{}

inline
Any::~Any ()
{
  reset();
}

inline bool
Any::plain_zero_type (TypeKind kind)
{
  switch (kind)
    {
    case UNTYPED: case BOOL: case INT32: case INT64: case FLOAT64: case ENUM:
      return true;      // simple, properly initialized with u {0}
    case STRING: case ANY: case SEQUENCE: case RECORD: case INSTANCE: case REMOTE:
    default:
      return false;     // complex types, needing special initializations
    }
}

inline void
ProtoMsg::reset()
{
  if (!buffermem)
    return;
  while (size() > 0)
    {
      buffermem[0].index--; // causes size()--
      switch (type_at (size()))
        {
        case STRING:    { ProtoUnion &u = getu(); ((String*) &u)->~String(); }; break;
        case ANY:       { ProtoUnion &u = getu(); delete u.vany; }; break;
        case SEQUENCE:
        case RECORD:    { ProtoUnion &u = getu(); ((ProtoMsg*) &u)->~ProtoMsg(); }; break;
        default: ;
        }
    }
}

inline void
ProtoMsg::add_any (const Any &vany, BaseConnection &bcon)
{
  ProtoUnion &u = addu (ANY);
  u.vany = bcon.any2remote (vany);
}

inline const Any&
ProtoReader::pop_any (BaseConnection &bcon)
{
  ProtoUnion &u = fb_popu (ANY);
  bcon.any2local (*u.vany);
  return *u.vany;
}

inline ServerConnection*
ObjectBroker::get_server_connection (ServerConnection *&var)
{
  if (AIDA_UNLIKELY (var == NULL))
    construct_server_connection (var);
  return var;
}

inline ClientConnection*
ObjectBroker::get_client_connection (ClientConnection *&var)
{
  if (AIDA_UNLIKELY (var == NULL))
    construct_client_connection (var);
  return var;
}

/// Initialize the ServerConnection of @a C and accept connections via @a protocol
template<class C> void
ObjectBroker::bind (const std::string &protocol, std::shared_ptr<C> object_ptr)
{
  AIDA_ASSERT (object_ptr != NULL);
  setup_connection_ctor_protocol (protocol.c_str());
  BaseConnection *new_connection = C::__aida_connection__();
  verify_connection_construction();
  ServerConnection *server_connection = dynamic_cast<ServerConnection*> (new_connection);
  AIDA_ASSERT (server_connection != NULL);
  server_connection->remote_origin (object_ptr);
}

/// Initialize the ClientConnection of @a H and accept connections via @a protocol, assigns errno.
template<class H> H
ObjectBroker::connect (const std::string &endpoint)
{
  H remote_handle;
  auto origin_cast = [&remote_handle] (RemoteHandle rh) {
    remote_handle = RemoteHandle::__aida_reinterpret_down_cast__<H> (rh);
  };
  connection_handshake (endpoint, H::__aida_connection__, origin_cast);
  return remote_handle;
}

} } // Rapicorn::Aida

// == Signals ==
#include "aidasignal.hh"

#endif // __RAPICORN_AIDA_HH__
