// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include <ui/container.hh>

namespace Rapicorn {

class TestContainer : public virtual ContainerImpl {
protected:
  explicit      TestContainer     ();
public:
  virtual String value            () const = 0;
  virtual void   value            (const String &val) = 0;
  virtual String assert_value     () const = 0;
  virtual void   assert_value     (const String &val) = 0;
  virtual double assert_left      () const = 0;
  virtual void   assert_left      (double val) = 0;
  virtual double assert_right     () const = 0;
  virtual void   assert_right     (double val) = 0;
  virtual double assert_top       () const = 0;
  virtual void   assert_top       (double val) = 0;
  virtual double assert_bottom    () const = 0;
  virtual void   assert_bottom    (double val) = 0;
  virtual double assert_width     () const = 0;
  virtual void   assert_width     (double val) = 0;
  virtual double assert_height    () const = 0;
  virtual void   assert_height    (double val) = 0;
  virtual double epsilon          () const = 0;
  virtual void   epsilon          (double val) = 0;
  virtual bool   paint_allocation () const = 0;
  virtual void   paint_allocation (bool   val) = 0;
  virtual bool   fatal_asserts    () const = 0;
  virtual void   fatal_asserts    (bool   val) = 0;
  virtual String accu             () const = 0;
  virtual void   accu             (const String &val) = 0;
  virtual String accu_history     () const = 0;
  virtual void   accu_history     (const String &val) = 0;
  static uint    seen_test_widgets  ();
  const PropertyList&                                    _property_list ();
  Aida::Signal<void (const String &assertion)>  sig_assertion_ok;
  Aida::Signal<void ()>                         sig_assertions_passed;
};

class TestBox : public virtual ContainerImpl {
public:
  virtual String      snapshot_file   () const = 0;
  virtual void        snapshot_file   (const String &val) = 0;
  const PropertyList& _property_list ();
};

} // Rapicorn
