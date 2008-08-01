/* Rapicorn
 * Copyright (C) 2006 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "testitems.hh"
#include "containerimpl.hh"
#include "painter.hh"
#include "root.hh"
#include <errno.h>

namespace Rapicorn {

TestContainer::TestContainer() :
  sig_assertion_ok (*this),
  sig_assertions_passed (*this)
{}

#define DFLTEPS (1e-8)

const PropertyList&
TestContainer::list_properties()
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
  static const PropertyList property_list (properties, Container::list_properties());
  return property_list;
}

static uint test_containers_rendered = 0;

uint
TestContainer::seen_test_items ()
{
  assert (rapicorn_thread_entered());
  return test_containers_rendered;
}

class TestContainerImpl : public virtual SingleContainerImpl, public virtual TestContainer {
  String m_value, m_assert_value;
  String m_accu, m_accu_history;
  double m_assert_left, m_assert_right;
  double m_assert_top, m_assert_bottom;
  double m_assert_width, m_assert_height;
  double m_epsilon;
  bool   m_test_container_counted;
  bool   m_fatal_asserts;
  bool   m_paint_allocation;
public:
  TestContainerImpl() :
    m_value (""), m_assert_value (""),
    m_accu (""), m_accu_history (""),
    m_assert_left (-INFINITY), m_assert_right (-INFINITY),
    m_assert_top (-INFINITY), m_assert_bottom (-INFINITY),
    m_assert_width (-INFINITY), m_assert_height (-INFINITY),
    m_epsilon (DFLTEPS), m_test_container_counted (false),
    m_fatal_asserts (true), m_paint_allocation (false)
  {}
  virtual String value           () const            { return m_value; }
  virtual void   value           (const String &val) { m_value = val; }
  virtual String assert_value    () const            { return m_assert_value; }
  virtual void   assert_value    (const String &val) { m_assert_value = val; }
  virtual double assert_left     () const        { return m_assert_left  ; }
  virtual void   assert_left     (double val)    { m_assert_left = val; invalidate(); }
  virtual double assert_right    () const        { return m_assert_right ; }
  virtual void   assert_right    (double val)    { m_assert_right = val; invalidate(); }
  virtual double assert_top      () const        { return m_assert_top   ; }
  virtual void   assert_top      (double val)    { m_assert_top = val; invalidate(); }
  virtual double assert_bottom   () const        { return m_assert_bottom; }
  virtual void   assert_bottom   (double val)    { m_assert_bottom = val; invalidate(); }
  virtual double assert_width    () const        { return m_assert_width ; }
  virtual void   assert_width    (double val)    { m_assert_width = val; invalidate(); }
  virtual double assert_height   () const        { return m_assert_height; }
  virtual void   assert_height   (double val)    { m_assert_height = val; invalidate(); }
  virtual double epsilon         () const        { return m_epsilon  ; }
  virtual void   epsilon         (double val)    { m_epsilon = val; invalidate(); }
  virtual bool   paint_allocation() const        { return m_paint_allocation; }
  virtual void   paint_allocation(bool   val)    { m_paint_allocation = val; invalidate(); }
  virtual bool   fatal_asserts   () const        { return m_fatal_asserts; }
  virtual void   fatal_asserts   (bool   val)    { m_fatal_asserts = val; invalidate(); }
  virtual String accu            () const            { return m_accu; }
  virtual void   accu            (const String &val) { m_accu = val; m_accu_history += val; }
  virtual String accu_history    () const            { return m_accu_history; }
  virtual void   accu_history    (const String &val) { m_accu_history = val; }
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
    if (delta > m_epsilon)
      {
        if (m_fatal_asserts)
          error ("similarity exceeded: %s=%f real-value=%f delta=%f (epsilon=%g)",
                 assertion_name, value, rvalue, delta, m_epsilon);
        else
          warning ("similarity exceeded: %s=%f real-value=%f delta=%f (epsilon=%g)",
                   assertion_name, value, rvalue, delta, m_epsilon);
      }
    else
      {
        String msg = string_printf ("%s == %f", assertion_name, value);
        sig_assertion_ok.emit (msg);
      }
  }
  virtual void
  render (Display &display)
  {
    assert (rapicorn_thread_entered());

    SingleContainerImpl::render (display);

    if (m_paint_allocation)
      {
        IRect ia = allocation();
        Plane &plane = display.create_plane();
        Painter painter (plane);
        painter.draw_filled_rect (ia.x, ia.y, ia.width, ia.height, black());
      }

    Allocation rarea = get_root()->allocation();
    double width = allocation().width, height = allocation().height;
    double x1 = allocation().x, x2 = rarea.width - x1 - width;
    double y1 = allocation().y, y2 = rarea.height - y1 - height;
    assert_value ("assert-bottom", m_assert_bottom, y1, y2);
    assert_value ("assert-right",  m_assert_right,  x1, x2);
    assert_value ("assert-top",    m_assert_top,    y1, y2);
    assert_value ("assert-left",   m_assert_left,   x1, x2);
    assert_value ("assert-width",  m_assert_width, width, width);
    assert_value ("assert-height", m_assert_height, height, height);
    if (m_assert_value != m_value)
      {
        if (m_fatal_asserts)
          error ("value has unexpected contents: %s (expected: %s)", m_value.c_str(), m_assert_value.c_str());
        else
          warning ("value has unexpected contents: %s (expected: %s)", m_value.c_str(), m_assert_value.c_str());
      }
    sig_assertions_passed.emit ();
    /* count containers for seen_test_containers() */
    if (!m_test_container_counted)
      {
        m_test_container_counted = true;
        test_containers_rendered++;
      }
  }
};
static const ItemFactory<TestContainerImpl> test_container_factory ("Rapicorn::Factory::TestContainer");

