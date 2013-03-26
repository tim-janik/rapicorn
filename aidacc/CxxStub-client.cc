
#ifndef AIDA_CHECK
#define AIDA_CHECK(cond,errmsg) do { if (cond) break; throw std::runtime_error (std::string ("AIDA-ERROR: ") + errmsg); } while (0)
#endif

namespace { // Anon
using Rapicorn::Aida::uint64_t;

namespace __AIDA_Local__ {
using namespace Rapicorn::Aida;

// connection
static Rapicorn::Aida::ClientConnection *client_connection = NULL;
static Rapicorn::Init init_client_connection ([]() {
  client_connection = ObjectBroker::new_client_connection();
});

// helper

static FieldBuffer*
invoke (FieldBuffer *fb)                // async remote call, transfers memory
{
  return client_connection->call_remote (fb);
}

static bool
signal_disconnect (size_t signal_handler_id)
{
  return client_connection->signal_disconnect (signal_handler_id);
}

static size_t
signal_connect (uint64_t hhi, uint64_t hlo, const SmartHandle &sh, SignalEmitHandler seh, void *data)
{
  return client_connection->signal_connect (hhi, hlo, sh._orbid(), seh, data);
}

static inline uint64_t
smh2id (const SmartHandle &sh)
{
  return sh._orbid();
}

template<class SMH> SMH
smh2cast (const SmartHandle &handle)
{
  const uint64_t orbid = __AIDA_Local__::smh2id (handle);
  SMH target;
  const uint64_t input[2] = { orbid, target._orbid() };
  Rapicorn::Aida::ObjectBroker::dup_handle (input, target);
  return target;
}

static inline void
add_header2_call (FieldBuffer &fb, const SmartHandle &sh, uint64_t h, uint64_t l)
{
  fb.add_header2 (Rapicorn::Aida::MSGID_TWOWAY_CALL, ObjectBroker::connection_id_from_handle (sh),
                  client_connection->connection_id(), h, l);
}

static inline void
add_header1_call (FieldBuffer &fb, const SmartHandle &sh, uint64_t h, uint64_t l)
{
  fb.add_header1 (Rapicorn::Aida::MSGID_ONEWAY_CALL, ObjectBroker::connection_id_from_handle (sh), h, l);
}

static inline FieldBuffer*
new_emit_result (const FieldBuffer *fb, uint64_t h, uint64_t l, uint32_t n)
{
  return ObjectBroker::renew_into_result (const_cast<FieldBuffer*> (fb),
                                          Rapicorn::Aida::MSGID_EMIT_RESULT,
                                          ObjectBroker::receiver_connection_id (fb->first_id()),
                                          h, l, n);
}

} } // Anon::__AIDA_Local__
