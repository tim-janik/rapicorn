/* Plic Binding utilities
 * Copyright (C) 2010 Tim Janik
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
#ifndef __PLIC_UTILITIES_HH__
#define __PLIC_UTILITIES_HH__

#include <string>
#include <vector>
#include <memory>               // auto_ptr
#include <stdint.h>             // uint32_t
#include <tr1/memory>           // shared_ptr

namespace Plic {
using std::tr1::bad_weak_ptr;
using std::tr1::enable_shared_from_this;
using std::tr1::shared_ptr;
using std::tr1::weak_ptr;

/* === Auxillary macros === */
#define PLIC_CPP_STRINGIFYi(s)  #s // indirection required to expand __LINE__ etc
#define PLIC_CPP_STRINGIFY(s)   PLIC_CPP_STRINGIFYi (s)
#if     __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)
#define PLIC_UNUSED             __attribute__ ((__unused__))
#define PLIC_DEPRECATED         __attribute__ ((__deprecated__))
#define PLIC_PRINTF(fix, arx)   __attribute__ ((__format__ (__printf__, fix, arx)))
#define PLIC_BOOLi(expr)        __extension__ ({ bool _plic__bool; if (expr) _plic__bool = 1; else _plic__bool = 0; _plic__bool; })
#define PLIC_ISLIKELY(expr)     __builtin_expect (PLIC_BOOLi (expr), 1)
#define PLIC_UNLIKELY(expr)     __builtin_expect (PLIC_BOOLi (expr), 0)
#else   /* !__GNUC__ */
#define PLIC_UNUSED
#define PLIC_DEPRECATED
#define PLIC_PRINTF(fix, arx)
#define PLIC_ISLIKELY(expr)     expr
#define PLIC_UNLIKELY(expr)     expr
#endif
#define PLIC_LIKELY             PLIC_ISLIKELY

/* === Standard Types === */
typedef std::string String;
using std::vector;
typedef int8_t                 int8;
typedef uint8_t                uint8;
typedef int16_t                int16;
typedef uint16_t               uint16;
typedef uint32_t               uint;
typedef signed long long int   int64; // int64_t is a long on AMD64 which breaks printf
typedef unsigned long long int uint64; // int64_t is a long on AMD64 which breaks printf

/* === Constants === */
static const uint64 msgid_ok     = 0x0000000000000000ULL;
static const uint64 msgid_result = 0x1000000000000000ULL; // result bit
static const uint64 msgid_oneway = 0x2000000000000000ULL;
static const uint64 msgid_twoway = 0x3000000000000000ULL; // result bit
static const uint64 msgid_discon = 0x4000000000000000ULL;
static const uint64 msgid_sigcon = 0x5000000000000000ULL; // result bit
static const uint64 msgid_event  = 0x6000000000000000ULL;
//     const uint64 msgid_signal = 0x7000000000000000ULL; // result bit
static const uint64 msgid_error  = 0x8000000000000000ULL; // error bit
inline bool msgid_has_result (const uint64 id) { return id & msgid_result; }
inline bool is_msgid_ok      (const uint64 id) { return id >> 60 == msgid_ok     >> 60; }
inline bool is_msgid_result  (const uint64 id) { return id >> 60 == msgid_result >> 60; }
inline bool is_msgid_oneway  (const uint64 id) { return id >> 60 == msgid_oneway >> 60; }
inline bool is_msgid_twoway  (const uint64 id) { return id >> 60 == msgid_twoway >> 60; }
inline bool is_msgid_discon  (const uint64 id) { return id >> 60 == msgid_discon >> 60; }
inline bool is_msgid_sigcon  (const uint64 id) { return id >> 60 == msgid_sigcon >> 60; }
inline bool is_msgid_event   (const uint64 id) { return id >> 60 == msgid_event  >> 60; }
inline bool is_msgid_error   (const uint64 id) { return id >> 60 == msgid_error  >> 60; }
inline bool msgid_has_error  (const uint64 id) { return id & msgid_error; }

