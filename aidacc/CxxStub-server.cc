
#ifndef AIDA_CHECK
#define AIDA_CHECK(cond,errmsg) do { if (cond) break; throw std::runtime_error (std::string ("AIDA-ERROR: ") + errmsg); } while (0)
#endif

namespace { // Anon
using Rapicorn::Aida::uint64_t;

namespace __AIDA_Local__ {
using namespace Rapicorn::Aida;

// types
typedef ServerConnection::EmitResultHandler EmitResultHandler;
typedef ServerConnection::MethodRegistry    MethodRegistry;
typedef ServerConnection::MethodEntry       MethodEntry;

static_assert (std::is_base_of<Rapicorn::Aida::ImplicitBase, $AIDA_iface_base$>::value,
               "IDL interface base '$AIDA_iface_base$' must derive 'Rapicorn::Aida::ImplicitBase'");

// connection
static Rapicorn::Aida::ServerConnection *server_connection = NULL;
static Rapicorn::Init init_server_connection ([]() {
  server_connection = ObjectBroker::new_server_connection ($AIDA_server_feature_keys$);
});

// EmitResultHandler
static inline void erhandler_add (size_t id, const EmitResultHandler &function)
{
  return server_connection->emit_result_handler_add (id, function);
}

// objects
template<class Target> static inline Target* id2obj (uint64_t oid)
{
  Rapicorn::Aida::ImplicitBase *instance = server_connection->orbid2instance (oid);
  return dynamic_cast<Target*> (instance);
}

static inline uint64_t obj2id  (Rapicorn::Aida::ImplicitBase *obj)
{
  return server_connection->instance2orbid (obj);
}

template<class Object> static inline Object*
smh2obj (const SmartHandle &sh)
{
  return id2obj<Object> (sh._orbid());
}

template<class SMH> static inline SMH
obj2smh ($AIDA_iface_base$ *self)
{
  const uint64_t orbid = obj2id (self);
  SMH target;
  const uint64_t input[2] = { orbid, target._orbid() };
  Rapicorn::Aida::ObjectBroker::dup_handle (input, target);
  return target;
}

// messages
static inline void
post_msg (FieldBuffer *fb)
{
  ObjectBroker::post_msg (fb);
}

static inline void
add_header1_discon (FieldBuffer &fb, size_t signal_handler_id, uint64_t h, uint64_t l)
{
  fb.add_header1 (Rapicorn::Aida::MSGID_DISCONNECT, ObjectBroker::connection_id_from_signal_handler_id (signal_handler_id), h, l);
}

static inline void
add_header1_emit (FieldBuffer &fb, size_t signal_handler_id, uint64_t h, uint64_t l)
{
  fb.add_header1 (Rapicorn::Aida::MSGID_EMIT_ONEWAY, ObjectBroker::connection_id_from_signal_handler_id (signal_handler_id), h, l);
}

static inline void
add_header2_emit (FieldBuffer &fb, size_t signal_handler_id, uint64_t h, uint64_t l)
{
  fb.add_header2 (Rapicorn::Aida::MSGID_EMIT_TWOWAY, ObjectBroker::connection_id_from_signal_handler_id (signal_handler_id),
                  server_connection->connection_id(), h, l);
}

static inline FieldBuffer*
new_call_result (FieldReader &fbr, uint64_t h, uint64_t l, uint32_t n = 1)
{
  return ObjectBroker::renew_into_result (fbr, Rapicorn::Aida::MSGID_CALL_RESULT, ObjectBroker::receiver_connection_id (fbr.field_buffer()->first_id()), h, l, n);
}

static inline FieldBuffer*
new_connect_result (FieldReader &fbr, uint64_t h, uint64_t l, uint32_t n = 1)
{
  return ObjectBroker::renew_into_result (fbr, Rapicorn::Aida::MSGID_CONNECT_RESULT, ObjectBroker::receiver_connection_id (fbr.field_buffer()->first_id()), h, l, n);
}

// slot
template<class SharedPtr, class R, class... Args> std::function<R (Args...)>
slot (SharedPtr sp, R (*fp) (const SharedPtr&, Args...))
{
  return [sp, fp] (Args... args) { return fp (sp, args...); };
}

} } // Anon::__AIDA_Local__