const PropertyList&
TestBox::list_properties()
{
  static Property *properties[] = {
    MakeProperty (TestBox, snapshot_file, _("Snapshot File Name"), _("PNG image file name to write snapshot to"), "rw"),
  };
  static const PropertyList property_list (properties, Container::list_properties());
  return property_list;
}

class TestBoxImpl : public virtual SingleContainerImpl, public virtual TestBox {
  String m_snapshot_file;
  uint   m_handler_id;
protected:
  virtual String snapshot_file () const                 { return m_snapshot_file; }
  virtual void   snapshot_file (const String &val)      { m_snapshot_file = val; invalidate(); }
  ~TestBoxImpl()
  {
    if (m_handler_id)
      {
        remove_exec (m_handler_id);
        m_handler_id = 0;
      }
  }
  void
  make_snapshot ()
  {
    Root *ritem = get_root();
    if (m_snapshot_file != "" && ritem)
      {
        const IRect area = allocation();
        Plane *plane = new Plane (area.x, area.y, area.width, area.height);
        ritem->render (*plane);
        bool saved = plane->save_png (m_snapshot_file, *plane, "TestBox snapshot");
        String err = saved ? "ok" : string_from_errno (errno);
        delete plane;
        printerr ("%s: wrote %s: %s\n", name().c_str(), m_snapshot_file.c_str(), err.c_str());
      }
    if (m_handler_id)
      {
        remove_exec (m_handler_id);
        m_handler_id = 0;
      }
  }
public:
  explicit TestBoxImpl()
  {}
  void
  render (Display &display)
  {
    SingleContainerImpl::render (display);
    if (!m_handler_id)
      {
        Root *ritem = get_root();
        if (ritem)
          {
            EventLoop *loop = ritem->get_loop();
            if (loop)
              m_handler_id = loop->exec_now (slot (*this, &TestBoxImpl::make_snapshot));
          }
      }
  }
};
static const ItemFactory<TestBoxImpl> test_box_factory ("Rapicorn::Factory::TestBox");


} // Rapicorn
