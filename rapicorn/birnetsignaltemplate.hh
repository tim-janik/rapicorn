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

/* this file is used to generate birnetsignalvariants.hh by mksignals.sh.
 * therein, certain phrases like "typename A1, typename A2, typename A3" are
 * rewritten into 0, 1, 2, ... 16 argument variants. so make sure all phrases
 * involving the signal argument count match those from mksignals.sh.
 */

/* --- Handler (basis signature) --- */
template<typename R0, typename A1, typename A2, typename A3> struct Emission3;
template<typename R0, typename A1, typename A2, typename A3>
struct Handler3 : public SignalBase::Link {
  /* signature type for all signal handlers, used for handler invocations by Emission */
  virtual R0 operator() (Emission3<R0, A1, A2, A3> &base_emission) = 0;
};

/* --- Emission --- */
template<typename R0, typename A1, typename A2, typename A3>
struct Emission3 : public EmissionBase {
  typedef Handler3<R0, A1, A2, A3> Handler;
  R0 m_result; A1 m_a1; A2 m_a2; A3 m_a3;
  void *m_emitter;
  Handler *m_last_handler;
  Emission3 (void *emitter, A1 a1, A2 a2, A3 a3) :
    m_result(), m_a1 (a1), m_a2 (a2), m_a3 (a3), m_emitter (emitter), m_last_handler (NULL)
  {}
  /* call Handler and store result, so handler templates need no <void> specialization */
  R0 call (Handler *handler)
  {
    if (m_last_handler != handler && handler->callable)
      {
        m_result = (*handler) (*this);
        m_last_handler = handler;
      }
    return m_result;
  }
};
template<typename A1, typename A2, typename A3>
struct Emission3 <void, A1, A2, A3> : public EmissionBase {
  typedef Handler3 <void, A1, A2, A3> Handler;
  void *m_emitter; A1 m_a1; A2 m_a2; A3 m_a3;
  Emission3 (void *emitter, A1 a1, A2 a2, A3 a3) :
    m_emitter (emitter), m_a1 (a1), m_a2 (a2), m_a3 (a3)
  {}
  /* call the handler and ignore result, so handler templates need no <void> specialization */
  void call (Handler *handler) { if (handler->callable) (*handler) (*this); }
};

