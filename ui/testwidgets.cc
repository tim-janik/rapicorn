// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "testwidgets.hh"
#include "container.hh"
#include "painter.hh"
#include "window.hh"
#include "factory.hh"
#include "selector.hh"
#include "selob.hh"
#include <errno.h>

namespace Rapicorn {

#define DFLTEPS (1e-8)

// == TestContainerImpl ==
static Atomic<uint> test_containers_rendered = 0;

TestContainerImpl::TestContainerImpl () :
  value_ (""), assert_value_ (""),
  accu_ (""), accu_history_ (""),
  assert_left_ (-INFINITY), assert_right_ (-INFINITY),
  assert_top_ (-INFINITY), assert_bottom_ (-INFINITY),
  assert_width_ (-INFINITY), assert_height_ (-INFINITY),
  epsilon_ (DFLTEPS), test_container_counted_ (false),
  fatal_asserts_ (false), paint_allocation_ (false)
{}

uint
TestContainerImpl::seen_test_widgets ()
{
  return test_containers_rendered;
}

String
TestContainerImpl::value () const
{
  return value_;
}

void
TestContainerImpl::value (const String &val)
{
  value_ = val;
  changed ("value");
}

String
TestContainerImpl::assert_value () const
{
  return assert_value_;
}

void
TestContainerImpl::assert_value (const String &val)
{
  assert_value_ = val;
  changed ("assert_value");
}

double
TestContainerImpl::assert_left () const
{
  return assert_left_;
}

void
TestContainerImpl::assert_left (double val)
{
  assert_left_ = val;
  invalidate();
  changed ("assert_left");
}

double
TestContainerImpl::assert_right () const
{
  return assert_right_;
}

void
TestContainerImpl::assert_right (double val)
{
  assert_right_ = val;
  invalidate();
  changed ("assert_right");
}

double
TestContainerImpl::assert_top () const
{
  return assert_top_;
}

void
TestContainerImpl::assert_top (double val)
{
  assert_top_ = val;
  invalidate();
  changed ("assert_top");
}

double
TestContainerImpl::assert_bottom () const
{
  return assert_bottom_;
}

void
TestContainerImpl::assert_bottom (double val)
{
  assert_bottom_ = val;
  invalidate();
  changed ("assert_bottom");
}

double
TestContainerImpl::assert_width () const
{
  return assert_width_;
}

void
TestContainerImpl::assert_width (double val)
{
  assert_width_ = val;
  invalidate();
  changed ("assert_width");
}

double
TestContainerImpl::assert_height () const
{
  return assert_height_;
}

void
TestContainerImpl::assert_height (double val)
{
  assert_height_ = val;
  invalidate();
  changed ("assert_height");
}

double
TestContainerImpl::epsilon () const
{
  return epsilon_;
}

void
TestContainerImpl::epsilon (double val)
{
  epsilon_ = val;
  invalidate();
  changed ("epsilon");
}

bool
TestContainerImpl::paint_allocation () const
{
  return paint_allocation_;
}

void
TestContainerImpl::paint_allocation (bool val)
{
  paint_allocation_ = val;
  invalidate();
  changed ("paint_allocation");
}

bool
TestContainerImpl::fatal_asserts () const
{
  return fatal_asserts_;
}

void
TestContainerImpl::fatal_asserts (bool val)
{
  fatal_asserts_ = val;
  invalidate();
  changed ("fatal_asserts");
}

String
TestContainerImpl::accu () const
{
  return accu_;
}

void
TestContainerImpl::accu (const String &val)
{
  accu_ = val;
  accu_history_ += val;
  changed ("accu");
}

String
TestContainerImpl::accu_history () const
{
  return accu_history_;
}

void
TestContainerImpl::accu_history (const String &val)
{
  accu_history_ = val;
  changed ("accu_history");
}

void
TestContainerImpl::assert_value (const char *assertion_name, double value, double first, double second)
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
      String msg = string_format ("%s == %f", assertion_name, value);
      sig_assertion_ok.emit (msg);
    }
}

