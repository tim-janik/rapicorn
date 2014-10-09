// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_MODELS_HH__
#define __RAPICORN_MODELS_HH__

#include <ui/widget.hh>

namespace Rapicorn {

class ListModelRelayImpl : public virtual ListModelRelayIface {
  struct RelayModel : public virtual ListModelIface {
    vector<Any>                 rows_;
    virtual int                 count           ()              { return rows_.size(); }
    virtual Any                 row             (int n);
    virtual void                delete_this     ()              { /* do nothing for embedded object */ }
  };
  RelayModel                    model_;
  void                          emit_updated            (UpdateKind kind, uint start, uint length);
  explicit                      ListModelRelayImpl      ();
protected:
  virtual                      ~ListModelRelayImpl      ();
  static ListModelRelayImpl&    create_list_model_relay ();
public:
  virtual void                  update          (const UpdateRequest &urequest);
  virtual void                  fill            (int first, const AnySeq &aseq);
  virtual ListModelIface*       model           ()              { return &model_; }
  void                          refill          (int start, int length);
};

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


} // Rapicorn

#endif  /* __RAPICORN_MODELS_HH__ */
