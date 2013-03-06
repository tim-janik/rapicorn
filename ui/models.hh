// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_MODELS_HH__
#define __RAPICORN_MODELS_HH__

#include <ui/widget.hh>

namespace Rapicorn {

class ListModelRelayImpl : public virtual ListModelRelayIface, protected virtual ListModelIface {
  vector<Any>                   rows_;
  explicit                      ListModelRelayImpl ();
protected:
  virtual                      ~ListModelRelayImpl ();
  static ListModelRelayImpl&    create_list_model_relay();
  void                          emit_updated    (UpdateKind kind, uint start, uint length);
public:
  // __aida_type_name__: hide the fact that we are *also* a ListModelIface, access to that aspect is provided via model()
  virtual std::string           __aida_type_name__ () { return ListModelRelayIface::__aida_type_name__(); }
  // model API
  virtual int                   count           ()              { return rows_.size(); }
  virtual Any                   row             (int n);
  // relay API
  virtual void                  update          (const UpdateRequest &urequest);
  virtual void                  fill            (int first, const AnySeq &aseq);
  virtual ListModelIface*       model           ()              { return this; }
  void                          refill          (int start, int length);
  virtual const PropertyList&   _property_list  ();
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
