// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "testitems.hh"
#include "container.hh"
#include "painter.hh"
#include "window.hh"
#include "factory.hh"
#include "selector.hh"
#include "selob.hh"
#include <errno.h>

namespace Rapicorn {

TestContainer::TestContainer()
{}

#define DFLTEPS (1e-8)

const PropertyList&
TestContainer::_property_list()
{
  /* not using _() here, because TestContainer is just a developer tool */
  static Property *properties[] = {
    MakeProperty (TestContainer, value,         "Value",         "Store string value for assertion",           "rw"),
    MakeProperty (TestContainer, assert_value,  "Assert Value",  "Assert a particular string value",           "rw"),
    MakeProperty (TestContainer, assert_left,   "Assert-Left",   "Assert positioning of the left item edge",   -INFINITY, +DBL_MAX, 3, "rw"),
    MakeProperty (TestContainer, assert_right,  "Assert-Right",  "Assert positioning of the right item edge",  -INFINITY, +DBL_MAX, 3, "rw"),
    MakeProperty (TestContainer, assert_bottom, "Assert-Bottom", "Assert positioning of the bottom item edge", -INFINITY, +DBL_MAX, 3, "rw"),
    MakeProperty (TestContainer, assert_top,    "Assert-Top",    "Assert positioning of the top item edge",    -INFINITY, +DBL_MAX, 3, "rw"),
    MakeProperty (TestContainer, assert_width,  "Assert-Width",  "Assert amount of the item width",            -INFINITY, +DBL_MAX, 3, "rw"),
    MakeProperty (TestContainer, assert_height, "Assert-Height", "Assert amount of the item height",           -INFINITY, +DBL_MAX, 3, "rw"),
    MakeProperty (TestContainer, epsilon,       "Epsilon",       "Epsilon within which assertions must hold",  0,         +DBL_MAX, 0.01, "rw"),
    MakeProperty (TestContainer, paint_allocation, "Paint Allocation", "Fill allocation rectangle with a solid color",  "rw"),
    MakeProperty (TestContainer, fatal_asserts, "Fatal-Asserts", "Handle assertion failures as fatal errors",  "rw"),
    MakeProperty (TestContainer, accu,          "Accumulator",   "Store string value and keep history",        "rw"),
    MakeProperty (TestContainer, accu_history,  "Accu-History",  "Concatenated accumulator history",           "rw"),
  };
  static const PropertyList property_list (properties, ContainerImpl::_property_list());
  return property_list;
}

static uint test_containers_rendered = 0;

uint
TestContainer::seen_test_items ()
{
  return test_containers_rendered;
}

class TestContainerImpl : public virtual SingleContainerImpl, public virtual TestContainer {
  String value_, assert_value_;
  String accu_, accu_history_;
  double assert_left_, assert_right_;
  double assert_top_, assert_bottom_;
  double assert_width_, assert_height_;
  double epsilon_;
  bool   test_container_counted_;
  bool   fatal_asserts_;
  bool   paint_allocation_;
public:
  TestContainerImpl() :
    value_ (""), assert_value_ (""),
    accu_ (""), accu_history_ (""),
    assert_left_ (-INFINITY), assert_right_ (-INFINITY),
    assert_top_ (-INFINITY), assert_bottom_ (-INFINITY),
    assert_width_ (-INFINITY), assert_height_ (-INFINITY),
    epsilon_ (DFLTEPS), test_container_counted_ (false),
    fatal_asserts_ (false), paint_allocation_ (false)
  {}
  virtual String value           () const            { return value_; }
  virtual void   value           (const String &val) { value_ = val; }
  virtual String assert_value    () const            { return assert_value_; }
  virtual void   assert_value    (const String &val) { assert_value_ = val; }
  virtual double assert_left     () const        { return assert_left_  ; }
  virtual void   assert_left     (double val)    { assert_left_ = val; invalidate(); }
  virtual double assert_right    () const        { return assert_right_ ; }
  virtual void   assert_right    (double val)    { assert_right_ = val; invalidate(); }
  virtual double assert_top      () const        { return assert_top_   ; }
  virtual void   assert_top      (double val)    { assert_top_ = val; invalidate(); }
  virtual double assert_bottom   () const        { return assert_bottom_; }
  virtual void   assert_bottom   (double val)    { assert_bottom_ = val; invalidate(); }
  virtual double assert_width    () const        { return assert_width_ ; }
  virtual void   assert_width    (double val)    { assert_width_ = val; invalidate(); }
  virtual double assert_height   () const        { return assert_height_; }
  virtual void   assert_height   (double val)    { assert_height_ = val; invalidate(); }
  virtual double epsilon         () const        { return epsilon_  ; }
  virtual void   epsilon         (double val)    { epsilon_ = val; invalidate(); }
  virtual bool   paint_allocation() const        { return paint_allocation_; }
  virtual void   paint_allocation(bool   val)    { paint_allocation_ = val; invalidate(); }
  virtual bool   fatal_asserts   () const        { return fatal_asserts_; }
  virtual void   fatal_asserts   (bool   val)    { fatal_asserts_ = val; invalidate(); }
  virtual String accu            () const            { return accu_; }
  virtual void   accu            (const String &val) { accu_ = val; accu_history_ += val; }
  virtual String accu_history    () const            { return accu_history_; }
  virtual void   accu_history    (const String &val) { accu_history_ = val; }
protected:
  virtual void
  size_request (Requisition &requisition)
  {
    SingleContainerImpl::size_request (requisition);
  }
  void
  assert_value (const char *assertion_name,
                double      value,
                double      first,
                double      second)
  {
    if (!isnormal (value))
      return;
    double rvalue;
    if (value >= 0)
      rvalue = first;
    else /* value < 0 */
      {
        rvalue = second;
        value = fabs (value);
      }
    double delta = fabs (rvalue - value);
    if (delta > epsilon_)
      {
        if (fatal_asserts_)
          fatal ("similarity exceeded: %s=%f real-value=%f delta=%f (epsilon=%g)",
                 assertion_name, value, rvalue, delta, epsilon_);
        else
          critical ("similarity exceeded: %s=%f real-value=%f delta=%f (epsilon=%g)",
                    assertion_name, value, rvalue, delta, epsilon_);
      }
    else
      {
        String msg = string_printf ("%s == %f", assertion_name, value);
        sig_assertion_ok.emit (msg);
      }
  }
  virtual void
  render (RenderContext &rcontext, const Rect &rect)
  {
    if (paint_allocation_)
      {
        IRect ia = allocation();
        cairo_t *cr = cairo_context (rcontext, rect);
        CPainter painter (cr);
        painter.draw_filled_rect (ia.x, ia.y, ia.width, ia.height, heritage()->black());
      }

    Allocation rarea = get_window()->allocation();
    double width = allocation().width, height = allocation().height;
    double x1 = allocation().x, x2 = rarea.width - x1 - width;
    double y1 = allocation().y, y2 = rarea.height - y1 - height;
    assert_value ("assert-bottom", assert_bottom_, y1, y2);
    assert_value ("assert-right",  assert_right_,  x1, x2);
    assert_value ("assert-top",    assert_top_,    y1, y2);
    assert_value ("assert-left",   assert_left_,   x1, x2);
    assert_value ("assert-width",  assert_width_, width, width);
    assert_value ("assert-height", assert_height_, height, height);
    if (assert_value_ != value_)
      {
        if (fatal_asserts_)
          fatal ("value has unexpected contents: %s (expected: %s)", value_.c_str(), assert_value_.c_str());
        else
          critical ("value has unexpected contents: %s (expected: %s)", value_.c_str(), assert_value_.c_str());
      }
    sig_assertions_passed.emit ();
    /* count containers for seen_test_containers() */
    if (!test_container_counted_)
      {
        test_container_counted_ = true;
        test_containers_rendered++;
      }
  }
  virtual Selector::Selob*
  pseudo_selector (Selector::Selob &selob, const String &ident, const String &arg, String &error)
  {
    if (ident == ":test-pass")
      return string_to_bool (arg) ? Selector::Selob::true_match() : NULL;
    else if (ident == "::test-parent")
      return Selector::SelobAllocator::selob_allocator (selob)->item_selob (*parent());
    else
      return NULL;
  }
};
static const ItemFactory<TestContainerImpl> test_container_factory ("Rapicorn::Factory::TestContainer");

const PropertyList&
TestBox::_property_list()
{
  static Property *properties[] = {
    MakeProperty (TestBox, snapshot_file, _("Snapshot File Name"), _("PNG image file name to write snapshot to"), "rw"),
  };
  static const PropertyList property_list (properties, ContainerImpl::_property_list());
  return property_list;
}

class TestBoxImpl : public virtual SingleContainerImpl, public virtual TestBox {
  String snapshot_file_;
  uint   handler_id_;
protected:
  virtual String snapshot_file () const                 { return snapshot_file_; }
  virtual void   snapshot_file (const String &val)      { snapshot_file_ = val; invalidate(); }
  ~TestBoxImpl()
  {
    if (handler_id_)
      {
        remove_exec (handler_id_);
        handler_id_ = 0;
      }
  }
  void
  make_snapshot ()
  {
    WindowImpl *witem = get_window();
    if (snapshot_file_ != "" && witem)
      {
        cairo_surface_t *isurface = witem->create_snapshot (allocation());
        cairo_status_t wstatus = cairo_surface_write_to_png (isurface, snapshot_file_.c_str());
        cairo_surface_destroy (isurface);
        String err = CAIRO_STATUS_SUCCESS == wstatus ? "ok" : cairo_status_to_string (wstatus);
        printerr ("%s: wrote %s: %s\n", name().c_str(), snapshot_file_.c_str(), err.c_str());
      }
    if (handler_id_)
      {
        remove_exec (handler_id_);
        handler_id_ = 0;
      }
  }
public:
  explicit TestBoxImpl() :
    handler_id_ (0)
  {}
  virtual void
  render (RenderContext &rcontext, const Rect &rect)
  {
    if (!handler_id_)
      {
        WindowImpl *witem = get_window();
        if (witem)
          {
            EventLoop *loop = witem->get_loop();
            if (loop)
              handler_id_ = loop->exec_now (Aida::slot (*this, &TestBoxImpl::make_snapshot));
          }
      }
  }
};
static const ItemFactory<TestBoxImpl> test_box_factory ("Rapicorn::Factory::TestBox");

class IdlTestItemImpl : public virtual ItemImpl, public virtual IdlTestItemIface {
  bool bool_; int int_; double float_; std::string string_; TestEnum enum_;
  Requisition rec_; StringSeq seq_; IdlTestItemIface *self_;
  virtual bool          bool_prop () const                      { return bool_; }
  virtual void          bool_prop (bool b)                      { bool_ = b; }
  virtual int           int_prop () const                       { return int_; }
  virtual void          int_prop (int i)                        { int_ = i; }
  virtual double        float_prop () const                     { return float_; }
  virtual void          float_prop (double f)                   { float_ = f; }
  virtual std::string   string_prop () const                    { return string_; }
  virtual void          string_prop (const std::string &s)      { string_ = s; }
  virtual TestEnum      enum_prop () const                      { return enum_; }
  virtual void          enum_prop (TestEnum v)                  { enum_ = v; }
  virtual Requisition   record_prop () const                    { return rec_; }
  virtual void          record_prop (const Requisition &r)      { rec_ = r; }
  virtual StringSeq     sequence_prop () const                  { return seq_; }
  virtual void          sequence_prop (const StringSeq &s)      { seq_ = s; }
  virtual IdlTestItemIface* self_prop () const                  { return self_; }
  virtual void          self_prop (IdlTestItemIface *s)         { self_ = s; }
  virtual void          size_request (Requisition &req)         { req = Requisition (12, 12); }
  virtual void          size_allocate (Allocation area, bool changed) {}
  virtual void          render (RenderContext &rcontext, const Rect &rect) {}
  virtual const PropertyList& _property_list () { return RAPICORN_AIDA_PROPERTY_CHAIN (ItemImpl::_property_list(), IdlTestItemIface::_property_list()); }
};
static const ItemFactory<IdlTestItemImpl> test_item_factory ("Rapicorn::Factory::IdlTestItem");

} // Rapicorn
