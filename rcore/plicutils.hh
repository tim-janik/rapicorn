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

/* === Forward Declarations === */
class SimpleServer;
class Connection;
union FieldUnion;
class FieldBuffer;
class FieldReader;
typedef FieldBuffer* (*DispatchFunc) (FieldReader&);

// === Message IDs ===
enum MessageId {
  MSGID_ONEWAY      = 0x2000000000000000ULL,      ///< One-way method call ID (void return).
  MSGID_TWOWAY      = 0x3000000000000000ULL,      ///< Two-way method call ID, returns result message.
  MSGID_DISCON      = 0x4000000000000000ULL,      ///< Signal handler disconnection ID.
  MSGID_SIGCON      = 0x5000000000000000ULL,      ///< Signal connection/disconnection request ID, returns result message.
  MSGID_EVENT       = 0x6000000000000000ULL,      ///< One-way signal event message ID.
  // MSGID_SIGNAL   = 0x7000000000000000ULL,      ///< Two-way signal message ID, returns result message.
};
inline bool msgid_has_result    (MessageId mid) { return (mid & 0x9000000000000000ULL) == 0x1000000000000000ULL; }
inline bool msgid_is_result     (MessageId mid) { return (mid & 0x9000000000000000ULL) == 0x9000000000000000ULL; }
inline bool msgid_is_error      (MessageId mid) { return (mid & 0xf000000000000000ULL) == 0x8000000000000000ULL; }
inline bool msgid_is_oneway     (MessageId mid) { return (mid & 0x7000000000000000ULL) == MSGID_ONEWAY; }
inline bool msgid_is_twoway     (MessageId mid) { return (mid & 0x7000000000000000ULL) == MSGID_TWOWAY; }
inline bool msgid_is_discon     (MessageId mid) { return (mid & 0x7000000000000000ULL) == MSGID_DISCON; }
inline bool msgid_is_sigcon     (MessageId mid) { return (mid & 0x7000000000000000ULL) == MSGID_SIGCON; }
inline bool msgid_is_event      (MessageId mid) { return (mid & 0x7000000000000000ULL) == MSGID_EVENT; }

// === NonCopyable ===
class NonCopyable {
  NonCopyable& operator=   (const NonCopyable&);
  /*copy*/     NonCopyable (const NonCopyable&);
protected:
  /*ctor*/     NonCopyable () {}
  /*dtor*/    ~NonCopyable () {}
};

/* === SmartHandle === */
class SmartHandle {
  uint64 m_rpc_id;
protected:
  typedef bool (SmartHandle::*_UnspecifiedBool) () const; // non-numeric operator bool() result
  static inline _UnspecifiedBool _unspecified_bool_true () { return &Plic::SmartHandle::_is_null; }
  typedef uint64 RpcId;
  explicit                  SmartHandle (uint64 ipcid);
  void                      _reset      ();
  void*                     _cast_iface () const;
  inline void*              _void_iface () const;
  void                      _void_iface (void *rpc_id_ptr);
public:
  explicit                  SmartHandle ();
  uint64                    _rpc_id     () const;
  bool                      _is_null    () const;
  virtual                  ~SmartHandle ();
};