/* === Forward Declarations === */
class SimpleServer;
class Coupler;
union FieldUnion;
class FieldBuffer;
class FieldBufferReader;
typedef FieldBuffer* (*DispatchFunc) (Coupler&);

/* === EventFd === */
class EventFd {
  int fds[2];
public:
  explicit EventFd   ();
  int      open      (); // -errno
  void     wakeup    (); // wakeup polling end
  int      inputfd   (); // fd for POLLIN
  void     flush     (); // clear pending wakeups
  /*Des*/ ~EventFd   ();
};

/* === Callback Wrapper === */
template<class R>
struct Callback0 {
  bool                   callable   () { return p.get() != NULL; }
  R                      operator() () { return (*p) (); }
  template<class F> void set (F f) { if (f) p.reset (new CallFun<F> (f)); else p.reset(); }
  template<class C> void set (C &c, R (C::*m) ()) { p.reset (new CallMem<C> (c, m));}
private:
  struct CallBase {
    virtual  ~CallBase   () {}
    virtual R operator() () = 0;
  };
  template<class C> struct CallMem : CallBase {
    explicit  CallMem    (C &o, R (C::*p) ()) : c (o), m (p) {}
    virtual R operator() () { return (c.*m) (); }
  private: C &c; R (C::*m) ();
  };
  template<class F> struct CallFun : CallBase {
    explicit  CallFun    (F p) : f (p) {}
    virtual R operator() () { return f (); }
  private: F f;
  };
  std::auto_ptr<CallBase> p;
};

/* === TypeHash === */
struct TypeHash {
  static const uint hash_size = 4;
  uint64            qwords[hash_size];
  inline bool       operator< (const TypeHash &rhs) const;
  inline            TypeHash (const uint64 qw[hash_size]);
  inline            TypeHash (uint64 qwa, uint64 qwb, uint64 qwc, uint64 qwd);
  inline uint64     id (uint n) const { return n < hash_size ? qwords[n] : 0; }
  String            to_string() const;
};

/* === SmartHandle === */
class SmartHandle {
  uint64 m_rpc_id;
protected:
  typedef bool (SmartHandle::*_unspecified_bool_type) () const; // non-numeric operator bool() result
  static inline _unspecified_bool_type _unspecified_bool_true () { return &Plic::SmartHandle::_is_null; }
  typedef uint64 RpcId;
  explicit                  SmartHandle ();
  void                      _reset      ();
  void                      _pop_rpc    (Coupler&, FieldBufferReader&);
  void*                     _cast_iface () const;
  inline void*              _void_iface () const;
  void                      _void_iface (void *rpc_id_ptr);
public:
  uint64                    _rpc_id     () const;
  bool                      _is_null    () const;
  virtual                  ~SmartHandle ();
  static SmartHandle*       _rpc_id2obj (uint64 rpc_id);
  static const SmartHandle &None;
};

/* === SimpleServer === */
class SimpleServer {
public:
  explicit             SimpleServer ();
  virtual             ~SimpleServer ();
  virtual uint64       _rpc_id      () const;
  static SimpleServer* _rpc_id2obj  (uint64 rpc_id);
};

/* === EventDispatcher === */
struct EventDispatcher {
  virtual             ~EventDispatcher  ();
  virtual FieldBuffer* dispatch_event   (Coupler&) = 0;
};

/* === DispatchRegistry === */
struct DispatcherEntry {
  uint64            hash_qwords[TypeHash::hash_size];
  DispatchFunc      dispatcher;
};
class DispatcherRegistry {
public:
  template<class T, size_t S>
  inline                DispatcherRegistry  (T (&)[S]);
  static void           register_dispatcher (const DispatcherEntry &dentry);
  static DispatchFunc   find_dispatcher     (const TypeHash        &type_hash);
  static FieldBuffer*   dispatch_call       (const FieldBuffer     &fbcall,
                                             Coupler               &coupler);
};

/* === FieldBuffer === */
typedef enum {
  VOID = 0,
  INT, FLOAT, STRING, ENUM,
  RECORD, SEQUENCE, FUNC, INSTANCE
} FieldType;

