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
#include "item.hh"

namespace Rapicorn {

class TestItem : public virtual Item {
protected:
  explicit      TestItem        ();
public:
  virtual float epsilon         () const = 0;
  virtual void  epsilon         (float  val) = 0;
  virtual float assert_left     () const = 0;
  virtual void  assert_left     (float  val) = 0;
  virtual float assert_right    () const = 0;
  virtual void  assert_right    (float  val) = 0;
  virtual float assert_top      () const = 0;
  virtual void  assert_top      (float  val) = 0;
  virtual float assert_bottom   () const = 0;
  virtual void  assert_bottom   (float  val) = 0;
  virtual float assert_width    () const = 0;
  virtual void  assert_width    (float  val) = 0;
  virtual float assert_height   () const = 0;
  virtual void  assert_height   (float  val) = 0;
  virtual bool  fatal_asserts   () const = 0;
  virtual void  fatal_asserts   (bool   val) = 0;
  const PropertyList&                               list_properties ();
  Signal<TestItem, void (const String &assertion)>  sig_assertion_ok;
  Signal<TestItem, void ()>                         sig_assertions_passed;
};

} // Rapicorn
