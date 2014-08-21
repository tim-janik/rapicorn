// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_LAYOUT_CONTAINERS_HH__
#define __RAPICORN_LAYOUT_CONTAINERS_HH__

#include <ui/container.hh>
#include <ui/table.hh>

namespace Rapicorn {

class AlignmentImpl : public virtual SingleContainerImpl, public virtual AlignmentIface {
  uint16 left_padding_, right_padding_;
  uint16 bottom_padding_, top_padding_;
  virtual int   padding         () const;
protected:
  virtual      ~AlignmentImpl   () override;
  virtual void  size_request    (Requisition &requisition) override;
  virtual void  size_allocate   (Allocation area, bool changed) override;
public:
  explicit      AlignmentImpl   ();
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
  virtual      ~HBoxImpl        () override;
  virtual void  add_child       (WidgetImpl &widget) override;
public:
  explicit      HBoxImpl        ();
  virtual bool  homogeneous     () const override;
  virtual void  homogeneous     (bool homogeneous_widgets) override;
  virtual int   spacing         () const override;
  virtual void  spacing         (int cspacing) override;
};

class VBoxImpl : public virtual TableLayoutImpl, public virtual VBoxIface {
protected:
  virtual      ~VBoxImpl        () override;
  virtual void  add_child       (WidgetImpl &widget) override;
public:
  explicit      VBoxImpl        ();
  virtual bool  homogeneous     () const override;
  virtual void  homogeneous     (bool homogeneous_widgets) override;
  virtual int   spacing         () const override;
  virtual void  spacing         (int vspacing) override;
};

} // Rapicorn

#endif  /* __RAPICORN_LAYOUT_CONTAINERS_HH__ */
