/* Rapicorn
 * Copyright (C) 2006 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "testitem.hh"
#include "itemimpl.hh"
#include "painter.hh"
#include "root.hh"

namespace Rapicorn {

TestItem::TestItem() :
  sig_assertion_ok (*this),
  sig_assertions_passed (*this)
{}

#define DFLTEPS (1e-8)

const PropertyList&
TestItem::list_properties()
{
  /* not using _() here, because TestItem is just a developer tool */
  static Property *properties[] = {
    MakeProperty (TestItem, epsilon,       "Epsilon",       "Epsilon within which assertions must hold",  DFLTEPS,   0,         +MAXFLOAT, 0.01, "rw"),
    MakeProperty (TestItem, assert_left,   "Assert-Left",   "Assert positioning of the left item edge",   -INFINITY, -INFINITY, +MAXFLOAT, 3, "rw"),
    MakeProperty (TestItem, assert_right,  "Assert-Right",  "Assert positioning of the right item edge",  -INFINITY, -INFINITY, +MAXFLOAT, 3, "rw"),
    MakeProperty (TestItem, assert_bottom, "Assert-Bottom", "Assert positioning of the bottom item edge", -INFINITY, -INFINITY, +MAXFLOAT, 3, "rw"),
    MakeProperty (TestItem, assert_top,    "Assert-Top",    "Assert positioning of the top item edge",    -INFINITY, -INFINITY, +MAXFLOAT, 3, "rw"),
    MakeProperty (TestItem, assert_width,  "Assert-Width",  "Assert amount of the item width",            -INFINITY, -INFINITY, +MAXFLOAT, 3, "rw"),
    MakeProperty (TestItem, assert_height, "Assert-Height", "Assert amount of the item height",           -INFINITY, -INFINITY, +MAXFLOAT, 3, "rw"),
    MakeProperty (TestItem, fatal_asserts, "Fatal-Asserts", "Handle assertion failures as fatal errors",  false, "rw"),
  };
  static const PropertyList property_list (properties, Item::list_properties());
  return property_list;
}

class TestItemImpl : public virtual TestItem, public virtual ItemImpl {
  float m_assert_left, m_assert_right;
  float m_assert_top, m_assert_bottom;
  float m_assert_width, m_assert_height;
  float m_epsilon;
  bool  m_fatal_asserts;
public:
  TestItemImpl() :
    m_assert_left (-INFINITY), m_assert_right (-INFINITY),
    m_assert_top (-INFINITY), m_assert_bottom (-INFINITY),
    m_assert_width (-INFINITY), m_assert_height (-INFINITY),
    m_epsilon (DFLTEPS), m_fatal_asserts (false)
  {}
  virtual float epsilon         () const        { return m_epsilon  ; }
  virtual void  epsilon         (float  val)    { m_epsilon = val; invalidate(); }
  virtual float assert_left     () const        { return m_assert_left  ; }
  virtual void  assert_left     (float  val)    { m_assert_left = val; invalidate(); }
  virtual float assert_right    () const        { return m_assert_right ; }
  virtual void  assert_right    (float  val)    { m_assert_right = val; invalidate(); }
  virtual float assert_top      () const        { return m_assert_top   ; }
  virtual void  assert_top      (float  val)    { m_assert_top = val; invalidate(); }
  virtual float assert_bottom   () const        { return m_assert_bottom; }
  virtual void  assert_bottom   (float  val)    { m_assert_bottom = val; invalidate(); }
  virtual float assert_width    () const        { return m_assert_width ; }
  virtual void  assert_width    (float  val)    { m_assert_width = val; invalidate(); }
  virtual float assert_height   () const        { return m_assert_height; }
  virtual void  assert_height   (float  val)    { m_assert_height = val; invalidate(); }
  virtual bool  fatal_asserts   () const        { return m_fatal_asserts; }
  virtual void  fatal_asserts   (bool   val)    { m_fatal_asserts = val; invalidate(); }
protected:
  virtual void
  size_request (Requisition &requisition)
  {
    requisition.width = 7;
    requisition.height = 7;
  }
  virtual void
  size_allocate (Allocation area)
  {
    allocation (area);
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
    Allocation area = allocation();
    Plane &plane = display.create_plane();
    Painter painter (plane);
    painter.draw_filled_rect (area.x, area.y, area.width, area.height, black());
    Allocation rarea = root()->allocation();
    float x1 = area.x, x2 = rarea.width - area.x - area.width;
    float y1 = area.y, y2 = rarea.height - area.y - area.height;
    assert_value ("assert-bottom", m_assert_bottom, y1, y2);
    assert_value ("assert-right",  m_assert_right,  x1, x2);
    assert_value ("assert-top",    m_assert_top,    y1, y2);
    assert_value ("assert-left",   m_assert_left,   x1, x2);
    assert_value ("assert-width",  m_assert_width, area.width, area.width);
    assert_value ("assert-height", m_assert_height, area.height, area.height);
    sig_assertions_passed.emit ();
  }
};
static const ItemFactory<TestItemImpl> test_item_factory ("Rapicorn::TestItem");

} // Rapicorn
