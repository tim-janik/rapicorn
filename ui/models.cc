// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "models.hh"
#include "application.hh"

namespace Rapicorn {

ListModelRelayImpl::ListModelRelayImpl (int n_columns) :
  m_size (0), m_columns (n_columns)
{}

ListModelRelayImpl::~ListModelRelayImpl ()
{}

int
ListModelRelayImpl::columns ()
{
  return 1;
}

AnySeqImpl
ListModelRelayImpl::row (int n)
{
  return n >= 0 && size_t (n) < m_rows.size() ? m_rows[n] : AnySeqImpl();
}

void
ListModelRelayImpl::resize (int _size)
{
  const int old = m_size;
  m_size = _size;
  m_rows.resize (m_size);
  if (m_size < old)
    sig_removed.emit (m_size, old-1);
  else if (m_size > old)
    sig_inserted.emit (old, m_size-1);
}

void
ListModelRelayImpl::inserted (int first, int last)
{
  assert_return (first >= 0);
  assert_return (first <= m_size);
  if (last)
    assert_return (last >= first);
  else
    last = first;
  m_size += last - first + 1;
  m_rows.resize (m_size);
  sig_inserted.emit (first, last);
}

void
ListModelRelayImpl::changed (int first, int last)
{
  assert_return (first >= 0);
  assert_return (first < m_size);
  if (last)
    {
      assert_return (last >= first);
      assert_return (last < m_size);
    }
  else
    last = first;
  refill (first, last);
  // sig_changed.emit (first, last);
}

void
ListModelRelayImpl::removed (int first, int last)
{
  assert_return (first >= 0);
  assert_return (first < m_size);
  if (last)
    {
      assert_return (last >= first);
      assert_return (last < m_size);
    }
  else
    last = first;
  m_size -= last - first + 1;
  m_rows.resize (m_size);
  sig_removed.emit (first, last);
}

void
ListModelRelayImpl::fill (int first, const AnySeqSeqImpl &ss)
{
  assert_return (first >= 0);
  if (first >= m_size)
    return;
  size_t i = first;
  for (size_t j = 0; i < m_rows.size() && j < ss.size(); i++, j++)
    m_rows[i] = ss[j];
  if (i > size_t (first))
    sig_changed.emit (first, i-1);
}

void
ListModelRelayImpl::refill (int first, int last)
{
  assert_return (first >= 0);
  if (last)
    assert_return (last >= first);
  else
    last = first;
  if (first < m_size)
    {
      last = min (last, m_size);
      sig_refill.emit (first, last);
    }
}

ListModelRelayImpl&
ListModelRelayImpl::create_list_model_relay (int n_columns)
{
  ListModelRelayImpl *relay = new ListModelRelayImpl (n_columns);
  return *relay;
}

const PropertyList&
ListModelRelayImpl::_property_list ()
{
  return RAPICORN_AIDA_PROPERTY_CHAIN (ListModelRelayIface::_property_list(), ListModelIface::_property_list());
}

ListModelRelayIface*
ApplicationImpl::create_list_model_relay (int                n_columns,
                                          const std::string &name)
{
  struct Accessor : public ListModelRelayImpl { using ListModelRelayImpl::create_list_model_relay; };
  ListModelRelayImpl &lmr = Accessor::create_list_model_relay (n_columns);
  if (ApplicationImpl::the().xurl_add (name, lmr))
    return &lmr;
  else
    {
      unref (ref_sink (lmr));
      return NULL;
    }
}

MemoryListStore::MemoryListStore (int n_columns) :
  m_columns (n_columns)
{
  assert_return (n_columns > 0);
}

AnySeqImpl
MemoryListStore::row (int n)
{
  assert_return (n >= -1, AnySeqImpl());
  if (n < 0)
    n = m_rows.size() - 1;
  assert_return (size_t (n) < m_rows.size(), AnySeqImpl());
  return m_rows[n];
}

void
MemoryListStore::insert (int n, const AnySeqImpl &aseq)
{
  assert_return (aseq.size() == m_columns);
  assert_return (n >= -1);
  assert_return (n <= signed (m_rows.size()));
  if (n < 0)
    n = m_rows.size(); // append
  m_rows.insert (m_rows.begin() + n, aseq);
  sig_inserted.emit (n, n);
}

void
MemoryListStore::update (uint n, const AnySeqImpl &aseq)
{
  assert_return (aseq.size() == m_columns);
  assert_return (n < m_rows.size());
  m_rows[n] = aseq;
  sig_changed.emit (n, n);
}

void
MemoryListStore::remove (uint first, uint last)
{
  assert_return (first < m_rows.size());
  if (last < first)
    last = first;
  m_rows.erase (m_rows.begin() + first, m_rows.begin() + last + 1);
  sig_removed.emit (first, last);
}

} // Rapicorn
