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
template<typename R0, typename A1, typename A2, typename A3>
struct Handler3 : public SignalBase::Link {
  /* signature type for all signal handlers, used for handler invocations by Emission */
  virtual R0 operator() (A1 a1, A2 a2, A3 a3) = 0;
};

/* --- Handler (plain) --- */
template<typename R0, typename A1, typename A2, typename A3>
class HandlerF3 : public Handler3 <R0, A1, A2, A3> {
  friend void FIXME_dummy_friend_for_gcc33();
  typedef R0 (*Callback) (A1, A2, A3);
  Callback callback;
  virtual R0 operator() (A1 a1, A2 a2, A3 a3)
  { return callback (a1, a2, a3); }
  ~HandlerF3() {}
public:
  HandlerF3 (Callback c) :
    callback (c)
  {}
};
template<class Class, typename R0, typename A1, typename A2, typename A3>
class HandlerM3 : public Handler3 <R0, A1, A2, A3> {
  friend void FIXME_dummy_friend_for_gcc33();
  typedef R0 (Class::*Method) (A1, A2, A3);
  Class *instance;
  Method method;
  virtual R0 operator() (A1 a1, A2 a2, A3 a3)
  { return (instance->*method) (a1, a2, a3); }
  ~HandlerM3() {}
public:
  HandlerM3 (Class &obj, Method m) :
    instance (&obj), method (m)
  {}
};

/* --- Handler with Data --- */
template<typename R0, typename A1, typename A2, typename A3, typename Data>
class HandlerDF3 : public Handler3 <R0, A1, A2, A3> {
  friend void FIXME_dummy_friend_for_gcc33();
  typedef R0 (*Callback) (A1, A2, A3, Data);
  Callback callback; Data data;
  virtual R0 operator() (A1 a1, A2 a2, A3 a3)
  { return callback (a1, a2, a3, data); }
  ~HandlerDF3() {}
public:
  HandlerDF3 (Callback c, const Data &d) :
    callback (c), data (d)
  {}
};
template<class Class, typename R0, typename A1, typename A2, typename A3, typename Data>
class HandlerDM3 : public Handler3 <R0, A1, A2, A3> {
  friend void FIXME_dummy_friend_for_gcc33();
  typedef R0 (Class::*Method) (A1, A2, A3, Data);
  Class *instance; Method method; Data data;
  virtual R0 operator() (A1 a1, A2 a2, A3 a3)
  { return (instance->*method) (a1, a2, a3, data); }
  ~HandlerDM3() {}
public:
  HandlerDM3 (Class &obj, Method m, const Data &d) :
    instance (&obj), method (m), data (d)
  {}
};

/* --- Slots (Handler wrappers) --- */
template<typename R0, typename A1, typename A2, typename A3, class Emitter = void>
struct Slot3 : SlotBase {
  Slot3 (Handler3<R0, A1, A2, A3>           *handler) : SlotBase (handler) {}
};
/* slot constructors */
template<typename R0, typename A1, typename A2, typename A3> Slot3<R0, A1, A2, A3>
slot (R0 (*callback) (A1, A2, A3))
{
  return Slot3<R0, A1, A2, A3> (new HandlerF3<R0, A1, A2, A3> (callback));
}
template<typename R0, typename A1, typename A2, typename A3, typename Data> Slot3<R0, A1, A2, A3>
slot (R0 (*callback) (A1, A2, A3, Data), const Data &data)
{
  return Slot3<R0, A1, A2, A3> (new HandlerDF3<R0, A1, A2, A3, Data> (callback, data));
}
template<class Class, typename R0, typename A1, typename A2, typename A3> Slot3<R0, A1, A2, A3>
slot (Class &obj, R0 (Class::*method) (A1, A2, A3))
{
  return Slot3<R0, A1, A2, A3> (new HandlerM3<Class, R0, A1, A2, A3> (obj, method));
}
template<class Class, typename R0, typename A1, typename A2, typename A3, typename Data> Slot3<R0, A1, A2, A3>
slot (Class &obj, R0 (Class::*method) (A1, A2, A3), const Data &data)
{
  return Slot3<R0, A1, A2, A3> (new HandlerDM3<Class, R0, A1, A2, A3, Data> (obj, method, data));
}
