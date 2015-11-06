// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "models.hh"
#include "application.hh"

namespace Rapicorn {

ListModelRelayImpl::ListModelRelayImpl () :
  model_ (std::make_shared<RelayModel>())
{}

ListModelRelayImpl::~ListModelRelayImpl ()
{
  model_.reset();
}

Any
ListModelRelayImpl::RelayModel::row (int r)
{
  const size_t row = r;
  return r >= 0 && row < rows_.size() ? rows_[row] : Any();
}

void
ListModelRelayImpl::emit_updated (UpdateKind kind, uint start, uint length)
{
  model_->sig_updated.emit (UpdateRequest (kind, UpdateSpan (start, length)));
}

void
ListModelRelayImpl::update (const UpdateRequest &urequest)
{
  switch (urequest.kind)
    {
    case UPDATE_INSERTION:
      assert_return (urequest.rowspan.start >= 0);
      assert_return (urequest.rowspan.start <= model_->count());
      assert_return (urequest.rowspan.length >= 0);
      model_->rows_.insert (model_->rows_.begin() + urequest.rowspan.start, urequest.rowspan.length, Any());
      model_->sig_updated.emit (urequest);
      refill (urequest.rowspan.start, urequest.rowspan.length);
      break;
    case UPDATE_CHANGE:
      assert_return (urequest.rowspan.start >= 0);
      assert_return (urequest.rowspan.start <= model_->count());
      assert_return (urequest.rowspan.length >= 0);
      assert_return (urequest.rowspan.start + urequest.rowspan.length <= model_->count());
      refill (urequest.rowspan.start, urequest.rowspan.length); // emits UPDATE_CHANGE later
      break;
    case UPDATE_DELETION:
      assert_return (urequest.rowspan.start >= 0);
      assert_return (urequest.rowspan.start <= model_->count());
      assert_return (urequest.rowspan.length >= 0);
      assert_return (urequest.rowspan.start + urequest.rowspan.length <= model_->count());
      model_->rows_.erase (model_->rows_.begin() + urequest.rowspan.start, model_->rows_.begin() + urequest.rowspan.start + urequest.rowspan.length);
      model_->sig_updated.emit (urequest);
      break;
    case UPDATE_READ: ;
    }
}

void
ListModelRelayImpl::fill (int first, const AnySeq &anyseq)
{
  assert_return (first >= 0);
  if (first >= model_->count())
    return;
  size_t i;
  for (i = 0; i < anyseq.size() && first + i < model_->rows_.size(); i++)
    model_->rows_[first + i] = anyseq[i];
  if (i)
    emit_updated (UPDATE_CHANGE, first, i);
}

void
ListModelRelayImpl::refill (int start, int length)
{
  assert_return (start >= 0);
  assert_return (start < model_->count());
  assert_return (length >= 0);
  length = MIN (start + length, model_->count()) - start;
  if (length)
    sig_refill.emit (UpdateRequest (UPDATE_READ, UpdateSpan (start, length)));
}

ListModelRelayImplP
ListModelRelayImpl::create_list_model_relay ()
{
  return FriendAllocator<ListModelRelayImpl>::make_shared ();
}

ListModelRelayIfaceP
ApplicationImpl::create_list_model_relay ()
{
  struct Accessor : public ListModelRelayImpl { using ListModelRelayImpl::create_list_model_relay; };
  return Accessor::create_list_model_relay();
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
