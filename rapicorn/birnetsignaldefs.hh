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

/* this file is repeatedly included from birnetsignal.hh */


#if BIRNET_SIG_NAME(900) == 9003


/* --- Handlers with emitter --- */
template<typename R0, class Emitter, typename A1, typename A2, typename A3>
struct HandlerE3 : HandlerBase {
  HandlerE3 () : HandlerBase () {}
  virtual R0 operator() (Emitter &emitter, A1 a1, A2 a2, A3 a3) = 0;
};
template<typename R0, class Emitter, typename A1, typename A2, typename A3>
struct HandlerEF3 : HandlerE3 <R0, Emitter, A1, A2, A3> {
  typedef R0 (*Callback) (Emitter&, A1, A2, A3);
  HandlerEF3 (Callback c) : callback (c) {}
  virtual R0 operator() (Emitter &emitter, A1 a1, A2 a2, A3 a3)
  { return callback (emitter, a1, a2, a3); }
protected:
  Callback callback;
private:
  ~HandlerEF3() {}
  friend void FIXME_dummy_friend_for_gcc33();
};
template<class Class, typename R0, class Emitter, typename A1, typename A2, typename A3>
struct HandlerEM3 : HandlerE3 <R0, Emitter, A1, A2, A3> {
  typedef R0 (Class::*Method) (Emitter&, A1, A2, A3);
  HandlerEM3 (Class &obj, Method m) : instance (&obj), method (m) {}
  virtual R0 operator() (Emitter &emitter, A1 a1, A2 a2, A3 a3)
  { return (instance->*method) (emitter, a1, a2, a3); }
protected:
  Class *instance;
  Method method;
private:
  ~HandlerEM3() {}
  friend void FIXME_dummy_friend_for_gcc33();
};
/* --- Slots with emitters --- */
template<typename R0, class Emitter, typename A1, typename A2, typename A3>
struct SlotE3 : SlotBase <HandlerE3 <R0, Emitter, A1, A2, A3> > {
  typedef HandlerE3 <R0, Emitter, A1, A2, A3>  Handler;
  SlotE3 (R0 (*callback) (Emitter&, A1, A2, A3)) :
    SlotBase<Handler> (new HandlerEF3 <R0, Emitter, A1, A2, A3> (callback))
  {}
  template<class Class>
  SlotE3 (Class &instance, R0 (Class::*method) (Emitter&, A1, A2, A3)) :
    SlotBase<Handler> (new HandlerEM3 <Class, R0, Emitter, A1, A2, A3> (instance, method))
  {}
};
/* function slot constructor */
template<typename R0, class Emitter, typename A1, typename A2, typename A3> SlotE3<R0, Emitter, A1, A2, A3>
slot (R0 (*callback) (Emitter&, A1, A2, A3))
{ return SlotE3 <R0, Emitter, A1, A2, A3> (callback); }
/* method slot constructor */
template<class Class, typename R0, class Emitter, typename A1, typename A2, typename A3> SlotE3<R0, Emitter, A1, A2, A3>
slot (Class &obj, R0 (Class::*method) (Emitter&, A1, A2, A3))
{ return SlotE3 <R0, Emitter, A1, A2, A3> (obj, method); }

