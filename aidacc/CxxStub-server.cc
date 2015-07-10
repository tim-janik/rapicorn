
#ifndef AIDA_CHECK
#define AIDA_CHECK(cond,errmsg) do { if (cond) break; Rapicorn::Aida::fatal_error (__FILE__, __LINE__, errmsg); } while (0)
#endif

namespace { // Anon
using Rapicorn::Aida::uint64;

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

// objects
template<class Target> static inline std::shared_ptr<Target>
proto_reader_pop_interface (Rapicorn::Aida::ProtoReader &fr)
{
  return std::dynamic_pointer_cast<Target> (server_connection->pop_interface (fr));
}

static inline void
add_header1_discon (ProtoMsg &fb, uint64 h, uint64 l)
{
  fb.add_header1 (Rapicorn::Aida::MSGID_DISCONNECT, h, l);
}

static inline ProtoMsg*
new_call_result (ProtoReader &fbr, uint64 h, uint64 l, uint32 n = 1)
{
  return ProtoMsg::renew_into_result (fbr, Rapicorn::Aida::MSGID_CALL_RESULT, h, l, n);
}

static inline ProtoMsg*
new_connect_result (ProtoReader &fbr, uint64 h, uint64 l, uint32 n = 1)
{
  return ProtoMsg::renew_into_result (fbr, Rapicorn::Aida::MSGID_CONNECT_RESULT, h, l, n);
}

// slot
template<class SharedPtr, class R, class... Args> std::function<R (Args...)>
slot (SharedPtr sp, R (*fp) (const SharedPtr&, Args...))
{
  return [sp, fp] (Args... args) { return fp (sp, args...); };
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
  fb.add_any (v, *__AIDA_Local__::server_connection);
}
static void
operator>>= (Rapicorn::Aida::ProtoReader &fr, Rapicorn::Any &v)
{
  v = fr.pop_any (*__AIDA_Local__::server_connection);
}
#endif // RAPICORN_AIDA_OPERATOR_SHLEQ_FB_ANY
} } // Rapicorn::Aida