/* --- Handlers with emitter --- */
template<typename R0, class Emitter, typename A1, typename A2, typename A3>
class HandlerE3 : public Handler3 <R0, A1, A2, A3> {
  /* signature type for all signal handlers with emitters, used by emitter slots */
};
template<typename R0, class Emitter, typename A1, typename A2, typename A3>
class HandlerEF3 : public HandlerE3 <R0, Emitter, A1, A2, A3> {
  typedef R0 (*Callback) (Emitter&, A1, A2, A3);
  typedef Emission3 <R0, A1, A2, A3> Emission;
  friend void FIXME_dummy_friend_for_gcc33();
  Callback callback;
  virtual R0 operator() (Emission &emission)
  { return callback (*static_cast<Emitter*> (emission.m_emitter), emission.m_a1, emission.m_a2, emission.m_a3); }
  ~HandlerEF3() {}
public:
  HandlerEF3 (Callback c) :
    callback (c)
  {}
};
template<class Class, typename R0, class Emitter, typename A1, typename A2, typename A3>
class HandlerEM3 : public HandlerE3 <R0, Emitter, A1, A2, A3> {
  typedef R0 (Class::*Method) (Emitter&, A1, A2, A3);
  typedef Emission3 <R0, A1, A2, A3> Emission;
  friend void FIXME_dummy_friend_for_gcc33();
  Class *instance;
  Method method;
  virtual R0 operator() (Emission &emission)
  { return (instance->*method) (*static_cast<Emitter*> (emission.m_emitter), emission.m_a1, emission.m_a2, emission.m_a3); }
  ~HandlerEM3() {}
public:
  HandlerEM3 (Class &obj, Method m) :
    instance (&obj), method (m)
  {}
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
template<class Emitter, typename R0, typename A1, typename A2, typename A3, class Collector>
struct SignalEmittable3 : SignalBase {
  typedef Emission3 <R0, A1, A2, A3>      Emission;
  typedef Handler3 <R0, A1, A2, A3>       Handler;
  typedef typename Collector::result_type Result;
  struct Iterator : public SignalBase::Iterator<Emission> {
    Iterator (Emission &emission, Link *link) : SignalBase::Iterator<Emission> (emission, link) {}
    R0 operator* () { return this->emission.call (static_cast<Handler*> (this->current)); }
  };
  inline Result emit (A1 a1, A2 a2, A3 a3)
  {
    if (this->referencable)
      this->referencable->ref();
    Emission emission (NULL, a1, a2, a3); // FIXME: emitter
    Iterator it (emission, &start), last (emission, &start);
    ++it; /* walk from start to first */
    Collector collector;
    Result result = collector (it, last);
    if (this->referencable)
      this->referencable->unref();
    return result;
  }
  explicit SignalEmittable3 (ReferencableBase *referencable) :
    SignalBase (referencable)
  {}
};
/* SignalEmittable3 for void returns */
template<class Emitter, typename A1, typename A2, typename A3, class Collector>
struct SignalEmittable3 <Emitter, void, A1, A2, A3, Collector> : SignalBase {
  typedef Emission3 <void, A1, A2, A3> Emission;
  typedef Handler3 <void, A1, A2, A3>  Handler;
  struct Iterator : public SignalBase::Iterator<Emission> {
    Iterator (Emission &emission, Link *link) : SignalBase::Iterator<Emission> (emission, link) {}
    void operator* () { return this->emission.call (static_cast<Handler*> (this->current)); }
  };
  inline void emit (A1 a1, A2 a2, A3 a3)
  {
    if (this->referencable)
      this->referencable->ref();
    Emission emission (NULL, a1, a2, a3); // FIXME: emitter
    Iterator it (emission, &start), last (emission, &start);
    ++it; /* walk from start to first */
    Collector collector;
    collector (it, last);
    if (this->referencable)
      this->referencable->unref();
  }
  explicit SignalEmittable3 (ReferencableBase *referencable) :
    SignalBase (referencable)
  {}
};

/* --- Signal3 --- */
/* Signal* */
template<class Emitter, typename R0, typename A1, typename A2, typename A3, class Collector = CollectorDefault<R0> >
struct Signal3 : SignalEmittable3<Emitter, R0, A1, A2, A3, Collector>
{
  typedef Emission3<R0, A1, A2, A3>                            Emission;
  typedef SlotE3<R0, Emitter, A1, A2, A3>                      Slot;
  typedef typename SignalBase::Link                            Link;
  typedef SignalEmittable3<Emitter, R0, A1, A2, A3, Collector> SignalEmittable;
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
  inline void connect (const Slot &s)   { connect_link (s.get_handler()); }
  template<class Class>
  inline void connect (Class   *obj, R0 (Class::*method) (Emitter&, A1, A2, A3))
  {
    connect_handler (new HandlerEM3 <Class, R0, Emitter, A1, A2, A3> (obj, method));
    connect (SlotE3<R0, Emitter, A1, A2, A3> (obj, method)); // FIXME: remove
  }
  typedef R0  (*Callback) (Emitter&, A1, A2, A3);
  Signal3&      operator+= (const Slot &s)      { connect_link (s.get_handler()); return *this; }
  Signal3&      operator+= (const Callback c)   { connect (c); return *this; }
  BIRNET_PRIVATE_CLASS_COPY (Signal3);
};

/* --- Signal<> --- */
template<class Emitter, typename R0, typename A1, typename A2, typename A3, class Collector>
struct Signal<Emitter, R0 (A1, A2, A3), Collector> : Signal3<Emitter, R0, A1, A2, A3, Collector>
{
  typedef Signal3<Emitter, R0, A1, A2, A3, Collector> Signal3;
  explicit Signal (Emitter &referencable) :
    Signal3 (referencable)
    {}
  template<class Class>
  explicit Signal (Emitter &referencable, R0 (Class::*method) (Emitter&, A1, A2, A3)) :
    Signal3 (referencable, method)
    {}
  BIRNET_PRIVATE_CLASS_COPY (Signal);
};

