#!/usr/bin/env python
# plic - Pluggable IDL Compiler                                -*-mode:python-*-
# Copyright (C) 2010 Tim Janik
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
import Decls
true, false, length = (True, False, len)

def static_vars (*varname_value_list):
  def decorate (func):
    for tup in varname_value_list:
      varname, value = tup
      setattr (func, varname, value)
    return func
  return decorate

@static_vars (("iddict", {}), ("idcounter", 0))
def type_id (type):
  self = type_id
  otype = type
  types = []
  while type:
    types += [ type ]
    if hasattr (type, 'ownertype'):
      type = type.ownertype
    elif hasattr (type, 'namespace'):
      type = type.namespace
    else:
      type = None
  types = tuple (types)
  if not self.iddict.has_key (types):
    self.idcounter += 1
    if otype.storage == Decls.FUNC:
      if otype.rtype == None or otype.rtype.storage == Decls.VOID:
        self.iddict[types] = self.idcounter + 0x01000000
      else:
        self.iddict[types] = self.idcounter + 0x02000000
    else:
      self.iddict[types] = self.idcounter + 0x03000000
  return self.iddict[types]
