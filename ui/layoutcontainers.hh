// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_LAYOUT_CONTAINERS_HH__
#define __RAPICORN_LAYOUT_CONTAINERS_HH__

#include <ui/container.hh>

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

class HBox : public virtual ContainerImpl {
protected:
  virtual const PropertyList&   __aida_properties__ ();
public:
  virtual bool  homogeneous     () const = 0;
  virtual void  homogeneous     (bool chomogeneous_widgets) = 0;
  virtual int   spacing         () const = 0;
  virtual void  spacing         (int cspacing) = 0;
};

class VBox : public virtual ContainerImpl {
protected:
  virtual const PropertyList&   __aida_properties__ ();
public:
  virtual bool  homogeneous     () const = 0;
  virtual void  homogeneous     (bool chomogeneous_widgets) = 0;
  virtual int   spacing         () const = 0;
  virtual void  spacing         (int cspacing) = 0;
};

} // Rapicorn

#endif  /* __RAPICORN_LAYOUT_CONTAINERS_HH__ */
