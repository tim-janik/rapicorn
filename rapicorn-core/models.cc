/* Rapicorn
 * Copyright (C) 2008 Tim Janik
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
#include "models.hh"

namespace Rapicorn {

Model0::Model0 (Type t) :
  m_type (t), m_value (t.storage()),
  sig_changed (*this, &Model0::changed)
{}

Model0::~Model0 ()
{}

void
Model0::Model0Value::changed()
{
  ssize_t vvoffset = (ssize_t) ((char*) &((Model0*) 0x10000)->m_value) - 0x10000;
  Model0 *var = (Model0*) ((char*) this - vvoffset);
  var->sig_changed.emit();
}

void
Model0::changed()
{}

} // Rapicorn
