// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html

/* this file is used to generate rapicornsignalvariants.hh by mksignals.sh.
 * therein, certain phrases like "typename A1, typename A2, typename A3" are
 * rewritten into 0, 1, 2, ... 16 argument variants. so make sure all phrases
 * involving the signal argument count match those from mksignals.sh.
 */

// === Emission ===
template<class Emitter, typename R0, typename A1, typename A2, typename A3>
struct Emission3 : public EmissionBase {
  typedef Trampoline3<R0, A1, A2, A3>           Trampoline;
  typedef Trampoline4<R0, Emitter&, A1, A2, A3> TrampolineE;
  Emitter *m_emitter;
  R0 m_result; A1 m_a1; A2 m_a2; A3 m_a3;
  TrampolineLink *m_last_link;
  Emission3 (Emitter *emitter, A1 a1, A2 a2, A3 a3) :
    m_emitter (emitter), m_result(), m_a1 (a1), m_a2 (a2), m_a3 (a3), m_last_link (NULL)
  {}
  /* call Trampoline and store result, so trampoline templates need no <void> specialization */
  R0 call (TrampolineLink *link)
  {
    if (m_last_link != link)
      {
        if (link->with_emitter())
          {
            TrampolineE *trampoline = trampoline_cast<TrampolineE*> (link);
            if (trampoline->callable())
              m_result = (*trampoline) (*m_emitter, m_a1, m_a2, m_a3);
          }
        else
          {
            Trampoline *trampoline = trampoline_cast<Trampoline*> (link);
            if (trampoline->callable())
              m_result = (*trampoline) (m_a1, m_a2, m_a3);
          }
        m_last_link = link;
      }
    return m_result;
  }
};
template<class Emitter, typename A1, typename A2, typename A3>
struct Emission3 <Emitter, void, A1, A2, A3> : public EmissionBase {
  typedef Trampoline3<void, A1, A2, A3>           Trampoline;
  typedef Trampoline4<void, Emitter&, A1, A2, A3> TrampolineE;
  Emitter *m_emitter;
  A1 m_a1; A2 m_a2; A3 m_a3;
  Emission3 (Emitter *emitter, A1 a1, A2 a2, A3 a3) :
    m_emitter (emitter), m_a1 (a1), m_a2 (a2), m_a3 (a3)
  {}
  /* call the trampoline and ignore result, so trampoline templates need no <void> specialization */
  void call (TrampolineLink *link)
  {
    if (link->with_emitter())
      {
        TrampolineE *trampoline = trampoline_cast<TrampolineE*> (link);
        if (trampoline->callable())
          (*trampoline) (*m_emitter, m_a1, m_a2, m_a3);
      }
    else
      {
        Trampoline *trampoline = trampoline_cast<Trampoline*> (link);
        if (trampoline->callable())
          (*trampoline) (m_a1, m_a2, m_a3);
      }
  }
};

// === SignalEmittable3 ===
template<class Emitter, typename R0, typename A1, typename A2, typename A3, class Collector, class Ancestor>
struct SignalEmittable3 : public Ancestor {
  typedef Emission3 <Emitter, R0, A1, A2, A3> Emission;
  typedef typename Collector::result_type     Result;
  struct Iterator : public SignalBase::Iterator<Emission> {
    Iterator (Emission &emission, TrampolineLink *link) : SignalBase::Iterator<Emission> (emission, link) {}
    R0 operator* () { return this->emission.call (this->current); }
  };
  explicit SignalEmittable3 (Emitter *emitter) : m_emitter (emitter) {}
  inline Result emit (A1 a1, A2 a2, A3 a3)
  {
    ScopeReference<Emitter, Collector> lref (*m_emitter);
    Emission emission (m_emitter, a1, a2, a3);
    Iterator it (emission, Ancestor::start_link()), last (emission, Ancestor::start_link());
    ++it; /* walk from start to first */
    Collector collector;
    return collector (it, last); // Result may be void
  }
  inline Emitter* emitter () const { return m_emitter; }
private:
  Emitter *m_emitter;
};

// === Signal3 ===
template<class Emitter, typename R0, typename A1, typename A2, typename A3,
         class Collector = CollectorDefault<R0>, class Ancestor  = SignalBase>
