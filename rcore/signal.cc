// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "signal.hh"

namespace Rapicorn {
namespace Signals {

/* --- TrampolineLink --- */
void
TrampolineLink::owner_ref ()
{
  assert_return (m_linking_owner == false);
  ref_sink();
  m_linking_owner = true;
}

void
TrampolineLink::unlink()
{
  callable (false);
  if (m_linking_owner)
    {
      m_linking_owner = false;
      next->prev = prev;
      prev->next = next;
      // leave ->next, ->prev intact for iterators still holding a ref
      unref();
    }
}

TrampolineLink::~TrampolineLink()
{
  assert_return (m_linking_owner == false);
  if (next || prev)
    {
      // unlink() might have left stale next and prev
      prev = next = NULL;
    }
}

/* --- SignalBase --- */
bool
SignalBase::EmbeddedLink::operator== (const TrampolineLink &other) const
{
  return false;
}

ConId
SignalBase::connect_link (TrampolineLink *link,
                          bool            with_emitter)
{
  assert_return (link->m_linking_owner == false && link->prev == NULL && link->next == NULL, 0);
  const bool connected_before = has_connections();
  link->owner_ref();
  link->prev = start.prev;
  link->next = &start;
  start.prev->next = link;
  start.prev = link;
  link->with_emitter (with_emitter);
  if (!connected_before)
    connections_changed (true);
  return link->con_id();
}

uint
SignalBase::disconnect_equal_link (const TrampolineLink &link,
                                   bool                  with_emitter)
{
  for (TrampolineLink *walk = start.next; walk != &start; walk = walk->next)
    if (walk->with_emitter() == with_emitter and *walk == link)
      {
        assert_return (walk->m_linking_owner == true, 0);
        walk->unlink(); // unrefs
        if (!has_connections())
          connections_changed (false);
        return 1;
      }
  return 0;
}

uint
SignalBase::disconnect_link_id (ConId con_id)
{
  for (TrampolineLink *walk = start.next; walk != &start; walk = walk->next)
    if (con_id == walk->con_id())
      {
        assert_return (walk->m_linking_owner == true, 0);
        walk->unlink(); // unrefs
        if (!has_connections())
          connections_changed (false);
        return 1;
      }
  return 0;
}

SignalBase::~SignalBase()
{
  const bool connected_before = has_connections();
  while (start.next != &start)
    {
      assert_return (start.next->m_linking_owner == true);
      start.next->unlink();
    }
  RAPICORN_ASSERT (start.next == &start);
  RAPICORN_ASSERT (start.prev == &start);
  if (connected_before)
    connections_changed (false);
  start.prev = start.next = NULL;
  start.check_last_ref();
  start.unref();
}

void
SignalBase::EmbeddedLink::delete_this ()
{
  /* not deleting, because this structure is always embedded as SignalBase::start */
}

// === SignalProxyBase ===
SignalProxyBase&
SignalProxyBase::operator= (const SignalProxyBase &other)
{
  if (m_signal != other.m_signal)
    {
      divorce();
      m_signal = other.m_signal;
    }
  return *this;
}

SignalProxyBase::SignalProxyBase (const SignalProxyBase &other) :
  m_signal (other.m_signal)
{}

SignalProxyBase::SignalProxyBase (SignalBase &signal) :
  m_signal (&signal)
{
  // FIXME: assert_return (NULL != &signal);
}

ConId
SignalProxyBase::connect_link (TrampolineLink *link, bool with_emitter)
{
  return m_signal ? m_signal->connect_link (link, with_emitter) : 0;
}

uint
SignalProxyBase::disconnect_equal_link (const TrampolineLink &link, bool with_emitter)
{
  return m_signal ? m_signal->disconnect_equal_link (link, with_emitter) : 0;
}

uint
SignalProxyBase::disconnect_link_id (ConId id)
{
  return m_signal ? m_signal->disconnect_link_id (id) : 0;
}

void
SignalProxyBase::divorce ()
{
  m_signal = NULL;
}

} // Signals
} // Rapicorn
