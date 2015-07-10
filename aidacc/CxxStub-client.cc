
#ifndef AIDA_CHECK
#define AIDA_CHECK(cond,errmsg) do { if (cond) break; Rapicorn::Aida::fatal_error (__FILE__, __LINE__, errmsg); } while (0)
#endif

namespace { // Anon
using Rapicorn::Aida::uint64;

namespace __AIDA_Local__ {
using namespace Rapicorn::Aida;

// connection
static Rapicorn::Aida::ClientConnection *client_connection = NULL;

// helper
static inline ProtoMsg*
new_emit_result (const ProtoMsg *fb, uint64 h, uint64 l, uint32 n)
{
  return ProtoMsg::renew_into_result (const_cast<ProtoMsg*> (fb), Rapicorn::Aida::MSGID_EMIT_RESULT, h, l, n);
}

} } // Anon::__AIDA_Local__

namespace Rapicorn { namespace Aida {
#ifndef RAPICORN_AIDA_OPERATOR_SHLEQ_FB_ANY
#define RAPICORN_AIDA_OPERATOR_SHLEQ_FB_ANY
// namespace Rapicorn::Aida needed for argument dependent lookups of the operators
static void operator<<= (Rapicorn::Aida::ProtoMsg &fb, const Rapicorn::Aida::Any &v) __attribute__ ((unused));
static void operator>>= (Rapicorn::Aida::ProtoReader &fr, Rapicorn::Aida::Any &v) __attribute__ ((unused));
static void
operator<<= (Rapicorn::Aida::ProtoMsg &fb, const Rapicorn::Any &v)
{
  fb.add_any (v, *__AIDA_Local__::client_connection);
}
static void
operator>>= (Rapicorn::Aida::ProtoReader &fr, Rapicorn::Any &v)
{
  v = fr.pop_any (*__AIDA_Local__::client_connection);
}
#endif // RAPICORN_AIDA_OPERATOR_SHLEQ_FB_ANY
} } // Rapicorn::Aida
