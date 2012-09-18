// CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0/
#ifndef __AIDA_CXXSTUBAUX_HH__
#define __AIDA_CXXSTUBAUX_HH__

#include "runtime.hh"

namespace Aida {
namespace CxxStub /// Internal types, used by the CxxStub code generator.
{

/// Handles remote (dis-)connection and client side dispatching of events via Rapicorn signals.
template<class Handle, typename SignalSignature>
class SignalHandler : Aida::ClientConnection::EventHandler
{
  static inline Aida::ClientConnection
  __client_connection__ (void)
  {
    struct _Handle : Handle { using Handle::__client_connection__; };
    return _Handle::__client_connection__();
  }
public:
  typedef Rapicorn::Signals::SignalProxy<Handle, SignalSignature> ProxySignal;
  typedef Rapicorn::Signals::Signal<
    Handle, SignalSignature,
    Rapicorn::Signals::CollectorDefault<typename Rapicorn::Signals::Signature<SignalSignature>::result_type>,
    Rapicorn::Signals::SignalConnectionRelay<SignalHandler>
    > RelaySignal;
  Aida::uint64_t m_handler_id, m_connection_id;
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
        __client_connection__().delete_event_handler (m_handler_id);
        m_handler_id = 0;
      }
  }
  void
  connections_changed (bool hasconnections)
  {
    Aida::FieldBuffer &fb = *Aida::FieldBuffer::_new (2 + 1 + 2);
    fb.add_msgid (hhi, hlo);
    fb <<= *rsignal.emitter();
    if (hasconnections)
      {
        if (!m_handler_id)      // signal connected
          m_handler_id = __client_connection__().register_event_handler (this);
        fb <<= m_handler_id;    // handler connection request
        fb <<= 0;               // no disconnection
      }
    else
      {                         // signal disconnected
        if (m_handler_id)
          ; // FIXME: deletion! AIDA_CONNECTION().delete_event_handler (m_handler_id), m_handler_id = 0;
        fb <<= 0;               // no handler connection
        fb <<= m_connection_id; // disconnection request
        m_connection_id = 0;
      }
    Aida::FieldBuffer *fr = __client_connection__().call_remote (&fb); // deletes fb
    if (fr)
      { // FIXME: assert that fr is a non-NULL FieldBuffer with result message
        Aida::FieldReader frr (*fr);
        frr.skip_msgid();       // FIXME: msgid for return?
        if (frr.remaining() && m_handler_id)
          frr >>= m_connection_id;
        delete fr;
      }
  }
  virtual Aida::FieldBuffer*
  handle_event (Aida::FieldBuffer &fb)
  {
    FieldTools<SignalSignature>::emit (fb, rsignal);
    return NULL;
  }
};

} // CxxStub
} // Aida

#endif // __AIDA_CXXSTUBAUX_HH__