/* --- SignalEmittable3 --- */
template<class Emitter, typename R0, typename A1, typename A2, typename A3, template<typename> class Accu>
struct SignalEmittable3 : SignalBase< HandlerE3<R0, Emitter, A1, A2, A3> >
{
  typedef HandlerE3<R0, Emitter, A1, A2, A3> Handler;
  typedef SignalBase<Handler>                SignalBase;
  typedef typename SignalBase::Iterator      Iterator;
  inline R0 emit (A1 a1, A2 a2, A3 a3)
  {
    if (this->referencable)
      this->referencable->ref();
    R0 result = R0();    // default initialization
    accu.init (result);  // custom setup
    for (Iterator it = this->handlers.begin(); it != this->handlers.end(); it++)
      if (!accu.collect (result, (*it)->operator() (*(Emitter*) 0, a1, a2, a3))) // FIXME
        break;
    if (this->referencable)
      this->referencable->unref();
    return result;
  }
  explicit SignalEmittable3 (ReferencableBase *referencable) :
    SignalBase (referencable)
  {}
private:
  Accu<R0> accu;
};
/* SignalEmittable3 for void returns */
template<class Emitter, typename A1, typename A2, typename A3, template<typename> class Accu>
struct SignalEmittable3<Emitter, void, A1, A2, A3, Accu> : SignalBase< HandlerE3<void, Emitter, A1, A2, A3> >
{
  typedef HandlerE3<void, Emitter, A1, A2, A3> Handler;
  typedef SignalBase<Handler>                  SignalBase;
  typedef typename SignalBase::Iterator        Iterator;
  inline void emit (A1 a1, A2 a2, A3 a3)
  {
    if (this->referencable)
      this->referencable->ref();
    for (Iterator it = this->handlers.begin(); it != this->handlers.end(); it++)
      (*it)->operator() (*(Emitter*) 0, a1, a2, a3);
    if (this->referencable)
      this->referencable->unref();
  }
  explicit SignalEmittable3 (ReferencableBase *referencable) :
    SignalBase (referencable)
  {}
};

/* --- Signal3 --- */
/* Signal* */
template<class Emitter, typename R0, typename A1, typename A2, typename A3, template<typename> class Accu = AccumulatorLast>
struct Signal3 : SignalEmittable3<Emitter, R0, A1, A2, A3, Accu>
{
  typedef HandlerE3<R0, Emitter, A1, A2, A3>               Handler;
  typedef SlotE3<R0, Emitter, A1, A2, A3>                  Slot;
  typedef SignalBase<Handler>                              SignalBase;
  typedef typename SignalBase::Iterator                    Iterator;
  typedef SignalEmittable3<Emitter, R0, A1, A2, A3, Accu>  SignalEmittable;
  explicit Signal3 (Emitter &referencable) :
    SignalEmittable (new ReferencableWrapper<Emitter> (referencable))
  { assert (&referencable != NULL); }
  template<class Class>
  explicit Signal3 (Emitter &referencable, R0 (Class::*method) (Emitter&, A1, A2, A3)) :
    SignalEmittable (new ReferencableWrapper<Emitter> (referencable))
  {
    assert (&referencable != NULL);
    connect_handler (new HandlerEM3 <Class, R0, Emitter, A1, A2, A3> (referencable, method));
  }
  inline void connect (Handler *c)      { connect_handler (c); }
  inline void connect (const Slot &s)   { connect_handler (s.get_handler()); }
  template<class Class>
  inline void connect (Class   *obj, R0 (Class::*method) (Emitter&, A1, A2, A3))
  {
    connect_handler (new HandlerEM3 <Class, R0, Emitter, A1, A2, A3> (obj, method));
    connect (SlotE3<R0, Emitter, A1, A2, A3> (obj, method)); // FIXME: remove
  }
  typedef R0  (*Callback) (Emitter&, A1, A2, A3);
  Signal3&      operator+= (Handler *c)         { connect_handler (c); return *this; }
  Signal3&      operator+= (const Slot &s)      { connect_handler (s.get_handler()); return *this; }
  Signal3&      operator+= (const Callback c)   { connect (c); return *this; }
};


#endif

#if 0
/* --- Closure*: argument number specific closure base type --- */
#if 0 // Closure3 sample
template<typename R0, typename A1, typename A2, typename A3>
struct Closure3 : ClosureBase {
  Closure3 () : ClosureBase () {}
  virtual R0 operator() (A1 a1, A2 a2, A3 a3) = 0;
};
#endif
/* Closure* */
template<typename R0 BIRNET_SIG_c_TYPENAMES>
struct BIRNET_SIG_NAME (Closure) : ClosureBase {
  BIRNET_SIG_NAME (Closure) () : ClosureBase () {}
  virtual R0 operator() ( BIRNET_SIG_TYPED_VARS ) = 0;
};

