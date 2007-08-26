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
#include <rapicorn/item.hh>

namespace Rapicorn {

class TestItem : public virtual Item {
protected:
  explicit      TestItem        ();
public:
  virtual double epsilon         () const = 0;
  virtual void   epsilon         (double val) = 0;
  virtual double assert_left     () const = 0;
  virtual void   assert_left     (double val) = 0;
  virtual double assert_right    () const = 0;
  virtual void   assert_right    (double val) = 0;
  virtual double assert_top      () const = 0;
  virtual void   assert_top      (double val) = 0;
  virtual double assert_bottom   () const = 0;
  virtual void   assert_bottom   (double val) = 0;
  virtual double assert_width    () const = 0;
  virtual void   assert_width    (double val) = 0;
  virtual double assert_height   () const = 0;
  virtual void   assert_height   (double val) = 0;
  virtual bool   fatal_asserts   () const = 0;
  virtual void   fatal_asserts   (bool   val) = 0;
  virtual String accu            () const = 0;
  virtual void   accu            (const String &val) = 0;
  virtual String accu_history    () const = 0;
  virtual void   accu_history    (const String &val) = 0;
  static uint    seen_test_items ();
  const PropertyList&                               list_properties ();
  Signal<TestItem, void (const String &assertion)>  sig_assertion_ok;
  Signal<TestItem, void ()>                         sig_assertions_passed;
};

} // Rapicorn
