/* Birnet Programming Utilities
 * Copyright (C) 2003,2005 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __BIRNET_UTILS_HH__
#define __BIRNET_UTILS_HH__

/* pull in common system headers */
#include <assert.h>
#include <string>
#include <vector>
#include <map>

namespace Birnet {

/* --- compile time assertions --- */
#define BIRNET_CPP_PASTE2(a,b)                  a ## b
#define BIRNET_CPP_PASTE(a,b)                   BIRNET_CPP_PASTE2 (a, b)
#define BIRNET_STATIC_ASSERT_NAMED(expr,asname) typedef struct { char asname[(expr) ? 1 : -1]; } BIRNET_CPP_PASTE (StaticAssertion_LINE, __LINE__)
#define BIRNET_STATIC_ASSERT(expr)              BIRNET_STATIC_ASSERT_NAMED (expr, compile_time_assertion_failed)


/* --- common type shorthands --- */
typedef unsigned int            uint;
typedef unsigned char           uint8;
typedef unsigned short          uint16;
typedef unsigned int            uint32;
typedef unsigned long long      uint64;
typedef signed char             int8;
typedef signed short            int16;
typedef signed int              int32;
typedef signed long long        int64;
BIRNET_STATIC_ASSERT (sizeof (int8) == 1);
BIRNET_STATIC_ASSERT (sizeof (int16) == 2);
BIRNET_STATIC_ASSERT (sizeof (int32) == 4);
BIRNET_STATIC_ASSERT (sizeof (int64) == 8);
typedef uint32                  unichar;

/* --- type alias frequently used standard lib types --- */
typedef std::string String;
using std::vector;
using std::map;

/* --- GCC macros --- */
#if     __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
#define BIRNET_PRETTY_FUNCTION                  (__PRETTY_FUNCTION__)
#define BIRNET_PURE                             __attribute__((__pure__))
#define BIRNET_MALLOC                           __attribute__((__malloc__))
#define BIRNET_PRINTF(format_idx, arg_idx)      __attribute__((__format__ (__printf__, format_idx, arg_idx)))
#define BIRNET_SCANF(format_idx, arg_idx)       __attribute__((__format__ (__scanf__, format_idx, arg_idx)))
#define BIRNET_FORMAT(arg_idx)                  __attribute__((__format_arg__ (arg_idx)))
#define BIRNET_NORETURN                         __attribute__((__noreturn__))
#define BIRNET_CONST                            __attribute__((__const__))
#define BIRNET_UNUSED                           __attribute__((__unused__))
#define BIRNET_NO_INSTRUMENT                    __attribute__((__no_instrument_function__))
#define BIRNET_EXTENSION                        __extension__
#define BIRNET_DEPRECATED                       __attribute__((__deprecated__))
#else   /* !__GNUC__ */
#define BIRNET_PRETTY_FUNCTION                  (__func__)
#define BIRNET_PURE
#define BIRNET_MALLOC
#define BIRNET_PRINTF(format_idx, arg_idx)
#define BIRNET_SCANF(format_idx, arg_idx)
#define BIRNET_FORMAT(arg_idx)
#define BIRNET_NORETURN
#define BIRNET_CONST
#define BIRNET_UNUSED
#define BIRNET_NO_INSTRUMENT
#define BIRNET_EXTENSION
#define BIRNET_DEPRECATED
#endif  /* !__GNUC__ */

#if __GNUC__ >= 3 && defined __OPTIMIZE__
#define BIRNET_BOOL_EXPR(expr)  __extension__ ({ bool b = 0; if (expr) b = 1; b; })
#define BIRNET_EXPECT(expr,val) __builtin_expect (BIRNET_BOOL_EXPR (expr), val)
#define BIRNET_LIKELY(expr)     __builtin_expect (BIRNET_BOOL_EXPR (expr), 1)
#define BIRNET_UNLIKELY(expr)   __builtin_expect (BIRNET_BOOL_EXPR (expr), 0)
#else
#define BIRNET_EXPECT(expr,val) expr
#define BIRNET_LIKELY(expr)     expr
#define BIRNET_UNLIKELY(expr)   expr
#endif

/* --- helper macros --- */
#define BIRNET_PRIVATE_CLASS_COPY(Class)       private: Class (const Class&); Class& operator= (const Class&);

/* --- common utilities --- */
using ::std::swap;
using ::std::min;
using ::std::max;
#undef abs
template<typename T> inline const T&
abs (const T &value)
{
  return max (value, -value);
}
#undef clamp
template<typename T> inline const T&
clamp (const T &value, const T &minimum, const T &maximum)
{
  if (minimum > value)
    return minimum;
  if (maximum < value)
    return maximum;
  return value;
}

/* --- string functionality --- */
String  string_printf           (const char *format, ...) BIRNET_PRINTF (1, 2);
String  string_vprintf          (const char *format, va_list vargs);
bool    string_to_bool          (const String &string);
String  string_from_bool        (bool value);
uint64  string_to_uint          (const String &string);
String  string_from_uint        (uint64 value);
bool    string_has_int          (const String &string);
int64   string_to_int           (const String &string);
String  string_from_int         (int64 value);
double  string_to_float         (const String &string);
String  string_from_float       (float value);
String  string_from_float       (double value);
template<typename Type> Type    string_to_type           (const String &string);
template<typename Type> String  string_from_type         (Type          value);
template<> inline double        string_to_type<double>   (const String &string) { return string_to_float (string); }
template<> inline String        string_from_type<double> (double         value) { return string_from_float (value); }
template<> inline float         string_to_type<float>    (const String &string) { return string_to_float (string); }
template<> inline String        string_from_type<float>  (float         value)  { return string_from_float (value); }
template<> inline bool          string_to_type<bool>     (const String &string) { return string_to_bool (string); }
template<> inline String        string_from_type<bool>   (bool         value)   { return string_from_bool (value); }
template<> inline int16         string_to_type<int16>    (const String &string) { return string_to_int (string); }
template<> inline String        string_from_type<int16>  (int16         value)  { return string_from_int (value); }
template<> inline uint16        string_to_type<uint16>   (const String &string) { return string_to_uint (string); }
template<> inline String        string_from_type<uint16> (uint16        value)  { return string_from_uint (value); }
template<> inline int           string_to_type<int>      (const String &string) { return string_to_int (string); }
template<> inline String        string_from_type<int>    (int         value)    { return string_from_int (value); }
template<> inline uint          string_to_type<uint>     (const String &string) { return string_to_uint (string); }
template<> inline String        string_from_type<uint>   (uint           value) { return string_from_uint (value); }
template<> inline int64         string_to_type<int64>    (const String &string) { return string_to_int (string); }
template<> inline String        string_from_type<int64>  (int64         value)  { return string_from_int (value); }
template<> inline uint64        string_to_type<uint64>   (const String &string) { return string_to_uint (string); }
template<> inline String        string_from_type<uint64> (uint64         value) { return string_from_uint (value); }

/* --- assertions, warnings, errors --- */
void    error                           (const char *format, ...) BIRNET_PRINTF (1, 2);
void    error                           (const String &s);
void    warning                         (const char *format, ...) BIRNET_PRINTF (1, 2);
void    warning                         (const String &s);
void    diag                            (const char *format, ...) BIRNET_PRINTF (1, 2);
void    diag                            (const String &s);
void    warning_expr_failed             (const char *file, uint line, const char *function, const char *expression);
void    error_expr_failed               (const char *file, uint line, const char *function, const char *expression);
void    warning_expr_reached            (const char *file, uint line, const char *function);
#define BIRNET_RETURN_IF_FAIL(e)        do { if (LIKELY (e)) {} else { warning_expr_failed (__FILE__, __LINE__, __func__, #e); return; } } while (0)
#define BIRNET_RETURN_VAL_IF_FAIL(e,v)  do { if (LIKELY (e)) {} else { warning_expr_failed (__FILE__, __LINE__, __func__, #e); return v; } } while (0)
#define BIRNET_ASSERT(e)                do { if (LIKELY (e)) {} else error_expr_failed (__FILE__, __LINE__, __func__, #e); } while (0)
#define BIRNET_ASSERT_NOT_REACHED()     do { warning_expr_reached (__FILE__, __LINE__, __func__); abort(); } while (0)
#define BIRNET_RETURN_IF_REACHED()      do { warning_expr_reached (__FILE__, __LINE__, __func__); return; } while (0)
#define BIRNET_RETURN_VAL_IF_REACHED(v) do { warning_expr_reached (__FILE__, __LINE__, __func__); return v; } while (0)
void    raise_sigtrap           ();
#if (defined __i386__ || defined __x86_64__) && defined __GNUC__ && __GNUC__ >= 2
extern inline void BREAKPOINT()            { __asm__ __volatile__ ("int $03"); }
#elif defined __alpha__ && !defined __osf__ && defined __GNUC__ && __GNUC__ >= 2
extern inline void BREAKPOINT()            { __asm__ __volatile__ ("bpt"); }
#else   /* !__i386__ && !__alpha__ */
extern inline void BREAKPOINT()            { raise_sigtrap(); }
#endif  /* __i386__ */

/* --- exceptions --- */
struct Exception : std::exception {
public:
  // BROKEN(g++-3.3,g++-3.4): Exception  (const char *reason_format, ...) BIRNET_PRINTF (2, 3);
  explicit            Exception  (const String &s1, const String &s2 = String(), const String &s3 = String(), const String &s4 = String());
  void                set        (const String &s);
  virtual const char* what       () const throw() { return reason ? reason : "out of memory"; }
  /*Copy*/            Exception  (const Exception &e);
  /*Des*/             ~Exception () throw();
private:
  Exception&          operator=  (const Exception&);
  char *reason;
};
struct NullInterface : std::exception {};
struct NullPointer : std::exception {};
/* allow 'dothrow' as funciton argument analogous to 'nothrow' */
extern const std::nothrow_t dothrow; /* indicate "with exception" semantics */
using std::nothrow_t;
using std::nothrow;

/* --- exception assertions --- */
template<typename Pointer> inline void
throw_if_null (Pointer data)
{
  if (data)
    ;
  else
    throw NullPointer();
}

/* --- derivation checks --- */
template<class Derived, class Base>     // ex: EnforceDerivedFrom<Child, Base> assertion;
struct EnforceDerivedFrom {
  EnforceDerivedFrom (Derived *derived = 0,
                      Base    *base = 0)
  {
    base = derived;
  }
};
template<class Derived, class Base>     // ex: EnforceDerivedFrom<Child*, Base*> assertion;
struct EnforceDerivedFrom<Derived*, Base*> {
  EnforceDerivedFrom (Derived *derived = 0,
                      Base    *base = 0)
  {
    base = derived;
  }
};
template<class Derived, class Base> void        // ex: assert_derived_from<Child, Base>();
assert_derived_from (void)
{
  EnforceDerivedFrom<Derived, Base> assertion;
}

/* --- Deletable --- */
class Deletable {
protected:
  virtual ~Deletable() {}
};

/* --- type dereferencing --- */
template<typename Type> struct Dereference;
template<typename Type> struct Dereference<Type*> {
  typedef Type* Pointer;
  typedef Type  Value;
};
template<typename Type> struct Dereference<Type*const> {
  typedef Type* Pointer;
  typedef Type  Value;
};
template<typename Type> struct Dereference<const Type*> {
  typedef const Type* Pointer;
  typedef const Type  Value;
};
template<typename Type> struct Dereference<const Type*const> {
  typedef const Type* Pointer;
  typedef const Type  Value;
};

/* --- PointerIterator - iterator object from pointer --- */
template<typename Value>
class PointerIterator {
protected:
  Value *current;
public:
  typedef std::random_access_iterator_tag       iterator_category;
  typedef Value                                 value_type;
  typedef ptrdiff_t                             difference_type;
  typedef Value*                                pointer;
  typedef Value&                                reference;
public:
  explicit          PointerIterator ()                          : current() {}
  explicit          PointerIterator (value_type *v)             : current (v) {}
  /*Copy*/          PointerIterator (const PointerIterator &x)  : current (x.current) {}
  Value*            base()                              const   { return current; }
  reference         operator*       ()                  const   { return *current; }
  pointer           operator->      ()                  const   { return &(operator*()); }
  PointerIterator&  operator++      ()                          { ++current; return *this; }
  PointerIterator   operator++      (int)                       { PointerIterator tmp = *this; ++current; return tmp; }
  PointerIterator&  operator--      ()                          { --current; return *this; }
  PointerIterator   operator--      (int)                       { PointerIterator tmp = *this; --current; return tmp; }
  PointerIterator   operator+       (difference_type n) const   { return PointerIterator (current + n); }
  PointerIterator&  operator+=      (difference_type n)         { current += n; return *this; }
  PointerIterator   operator-       (difference_type n) const   { return PointerIterator (current - n); }
  PointerIterator&  operator-=      (difference_type n)         { current -= n; return *this; }
  reference         operator[]      (difference_type n) const   { return *(*this + n); }
};
template<typename Value> PointerIterator<Value>
pointer_iterator (Value * const val)
{ return PointerIterator<Value> (val); }
template<typename Value> inline bool
operator== (const PointerIterator<Value> &x, const PointerIterator<Value> &y)
{ return x.base() == y.base(); }
template<typename Value> inline bool
operator!= (const PointerIterator<Value> &x, const PointerIterator<Value> &y)
{ return x.base() != y.base(); }
template<typename Value> inline bool
operator< (const PointerIterator<Value> &x, const PointerIterator<Value> &y)
{ return x.base() < y.base(); }
template<typename Value> inline bool
operator<= (const PointerIterator<Value> &x, const PointerIterator<Value> &y)
{ return x.base() <= y.base(); }
template<typename Value> inline bool
operator> (const PointerIterator<Value> &x, const PointerIterator<Value> &y)
{ return x.base() > y.base(); }
template<typename Value> inline bool
operator>= (const PointerIterator<Value> &x, const PointerIterator<Value> &y)
{ return x.base() >= y.base(); }
template<typename Value> inline typename PointerIterator<Value>::difference_type
operator- (const PointerIterator<Value> &x, const PointerIterator<Value> &y)
{ return x.base() - y.base(); }
template<typename Value> inline PointerIterator<Value>
operator+ (typename PointerIterator<Value>::difference_type n, const PointerIterator<Value> &x)
{ return x.operator+ (n); }

/* --- ValueIterator - dereferencing iterator --- */
template<class Iterator> struct ValueIterator : public Iterator {
  typedef Iterator                              iterator_type;
  typedef typename Iterator::iterator_category  iterator_category;
  typedef typename Iterator::difference_type    difference_type;
  typedef typename Iterator::value_type         pointer;
  typedef typename Dereference<pointer>::Value  value_type;
  typedef value_type&                           reference;
  Iterator       base          () const { return *this; }
  reference      operator*     () const { return *Iterator::operator*(); }
  pointer        operator->    () const { return *Iterator::operator->(); }
  ValueIterator& operator=     (const Iterator &it) { Iterator::operator= (it); return *this; }
  explicit       ValueIterator (Iterator it) : Iterator (it) {}
  /*Copy*/       ValueIterator (const ValueIterator &dup) : Iterator (dup.base()) {}
  /*Con*/        ValueIterator () : Iterator() {}
  template<typename Iter>
  /*Con*/        ValueIterator (const ValueIterator<Iter> &src) : Iterator (src.base()) {}
};
template<class Iterator> ValueIterator<Iterator>
value_iterator (const Iterator &iter)
{ return ValueIterator<Iterator> (iter); }
template<class Iterator> inline bool
operator== (const ValueIterator<Iterator> &x, const ValueIterator<Iterator> &y)
{ return x.base() == y.base(); }
template<class Iterator> inline bool
operator!= (const ValueIterator<Iterator> &x, const ValueIterator<Iterator> &y)
{ return x.base() != y.base(); }
template<class Iterator> inline bool
operator< (const ValueIterator<Iterator> &x, const ValueIterator<Iterator> &y)
{ return x.base() < y.base(); }
template<class Iterator> inline bool
operator<= (const ValueIterator<Iterator> &x, const ValueIterator<Iterator> &y)
{ return x.base() <= y.base(); }
template<class Iterator> inline bool
operator> (const ValueIterator<Iterator> &x, const ValueIterator<Iterator> &y)
{ return x.base() > y.base(); }
template<class Iterator> inline bool
operator>= (const ValueIterator<Iterator> &x, const ValueIterator<Iterator> &y)
{ return x.base() >= y.base(); }
template<class Iterator> inline typename ValueIterator<Iterator>::difference_type
operator- (const ValueIterator<Iterator> &x, const ValueIterator<Iterator> &y)
{ return x.base() - y.base(); }
template<class Iterator> inline ValueIterator<Iterator>
operator+ (typename ValueIterator<Iterator>::difference_type n, const ValueIterator<Iterator> &x)
{ return x.operator+ (n); }

/* --- IteratorRange (iterator range wrappers) --- */
template<class Iterator> class IteratorRange {
  Iterator ibegin, iend;
public:
  typedef Iterator                              iterator;
  typedef typename Iterator::iterator_category  iterator_category;
  typedef typename Iterator::value_type         value_type;
  typedef typename Iterator::difference_type    difference_type;
  typedef typename Iterator::reference          reference;
  typedef typename Iterator::pointer            pointer;
  explicit IteratorRange (const Iterator &cbegin, const Iterator &cend) :
    ibegin (cbegin), iend (cend)
  {}
  explicit IteratorRange()
  {}
  iterator       begin()       const { return ibegin; }
  iterator       end()         const { return iend; }
  bool           done()        const { return ibegin == iend; }
  bool           has_next()    const { return ibegin != iend; }
  reference      operator* ()  const { return ibegin.operator*(); }
  pointer        operator-> () const { return ibegin.operator->(); }
  IteratorRange& operator++ ()       { ibegin.operator++(); return *this; }
  IteratorRange  operator++ (int)    { IteratorRange dup (*this); ibegin.operator++(); return dup; }
  bool           operator== (const IteratorRange &w) const
  {
    return ibegin == w.begin && iend == w.end;
  }
  bool        operator!= (const IteratorRange &w) const
  {
    return ibegin != w.begin || iend != w.end;
  }
};
template<class Iterator> IteratorRange<Iterator>
iterator_range (const Iterator &begin, const Iterator &end)
{
  return IteratorRange<Iterator> (begin, end);
}

/* --- ValueIteratorRange (iterator range wrappers) --- */
template<class Iterator> class ValueIteratorRange {
  Iterator ibegin, iend;
public:
  typedef Iterator                              iterator;
  typedef typename Iterator::iterator_category  iterator_category;
  typedef typename Iterator::value_type         pointer;
  // typedef typeof (*(pointer) 0)              value_type;
  typedef typename Dereference<pointer>::Value  value_type;
  typedef typename Iterator::difference_type    difference_type;
  typedef value_type&                           reference;
  explicit ValueIteratorRange (const Iterator &cbegin, const Iterator &cend) :
    ibegin (cbegin), iend (cend)
  {}
  explicit ValueIteratorRange()
  {}
  iterator            begin()       const { return ibegin; }
  iterator            end()         const { return iend; }
  bool                done()        const { return ibegin == iend; }
  bool                has_next()    const { return ibegin != iend; }
  reference           operator* ()  const { return *ibegin.operator*(); }
  pointer             operator-> () const { return ibegin.operator*(); }
  ValueIteratorRange& operator++ ()       { ibegin.operator++(); return *this; }
  ValueIteratorRange  operator++ (int)    { ValueIteratorRange dup (*this); ibegin.operator++(); return dup; }
  bool                operator== (const ValueIteratorRange &w) const
  {
    return ibegin == w.begin && iend == w.end;
  }
  bool                operator!= (const ValueIteratorRange &w) const
  {
    return ibegin != w.begin || iend != w.end;
  }
};
template<class Iterator> ValueIteratorRange<Iterator>
value_iterator_range (const Iterator &begin, const Iterator &end)
{
  return ValueIteratorRange<Iterator> (begin, end);
}

/* --- Walker (easy to use iterator_range (begin(), end()) substitute) --- */
template<typename Value> class Walker {
  /* auxillary Walker classes */
  struct AdapterBase : Deletable {
    virtual bool         done    () = 0;
    virtual void         inc     () = 0;
    virtual AdapterBase* clone   () const = 0;
    virtual void*        element () const = 0;
    virtual bool         equals  (const AdapterBase &eb) const = 0;
  };
  AdapterBase *adapter;
public:
  template<class Iterator> class Adapter : public AdapterBase {
    Iterator ibegin, iend;
  public:
    explicit Adapter (const Iterator &cbegin, const Iterator &cend) :
      ibegin (cbegin), iend (cend)
    {}
    virtual bool         done    ()              { return ibegin == iend; }
    virtual void         inc     ()              { ibegin.operator++(); }
    virtual AdapterBase* clone   () const        { return new Adapter (ibegin, iend); }
    virtual void*        element () const        { return (void*) ibegin.operator->(); }
    virtual bool         equals  (const AdapterBase &eb) const
    {
      const Adapter *we = dynamic_cast<const Adapter*> (&eb);
      return we && ibegin == we->ibegin && iend == we->iend;
    }
  };
  /* Walker API */
  typedef Value       value_type;
  typedef value_type& reference;
  typedef value_type* pointer;
  Walker&   operator= (const Walker &w)         { adapter = w.adapter ? w.adapter->clone() : NULL; return *this; }
  bool      done       () const                 { return !adapter || adapter->done(); }
  bool      has_next   () const                 { return !done(); }
  pointer   operator-> () const                 { return reinterpret_cast<pointer> (adapter ? adapter->element() : NULL); }
  reference operator*  () const                 { return *operator->(); }
  Walker&   operator++ ()                       { if (adapter) adapter->inc(); return *this; }
  Walker    operator++ (int)                    { Walker dup (*this); if (adapter) adapter->inc(); return dup; }
  bool      operator== (const Walker &w) const  { return w.adapter == adapter || (adapter && w.adapter && adapter->equals (*w.adapter)); }
  bool      operator!= (const Walker &w) const  { return !operator== (w); }
  ~Walker()                                     { if (adapter) delete adapter; adapter = NULL; }
  Walker (const Walker &w)                      : adapter (w.adapter ? w.adapter->clone() : NULL) {}
  Walker (AdapterBase *cadapter = NULL)         : adapter (cadapter) {}
};
template<class Container> Walker<const typename Container::const_iterator::value_type>
walker (const Container &container)
{
  typedef typename Container::const_iterator          Iterator;
  typedef Walker<const typename Iterator::value_type> Walker;
  typedef typename Walker::template Adapter<Iterator> Adapter;
  return Walker (new Adapter (container.begin(), container.end()));
}
template<class Container> Walker<typename Container::iterator::value_type>
walker (Container &container)
{
  typedef typename Container::iterator                Iterator;
  typedef Walker<typename Iterator::value_type>       Walker;
  typedef typename Walker::template Adapter<Iterator> Adapter;
  return Walker (new Adapter (container.begin(), container.end()));
}
template<class Container> Walker<typename Dereference<const typename Container::const_iterator::value_type>::Value>
value_walker (const Container &container)
{
  typedef typename Container::const_iterator                               Iterator;
  typedef typename Dereference<const typename Iterator::value_type>::Value Value;
  typedef Walker<Value>                                                    Walker;
  typedef ValueIterator<Iterator>                                          VIterator;
  typedef typename Walker::template Adapter<VIterator>                     Adapter;
  return Walker (new Adapter (VIterator (container.begin()), VIterator (container.end())));
}
template<class Container> Walker<typename Dereference<typename Container::iterator::value_type>::Value>
value_walker (Container &container)
{
  typedef typename Container::iterator                               Iterator;
  typedef typename Dereference<typename Iterator::value_type>::Value Value;
  typedef Walker<Value>                                              Walker;
  typedef ValueIterator<Iterator>                                    VIterator;
  typedef typename Walker::template Adapter<VIterator>               Adapter;
  return Walker (new Adapter (VIterator (container.begin()), VIterator (container.end())));
}
template<class Iterator> Walker<typename Iterator::value_type>
walker (const Iterator &begin, const Iterator &end)
{
  typedef Walker<typename Iterator::value_type>       Walker;
  typedef typename Walker::template Adapter<Iterator> Adapter;
  return Walker (new Adapter (begin, end));
}
template<class Iterator> Walker<typename Dereference<typename Iterator::value_type>::Value>
value_walker (const Iterator &begin, const Iterator &end)
{
  typedef typename Dereference<typename Iterator::value_type>::Value Value;
  typedef Walker<Value>                                              Walker;
  typedef ValueIterator<Iterator>                                    VIterator;
  typedef typename Walker::template Adapter<VIterator>               Adapter;
  return Walker (new Adapter (VIterator (begin), VIterator (end)));
}

/* --- ReferenceCountImpl --- */
class ReferenceCountImpl : public virtual Deletable
{
  unsigned int m_ref_count : 31;
  unsigned int m_floating : 1;
public:
  ReferenceCountImpl() :
    m_ref_count (1),
    m_floating (1)
  {}
  void
  ref()
  {
    assert (m_ref_count > 0);
    assert (m_ref_count < 2147483647);
    m_ref_count++;
  }
  void
  ref_sink()
  {
    assert (m_ref_count > 0);
    ref();
    if (m_floating)
      {
        m_floating = 0;
        unref();
      }
  }
  virtual void
  finalize()
  {}
  bool
  finalizing()
  {
    return m_ref_count < 1;
  }
  void
  unref()
  {
    assert (m_ref_count > 0);
    m_ref_count--;
    if (!m_ref_count)
      {
        finalize();
        delete_this(); // effectively: delete this;
      }
  }
  void
  ref_diag(const char *msg = NULL)
  {
    diag ("%s: this=%p ref_count=%d floating=%d", msg ? msg : "ReferenceCountImpl", this, m_ref_count, m_floating);
  }
  template<class Obj> static Obj& ref      (Obj &obj) { obj.ref();       return obj; }
  template<class Obj> static Obj* ref      (Obj *obj) { obj->ref();      return obj; }
  template<class Obj> static Obj& ref_sink (Obj &obj) { obj.ref_sink();  return obj; }
  template<class Obj> static Obj* ref_sink (Obj *obj) { obj->ref_sink(); return obj; }
  template<class Obj> static void unref    (Obj &obj) { obj.unref(); }
  template<class Obj> static void unref    (Obj *obj) { obj->unref(); }
  template<class Obj> static void sink     (Obj &obj) { obj.ref_sink(); obj.unref(); }
  template<class Obj> static void sink     (Obj *obj) { obj->ref_sink(); obj->unref(); }
protected:
  virtual
  ~ReferenceCountImpl()
  { assert (m_ref_count == 0); }
  virtual void
  delete_this ()
  { delete this; }
};
template<class Obj> static Obj& ref      (Obj &obj) { obj.ref();       return obj; }
template<class Obj> static Obj* ref      (Obj *obj) { obj->ref();      return obj; }
template<class Obj> static Obj& ref_sink (Obj &obj) { obj.ref_sink();  return obj; }
template<class Obj> static Obj* ref_sink (Obj *obj) { obj->ref_sink(); return obj; }
template<class Obj> static void unref    (Obj &obj) { obj.unref(); }
template<class Obj> static void unref    (Obj *obj) { obj->unref(); }
template<class Obj> static void sink     (Obj &obj) { obj.ref_sink(); obj.unref(); }
template<class Obj> static void sink     (Obj *obj) { obj->ref_sink(); obj->unref(); }

/* --- generic named data --- */
template<typename Type>
class DataKey {
private:
  /*Copy*/        DataKey    (const DataKey&);
  DataKey&        operator=  (const DataKey&);
public:
  /* explicit */  DataKey    ()                 { }
  virtual Type    fallback   ()                 { Type d = Type(); return d; }
  virtual void    destroy    (Type data)        { /* destruction hook */ }
};

class DataList {
  class NodeBase {
  protected:
    NodeBase      *next;
    DataKey<void> *key;
    explicit       NodeBase (DataKey<void> *k) :
      next (NULL),
      key (k)
    {}
    virtual        ~NodeBase ();
    friend         class DataList;
  };
  template<typename T>
  class Node : public NodeBase {
    T data;
  public:
    T   get_data ()     { return data; }
    T   swap     (T d)  { T result = data; data = d; return result; }
    ~Node()
    {
      if (key)
        {
          DataKey<T> *dkey = reinterpret_cast<DataKey<T>*> (key);
          dkey->destroy (data);
        }
    }
    Node (DataKey<T> *k,
          T           d) :
      NodeBase (reinterpret_cast<DataKey<void>*> (k)),
      data (d)
    {}
  };
  NodeBase *nodes;
public:
  DataList() :
    nodes (NULL)
  {}
  template<typename T> void
  set (DataKey<T> *key,
       T           data)
  {
    Node<T> *node = new Node<T> (key, data);
    set_data (node);
  }
  template<typename T> T
  get (DataKey<T> *key)
  {
    NodeBase *nb = get_data (reinterpret_cast<DataKey<void>*> (key));
    if (nb)
      {
        Node<T> *node = reinterpret_cast<Node<T>*> (nb);
        return node->get_data();
      }
    else
      return key->fallback();
  }
  template<typename T> T
  swap (DataKey<T> *key,
        T           data)
  {
    NodeBase *nb = get_data (reinterpret_cast<DataKey<void>*> (key));
    if (nb)
      {
        Node<T> *node = reinterpret_cast<Node<T>*> (nb);
        return node->swap (data);
      }
    else
      {
        set (key, data);
        return key->fallback();
      }
  }
  template<typename T> T
  swap (DataKey<T> *key)
  {
    NodeBase *nb = rip_data (reinterpret_cast<DataKey<void>*> (key));
    if (nb)
      {
        Node<T> *node = reinterpret_cast<Node<T>*> (nb);
        T d = node->get_data();
        nb->key = NULL; // rip key to prevent data destruction
        delete nb;
        return d;
      }
    else
      return key->fallback();
  }
  template<typename T> void
  del (DataKey<T> *key)
  {
    NodeBase *nb = rip_data (reinterpret_cast<DataKey<void>*> (key));
    if (nb)
      delete nb;
  }
  ~DataList();
private:
  void      set_data (NodeBase      *node);
  NodeBase* get_data (DataKey<void> *key);
  NodeBase* rip_data (DataKey<void> *key);
};


/* --- binary lookup --- */
template<typename RandIter, class Cmp, typename Arg, int case_lookup_or_sibling_or_insertion>
static inline std::pair<RandIter,bool>
binary_lookup_fuzzy (RandIter  begin,
                     RandIter  end,
                     Cmp       cmp_elements,
                     const Arg &arg)
{
  RandIter current = end;
  ssize_t cmp = 0, n_elements = end - begin, offs = 0;
  const bool want_lookup = case_lookup_or_sibling_or_insertion == 0;
  const bool want_insertion_pos = case_lookup_or_sibling_or_insertion > 1;
  while (offs < n_elements)
    {
      size_t i = (offs + n_elements) >> 1;
      current = begin + i;
      cmp = cmp_elements (arg, *current);
      if (cmp == 0)
        return want_insertion_pos ? std::make_pair (current, true) : std::make_pair (current, /*ignored*/false);
      else if (cmp < 0)
        n_elements = i;
      else /* (cmp > 0) */
        offs = i + 1;
    }
  /* check is last mismatch, cmp > 0 indicates greater key */
  return (want_lookup
          ? make_pair (end, /*ignored*/false)
          : (want_insertion_pos && cmp > 0)
          ? make_pair (current + 1, false)
          : make_pair (current, false));
}
template<typename RandIter, class Cmp, typename Arg>
static inline std::pair<RandIter,bool>
binary_lookup_insertion_pos (RandIter  begin,
                             RandIter  end,
                             Cmp       cmp_elements,
                             const Arg &arg)
{
  /* return (end,false) for end-begin==0, or return (position,true) for exact match,
   * otherwise return (position,false) where position indicates the location for
   * the key to be inserted (and may equal end).
   */
  return binary_lookup_fuzzy<RandIter,Cmp,Arg,2> (begin, end, cmp_elements, arg);
}
template<typename RandIter, class Cmp, typename Arg>
static inline RandIter
binary_lookup_sibling (RandIter  begin,
                       RandIter  end,
                       Cmp       cmp_elements,
                       const Arg &arg)
{
  /* return end for end-begin==0, otherwise return the exact match element, or,
   * if there's no such element, return the element last visited, which is pretty
   * close to an exact match (will be one off into either direction).
   */
  return binary_lookup_fuzzy<RandIter,Cmp,Arg,1> (begin, end, cmp_elements, arg).first;
}
template<typename RandIter, class Cmp, typename Arg>
static inline RandIter
binary_lookup (RandIter  begin,
               RandIter  end,
               Cmp       cmp_elements,
               const Arg &arg)
{
  /* return end or exact match */
  return binary_lookup_fuzzy<RandIter,Cmp,Arg,0> (begin, end, cmp_elements, arg).first;
}

} // Birnet

/* --- signals --- */
#include <rapicorn/birnetsignal.hh>

#endif  /* __BIRNET_UTILS_HH__ */