/* === SimpleServer === */
class SimpleServer {
public:
  explicit             SimpleServer ();
  virtual             ~SimpleServer ();
  virtual uint64       _rpc_id      () const;
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
  friend class FieldReader;
  void               check_internal ();
  inline FieldUnion& upeek (uint n) const { return buffermem[offset() + n]; }
protected:
  FieldUnion        *buffermem;
  inline void        check ()      { if (PLIC_UNLIKELY (size() > capacity())) check_internal(); }
  inline uint        offset () const { const uint offs = 1 + (capacity() + 7) / 8; return offs; }
  inline FieldType   type_at (uint n) const { return FieldType (buffermem[1 + n/8].bytes[n%8]); }
  inline void        set_type (FieldType ft)    { buffermem[1 + size()/8].bytes[size()%8] = ft; }
  inline uint        capacity () const          { return buffermem[0].capacity; }
  inline uint        size () const              { return buffermem[0].index; }
  inline FieldUnion& getu () const              { return buffermem[offset() + size()]; }
  inline FieldUnion& addu (FieldType ft) { set_type (ft); FieldUnion &u = getu(); buffermem[0].index++; check(); return u; }
  inline FieldUnion& uat (uint n) const { return n < size() ? upeek (n) : *(FieldUnion*) NULL; }
  explicit           FieldBuffer (uint _ntypes);
  explicit           FieldBuffer (uint, FieldUnion*, uint);
public:
  virtual     ~FieldBuffer();
  inline uint64 first_id () const { return buffermem && size() && type_at (0) == INT ? upeek (0).vint64 : 0; }
  inline void add_int64  (int64  vint64)  { FieldUnion &u = addu (INT); u.vint64 = vint64; }
  inline void add_evalue (int64  vint64)  { FieldUnion &u = addu (ENUM); u.vint64 = vint64; }
  inline void add_double (double vdouble) { FieldUnion &u = addu (FLOAT); u.vdouble = vdouble; }
  inline void add_string (const String &s) { FieldUnion &u = addu (STRING); new (&u) String (s); }
  inline void add_func   (const String &s) { FieldUnion &u = addu (FUNC); new (&u) String (s); }
  inline void add_object (uint64 objid) { FieldUnion &u = addu (INSTANCE); u.vint64 = objid; }
  inline void add_msgid  (uint64 h, uint64 l) { add_int64 (h); add_int64 (l); }
  inline FieldBuffer& add_rec (uint nt) { FieldUnion &u = addu (RECORD); return *new (&u) FieldBuffer (nt); }
  inline FieldBuffer& add_seq (uint nt) { FieldUnion &u = addu (SEQUENCE); return *new (&u) FieldBuffer (nt); }
  inline void         reset();
  String              first_id_str() const;
  static FieldBuffer* _new (uint _ntypes); // Heap allocated FieldBuffer
  static FieldBuffer* new_error (const String &msg, const String &domain = "");
  static FieldBuffer* new_result();
};

class FieldBuffer8 : public FieldBuffer { // Stack contained buffer for up to 8 fields
  FieldUnion bmem[1 + 1 + 8];
public:
  virtual ~FieldBuffer8 () { reset(); buffermem = NULL; }
  inline   FieldBuffer8 (uint ntypes = 8) : FieldBuffer (ntypes, bmem, sizeof (bmem)) {}
};

class FieldReader { // read field buffer contents
  const FieldBuffer *m_fb;
  uint               m_nth;
  void               check_request (int type);
  inline void        request (int t) { if (PLIC_UNLIKELY (m_nth >= n_types() || get_type() != t)) check_request (t); }
  inline FieldUnion& fb_getu (int t) { request (t); return m_fb->upeek (m_nth); }
  inline FieldUnion& fb_popu (int t) { request (t); FieldUnion &u = m_fb->upeek (m_nth++); return u; }
public:
  explicit                 FieldReader (const FieldBuffer &fb) : m_fb (&fb), m_nth (0) {}
  inline void               reset      (const FieldBuffer &fb) { m_fb = &fb; m_nth = 0; }
  inline void               reset      () { m_fb = NULL; m_nth = 0; }
  inline uint               remaining  () { return n_types() - m_nth; }
  inline void               skip       () { if (PLIC_UNLIKELY (m_nth >= n_types())) check_request (0); m_nth++; }
  inline void               skip_msgid () { skip(); skip(); }
  inline uint               n_types    () { return m_fb->size(); }
  inline FieldType          get_type   () { return m_fb->type_at (m_nth); }
  inline int64              get_int64  () { FieldUnion &u = fb_getu (INT); return u.vint64; }
  inline int64              get_evalue () { FieldUnion &u = fb_getu (ENUM); return u.vint64; }
  inline double             get_double () { FieldUnion &u = fb_getu (FLOAT); return u.vdouble; }
  inline const String&      get_string () { FieldUnion &u = fb_getu (STRING); return *(String*) &u; }
  inline const String&      get_func   () { FieldUnion &u = fb_getu (FUNC); return *(String*) &u; }
  inline uint64             get_object () { FieldUnion &u = fb_getu (INSTANCE); return u.vint64; }
  inline const FieldBuffer& get_rec    () { FieldUnion &u = fb_getu (RECORD); return *(FieldBuffer*) &u; }
  inline const FieldBuffer& get_seq    () { FieldUnion &u = fb_getu (SEQUENCE); return *(FieldBuffer*) &u; }
  inline int64              pop_int64  () { FieldUnion &u = fb_popu (INT); return u.vint64; }
  inline int64              pop_evalue () { FieldUnion &u = fb_popu (ENUM); return u.vint64; }
  inline double             pop_double () { FieldUnion &u = fb_popu (FLOAT); return u.vdouble; }
  inline const String&      pop_string () { FieldUnion &u = fb_popu (STRING); return *(String*) &u; }
  inline const String&      pop_func   () { FieldUnion &u = fb_popu (FUNC); return *(String*) &u; }
  inline uint64             pop_object () { FieldUnion &u = fb_popu (INSTANCE); return u.vint64; }
  inline const FieldBuffer& pop_rec    () { FieldUnion &u = fb_popu (RECORD); return *(FieldBuffer*) &u; }
  inline const FieldBuffer& pop_seq    () { FieldUnion &u = fb_popu (SEQUENCE); return *(FieldBuffer*) &u; }
};

