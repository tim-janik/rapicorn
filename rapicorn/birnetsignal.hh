/* BirnetSignal
 * Copyright (C) 2005 Tim Janik
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
#ifndef __BIRNET_SIGNAL_HH__
#define __BIRNET_SIGNAL_HH__

#include <rapicorn/birnetutils.hh>

namespace Birnet {

/* --- ClosureBase --- */
struct ClosureBase : virtual public ReferenceCountImpl {
  explicit ClosureBase ()
  {}
};

/* --- SlotBase --- */
template <typename Closure>
class SlotBase {
protected:
  Closure *c;
  void
  set_closure (Closure *cl)
  {
    if (cl)
      cl->ref_sink();
    if (c)
      c->unref();
    c = cl;
  }
public:
  SlotBase (Closure *closure) :
    c (NULL)
  { set_closure (closure); }
  SlotBase (const SlotBase &src) :
    c (NULL)
  { *this = src; }
  SlotBase&
  operator= (const SlotBase &src)
  {
    set_closure (src.c);
    return *this;
  }
  Closure*
  get_closure() const
  { return c; }
};

/* --- ReferencableWrapper --- */
struct ReferencableBase {
  virtual void ref() = 0;
  virtual void unref() = 0;
};
template<class Referencable>
class ReferencableWrapper : public ReferencableBase {
  Referencable &obj;
public:
  ReferencableWrapper (Referencable &cobj) :
    obj (cobj)
  {}
  virtual void
  ref()
  { obj.ref(); }
  virtual void
  unref()
  { obj.unref(); }
};

/* --- SignalBase --- */
template<typename Closure>
class SignalBase {
protected:
  vector<Closure*> closures;
  typedef typename vector<Closure*>::iterator Iterator;
  void
  connect_closure (Closure *closure)
  {
    closure->ref_sink();
    closures.push_back (closure);
  }
public:
  explicit SignalBase (ReferencableBase *rbase) :
    referencable (rbase)
  {}
  ~SignalBase()
  {
    if (referencable)
      delete referencable;
  }
protected:
  ReferencableBase *referencable;
private:
  SignalBase& operator=  (const SignalBase&);
  /*Con*/     SignalBase (const SignalBase&);
};

/* --- Accumulator --- */
template<typename Result>
struct AccumulatorLast {
  void init (Result &accu) {}
  bool
  collect (Result &accu,
           Result  value)
  {
    accu = value;
    return true; // continue emission
  }
};
template<typename Result>
struct AccumulatorPass0 {
  void init (Result &accu) {}
  bool
  collect (Result &accu,
           Result  value)
  {
    accu = value;
    return !accu;  // continue emission
  }
};
template<typename Result>
struct AccumulatorSum {
  void init (Result &accu) {}
  bool
  collect (Result &accu,
           Result  value)
  {
    accu = accu + value;
    return true; // continue emission
  }
};

/* --- Closure + Slot + Signal generation --- */
#define BIRNET_SIG_MATCH(x)             (x >= 0 && x <= 16)
#define BIRNET_SIG_ARG_TYPENAME(x)      typename A##x
#define BIRNET_SIG_ARG_c_TYPENAME(x)    , typename A##x
#define BIRNET_SIG_ARG_TYPE(x)          A##x
#define BIRNET_SIG_ARG_c_TYPE(x)        , A##x
#define BIRNET_SIG_ARG_VAR(x)           a##x
#define BIRNET_SIG_ARG_c_VAR(x)         , a##x
#define BIRNET_SIG_ARG_TYPED_VAR(x)     A##x a##x
#define BIRNET_SIG_ARG_c_TYPED_VAR(x)   , A##x a##x
#include <rapicorn/birnetsignalinc.hh>  // includes "birnetsignaldefs.hh" multiple times
#undef BIRNET_SIG_MATCH
#undef BIRNET_SIG_ARG_TYPENAME
#undef BIRNET_SIG_ARG_c_TYPENAME
#undef BIRNET_SIG_ARG_TYPE
#undef BIRNET_SIG_ARG_c_TYPE
#undef BIRNET_SIG_ARG_VAR
#undef BIRNET_SIG_ARG_c_VAR
#undef BIRNET_SIG_ARG_TYPED_VAR
#undef BIRNET_SIG_ARG_c_TYPED_VAR

} // Birnet

#endif  /* __BIRNET_SIGNAL_HH__ */