struct Signal3 : SignalEmittable3<Emitter, R0, A1, A2, A3, Collector, Ancestor>
{
  typedef Emission3 <Emitter, R0, A1, A2, A3>                             Emission;
  typedef Slot3 <R0, A1, A2, A3>                                          Slot;
  typedef Slot4 <R0, Emitter&, A1, A2, A3>                                SlotE;
  typedef SignalEmittable3 <Emitter, R0, A1, A2, A3, Collector, Ancestor> SignalEmittable;
  explicit Signal3 (Emitter &emitter) :
    SignalEmittable (&emitter)
  { RAPICORN_ASSERT (&emitter != NULL); }
  explicit Signal3 (Emitter &emitter, R0 (Emitter::*method) (A1, A2, A3)) :
    SignalEmittable (&emitter)
  {
    RAPICORN_ASSERT (&emitter != NULL);
    connect (slot (emitter, method));
  }
  inline ConId connect   (const Slot  &s) { return connect_link (s.get_trampoline_link()); }
  inline ConId connect   (const SlotE &s) { return connect_link (s.get_trampoline_link(), true); }
  inline uint disconnect (const Slot  &s) { return disconnect_equal_link (*s.get_trampoline_link()); }
  inline uint disconnect (const SlotE &s) { return disconnect_equal_link (*s.get_trampoline_link(), true); }
  inline uint disconnect (ConId    conid) { return this->disconnect_link_id (conid); }
  Signal3&    operator+= (const Slot  &s) { connect (s); return *this; }
  Signal3&    operator+= (const SlotE &s) { connect (s); return *this; }
  Signal3&    operator+= (R0 (*callback) (A1, A2, A3))            { connect (slot (callback)); return *this; }
  Signal3&    operator+= (R0 (*callback) (Emitter&, A1, A2, A3))  { connect (slot (callback)); return *this; }
  Signal3&    operator-= (const Slot  &s) { disconnect (s); return *this; }
  Signal3&    operator-= (const SlotE &s) { disconnect (s); return *this; }
  Signal3&    operator-= (R0 (*callback) (A1, A2, A3))            { disconnect (slot (callback)); return *this; }
  Signal3&    operator-= (R0 (*callback) (Emitter&, A1, A2, A3))  { disconnect (slot (callback)); return *this; }
};

// === Signal<> ===
template<class Emitter, typename R0, typename A1, typename A2, typename A3, class Collector, class Ancestor>
class Signal<Emitter, R0 (A1, A2, A3), Collector, Ancestor> : public Signal3<Emitter, R0, A1, A2, A3, Collector, Ancestor>
{
  typedef Signal3<Emitter, R0, A1, A2, A3, Collector, Ancestor> SignalVariant;
public:
  explicit Signal (Emitter &emitter) : SignalVariant (emitter) {}
  explicit Signal (Emitter &emitter, R0 (Emitter::*method) (A1, A2, A3)) : SignalVariant (emitter, method) {}
};

// === SignalProxy ===
template<class Emitter, typename R0, typename A1, typename A2, typename A3>
struct SignalProxy<Emitter, R0 (A1, A2, A3)> : SignalProxyBase {
  typedef Slot3 <R0, A1, A2, A3>           Slot;
  typedef Slot4 <R0, Emitter&, A1, A2, A3> SlotE;
  inline ConId connect    (const Slot  &s) { return connect_link (s.get_trampoline_link()); }
  inline ConId connect    (const SlotE &s) { return connect_link (s.get_trampoline_link(), true); }
  inline uint  disconnect (const Slot  &s) { return disconnect_equal_link (*s.get_trampoline_link()); }
  inline uint  disconnect (const SlotE &s) { return disconnect_equal_link (*s.get_trampoline_link(), true); }
  inline uint  disconnect (ConId    conid) { return this->disconnect_link_id (conid); }
  SignalProxy& operator+= (const Slot  &s) { connect (s); return *this; }
  SignalProxy& operator+= (const SlotE &s) { connect (s); return *this; }
  SignalProxy& operator+= (R0 (*callback) (A1, A2, A3))           { connect (slot (callback)); return *this; }
  SignalProxy& operator+= (R0 (*callback) (Emitter&, A1, A2, A3)) { connect (slot (callback)); return *this; }
  SignalProxy& operator-= (const Slot  &s)                        { disconnect (s); return *this; }
  SignalProxy& operator-= (const SlotE &s)                        { disconnect (s); return *this; }
  SignalProxy& operator-= (R0 (*callback) (A1, A2, A3))           { disconnect (slot (callback)); return *this; }
  SignalProxy& operator-= (R0 (*callback) (Emitter&, A1, A2, A3)) { disconnect (slot (callback)); return *this; }
  template<class Collector, class Ancestor>
  SignalProxy (Signal<Emitter, R0 (A1, A2, A3), Collector, Ancestor> &sig) : SignalProxyBase (sig) {}
  SignalProxy (const SignalProxy &other) : SignalProxyBase (other) {}
};
