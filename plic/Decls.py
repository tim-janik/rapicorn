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

decl_types = ('builtin', 'Const', 'record', 'sequence', 'enum')

class Namespace (BaseDecl):
  def __init__ (self, name):
    self.name = name
    self.members = [] # holds: (name, decl_type, contents)
    self.member_dict = {}
  def add (self, name, kind, content):
    assert kind in decl_types
    assert kind != 'enum'
    self.members += [ (name, kind, content) ]
    self.member_dict[name] = self.members[-1]
  def add_enum (self, enum):
    assert isinstance (enum, Enum)
    self.members += [ (enum.name, 'enum', enum) ]
    self.member_dict[enum.name] = self.members[-1]
  def find (self, name, fallback = None):
    return self.member_dict.get (name, fallback)

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
