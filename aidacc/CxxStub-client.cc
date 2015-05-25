
#ifndef AIDA_CHECK
#define AIDA_CHECK(cond,errmsg) do { if (cond) break; Rapicorn::Aida::fatal_error (std::string ("AIDA-ERROR: ") + errmsg); } while (0)
#endif

namespace { // Anon
using Rapicorn::Aida::uint64;

namespace __AIDA_Local__ {
using namespace Rapicorn::Aida;

// connection
static Rapicorn::Aida::ClientConnection *client_connection = NULL;
static Rapicorn::Init init_client_connection ([]() {
  client_connection = ObjectBroker::new_client_connection ($AIDA_client_feature_keys$);
});

// helper

static AIDA_UNUSED FieldBuffer*
invoke (FieldBuffer *fb)                // async remote call, transfers memory
{
  return client_connection->call_remote (fb);
}

static AIDA_UNUSED bool
signal_disconnect (size_t signal_handler_id)
{
  return client_connection->signal_disconnect (signal_handler_id);
}

static AIDA_UNUSED size_t
signal_connect (uint64 hhi, uint64 hlo, const RemoteHandle &rh, SignalEmitHandler seh, void *data)
{
  return client_connection->signal_connect (hhi, hlo, rh, seh, data);
}

static inline void
add_header2_call (FieldBuffer &fb, const RemoteHandle &sh, uint64 h, uint64 l)
{
  fb.add_header2 (Rapicorn::Aida::MSGID_CALL_TWOWAY, ObjectBroker::connection_id_from_handle (sh),
                  client_connection->connection_id(), h, l);
}

static inline void
add_header1_call (FieldBuffer &fb, const RemoteHandle &sh, uint64 h, uint64 l)
{
  fb.add_header1 (Rapicorn::Aida::MSGID_CALL_ONEWAY, ObjectBroker::connection_id_from_handle (sh), h, l);
}

static inline FieldBuffer*
new_emit_result (const FieldBuffer *fb, uint64 h, uint64 l, uint32 n)
{
  return FieldBuffer::renew_into_result (const_cast<FieldBuffer*> (fb),
                                         Rapicorn::Aida::MSGID_EMIT_RESULT,
                                         ObjectBroker::sender_connection_id (fb->first_id()),
                                         h, l, n);
}

} } // Anon::__AIDA_Local__

namespace Rapicorn { namespace Aida {
#ifndef RAPICORN_AIDA_OPERATOR_SHLEQ_FB_ANY
#define RAPICORN_AIDA_OPERATOR_SHLEQ_FB_ANY
// namespace Rapicorn::Aida needed for argument dependent lookups of the operators
static void operator<<= (Rapicorn::Aida::FieldBuffer &fb, const Rapicorn::Aida::Any &v) __attribute__ ((unused));
static void operator>>= (Rapicorn::Aida::FieldReader &fr, Rapicorn::Aida::Any &v) __attribute__ ((unused));
static void
operator<<= (Rapicorn::Aida::FieldBuffer &fb, const Rapicorn::Any &v)
{
  fb.add_any (v, *__AIDA_Local__::client_connection);
}
static void
operator>>= (Rapicorn::Aida::FieldReader &fr, Rapicorn::Any &v)
{
  v = fr.pop_any (*__AIDA_Local__::client_connection);
}
#endif // RAPICORN_AIDA_OPERATOR_SHLEQ_FB_ANY
} } // Rapicorn::Aida
