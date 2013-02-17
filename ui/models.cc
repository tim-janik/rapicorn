// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "models.hh"
#include "application.hh"

namespace Rapicorn {

ListModelRelayImpl::ListModelRelayImpl (int n_columns) :
  columns_ (n_columns)
{}

ListModelRelayImpl::~ListModelRelayImpl ()
{}

AnySeq
ListModelRelayImpl::row (int r)
{
  const size_t row = r;
  return r >= 0 && row < rows_.size() ? rows_[row] : AnySeq();
}

Any
ListModelRelayImpl::cell (int r, int c)
{
  const size_t row = r, col = c;
  const size_t n_columns = columns_;
  return r >= 0 && row < rows_.size() && c >= 0 && col < n_columns ? rows_[row][col] : Any();
}

void
ListModelRelayImpl::emit_updated (UpdateKind kind, uint start, uint length)
{
  sig_updated.emit (UpdateRequest (kind, UpdateSpan (start, length), UpdateSpan (0, columns_)));
}

void
ListModelRelayImpl::relay (const UpdateRequest &urequest)
{
  switch (urequest.kind)
    {
    case UPDATE_INSERTION:
      assert_return (urequest.rowspan.start >= 0);
      assert_return (urequest.rowspan.start <= size());
      assert_return (urequest.rowspan.length >= 0);
      rows_.insert (rows_.begin() + urequest.rowspan.start, urequest.rowspan.length,
                     ({ AnySeq adefault; adefault.resize (columns_); adefault; }));
      sig_updated.emit (urequest);
      refill (urequest.rowspan.start, urequest.rowspan.length);
      break;
    case UPDATE_CHANGE:
      assert_return (urequest.rowspan.start >= 0);
      assert_return (urequest.rowspan.start <= size());
      assert_return (urequest.rowspan.length >= 0);
      assert_return (urequest.rowspan.start + urequest.rowspan.length <= size());
      refill (urequest.rowspan.start, urequest.rowspan.length); // emits UPDATE_CHANGE later
      break;
    case UPDATE_DELETION:
      assert_return (urequest.rowspan.start >= 0);
      assert_return (urequest.rowspan.start <= size());
      assert_return (urequest.rowspan.length >= 0);
      assert_return (urequest.rowspan.start + urequest.rowspan.length <= size());
      rows_.erase (rows_.begin() + urequest.rowspan.start, rows_.begin() + urequest.rowspan.start + urequest.rowspan.length);
      sig_updated.emit (urequest);
      break;
    case UPDATE_READ: ;
    }
}

void
ListModelRelayImpl::fill (int first, const AnySeqSeq &anyseq)
{
  assert_return (first >= 0);
  if (first >= size())
    return;
  const size_t n_columns = columns_;
  for (size_t i = 0; i < anyseq.size(); i++)
    assert_return (anyseq[i].size() == n_columns);
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
  assert_return (start < size());
  assert_return (length >= 0);
  length = MIN (start + length, size()) - start;
  if (length)
    sig_refill.emit (UpdateRequest (UPDATE_READ, UpdateSpan (start, length), UpdateSpan (0, columns_)));
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
  columns_ (n_columns)
{
  assert_return (n_columns > 0);
}

AnySeq
MemoryListStore::row (int n)
{
  if (n < 0)
    n = rows_.size() + n;
  assert_return (size_t (n) < rows_.size(), AnySeq());
  return rows_[n];
}

Any
MemoryListStore::cell (int r, int c)
{
  if (r < 0)
    r += rows_.size();
  if (c < 0)
    c += columns_;
  assert_return (size_t (r) < rows_.size(), Any());
  assert_return (size_t (c) < columns_, Any());
  return rows_[r][c];
}

void
MemoryListStore::emit_updated (UpdateKind kind, uint start, uint length)
{
  sig_updated.emit (UpdateRequest (kind, UpdateSpan (start, length), UpdateSpan (0, columns_)));
}

void
MemoryListStore::insert (int n, const AnySeq &aseq)
{
  assert_return (aseq.size() == columns_);
  assert_return (n >= -1);
  assert_return (n <= signed (rows_.size()));
  if (n < 0)
    n = rows_.size(); // append
  rows_.insert (rows_.begin() + n, aseq);
  emit_updated (UPDATE_INSERTION, n, 1);
}

void
MemoryListStore::update_row (uint n, const AnySeq &aseq)
{
  assert_return (aseq.size() == columns_);
  assert_return (n < rows_.size());
  rows_[n] = aseq;
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
