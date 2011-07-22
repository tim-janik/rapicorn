/* RapicornSignal
 * Copyright (C) 2005-2006 Tim Janik
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
#ifndef __RAPICORN_SIGNAL_HH__
#define __RAPICORN_SIGNAL_HH__

#include <rcore/utilities.hh>

namespace Rapicorn {
namespace Signals {

/* --- EmissionBase --- */
struct EmissionBase : protected NonCopyable {
  bool restart_emission;
  bool stop_emission;
  EmissionBase() :
    restart_emission (true),
    stop_emission (false)
  {}
};

/* --- TrampolineLink --- */
typedef ptrdiff_t ConId;
class TrampolineLink : public ReferenceCountable, protected NonCopyable {
  friend    class SignalBase;
  TrampolineLink *next, *prev;
  uint            m_callable : 1;
  uint            m_with_emitter : 1;
  uint            m_linking_owner : 1;
  inline void     with_emitter  (bool b) { m_with_emitter = b; }
protected:
  inline void     callable      (bool b) { m_callable = b; }
  inline ConId    con_id        () const { return (ptrdiff_t) this; }
public:
  void            owner_ref     ();
  inline bool     callable      () { return m_callable; }
  inline bool     with_emitter  () { return m_with_emitter; }
  virtual bool    operator==    (const TrampolineLink &other) const = 0;
  virtual        ~TrampolineLink();
  void            unlink        ();
  TrampolineLink (uint allow_stack_magic = 0) :
    ReferenceCountable (allow_stack_magic),
    next (NULL), prev (NULL), m_callable (true), m_with_emitter (false), m_linking_owner (false)
  {}
};

/* --- SignalBase --- */
class SignalBase : protected NonCopyable {
  friend             class SignalProxyBase;
  class EmbeddedLink : public TrampolineLink {
    virtual bool           operator==            (const TrampolineLink &other) const;
    virtual void           delete_this           (); // NOP for embedded link
  public:
    explicit               EmbeddedLink          () : TrampolineLink (0xbadbad) {}
    void                   check_last_ref        () const { RAPICORN_ASSERT (ref_count() == 1); }
  };
  EmbeddedLink             start;
protected:
  template<class Emission> struct Iterator;
  inline TrampolineLink*   start_link            () { return &start; }
  ConId                    connect_link          (TrampolineLink       *link,
                                                  bool                  with_emitter = false);
  uint                     disconnect_equal_link (const TrampolineLink &link,
                                                  bool                  with_emitter = false);
  uint                     disconnect_link_id    (ConId                 id);
  virtual void             connections_changed   (bool                  hasconnections) {}
public:
  virtual                 ~SignalBase            ();
  inline                   SignalBase            () { start.next = start.prev = &start; start.ref_sink(); }
};

// === SignalProxyBase ===
class SignalProxyBase {
  SignalBase   *m_signal;
protected:
  ConId         connect_link          (TrampolineLink *link, bool with_emitter = false);
  uint          disconnect_equal_link (const TrampolineLink &link, bool with_emitter = false);
  uint          disconnect_link_id    (ConId                 id);
  SignalProxyBase&    operator=       (const SignalProxyBase &other);
  explicit      SignalProxyBase       (const SignalProxyBase &other);
  explicit      SignalProxyBase       (SignalBase &signal);
  virtual      ~SignalProxyBase       () {}
public:
  void          divorce               ();
};

/* --- Signal Iterator --- */
template<class Emission>
class SignalBase::Iterator : protected NonCopyable {
  Iterator& operator= (const Iterator&);
public:
  Emission &emission;
  TrampolineLink *current, *start;
  Iterator (Emission       &_emission,
            TrampolineLink *last) :
    emission (_emission),
    current (ref (last)),
    start (ref (last))
  {}
  Iterator (const Iterator &dup) :
    emission (dup.emission),
    current (ref (dup.current)),
    start (ref (dup.start))
  {}
  ~Iterator()
  {
    current->unref();
    current = NULL;
    start->unref();
    start = NULL;
  }
  Iterator& operator++ ()
  {
    if (emission.restart_emission)
      {
        current->unref();
        current = start->next;
        current->ref();
        emission.restart_emission = false;
      }
    else
      do
        {
          TrampolineLink *next = current->next;
          next->ref();
          current->unref();
          current = next;
        }
      while (!current->callable() && current != start);
    if (emission.stop_emission)
      {
        current->unref();
        current = start;
        current->ref();
      }
    return *this;
  }
  void operator++ (int)
  {
    /* not supporting iterator copies for post-increment */
    operator++();
  }
  bool operator!= (const Iterator &b) const
  {
    return current != b.current || &emission != &b.emission;
  }
  bool operator== (const Iterator &b) const
  {
    return !operator!= (b);
  }
};

