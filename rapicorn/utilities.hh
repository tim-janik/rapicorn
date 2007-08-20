/* Rapicorn
 * Copyright (C) 2005 Tim Janik
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
#ifndef __RAPICORN_UTILITIES_HH__
#define __RAPICORN_UTILITIES_HH__

#include <rapicorn/rapicorncore.hh>
#include <typeinfo>
#ifdef  RAPICORN_PRIVATE
#include <rapicorn/private.hh>
#endif

namespace Rapicorn {
class RapicornTester;	// allow Rapicorn test implementations

/* --- i18n macros --- */
void          rapicorn_init             (int        *argcp,
                                         char     ***argvp,
                                         const char *app_name);
void          rapicorn_thread_enter     ();
bool          rapicorn_thread_try_enter ();
bool          rapicorn_thread_entered   ();
void          rapicorn_thread_leave     ();
const char*   rapicorn_gettext          (const char *text);

/* --- standard utlities --- */
//template<typename T> inline const T& min   (const T &a, const T &b) { return ::std::min<T> (a, b); }
//template<typename T> inline const T& max   (const T &a, const T &b) { return ::std::min<T> (a, b); }
using ::std::min;
using ::std::max;
inline double min     (double a, int64  b) { return min<double> (a, b); }
inline double min     (int64  a, double b) { return min<double> (a, b); }
inline double max     (double a, int64  b) { return max<double> (a, b); }
inline double max     (int64  a, double b) { return max<double> (a, b); }

/* --- text encoding --- */
bool    text_convert    (const char        *to_charset,
                         std::string       &output_string,
                         const char        *from_charset,
                         const std::string &input_string,
                         const char        *fallback_charset = "ISO-8859-1",
                         const std::string &output_mark = "");

/* --- exceptions --- */
struct Exception : std::exception {
public:
  // BROKEN(g++-3.3,g++-3.4): Exception  (const char *reason_format, ...) RAPICORN_PRINTF (2, 3);
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

/* --- Convertible --- */
class Convertible : public virtual ReferenceCountImpl {
public:
  /* interface matching base class */
  class InterfaceMatch {
    RAPICORN_PRIVATE_CLASS_COPY (InterfaceMatch);
  public:
    virtual            ~InterfaceMatch  () {}
    explicit            InterfaceMatch  (const String &ident) : m_ident (ident), m_match_found (false) {}
    bool                done            () { return m_match_found; }
    virtual  bool       match           (Convertible  *object,
                                         const String &ident = String()) = 0;
    const String&       identifier      () { return m_ident; }
  protected:
    const String       &m_ident;
    bool                m_match_found;
  };
  typedef Signal<Convertible, Convertible* (const InterfaceMatch&, const String&), CollectorWhile0<Convertible*> > SignalFindInterface;
  /* SignalFindInterface sig_find_interface; */
private:
  /* interface matching base template class */
  template<typename Type>
  struct InterfaceCast : InterfaceMatch {
    typedef Type        Interface;
    explicit            InterfaceCast   (const String &ident) : InterfaceMatch (ident), m_instance (NULL) {}
    virtual bool        match           (Convertible *obj,
                                         const String &ident);
  protected:
    Interface          *m_instance;
  };
  /* interface matching template class implementation for Type, Type& and Type* */
  template<typename Type>
  struct InterfaceType : InterfaceCast<Type> {
    typedef Type&       Result;
    explicit            InterfaceType   (const String &ident) : InterfaceCast<Type> (ident) {}
    Type&               result          (bool may_throw);
  };
  template<typename Type>
  struct InterfaceType<Type&> : InterfaceType<Type> {
    explicit            InterfaceType   (const String &ident) : InterfaceType<Type> (ident) {}
  };
  template<typename Type>
  struct InterfaceType<Type*> : InterfaceCast<Type> {
    typedef Type*       Result;
    explicit            InterfaceType   (const String &ident) : InterfaceCast<Type> (ident) {}
    Type*               result          (bool may_throw);
  };
public: /* user API */
  explicit              Convertible     ();
  virtual bool          match_interface (InterfaceMatch &imatch) const;
  template<typename Type>
  typename
  InterfaceType<Type>::
  Result                interface       (const String         &ident = String(),
                                         const std::nothrow_t &nt = dothrow) const;
  template<typename Type>
  typename
  InterfaceType<Type>::
  Result                interface       (const std::nothrow_t &nt) const;
};

/* --- template implementations --- */
template<typename Type> bool
Convertible::InterfaceCast<Type>::match (Convertible  *obj,
                                         const String &ident)
{
  if (!m_instance)
    {
      const String &id = identifier();
      if (id.empty() || id == ident)
        {
          m_instance = dynamic_cast<Interface*> (obj);
          m_match_found = m_instance != NULL;
        }
    }
  return m_match_found;
}

template<typename Type> Type&
Convertible::InterfaceType<Type>::result (bool may_throw)
{
  if (!this->m_instance && may_throw)
    throw NullInterface();
  return *this->m_instance;
}

template<typename Type> Type*
Convertible::InterfaceType<Type*>::result (bool may_throw)
{
  return InterfaceCast<Type>::m_instance;
}

template<typename Type> typename Convertible::InterfaceType<Type>::Result
Convertible::interface (const String         &ident,
                        const std::nothrow_t &nt) const
{
  InterfaceType<Type> interface_type (ident);
  match_interface (interface_type);
  return interface_type.result (&nt == &dothrow);
}

template<typename Type> typename Convertible::InterfaceType<Type>::Result
Convertible::interface (const std::nothrow_t &nt) const
{
  return interface<Type> (String(), nt);
}

} // Rapicorn

#endif  /* __RAPICORN_UTILITIES_HH__ */