/* --- ClosureF*: function closures --- */
#if 0 // ClosureF3 sample
template<typename R0, typename A1, typename A2, typename A3>
struct ClosureF3 : Closure3 <R0, A1, A2, A3> {
  typedef R0 (*Callback) (A1, A2, A3);
  ClosureF3 (Callback c) : callback (c) {}
  R0 operator() (A1 a1, A2 a2, A3 a3)
  {
    return callback (a1, a2, a3);
  }
protected:
  Callback callback;
 private:
  ~ClosureF3() {}
};
#endif
/* ClosureF* */
template<typename R0 BIRNET_SIG_c_TYPENAMES >
struct BIRNET_SIG_NAME (ClosureF) : BIRNET_SIG_NAME (Closure) <R0 BIRNET_SIG_c_TYPES> {
  typedef R0 (*Callback) ( BIRNET_SIG_TYPES );
  BIRNET_SIG_NAME (ClosureF) (Callback c) : callback (c) {}
  R0 operator() ( BIRNET_SIG_TYPED_VARS )
  {
    return callback ( BIRNET_SIG_VARS );
  }
 protected:
  Callback callback;
 private:
  BIRNET_SIG_NAME (~ClosureF)() {}
 friend void FIXME_dummy_friend_for_gcc33();
};

/* --- ClosureM*: method closures --- */
#if 0 // ClosureM3 sample
template<class Class, typename R0, typename A1, typename A2, typename A3>
struct ClosureM3 : Closure3 <R0, A1, A2, A3> {
  typedef R0 (Class::*Method) (A1, A2, A3);
  ClosureM3 (Class *p, Method m) : instance (p), method (m) {}
  R0 operator() (A1 a1, A2 a2, A3 a3)
  {
    return (instance->*method) (a1, a2, a3);
  }
 protected:
  Class *instance;
  Method method;
 private:
  ~ClosureM3() {}
};
#endif
/* ClosureM* */
template<class Class, typename R0 BIRNET_SIG_c_TYPENAMES >
struct BIRNET_SIG_NAME (ClosureM) : BIRNET_SIG_NAME (Closure) <R0 BIRNET_SIG_c_TYPES> {
  typedef R0 (Class::*Method) ( BIRNET_SIG_TYPES );
  BIRNET_SIG_NAME (ClosureM) (Class *p, Method m) : instance (p), method (m) {}
  R0 operator() ( BIRNET_SIG_TYPED_VARS )
  {
    return (instance->*method) ( BIRNET_SIG_VARS );
  }
 protected:
  Class *instance;
  Method method;
 private:
  BIRNET_SIG_NAME (~ClosureM)() {}
 friend void FIXME_dummy_friend_for_gcc33();
};

