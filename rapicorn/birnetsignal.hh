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
namespace Signal {

/* --- HandlerBase --- */
struct HandlerBase : virtual public ReferenceCountImpl {
  explicit HandlerBase ()
  {}
};

/* --- SlotBase --- */
template <typename Handler>
class SlotBase {
protected:
  Handler *c;
  void
  set_handler (Handler *cl)
  {
    if (cl)
      cl->ref_sink();
    if (c)
      c->unref();
    c = cl;
  }
public:
  SlotBase (Handler *handler) :
    c (NULL)
  { set_handler (handler); }
  SlotBase (const SlotBase &src) :
    c (NULL)
  { *this = src; }
  SlotBase&
  operator= (const SlotBase &src)
  {
    set_handler (src.c);
    return *this;
  }
  Handler*
  get_handler() const
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
template<typename Handler>
class SignalBase {
protected:
  vector<Handler*> handlers;
  typedef typename vector<Handler*>::iterator Iterator;
  void
  connect_handler (Handler *handler)
  {
    handler->ref_sink();
    handlers.push_back (handler);
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

/* --- Handler + Slot + Signal generation --- */
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

} // Signal
} // Birnet

#endif  /* __BIRNET_SIGNAL_HH__ */