class _FakeFieldBuffer { FieldUnion *u; virtual ~_FakeFieldBuffer() {}; };

union FieldUnion {
  int64        vint64;
  double       vdouble;
  uint64       smem[(sizeof (String) + 7) / 8];           // String
  uint64       bmem[(sizeof (_FakeFieldBuffer) + 7) / 8]; // FieldBuffer
  uint8        bytes[8];                // FieldBuffer types
  struct { uint capacity, index; };     // FieldBuffer.buffermem[0]
};

class FieldBuffer { // buffer for marshalling procedure calls
  friend class FieldBufferReader;
protected:
  FieldUnion        *buffermem;
  inline uint        offset () const { const uint offs = 1 + (n_types() + 7) / 8; return offs; }
  inline FieldType   type_at (uint n) const { return FieldType (buffermem[1 + n/8].bytes[n%8]); }
  inline void        set_type (FieldType ft) { buffermem[1 + nth()/8].bytes[nth()%8] = ft; }
  inline uint        n_types () const { return buffermem[0].capacity; }
  inline uint        nth () const     { return buffermem[0].index; }
  inline FieldUnion& getu () const    { return buffermem[offset() + nth()]; }
  inline FieldUnion& addu (FieldType ft) { set_type (ft); FieldUnion &u = getu(); buffermem[0].index++; return u; }
  inline void        check() { /* FIXME: n_types() shouldn't be exceeded */ }
  inline FieldUnion& uat (uint n) const { return n < n_types() ? buffermem[offset() + n] : *(FieldUnion*) NULL; }
  explicit           FieldBuffer (uint _ntypes);
  explicit           FieldBuffer (uint, FieldUnion*, uint);
public:
  virtual     ~FieldBuffer();
  inline uint64 first_id () const { return buffermem && type_at (0) == INT ? uat (0).vint64 : 0; }
  inline void add_int64  (int64  vint64)  { FieldUnion &u = addu (INT); u.vint64 = vint64; check(); }
  inline void add_evalue (int64  vint64)  { FieldUnion &u = addu (ENUM); u.vint64 = vint64; check(); }
  inline void add_double (double vdouble) { FieldUnion &u = addu (FLOAT); u.vdouble = vdouble; check(); }
  inline void add_string (const String &s) { FieldUnion &u = addu (STRING); new (&u) String (s); check(); }
  inline void add_func   (const String &s) { FieldUnion &u = addu (FUNC); new (&u) String (s); check(); }
  inline void add_object (uint64 objid) { FieldUnion &u = addu (INSTANCE); u.vint64 = objid; check(); }
  inline FieldBuffer& add_rec (uint nt) { FieldUnion &u = addu (RECORD); return *new (&u) FieldBuffer (nt); check(); }
  inline FieldBuffer& add_seq (uint nt) { FieldUnion &u = addu (SEQUENCE); return *new (&u) FieldBuffer (nt); check(); }
  inline TypeHash first_type_hash () const;
  inline void     add_type_hash (uint64 a, uint64 b, uint64 c, uint64 d);
  inline void         reset();
  String              first_id_str() const;
  static FieldBuffer* _new (uint _ntypes); // Heap allocated FieldBuffer
  static FieldBuffer* new_error (const String &msg, const String &domain = "");
  static FieldBuffer* new_result();
  static FieldBuffer* new_ok();
};

class FieldBuffer8 : public FieldBuffer { // Stack contained buffer for up to 8 fields
  FieldUnion bmem[1 + 1 + 8];
public:
  virtual ~FieldBuffer8 () { reset(); buffermem = NULL; }
  inline   FieldBuffer8 (uint ntypes = 8) : FieldBuffer (ntypes, bmem, sizeof (bmem)) {}
};

