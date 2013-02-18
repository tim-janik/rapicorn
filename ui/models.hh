// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_MODELS_HH__
#define __RAPICORN_MODELS_HH__

#include <ui/widget.hh>

namespace Rapicorn {

class ListModelRelayImpl : public virtual ListModelRelayIface, public virtual ListModelIface,
                           public virtual Aida::PropertyHostInterface {
  int                           columns_;
  vector<AnySeq>            rows_;
  explicit                      ListModelRelayImpl (int n_columns);
protected:
  virtual                      ~ListModelRelayImpl ();
  static ListModelRelayImpl&    create_list_model_relay (int n_columns);
  void                          emit_updated    (UpdateKind kind, uint start, uint length);
public:
  // model API
  virtual int                   size            ()              { return rows_.size(); }
  virtual int                   columns         ()              { return columns_; }
  virtual AnySeq                row             (int n);
  virtual Any                   cell            (int r, int c);
  // relay API
  virtual void                  relay           (const UpdateRequest &urequest);
  virtual void                  fill            (int first, const AnySeqSeq &ss);
  virtual ListModelIface*       model           ()              { return this; }
  void                          refill          (int start, int length);
  virtual const PropertyList&   _property_list  ();
};

class MemoryListStore : public virtual ListModelIface {
  vector<AnySeq>        rows_;
  uint                  columns_;
  void                  emit_updated    (UpdateKind kind, uint start, uint length);
public:
  explicit              MemoryListStore (int n_columns);
  virtual int           size            ()              { return rows_.size(); }
  virtual int           columns         ()              { return columns_; }
  virtual AnySeq        row             (int n);
  virtual Any           cell            (int r, int c);
  void                  insert          (int  n, const AnySeq &aseq);
  void                  update_row      (uint n, const AnySeq &aseq);
  void                  remove          (uint start, uint length);
};


} // Rapicorn

#endif  /* __RAPICORN_MODELS_HH__ */
