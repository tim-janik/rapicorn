// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_LAYOUT_CONTAINERS_HH__
#define __RAPICORN_LAYOUT_CONTAINERS_HH__

#include <ui/container.hh>
#include <ui/table.hh>
#include <ui/painter.hh>

namespace Rapicorn {

class AlignmentImpl : public virtual SingleContainerImpl, public virtual AlignmentIface {
  uint16 left_padding_, right_padding_;
  uint16 bottom_padding_, top_padding_;
  virtual int   padding         () const;
protected:
  virtual void  size_request    (Requisition &requisition) override;
  virtual void  size_allocate   (Allocation area) override;
public:
  explicit      AlignmentImpl   ();
  virtual      ~AlignmentImpl   () override;
  virtual int   left_padding    () const override;
  virtual void  left_padding    (int c) override;
  virtual int   right_padding   () const override;
  virtual void  right_padding   (int c) override;
  virtual int   bottom_padding  () const override;
  virtual void  bottom_padding  (int c) override;
  virtual int   top_padding     () const override;
  virtual void  top_padding     (int c) override;
  virtual void  padding         (int c) override;
};

class HBoxImpl : public virtual TableLayoutImpl, public virtual HBoxIface {
protected:
  virtual String add_child   (WidgetImplP widget) override;
public:
  explicit       HBoxImpl    ();
  virtual        ~HBoxImpl   () override;
  virtual bool   homogeneous () const override;
  virtual void   homogeneous (bool homogeneous_widgets) override;
  virtual int    spacing     () const override;
  virtual void   spacing     (int cspacing) override;
};

class VBoxImpl : public virtual TableLayoutImpl, public virtual VBoxIface {
protected:
  virtual String add_child   (WidgetImplP widget) override;
public:
  explicit       VBoxImpl    ();
  virtual        ~VBoxImpl   () override;
  virtual bool   homogeneous () const override;
  virtual void   homogeneous (bool homogeneous_widgets) override;
  virtual int    spacing     () const override;
  virtual void   spacing     (int vspacing) override;
};

} // Rapicorn

#endif  /* __RAPICORN_LAYOUT_CONTAINERS_HH__ */
