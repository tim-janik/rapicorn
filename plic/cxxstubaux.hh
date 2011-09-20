// CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0/
#ifndef __PLIC_CXXSTUBAUX_HH__
#define __PLIC_CXXSTUBAUX_HH__

#include "runtime.hh"

namespace Plic {
namespace CxxStub /// Internal types, used by the CxxStub code generator.
{

/// Handles remote (dis-)connection and client side dispatching of events via Rapicorn signals.
template<class Handle, typename SignalSignature>
struct SignalHandler : Plic::ClientConnection::EventHandler
{
  typedef Rapicorn::Signals::SignalProxy<Handle, SignalSignature> ProxySignal;
  typedef Rapicorn::Signals::Signal<
    Handle, SignalSignature,
    Rapicorn::Signals::CollectorDefault<typename Rapicorn::Signals::Signature<SignalSignature>::result_type>,
    Rapicorn::Signals::SignalConnectionRelay<SignalHandler>
    > RelaySignal;
  Plic::uint64_t m_handler_id, m_connection_id;
  RelaySignal rsignal;
  ProxySignal psignal;
  const long long unsigned hhi;
  const long long unsigned hlo;
  SignalHandler (Handle &handle, long long unsigned _hhi, long long unsigned _hlo) :
    m_handler_id (0), m_connection_id (0), rsignal (handle), psignal (rsignal), hhi (_hhi), hlo (_hlo)
  {
    rsignal.listener (*this, &SignalHandler::connections_changed);
  }
  ~SignalHandler ()
  {
    if (m_handler_id)
      {
        PLIC_CONNECTION().delete_event_handler (m_handler_id);
        m_handler_id = 0;
      }
  }
  void
  connections_changed (bool hasconnections)
  {
    Plic::FieldBuffer &fb = *Plic::FieldBuffer::_new (2 + 1 + 2);
    fb.add_msgid (hhi, hlo);
    fb <<= *rsignal.emitter();
    if (hasconnections)
      {
        if (!m_handler_id)      // signal connected
          m_handler_id = PLIC_CONNECTION().register_event_handler (this);
        fb <<= m_handler_id;    // handler connection request
        fb <<= 0;               // no disconnection
      }
    else
      {                         // signal disconnected
        if (m_handler_id)
          ; // FIXME: deletion! PLIC_CONNECTION().delete_event_handler (m_handler_id), m_handler_id = 0;
        fb <<= 0;               // no handler connection
        fb <<= m_connection_id; // disconnection request
        m_connection_id = 0;
      }
    Plic::FieldBuffer *fr = PLIC_CONNECTION().call_remote (&fb); // deletes fb
    if (fr)
      { // FIXME: assert that fr is a non-NULL FieldBuffer with result message
        Plic::FieldReader frr (*fr);
        frr.skip_msgid();       // FIXME: msgid for return?
        if (frr.remaining() && m_handler_id)
          frr >>= m_connection_id;
        delete fr;
      }
  }
  virtual Plic::FieldBuffer*
  handle_event (Plic::FieldBuffer &fb)
  {
    FieldTools<SignalSignature>::emit (fb, rsignal);
    return NULL;
  }
};

} // CxxStub
} // Plic

#endif // __PLIC_CXXSTUBAUX_HH__
