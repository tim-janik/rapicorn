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
    self.loc = ()
    self.hint = ''
    self.docu = ()

NUM, REAL, STRING, CHOICE, RECORD, SEQUENCE, INTERFACE = tuple ('ifserqc')

decl_types = ('type', 'Const', 'record', 'sequence', 'enum')

class Namespace (BaseDecl):
  def __init__ (self, name):
    self.name = name
    self.members = [] # holds: (name, decl_type, contents)
    self.member_dict = {}
  def add (self, name, kind, content):
    assert kind in decl_types
    assert kind not in ('enum', 'type')
    self.members += [ (name, kind, content) ]
    self.member_dict[name] = self.members[-1]
  def add_enum (self, enum):
    assert isinstance (enum, Enum)
    self.members += [ (enum.name, 'enum', enum) ]
    self.member_dict[enum.name] = self.members[-1]
  def add_type (self, type):
    assert isinstance (type, TypeInfo)
    self.members += [ (type.name, 'type', type) ]
    self.member_dict[type.name] = self.members[-1]
  def unknown (self, name):
    return self.member_dict.has_key (name)
  def find_const (self, name, fallback = None):
    ndc = self.member_dict.get (name, (None, None, None))
    if ndc[1] == 'Const':
      return ndc
    return fallback
  def find_type (self, name, fallback = None):
    ndc = self.member_dict.get (name, (None, None, None))
    if ndc[1] not in (None, 'Const'):
      return ndc
    return fallback

class Enum (BaseDecl):
  def __init__ (self, name):
    self.name = name
    self.members = [] # holds: (ident, label, blurb, number)
  def add (self, ident, label, blurb, number):
    assert isinstance (ident, str)
    assert isinstance (label, str)
    assert isinstance (blurb, str)
    assert isinstance (number, int)
    self.members += [ (ident, label, blurb, number) ]

class TypeInfo (BaseDecl):
  def __init__ (self, name, storage):
    assert storage in tuple ('ifserqc')
    self.name = name
    self.storage = storage
    self.enum = None
    self.record = None
    self.sequence = None
    self.interface = None