class FieldBufferReader { // read field buffer contents
  const FieldBuffer *m_fb;
  uint               m_nth;
  inline FieldUnion& fb_getu () { return m_fb->uat (m_nth); }
  inline FieldUnion& fb_popu () { FieldUnion &u = m_fb->uat (m_nth++); check(); return u; }
  inline void        check() { /* FIXME: n_types() shouldn't be exceeded */ }
public:
  FieldBufferReader (const FieldBuffer &fb) : m_fb (&fb), m_nth (0) {}
  inline void reset (const FieldBuffer &fb) { m_fb = &fb; m_nth = 0; }
  inline void               reset      () { m_fb = NULL; m_nth = 0; }
  inline uint               remaining  () { return n_types() - m_nth; }
  inline void               skip       () { m_nth++; check(); }
  inline void               skip_hash  () { m_nth += 4; check(); }
  inline uint               n_types    () { return m_fb->n_types(); }
  inline FieldType          get_type   () { return m_fb->type_at (m_nth); }
  inline int64              get_int64  () { FieldUnion &u = fb_getu(); return u.vint64; }
  inline int64              get_evalue () { FieldUnion &u = fb_getu(); return u.vint64; }
  inline double             get_double () { FieldUnion &u = fb_getu(); return u.vdouble; }
  inline const String&      get_string () { FieldUnion &u = fb_getu(); return *(String*) &u; }
  inline const String&      get_func   () { FieldUnion &u = fb_getu(); return *(String*) &u; }
  inline uint64             get_object () { FieldUnion &u = fb_getu(); return u.vint64; }
  inline const FieldBuffer& get_rec () { FieldUnion &u = fb_getu(); return *(FieldBuffer*) &u; }
  inline const FieldBuffer& get_seq () { FieldUnion &u = fb_getu(); return *(FieldBuffer*) &u; }
  inline int64              pop_int64  () { FieldUnion &u = fb_popu(); return u.vint64; }
  inline int64              pop_evalue () { FieldUnion &u = fb_popu(); return u.vint64; }
  inline double             pop_double () { FieldUnion &u = fb_popu(); return u.vdouble; }
  inline const String&      pop_string () { FieldUnion &u = fb_popu(); return *(String*) &u; }
  inline const String&      pop_func   () { FieldUnion &u = fb_popu(); return *(String*) &u; }
  inline uint64             pop_object () { FieldUnion &u = fb_popu(); return u.vint64; }
  inline const FieldBuffer& pop_rec () { FieldUnion &u = fb_popu(); return *(FieldBuffer*) &u; }
  inline const FieldBuffer& pop_seq () { FieldUnion &u = fb_popu(); return *(FieldBuffer*) &u; }
  inline const FieldBuffer* get     () { return m_fb; }
};

/* === Channel === */
class Channel {
  class Priv; Priv &priv;
  Callback0<void>   wakeup0;
  FieldBuffer*      fetch_msg   (bool advance, bool block = false);
public:
  explicit      Channel         ();
  bool          push_msg        (FieldBuffer *fbmsg);           // takes fbmsg ownership
  bool          has_msg         () { return fetch_msg (false); }
  FieldBuffer*  pop_msg         () { return fetch_msg (true); } // passes fbmsg ownership
  void          wait_msg        () { fetch_msg (false, true); }
  virtual      ~Channel         ();
  void          set_wakeup      (void (*f) ()) { wakeup0.set (f); }
  template<class C>
  void          set_wakeup      (C &c, void (C::*m) ()) { wakeup0.set (c, m); }
};