/* --- Slot generation --- */
#if 0 // Slot3 sample
template<typename R0, typename A1, typename A2, typename A3>
struct Slot3 : SlotBase <Closure3 <R0, A1, A2, A3> > {
  typedef Closure3 <R0, A1, A2, A3>  Closure;
  Slot3 (R0 (*callback) (A1, A2, A3)) :
    SlotBase<Closure> (new ClosureF3 <R0, A1, A2, A3> (callback))
  {}
  template<class Class>
  Slot3 (Class *instance, R0 (Class::*method) (A1, A2, A3)) :
    SlotBase<Closure> (new ClosureM3 <Class, R0, A1, A2, A3> (instance, method))
  {}
};
template<typename R0, typename A1, typename A2, typename A3>
Slot3<R0, A1, A2, A3>
slot (R0 (*callback) (A1, A2, A3))
{
  return Slot3<R0, A1, A2, A3> (callback);
}
template<class Class, typename R0, typename A1, typename A2, typename A3>
Slot3<R0, A1, A2, A3>
slot (Class *p, R0 (Class::*method) (A1, A2, A3))
{
  return Slot3<R0, A1, A2, A3> (p, method);
}
#endif
/* Slot* */
template<typename R0 BIRNET_SIG_c_TYPENAMES >
struct BIRNET_SIG_NAME (Slot) : SlotBase <BIRNET_SIG_NAME (Closure) <R0 BIRNET_SIG_c_TYPES > > {
  typedef BIRNET_SIG_NAME (Closure) <R0 BIRNET_SIG_c_TYPES >  Closure;
  BIRNET_SIG_NAME (Slot) (R0 (*callback) ( BIRNET_SIG_TYPES )) :
    SlotBase<Closure> (new BIRNET_SIG_NAME (ClosureF) <R0 BIRNET_SIG_c_TYPES> (callback))
  {}
  template<class Class>
  BIRNET_SIG_NAME (Slot) (Class *instance, R0 (Class::*method) ( BIRNET_SIG_TYPES )) :
    SlotBase<Closure> (new BIRNET_SIG_NAME (ClosureM) <Class, R0 BIRNET_SIG_c_TYPES> (instance, method))
  {}
};
/* function slot constructor */
template<typename R0 BIRNET_SIG_c_TYPENAMES >
BIRNET_SIG_NAME (Slot) <R0 BIRNET_SIG_c_TYPES>
slot (R0 (*callback) ( BIRNET_SIG_TYPES ))
{
  return BIRNET_SIG_NAME (Slot) <R0 BIRNET_SIG_c_TYPES > (callback);
}
/* method slot constructor */
template<class Class, typename R0 BIRNET_SIG_c_TYPENAMES >
BIRNET_SIG_NAME (Slot) <R0 BIRNET_SIG_c_TYPES>
slot (Class *p, R0 (Class::*method) ( BIRNET_SIG_TYPES ))
{
  return BIRNET_SIG_NAME (Slot) <R0 BIRNET_SIG_c_TYPES > (p, method);
}

/* --- SignalEmittable generation --- */
#if 0 // SignalEmittable3 sample
template<typename Closure, typename R0, typename A1, typename A2, typename A3, template<typename> class Accu>
struct SignalEmittable3 : SignalBase<Closure>
{
  typedef SignalBase<Closure>           SignalBase;
  typedef typename SignalBase::Iterator Iterator;
  inline R0 emit (A1 a1, A2 a2, A3 a3)
  { // pseudo code:
    for (Iterator it = this->closures.begin(); it != this->closures.end(); it++)
      (*it)->operator() (a1, a2, a3);
    return 0;
  }
};
#endif
/* SignalEmittable* */
template<typename Closure, typename R0 BIRNET_SIG_c_TYPENAMES, template<typename> class Accu>
struct BIRNET_SIG_NAME (SignalEmittable) : SignalBase<Closure>
{
  typedef SignalBase<Closure>           SignalBase;
  typedef typename SignalBase::Iterator Iterator;
  inline R0 emit ( BIRNET_SIG_TYPED_VARS )
  {
    if (this->referencable)
      this->referencable->ref();
    R0 result = R0();    // default initialization
    accu.init (result);  // custom setup
    for (Iterator it = this->closures.begin(); it != this->closures.end(); it++)
      if (!accu.collect (result, (*it)->operator() (BIRNET_SIG_VARS)))
        break;
    if (this->referencable)
      this->referencable->unref();
    return result;
  }
  explicit BIRNET_SIG_NAME (SignalEmittable) (ReferencableBase *referencable) :
    SignalBase (referencable)
  {}
 private:
  Accu<R0> accu;
};
/* SignalEmittable* for void returns */
template<typename Closure BIRNET_SIG_c_TYPENAMES, template<typename> class Accu>
struct BIRNET_SIG_NAME (SignalEmittable) <Closure, void BIRNET_SIG_c_TYPES, Accu> : SignalBase<Closure>
{
  typedef SignalBase<Closure>           SignalBase;
  typedef typename SignalBase::Iterator Iterator;
  inline void emit ( BIRNET_SIG_TYPED_VARS )
  {
    if (this->referencable)
      this->referencable->ref();
    for (Iterator it = this->closures.begin(); it != this->closures.end(); it++)
      (*it)->operator() (BIRNET_SIG_VARS);
    if (this->referencable)
      this->referencable->unref();
  }
  explicit BIRNET_SIG_NAME (SignalEmittable) (ReferencableBase *referencable) :
    SignalBase (referencable)
  {}
};

