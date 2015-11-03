// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_MODELS_HH__
#define __RAPICORN_MODELS_HH__

#include <ui/widget.hh>

namespace Rapicorn {

class ListModelRelayImpl;
typedef std::shared_ptr<ListModelRelayImpl> ListModelRelayImplP;
class ListModelRelayImpl : public virtual ListModelRelayIface {
  struct RelayModel : public virtual ListModelIface {
    vector<Any>                 rows_;
    virtual int                 count           ()              { return rows_.size(); }
    virtual Any                 row             (int n);
    virtual void                delete_this     ()              { /* do nothing for embedded object */ }
  };
  typedef std::shared_ptr<RelayModel> RelayModelP;
  RelayModelP                   model_;
  void                          emit_updated            (UpdateKind kind, uint start, uint length);
  explicit                      ListModelRelayImpl      ();
  friend class                  FriendAllocator<ListModelRelayImpl>;    // provide make_shared for non-public ctor
protected:
  virtual                      ~ListModelRelayImpl      ();
  static ListModelRelayImplP    create_list_model_relay ();
public:
  virtual void                  update          (const UpdateRequest &urequest) override;
  virtual void                  fill            (int first, const AnySeq &aseq) override;
  virtual ListModelIfaceP       model           () override             { return model_; }
  void                          refill          (int start, int length);
};
typedef std::shared_ptr<ListModelRelayImpl> ListModelRelayImplP;
typedef std::weak_ptr  <ListModelRelayImpl> ListModelRelayImplW;

class MemoryListStore : public virtual ListModelIface {
  vector<Any>           rows_;
  uint                  columns_;
  void                  emit_updated    (UpdateKind kind, uint start, uint length);
public:
  explicit              MemoryListStore (int n_columns);
  virtual int           count           ()              { return rows_.size(); }
  virtual Any           row             (int n);
  void                  insert          (int  n, const Any &aseq);
  void                  update_row      (uint n, const Any &aseq);
  void                  remove          (uint start, uint length);
};
typedef std::shared_ptr<MemoryListStore> MemoryListStoreP;
typedef std::weak_ptr  <MemoryListStore> MemoryListStoreW;

} // Rapicorn

#endif  /* __RAPICORN_MODELS_HH__ */
