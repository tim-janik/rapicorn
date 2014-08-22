// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include <ui/container.hh>

namespace Rapicorn {

class TestContainerImpl : public virtual SingleContainerImpl, public virtual TestContainerIface {
  String value_, assert_value_, accu_, accu_history_;
  double assert_left_, assert_right_, assert_top_, assert_bottom_, assert_width_, assert_height_, epsilon_;
  bool   test_container_counted_, fatal_asserts_, paint_allocation_;
  void   assert_value (const char *assertion_name, double value, double first, double second);
protected:
  virtual Selector::
  Selob*         pseudo_selector   (Selector::Selob &selob, const String &ident, const String &arg, String &error) override;
  virtual void   render            (RenderContext &rcontext, const Rect &rect) override;
public:
  explicit       TestContainerImpl ();
  virtual String value             () const override;
  virtual void   value             (const String &val) override;
  virtual String assert_value      () const override;
  virtual void   assert_value      (const String &val) override;
  virtual double assert_left       () const override;
  virtual void   assert_left       (double val) override;
  virtual double assert_right      () const override;
  virtual void   assert_right      (double val) override;
  virtual double assert_top        () const override;
  virtual void   assert_top        (double val) override;
  virtual double assert_bottom     () const override;
  virtual void   assert_bottom     (double val) override;
  virtual double assert_width      () const override;
  virtual void   assert_width      (double val) override;
  virtual double assert_height     () const override;
  virtual void   assert_height     (double val) override;
  virtual double epsilon           () const override;
  virtual void   epsilon           (double val) override;
  virtual bool   paint_allocation  () const override;
  virtual void   paint_allocation  (bool   val) override;
  virtual bool   fatal_asserts     () const override;
  virtual void   fatal_asserts     (bool   val) override;
  virtual String accu              () const override;
  virtual void   accu              (const String &val) override;
  virtual String accu_history      () const override;
  virtual void   accu_history      (const String &val) override;
  static uint    seen_test_widgets ();
  Aida::Signal<void (const String &assertion)>  sig_assertion_ok;
  Aida::Signal<void ()>                         sig_assertions_passed;
};

class TestBox : public virtual ContainerImpl {
public:
  virtual String      snapshot_file   () const = 0;
  virtual void        snapshot_file   (const String &val) = 0;
  const PropertyList& __aida_properties__ ();
};

} // Rapicorn
