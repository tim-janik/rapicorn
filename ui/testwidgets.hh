// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
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
  virtual void   render            (RenderContext &rcontext) override;
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

class TestBoxImpl : public virtual SingleContainerImpl, public virtual TestBoxIface {
  String snapshot_file_;
  uint   handler_id_;
  void   make_snapshot ();
protected:
  virtual void        render          (RenderContext &rcontext) override;
public:
  explicit            TestBoxImpl     ();
  virtual            ~TestBoxImpl     () override;
  virtual String      snapshot_file   () const override;
  virtual void        snapshot_file   (const String &val) override;
};

class IdlTestWidgetImpl : public virtual WidgetImpl, public virtual IdlTestWidgetIface {
  bool bool_; int int_; double float_; std::string string_; TestEnum enum_;
  Requisition rec_; StringSeq seq_; IdlTestWidgetIfaceW self_;
protected:
  virtual void                size_request  (Requisition &req) override;
  virtual void                size_allocate (Allocation area, bool changed) override;
  virtual void                render        (RenderContext &rcontext) override;
public:
  virtual bool                bool_prop     () const override;
  virtual void                bool_prop     (bool b) override;
  virtual int                 int_prop      () const override;
  virtual void                int_prop      (int i) override;
  virtual double              float_prop    () const override;
  virtual void                float_prop    (double f) override;
  virtual std::string         string_prop   () const override;
  virtual void                string_prop   (const std::string &s) override;
  virtual TestEnum            enum_prop     () const override;
  virtual void                enum_prop     (TestEnum v) override;
  virtual Requisition         record_prop   () const override;
  virtual void                record_prop   (const Requisition &r) override;
  virtual StringSeq           sequence_prop () const override;
  virtual void                sequence_prop (const StringSeq &s) override;
  virtual IdlTestWidgetIfaceP self_prop     () const override;
  virtual void                self_prop     (IdlTestWidgetIface *s) override;
};

} // Rapicorn