/* --- Collector --- */
template<typename Result>
struct CollectorVector : protected NonCopyable {
  typedef std::vector<Result> result_type;
  template<typename InputIterator>
  result_type operator() (InputIterator begin, InputIterator end)
  {
    result_type result = result_type();
    while (begin != end)
      {
        result.push_back (*begin);
        ++begin;
      }
    return result;
  }
};
template<typename Result>
struct CollectorLast : protected NonCopyable {
  typedef Result result_type;
  template<typename InputIterator>
  Result operator() (InputIterator begin, InputIterator end)
  {
    Result result = Result();
    while (begin != end)
      {
        result = *begin;
        ++begin;
      }
    return result;
  }
};
template<typename Result>
struct CollectorDefault : CollectorLast<Result> {};
template<>
struct CollectorDefault<void> : protected NonCopyable {
  typedef void result_type;
  template<typename InputIterator>
  void operator() (InputIterator begin, InputIterator end)
  {
    while (begin != end)
      {
        *begin;
        ++begin;
      }
  }
};
template<typename Result>
struct CollectorSum : protected NonCopyable {
  typedef Result result_type;
  template<typename InputIterator>
  Result operator() (InputIterator begin, InputIterator end)
  {
    Result result = Result();
    while (begin != end)
      {
        result += *begin;
        ++begin;
      }
    return result;
  }
};
template<typename Result>
struct CollectorWhile0 : protected NonCopyable {
  typedef Result result_type;
  template<typename InputIterator>
  Result operator() (InputIterator begin, InputIterator end)
  {
    Result result = Result();   // result = 0
    while (begin != end)
      {
        result = *begin;
        if (result)             // result != 0
          break;
        ++begin;
      }
    return result;
  }
};
template<typename Result>
struct CollectorUntil0 : protected NonCopyable {
  typedef Result result_type;
  template<typename InputIterator>
  Result operator() (InputIterator begin, InputIterator end)
  {
    Result result = !Result();  // result = !0
    while (begin != end)
      {
        result = *begin;
        if (!result)            // result == 0
          break;
        ++begin;
      }
    return result;
  }
};

/* --- ScopeReference --- */
template<class Instance, typename Mark>
class ScopeReference : protected NonCopyable {
  Instance &m_instance;
public:
  ScopeReference  (Instance &instance) : m_instance (instance) { m_instance.ref(); }
  ~ScopeReference ()                                           { m_instance.unref(); }
};
struct ScopeReferenceFinalizationMark : CollectorDefault<void> {};
template<class Instance>
class ScopeReference<Instance, ScopeReferenceFinalizationMark> : protected NonCopyable {
  Instance &m_instance;
public:
  ScopeReference  (Instance &instance) : m_instance (instance) { RAPICORN_ASSERT (m_instance.finalizing() == true); }
  ~ScopeReference ()                                           { RAPICORN_ASSERT (m_instance.finalizing() == true); }
};

/* --- SlotBase --- */
class SlotBase : protected NonCopyable {
protected:
  TrampolineLink *m_link;
  void
  set_trampoline (TrampolineLink *cl)
  {
    if (cl)
      cl->ref_sink();
    if (m_link)
      m_link->unref();
    m_link = cl;
  }
  ~SlotBase()
  { set_trampoline (NULL); }
public:
  SlotBase (TrampolineLink *link) :
    m_link (NULL)
  { set_trampoline (link); }
  SlotBase (const SlotBase &src) :
    m_link (NULL)
  { *this = src; }
  SlotBase&
  operator= (const SlotBase &src)
  {
    set_trampoline (src.m_link);
    return *this;
  }
  TrampolineLink*
  get_trampoline_link() const
  { return m_link; }
};

