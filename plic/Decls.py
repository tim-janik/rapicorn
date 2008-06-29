#!/usr/bin/env python
# plic - Pluggable IDL Compiler                                -*-mode:python-*-
# Copyright (C) 2008 Tim Janik
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
import os, sys, re, shutil;
true, false, length = (True, False, len)

# --- declarations ---
class BaseDecl (object):
  def __init__ (self):
    self.name = None
    self.namespace = None
    self.loc = ()
    self.hint = ''
    self.docu = ()

NUM, REAL, STRING, ENUM, RECORD, SEQUENCE, INTERFACE = tuple ('ifserqc')

class Namespace (BaseDecl):
  def __init__ (self, name):
    super (Namespace, self).__init__()
    self.name = name
    self.members = [] # holds: (name, content)
    self.type_dict = {}
    self.const_dict = {}
  def add_const (self, name, content):
    self.members += [ (name, content) ]
    self.const_dict[name] = self.members[-1]
  def add_type (self, type):
    assert isinstance (type, TypeInfo)
    type.namespace = self
    self.members += [ (type.name, type) ]
    self.type_dict[type.name] = type
  def types (self):
    return self.type_dict.values()
  def unknown (self, name):
    return not (self.const_dict.has_key (name) or self.type_dict.has_key (name))
  def find_const (self, name):
    nc = self.const_dict.get (name, None)
    return nc and (nc[1],) or ()
  def find_type (self, name, fallback = None):
    return self.type_dict.get (name, fallback)

class TypeInfo (BaseDecl):
  def __init__ (self, name, storage):
    super (TypeInfo, self).__init__()
    assert storage in (NUM, REAL, STRING, ENUM, RECORD, SEQUENCE, INTERFACE)
    self.name = name
    self.storage = storage
    self.options = []           # holds: (ident, label, blurb, number)
    self.fields = []            # holds: (ident, TypeInfo)
    self.elements = None        # holds: ident, TypeInfo
    self.prerequisites = []
    self.auxdata = {}
  def clone (self, newname = None):
    if newname == None: newname = self.name
    ti = TypeInfo (newname, self.storage)
    ti.options += self.options
    ti.fields += self.fields
    ti.elements = self.elements
    ti.prerequisites += self.prerequisites
    ti.auxdata.update (self.auxdata)
    return ti
  def update_auxdata (self, auxdict):
    self.auxdata.update (auxdict)
  def add_option (self, ident, label, blurb, number):
    assert self.storage == ENUM
    assert isinstance (ident, str)
    assert isinstance (label, str)
    assert isinstance (blurb, str)
    assert isinstance (number, int)
    self.options += [ (ident, label, blurb, number) ]
  def add_field (self, ident, type):
    assert self.storage == RECORD
    assert isinstance (ident, str)
    assert isinstance (type, TypeInfo)
    self.fields += [ (ident, type) ]
  def set_elements (self, ident, type):
    assert self.storage == SEQUENCE
    assert isinstance (ident, str)
    assert isinstance (type, TypeInfo)
    self.elements = (ident, type)
  def full_name (self):
    s = self.name
    namespace = self.namespace
    while namespace:
      s = namespace.name + '::' + s
      namespace = namespace.namespace
    return s