/* === Coupler === */
class Coupler {
  class Priv; Priv    &priv;
  Coupler&             operator=         (const Coupler&); // not assignable
  /*Copy*/             Coupler           (const Coupler&); // not copyable
  Channel              callc, resultc, eventc;
public:
  explicit             Coupler           ();
  virtual             ~Coupler           ();
  // client API
  template<class C>
  void                 set_caller_wakeup (C &c, void (C::*m) ()) { resultc.set_wakeup (c, m); }
  void                 set_caller_wakeup (void (*f) ()) { resultc.set_wakeup (f); }
  FieldBuffer*         pop_result        (void) { resultc.wait_msg(); return resultc.pop_msg(); }
  bool                 push_call         (FieldBuffer *fbcall) { return callc.push_msg (fbcall); }
  virtual FieldBuffer* call_remote       (FieldBuffer *fbcall) = 0;
  // async msg queue
  template<class C>
  void                 set_event_wakeup  (C &c, void (C::*m) ()) { eventc.set_wakeup (c, m); }
  void                 set_event_wakeup  (void (*f) ()) { eventc.set_wakeup (f); }
  void                 wait_event        (void) { eventc.wait_msg(); }
  bool                 has_event         (void) { return eventc.has_msg(); }
  FieldBuffer*         pop_event         (void) { return eventc.pop_msg(); }
  // API for DispatchFunc
  FieldBufferReader    reader;
  void                 push_result       (FieldBuffer *rret) { resultc.push_msg (rret); }
  void                 push_event        (FieldBuffer *fbev) { eventc.push_msg (fbev); }
  uint                 dispatcher_add    (std::auto_ptr<EventDispatcher> evd); // takes object
  EventDispatcher*     dispatcher_lookup (uint         dfunc_id);
  void                 dispatcher_delete (uint         dfunc_id);
  // SmartHandle API
  inline uint64        pop_rpc_handle    (FieldBufferReader &fbr) { return fbr.pop_object(); }
  // server loop integration
  template<class C>
  void             set_dispatcher_wakeup (C &c, void (C::*m) ()) { callc.set_wakeup (c, m); }
  void             set_dispatcher_wakeup (void (*f) ()) { callc.set_wakeup (f); }
  bool             check_dispatch        () { return callc.has_msg(); }
  void             dispatch              ();
};

/* === inline implementations === */
inline
TypeHash::TypeHash (const uint64 qw[hash_size])
{
  wmemcpy ((wchar_t*) qwords, (wchar_t*) qw, sizeof (qwords) / sizeof (wchar_t));
}

inline
TypeHash::TypeHash (uint64 qwa, uint64 qwb, uint64 qwc, uint64 qwd)
{
  qwords[0] = qwa;
  qwords[1] = qwb;
  qwords[2] = qwc;
  qwords[3] = qwd;
}

inline bool
TypeHash::operator< (const TypeHash &rhs) const
{
  if (qwords[0] < rhs.qwords[0])
    return 1;
  else if (rhs.qwords[0] < qwords[0])
    return 0;
  for (uint i = 1; i < hash_size; i++)
    if (qwords[i] < rhs.qwords[i])
      return 1;
    else if (rhs.qwords[i] < qwords[i])
      return 0;
  return 0; // equal
}

inline void*
SmartHandle::_void_iface () const
{
  if (PLIC_UNLIKELY (m_rpc_id & 3))
    return _cast_iface();
  return (void*) m_rpc_id;
}

template<class T, size_t S> inline
DispatcherRegistry::DispatcherRegistry (T (&ha)[S])
{
  for (uint i = 0; i < S; i++)
    register_dispatcher (ha[i]);
}

inline TypeHash
FieldBuffer::first_type_hash () const
{
  return buffermem &&
    type_at (0) == INT && type_at (1) == INT && type_at (2) == INT && type_at (3) == INT ?
    TypeHash (uat (0).vint64, uat (1).vint64, uat (2).vint64, uat (3).vint64) :
    TypeHash (0, 0, 0, 0);
}

inline void
FieldBuffer::add_type_hash (uint64 a, uint64 b, uint64 c, uint64 d)
{
  FieldUnion &ua = addu (INT), &ub = addu (INT), &uc = addu (INT), &ud = addu (INT);
  ua.vint64 = a;
  ub.vint64 = b;
  uc.vint64 = c;
  ud.vint64 = d;
  check();
}

inline void
FieldBuffer::reset()
{
  if (!buffermem)
    return;
  while (nth() > 0)
    {
      buffermem[0].index--; // nth()--
      switch (type_at (nth()))
        {
        case STRING:
        case FUNC:    { FieldUnion &u = getu(); ((String*) &u)->~String(); }; break;
        case SEQUENCE:
        case RECORD:  { FieldUnion &u = getu(); ((FieldBuffer*) &u)->~FieldBuffer(); }; break;
        default: ;
        }
    }
}

} // Plic

#endif /* __PLIC_UTILITIES_HH__ */