/* --- Signature --- */
template<typename>                      struct Signature;
template<typename R0>                   struct Signature<R0 ()>   { typedef R0 result_type; };
template<typename R0, typename A1>      struct Signature<R0 (A1)> { typedef A1 argument1_type;  typedef R0 result_type; };
template<typename R0, typename A1, typename A2>
struct Signature<R0 (A1, A2)> {
  typedef A1 argument1_type;    typedef A2 argument2_type;      typedef R0 result_type;
};
template<typename R0, typename A1, typename A2, typename A3>
struct Signature<R0 (A1, A2, A3)> {
  typedef A1 argument1_type;    typedef A2 argument2_type;      typedef A3 argument3_type;      typedef R0 result_type;
};
template<typename R0, typename A1, typename A2, typename A3, typename A4>
struct Signature<R0 (A1, A2, A3, A4)> {
  typedef A1 argument1_type;    typedef A2 argument2_type;      typedef A3 argument3_type;      typedef A4 argument4_type;
  typedef R0 result_type;
};
template<typename R0, typename A1, typename A2, typename A3, typename A4, typename A5>
struct Signature<R0 (A1, A2, A3, A4, A5)> {
  typedef A1 argument1_type;    typedef A2 argument2_type;      typedef A3 argument3_type;      typedef A4 argument4_type;
  typedef A5 argument5_type;    typedef R0 result_type;
};
template<typename R0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
struct Signature<R0 (A1, A2, A3, A4, A5, A6)> {
  typedef A1 argument1_type;    typedef A2 argument2_type;      typedef A3 argument3_type;      typedef A4 argument4_type;
  typedef A5 argument5_type;    typedef A6 argument6_type;      typedef R0 result_type;
};
template<typename R0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
struct Signature<R0 (A1, A2, A3, A4, A5, A6, A7)> {
  typedef A1 argument1_type;    typedef A2 argument2_type;      typedef A3 argument3_type;      typedef A4 argument4_type;
  typedef A5 argument5_type;    typedef A6 argument6_type;      typedef A7 argument7_type;      typedef R0 result_type;
};
template<typename R0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
struct Signature<R0 (A1, A2, A3, A4, A5, A6, A7, A8)> {
  typedef A1 argument1_type;    typedef A2 argument2_type;      typedef A3 argument3_type;      typedef A4 argument4_type;
  typedef A5 argument5_type;    typedef A6 argument6_type;      typedef A7 argument7_type;      typedef A8 argument8_type;
  typedef R0 result_type;
};
template<typename R0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8,
         typename A9>
struct Signature<R0 (A1, A2, A3, A4, A5, A6, A7, A8, A9)> {
  typedef A1 argument1_type;    typedef A2 argument2_type;      typedef A3 argument3_type;      typedef A4 argument4_type;
  typedef A5 argument5_type;    typedef A6 argument6_type;      typedef A7 argument7_type;      typedef A8 argument8_type;
  typedef A9 argument9_type;    typedef R0 result_type;
};
template<typename R0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8,
         typename A9, typename A10>
struct Signature<R0 (A1, A2, A3, A4, A5, A6, A7, A8, A9, A10)> {
  typedef A1 argument1_type;    typedef A2 argument2_type;      typedef A3 argument3_type;      typedef A4 argument4_type;
  typedef A5 argument5_type;    typedef A6 argument6_type;      typedef A7 argument7_type;      typedef A8 argument8_type;
  typedef A9 argument9_type;    typedef A10 argument10_type;    typedef R0 result_type;
};
template<typename R0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8,
         typename A9, typename A10, typename A11>
struct Signature<R0 (A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11)> {
  typedef A1 argument1_type;    typedef A2 argument2_type;      typedef A3 argument3_type;      typedef A4 argument4_type;
  typedef A5 argument5_type;    typedef A6 argument6_type;      typedef A7 argument7_type;      typedef A8 argument8_type;
  typedef A9 argument9_type;    typedef A10 argument10_type;    typedef A11 argument11_type;    typedef R0 result_type;
};
template<typename R0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8,
         typename A9, typename A10, typename A11, typename A12>
struct Signature<R0 (A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12)> {
  typedef A1 argument1_type;    typedef A2 argument2_type;      typedef A3 argument3_type;      typedef A4 argument4_type;
  typedef A5 argument5_type;    typedef A6 argument6_type;      typedef A7 argument7_type;      typedef A8 argument8_type;
  typedef A9 argument9_type;    typedef A10 argument10_type;    typedef A11 argument11_type;    typedef A12 argument12_type;
  typedef R0 result_type;
};
template<typename R0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8,
         typename A9, typename A10, typename A11, typename A12, typename A13>
struct Signature<R0 (A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13)> {
  typedef A1 argument1_type;    typedef A2 argument2_type;      typedef A3 argument3_type;      typedef A4 argument4_type;
  typedef A5 argument5_type;    typedef A6 argument6_type;      typedef A7 argument7_type;      typedef A8 argument8_type;
  typedef A9 argument9_type;    typedef A10 argument10_type;    typedef A11 argument11_type;    typedef A12 argument12_type;
  typedef A13 argument13_type;  typedef R0 result_type;
};
template<typename R0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8,
         typename A9, typename A10, typename A11, typename A12, typename A13, typename A14>