void
TestContainerImpl::render (RenderContext &rcontext, const Rect &rect)
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

Selector::Selob*
TestContainerImpl::pseudo_selector (Selector::Selob &selob, const String &ident, const String &arg, String &error)
{
  if (ident == ":test-pass")
    return string_to_bool (arg) ? Selector::Selob::true_match() : NULL;
  else if (ident == "::test-parent")
    return Selector::SelobAllocator::selob_allocator (selob)->widget_selob (*parent());
  else
    return NULL;
}

static const WidgetFactory<TestContainerImpl> test_container_factory ("Rapicorn::Factory::TestContainer");

// == TestBoxImpl ==
const PropertyList&
TestBox::__aida_properties__()
{
  static Property *properties[] = {
    MakeProperty (TestBox, snapshot_file, _("Snapshot File Name"), _("PNG image file name to write snapshot to"), "rw"),
  };
  static const PropertyList property_list (properties, ContainerImpl::__aida_properties__());
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
    WindowImpl *wwidget = get_window();
    if (snapshot_file_ != "" && wwidget)
      {
        cairo_surface_t *isurface = wwidget->create_snapshot (allocation());
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
        WindowImpl *wwidget = get_window();
        if (wwidget)
          {
            EventLoop *loop = wwidget->get_loop();
            if (loop)
              handler_id_ = loop->exec_now (Aida::slot (*this, &TestBoxImpl::make_snapshot));
          }
      }
  }
};
static const WidgetFactory<TestBoxImpl> test_box_factory ("Rapicorn::Factory::TestBox");

class IdlTestWidgetImpl : public virtual WidgetImpl, public virtual IdlTestWidgetIface {
  bool bool_; int int_; double float_; std::string string_; TestEnum enum_;
  Requisition rec_; StringSeq seq_; IdlTestWidgetIface *self_;
  virtual bool          bool_prop () const                      { return bool_; }
  virtual void          bool_prop (bool b)                      { bool_ = b; changed ("bool_prop"); }
  virtual int           int_prop () const                       { return int_; }
  virtual void          int_prop (int i)                        { int_ = i; changed ("int_prop"); }
  virtual double        float_prop () const                     { return float_; }
  virtual void          float_prop (double f)                   { float_ = f; changed ("float_prop"); }
  virtual std::string   string_prop () const                    { return string_; }
  virtual void          string_prop (const std::string &s)      { string_ = s; changed ("string_prop"); }
  virtual TestEnum      enum_prop () const                      { return enum_; }
  virtual void          enum_prop (TestEnum v)                  { enum_ = v; changed ("enum_prop"); }
  virtual Requisition   record_prop () const                    { return rec_; }
  virtual void          record_prop (const Requisition &r)      { rec_ = r; changed ("record_prop"); }
  virtual StringSeq     sequence_prop () const                  { return seq_; }
  virtual void          sequence_prop (const StringSeq &s)      { seq_ = s; changed ("sequence_prop"); }
  virtual IdlTestWidgetIface* self_prop () const                { return self_; }
  virtual void                self_prop (IdlTestWidgetIface *s) { self_ = s; changed ("self_prop"); }
  virtual void          size_request (Requisition &req)         { req = Requisition (12, 12); }
  virtual void          size_allocate (Allocation area, bool changed) {}
  virtual void          render (RenderContext &rcontext, const Rect &rect) {}
  virtual const PropertyList& __aida_properties__ () { return RAPICORN_AIDA_PROPERTY_CHAIN (WidgetImpl::__aida_properties__(), IdlTestWidgetIface::__aida_properties__()); }
};
static const WidgetFactory<IdlTestWidgetImpl> test_widget_factory ("Rapicorn::Factory::IdlTestWidget");

} // Rapicorn
