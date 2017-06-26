// Licensed CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0
namespace Rapicorn { namespace Aida {

/// @ingroup AidaManifoldTypes
/// Unpack ProtoMsg as signal arguments and emit() the signal.
template<class R, class A1, class A2> static inline R
proto_msg_emit_signal (const Aida::ProtoMsg &fb, const std::function<R (A1, A2)> &func, uint64 &emit_result_id)
{
  const bool async = !std::is_void<R>::value;
  const size_t NARGS = 2;
  Aida::ProtoReader fbr (fb);
  fbr.skip_header();
  fbr.skip();   // skip handler_id
  if (async)
    emit_result_id = fbr.pop_int64();
  AIDA_ASSERT_RETURN (fbr.remaining() == NARGS, R());
  typename ValueType<A1>::T a1; typename ValueType<A2>::T a2;
  fbr >>= a1; fbr >>= a2;
  return func (a1, a2);
}

} } // Rapicorn::Aida
