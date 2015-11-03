// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include <rapicorn.hh>

namespace {
using namespace Rapicorn;

class ListStore : public virtual DataListContainer {
  vector<Any> rows_;
protected:
  typedef Aida::Signal<void (const UpdateRequest&)> UpdateSignal;
public:
  explicit ListStore ();
  int      count     () const;                          ///< Obtain the number of rows provided by this model.
  Any      row       (int n) const;                     ///< Read-out row @a n. In-order read outs are generally fastest.
  void     clear     ();                                ///< Remove all rows from the ListModel.
  void     insert    (int n, const Any &aseq);          ///< Insert row @a n.
  void     insert    (int n, const AnySeq &aseqseq);    ///< Insert multiple rows at @a n.
  void     update    (int n, const Any &aseq);          ///< Reassign data to row @a n.
  void     remove    (int n, int length);               ///< Remove @a length rows starting at @a n.
  virtual ~ListStore ();
  UpdateSignal sig_updates; ///< Notify about insertion, changes and deletion of a specific row range.
};

ListStore::ListStore()
{}

ListStore::~ListStore()
{
  clear();
}

int
ListStore::count () const
{
  return rows_.size();
}

Any
ListStore::row (int r) const
{
  return_unless (r >= 0 && r < count(), Any());
  return rows_[r];
}

void
ListStore::clear ()
{
  if (rows_.size())
    remove (0, rows_.size());
}

void
ListStore::insert (int first, const Any &any)
{
  assert_return (first >= 0 && first <= count());
  rows_.insert (rows_.begin() + first, any);
  sig_updates.emit (UpdateRequest (UPDATE_INSERTION, UpdateSpan (first, 1)));
  ApplicationH::the().test_counter_get();
}

void
ListStore::insert (int first, const AnySeq &aseq)
{
  assert_return (first >= 0 && first <= count());
  for (size_t i = 0; i < aseq.size(); i++)
    rows_.insert (rows_.begin() + i, aseq[i]);
  sig_updates.emit (UpdateRequest (UPDATE_INSERTION, UpdateSpan (first, aseq.size())));
  ApplicationH::the().test_counter_get();
}

void
ListStore::update (int first, const Any &any)
{
  assert_return (first >= 0 && first < count());
  rows_[first] = any;
  sig_updates.emit (UpdateRequest (UPDATE_CHANGE, UpdateSpan (first, 1)));
  ApplicationH::the().test_counter_get();
}

void
ListStore::remove (int start, int length)
{
  return_unless (start >= 0 && start < count());
  return_unless (length >= 0);
  return_unless (start + length <= count());
  rows_.erase (rows_.begin() + start, rows_.begin() + start + length);
  sig_updates.emit (UpdateRequest (UPDATE_DELETION, UpdateSpan (start, 1)));
  ApplicationH::the().test_counter_get();
}

} // anon