/* --- Signal generation --- */
#if 0 // Signal3 sample
template<typename R0, typename A1, typename A2, typename A3, template<typename> class Accu = AccumulatorLast>
struct Signal3 : SignalEmittable3 < Closure3 <R0, A1, A2, A3>, R0, A1, A2, A3, Accu>
{
  typedef Closure3 <R0, A1, A2, A3>     Closure;
  typedef Slot3 <R0, A1, A2, A3>        Slot;
  typedef SignalBase <Closure>          SignalBase;
  typedef typename SignalBase::Iterator Iterator;
  inline void connect (const Slot &s)
  {
    connect_closure (s.get_closure());
  }
  template<class Class> inline void
  connect (Class      *p,
           R0 (Class::*method) (A1, A2, A3))
  {
    connect_closure (new ClosureM3 <Class, R0, A1, A2, A3> (p, method));
  }
  /* ... */
};
#endif
/* Signal* */
template<typename R0 BIRNET_SIG_c_TYPENAMES, template<typename> class Accu = AccumulatorLast>
struct BIRNET_SIG_NAME (Signal) : BIRNET_SIG_NAME (SignalEmittable) < BIRNET_SIG_NAME (Closure) <R0 BIRNET_SIG_c_TYPES>, R0 BIRNET_SIG_c_TYPES, Accu>
{
  typedef BIRNET_SIG_NAME (Closure)<R0 BIRNET_SIG_c_TYPES>                        Closure;
  typedef BIRNET_SIG_NAME (Slot)<R0 BIRNET_SIG_c_TYPES>                           Slot;
  typedef SignalBase <Closure>                                                    SignalBase;
  typedef typename SignalBase::Iterator                                           Iterator;
  typedef BIRNET_SIG_NAME (SignalEmittable)<Closure, R0 BIRNET_SIG_c_TYPES, Accu> SignalEmittable;
  template<class Class> explicit
  BIRNET_SIG_NAME (Signal) (Class *referencable) :
    SignalEmittable (new ReferencableWrapper<Class> (*referencable))
  {
    assert (referencable != NULL);
  }
  template<class Class> explicit
  BIRNET_SIG_NAME (Signal) (Class      *referencable,
                            R0 (Class::*method) ( BIRNET_SIG_TYPES )) :
    SignalEmittable (new ReferencableWrapper<Class> (*referencable))
  {
    assert (referencable != NULL);
    connect_closure (new BIRNET_SIG_NAME (ClosureM) <Class, R0 BIRNET_SIG_c_TYPES> (referencable, method));
  }
  inline void connect (Closure *c)
  {
    connect_closure (c);
  }
  inline void connect (const Slot &s)
  {
    connect_closure (s.get_closure());
  }
  template<class Class> inline void
  connect (Class      *obj,
           R0 (Class::*method) ( BIRNET_SIG_TYPES ))
  {
    connect_closure (new BIRNET_SIG_NAME (ClosureM) <Class, R0 BIRNET_SIG_c_TYPES> (obj, method));
    // connect (BIRNET_SIG_NAME (Slot) <R0 BIRNET_SIG_c_TYPES> (obj, method));
  }
  BIRNET_SIG_NAME (Signal)&
  operator+= (Closure *c)
  {
    connect_closure (c);
    return *this;
  }
  BIRNET_SIG_NAME (Signal)&
  operator+= (const Slot &s)
  {
    connect_closure (s.get_closure());
    return *this;
  }
  typedef R0 (*Callback) ( BIRNET_SIG_TYPES );
  BIRNET_SIG_NAME (Signal)&
  operator+= (const Callback c)
  {
    connect (c);
    return *this;
  }
};
#endif