struct Signature<R0 (A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14)> {
  typedef A1 argument1_type;    typedef A2 argument2_type;      typedef A3 argument3_type;      typedef A4 argument4_type;
  typedef A5 argument5_type;    typedef A6 argument6_type;      typedef A7 argument7_type;      typedef A8 argument8_type;
  typedef A9 argument9_type;    typedef A10 argument10_type;    typedef A11 argument11_type;    typedef A12 argument12_type;
  typedef A13 argument13_type;  typedef A14 argument14_type;    typedef R0 result_type;
};
template<typename R0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8,
         typename A9, typename A10, typename A11, typename A12, typename A13, typename A14, typename A15>
struct Signature<R0 (A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15)> {
  typedef A1 argument1_type;    typedef A2 argument2_type;      typedef A3 argument3_type;      typedef A4 argument4_type;
  typedef A5 argument5_type;    typedef A6 argument6_type;      typedef A7 argument7_type;      typedef A8 argument8_type;
  typedef A9 argument9_type;    typedef A10 argument10_type;    typedef A11 argument11_type;    typedef A12 argument12_type;
  typedef A13 argument13_type;  typedef A14 argument14_type;    typedef A15 argument15_type;    typedef R0 result_type;
};
template<typename R0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8,
         typename A9, typename A10, typename A11, typename A12, typename A13, typename A14, typename A15, typename A16>
struct Signature<R0 (A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16)> {
  typedef A1 argument1_type;    typedef A2 argument2_type;      typedef A3 argument3_type;      typedef A4 argument4_type;
  typedef A5 argument5_type;    typedef A6 argument6_type;      typedef A7 argument7_type;      typedef A8 argument8_type;
  typedef A9 argument9_type;    typedef A10 argument10_type;    typedef A11 argument11_type;    typedef A12 argument12_type;
  typedef A13 argument13_type;  typedef A14 argument14_type;    typedef A15 argument15_type;    typedef A16 argument16_type;
  typedef R0 result_type;
};

template <class Emitter,
          typename SignalSignature,
          class Collector = CollectorDefault<typename Signature<SignalSignature>::result_type>,
          class SignalAncestor = SignalBase
          > struct Signal;

template <class Emitter,
          typename SignalSignature
          > struct SignalProxy;

/* --- Casting --- */
template<class TrampolineP> TrampolineP
trampoline_cast (TrampolineLink *link)
{
  if (1) // could be disabled for performance
    return dynamic_cast<TrampolineP> (link);
  else  // cheap cast
    return reinterpret_cast<TrampolineP> (link);
}

/* --- Trampoline + Slot + Signal generation --- */
#include <rcore/rapicornsignalvariants.hh> // contains multiple versions of "rapicornsignaltemplate.hh"

/* --- predefined slots --- */
typedef Slot0<void, void> VoidSlot;
typedef Slot0<bool, void> BoolSlot;

/* --- predefined signals --- */
template<class Emitter>
class SignalFinalize : public Signal0 <Emitter, void, ScopeReferenceFinalizationMark> {
  typedef Signal0 <Emitter, void, ScopeReferenceFinalizationMark> SignalVariant;
public:
  explicit SignalFinalize (Emitter &emitter)                             : SignalVariant (emitter) {}
  explicit SignalFinalize (Emitter &emitter, void (Emitter::*method) ()) : SignalVariant (emitter, method) {}
};

template<class Emitter>
class SignalVoid : public Signal0 <Emitter, void> {
  typedef Signal0 <Emitter, void> SignalVariant;
public:
  explicit SignalVoid (Emitter &emitter)                                 : SignalVariant (emitter) {}
  explicit SignalVoid (Emitter &emitter, void (Emitter::*method) (void)) : SignalVariant (emitter, method) {}
};

} // Signals

/* --- Rapicorn::Signal imports --- */
using Signals::CollectorDefault;
using Signals::CollectorWhile0;
using Signals::CollectorUntil0;
using Signals::CollectorVector;
using Signals::CollectorLast;
using Signals::CollectorSum;
using Signals::VoidSlot;
using Signals::BoolSlot;
using Signals::SignalProxy;
using Signals::SignalFinalize;
using Signals::SignalVoid;
using Signals::Signal;
using Signals::slot;

} // Rapicorn

#endif  /* __RAPICORN_SIGNAL_HH__ */
