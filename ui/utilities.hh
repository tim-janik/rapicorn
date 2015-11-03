// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_UTILITIES_HH__
#define __RAPICORN_UTILITIES_HH__

#include <rapicorn-core.hh>
#include <typeinfo>

#if !defined __RAPICORN_CLIENTAPI_HH_ && !defined __RAPICORN_UITHREAD_HH__ && !defined __RAPICORN_BUILD__
#error Only <rapicorn.hh> can be included directly.
#endif

/* --- internally used macros --- */
#ifdef __RAPICORN_BUILD__
// convenience macros
#define MakeNamedCommand                RAPICORN_MakeNamedCommand
#endif

namespace Rapicorn {

/* --- standard utlities --- */
//template<typename T> inline const T& min   (const T &a, const T &b) { return ::std::min<T> (a, b); }
//template<typename T> inline const T& max   (const T &a, const T &b) { return ::std::min<T> (a, b); }
using ::std::min;
using ::std::max;
inline double min     (double a, int64  b) { return min<double> (a, b); }
inline double min     (int64  a, double b) { return min<double> (a, b); }
inline double max     (double a, int64  b) { return max<double> (a, b); }
inline double max     (int64  a, double b) { return max<double> (a, b); }

/* --- exceptions --- */
struct Exception : std::exception {
public:
  explicit            Exception  (const String &s1, const String &s2 = String(), const String &s3 = String(), const String &s4 = String(),
                                  const String &s5 = String(), const String &s6 = String(), const String &s7 = String(), const String &s8 = String());
  void                set        (const String &s);
  virtual const char* what       () const throw() { return reason ? reason : "out of memory"; }
  /*Copy*/            Exception  (const Exception &e);
  /*Des*/             ~Exception () throw();
private:
  Exception&          operator=  (const Exception&);
  char *reason;
};
struct NullPointer : std::exception {};
/* allow 'dothrow' as function argument analogous to 'nothrow' */
extern const std::nothrow_t dothrow; /* indicate "with exception" semantics */
using std::nothrow_t;
using std::nothrow;

/* --- derivation assertions --- */
template<class Derived, class Base>
struct EnforceDerivedFrom {
  EnforceDerivedFrom (Derived *derived = 0,
                      Base    *base = 0)
  { base = derived; }
};
template<class Derived, class Base>
struct EnforceDerivedFrom<Derived*,Base*> {
  EnforceDerivedFrom (Derived *derived = 0,
                      Base    *base = 0)
  { base = derived; }
};
template<class Derived, class Base> void        // ex: assert_derived_from<Child, Base>();
assert_derived_from (void)
{
  EnforceDerivedFrom<Derived, Base> assertion;
}

/* --- derivation checks --- */
template<class Child, class Base>
class CheckDerivedFrom {
  static bool is_derived (void*) { return false; }
  static bool is_derived (Base*) { return true; }
public:
  static bool is_derived () { return is_derived ((Child*) (0)); }
};
template<class Child, class Base>
struct CheckDerivedFrom<Child*,Base*> : CheckDerivedFrom<Child,Base> {};
template<class Derived, class Base> bool
is_derived ()                                   // ex: if (is_derived<Child, Base>()) ...; */
{
  return CheckDerivedFrom<Derived,Base>::is_derived();
}

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

} // Rapicorn

#endif  /* __RAPICORN_UTILITIES_HH__ */
