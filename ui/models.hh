// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_MODELS_HH__
#define __RAPICORN_MODELS_HH__

#include <ui/item.hh>

namespace Rapicorn {

class ListModelRelayImpl : public virtual ListModelRelayIface, public virtual ListModelIface,
                           public virtual Aida::PropertyHostInterface {
  int                           m_size, m_columns;
  vector<AnySeqImpl>            m_rows;
  explicit                      ListModelRelayImpl (int n_columns);
protected:
  virtual                      ~ListModelRelayImpl ();
  static ListModelRelayImpl&    create_list_model_relay (int n_columns);
public:
  // model API
  virtual int                   size            ()              { return m_size; }
  virtual int                   columns         ();
  virtual AnySeqImpl            row             (int n);
  // relay API
  virtual void                  resize          (int size);
  virtual void                  inserted        (int first, int last);
  virtual void                  changed         (int first, int last);
  virtual void                  removed         (int first, int last);
  virtual void                  fill            (int first, const AnySeqSeqImpl &ss);
  virtual ListModelIface*       model           ()              { return this; }
  void                          refill          (int first, int last);
  virtual const PropertyList&   _property_list  ();
};

class MemoryListStore : public virtual ListModelIface {
  uint                  m_columns;
  vector<AnySeqImpl>    m_rows;
public:
  explicit              MemoryListStore (int n_columns);
  virtual int           size            ()              { return m_rows.size(); }
  virtual int           columns         ()              { return m_columns; }
  virtual AnySeqImpl    row             (int n);
  void                  insert          (int  n, const AnySeqImpl &aseq);
  void                  update          (uint n, const AnySeqImpl &aseq);
  void                  remove          (uint first, uint last = 0);
};

} // Rapicorn

#endif  /* __RAPICORN_MODELS_HH__ */
