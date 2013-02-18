// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "models.hh"
#include "application.hh"

namespace Rapicorn {

ListModelRelayImpl::ListModelRelayImpl ()
{}

ListModelRelayImpl::~ListModelRelayImpl ()
{}

Any
ListModelRelayImpl::row (int r)
{
  const size_t row = r;
  return r >= 0 && row < rows_.size() ? rows_[row] : Any();
}

void
ListModelRelayImpl::emit_updated (UpdateKind kind, uint start, uint length)
{
  sig_updated.emit (UpdateRequest (kind, UpdateSpan (start, length)));
}

void
ListModelRelayImpl::update (const UpdateRequest &urequest)
{
  switch (urequest.kind)
    {
    case UPDATE_INSERTION:
      assert_return (urequest.rowspan.start >= 0);
      assert_return (urequest.rowspan.start <= count());
      assert_return (urequest.rowspan.length >= 0);
      rows_.insert (rows_.begin() + urequest.rowspan.start, urequest.rowspan.length, Any());
      sig_updated.emit (urequest);
      refill (urequest.rowspan.start, urequest.rowspan.length);
      break;
    case UPDATE_CHANGE:
      assert_return (urequest.rowspan.start >= 0);
      assert_return (urequest.rowspan.start <= count());
      assert_return (urequest.rowspan.length >= 0);
      assert_return (urequest.rowspan.start + urequest.rowspan.length <= count());
      refill (urequest.rowspan.start, urequest.rowspan.length); // emits UPDATE_CHANGE later
      break;
    case UPDATE_DELETION:
      assert_return (urequest.rowspan.start >= 0);
      assert_return (urequest.rowspan.start <= count());
      assert_return (urequest.rowspan.length >= 0);
      assert_return (urequest.rowspan.start + urequest.rowspan.length <= count());
      rows_.erase (rows_.begin() + urequest.rowspan.start, rows_.begin() + urequest.rowspan.start + urequest.rowspan.length);
      sig_updated.emit (urequest);
      break;
    case UPDATE_READ: ;
    }
}

void
ListModelRelayImpl::fill (int first, const AnySeq &anyseq)
{
  assert_return (first >= 0);
  if (first >= count())
    return;
  size_t i;
  for (i = 0; i < anyseq.size() && first + i < rows_.size(); i++)
    rows_[first + i] = anyseq[i];
  if (i)
    emit_updated (UPDATE_CHANGE, first, i);
}

void
ListModelRelayImpl::refill (int start, int length)
{
  assert_return (start >= 0);
  assert_return (start < count());
  assert_return (length >= 0);
  length = MIN (start + length, count()) - start;
  if (length)
    sig_refill.emit (UpdateRequest (UPDATE_READ, UpdateSpan (start, length)));
}

ListModelRelayImpl&
ListModelRelayImpl::create_list_model_relay()
{
  ListModelRelayImpl *relay = new ListModelRelayImpl ();
  return *relay;
}

const PropertyList&
ListModelRelayImpl::_property_list ()
{
  return RAPICORN_AIDA_PROPERTY_CHAIN (ListModelRelayIface::_property_list(), ListModelIface::_property_list());
}

ListModelRelayIface*
ApplicationImpl::create_list_model_relay (const std::string &name)
{
  struct Accessor : public ListModelRelayImpl { using ListModelRelayImpl::create_list_model_relay; };
  ListModelRelayImpl &lmr = Accessor::create_list_model_relay();
  if (ApplicationImpl::the().xurl_add (name, lmr))
    return &lmr;
  else
    {
      unref (ref_sink (lmr));
      return NULL;
    }
}

MemoryListStore::MemoryListStore (int n_columns) :
  columns_ (n_columns)
{
  assert_return (n_columns > 0);
}

Any
MemoryListStore::row (int n)
{
  if (n < 0)
    n = rows_.size() + n;
  assert_return (size_t (n) < rows_.size(), Any());
  return rows_[n];
}

void
MemoryListStore::emit_updated (UpdateKind kind, uint start, uint length)
{
  sig_updated.emit (UpdateRequest (kind, UpdateSpan (start, length), UpdateSpan (0, columns_)));
}

void
MemoryListStore::insert (int n, const Any &any)
{
  assert_return (n >= -1);
  assert_return (n <= signed (rows_.size()));
  if (n < 0)
    n = rows_.size(); // append
  rows_.insert (rows_.begin() + n, any);
  emit_updated (UPDATE_INSERTION, n, 1);
}

void
MemoryListStore::update_row (uint n, const Any &any)
{
  assert_return (n < rows_.size());
  rows_[n] = any;
  emit_updated (UPDATE_CHANGE, n, 1);
}

void
MemoryListStore::remove (uint start, uint length)
{
  assert_return (start < rows_.size());
  assert_return (start + length <= rows_.size());
  rows_.erase (rows_.begin() + start, rows_.begin() + start + length);
  emit_updated (UPDATE_DELETION, start, length);
}

} // Rapicorn