// === Connection ===
class Connection                                         /// Connection context for IPC. @nosubgrouping
{
public: /// @name API for syncronous remote calls
  virtual FieldBuffer* call_remote   (FieldBuffer*) = 0; ///< Carry out a syncronous remote call, transfers memory, called by client.
  virtual void         send_result   (FieldBuffer*) = 0; ///< Return result from remote call, transfers memory, called by server.
public: /// @name API for asyncronous event delivery
  virtual void   send_event    (FieldBuffer*) = 0;       ///< Send event to remote asyncronously, transfers memory, called by server.
  virtual int    event_inputfd () = 0;                   ///< Returns fd for POLLIN, to wake up on incomming events.
  bool           has_event     ();                       ///< Returns true if events for fetch_event() are pending.
  FieldBuffer*   pop_event     (bool blocking = false);  ///< Pop an event from event queue, possibly blocking, transfers memory, called by client.
protected:
  virtual FieldBuffer* fetch_event (int blockpop) = 0;   ///< Block for (-1), peek (0) or pop (+1) an event from queue.
public: /// @name API for event handler bookkeeping
  struct EventHandler {
    virtual             ~EventHandler () {}
    virtual FieldBuffer* handle_event (Plic::FieldBuffer &event_fb) = 0; ///< Process an event and possibly return an error.
  };
  virtual uint64        register_event_handler (EventHandler *evh) = 0; ///< Register an event handler, transfers memory.
  virtual EventHandler* find_event_handler     (uint64 handler_id) = 0; ///< Find event handler by id.
  virtual bool          delete_event_handler   (uint64 handler_id) = 0; ///< Delete a registered event handler, returns success.
public: /// @name Registry for IPC method lookups
  struct MethodEntry    { uint64 hashhi, hashlow; DispatchFunc dispatcher; };  ///< Structure to register methods for IPC.
  struct MethodRegistry {
    template<class T, size_t S> MethodRegistry  (T (&static_const_entries)[S]) /// Register static const MethodEntry structs.
    { for (size_t i = 0; i < S; i++) register_method (static_const_entries[i]); }
  private: static void register_method (const MethodEntry &mentry);
  };
protected:
  static DispatchFunc  find_method  (uint64 hashhi, uint64 hashlow);          ///< Lookup method in registry.
};

/* === inline implementations === */
inline void*
SmartHandle::_void_iface () const
{
  if (PLIC_UNLIKELY (m_rpc_id & 3))
    return _cast_iface();
  return (void*) m_rpc_id;
}

inline void
FieldBuffer::reset()
{
  if (!buffermem)
    return;
  while (size() > 0)
    {
      buffermem[0].index--; // causes size()--
      switch (type_at (size()))
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
