#!/usr/bin/env python
# Rapicorn-CppStub                                             -*-mode:python-*-
# Copyright (C) 2009-2010 Tim Janik
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
"""Rapicorn-CppStub - C++ Stub (client glue code) generator for Rapicorn

More details at http://www.rapicorn.org
"""
import Decls, re

base_code = """
#include <string>
#include <vector>

// Base classes...
"""

class Generator:
  def __init__ (self):
    self.ntab = 26
  def tabwidth (self, n):
    self.ntab = n
  def format_to_tab (self, string, indent = ''):
    if len (string) >= self.ntab:
      return indent + string + ' '
    else:
      f = '%%-%ds' % self.ntab  # '%-20s'
      return indent + f % string
  def format_arg (self, ident, type):
    s = ''
    s += type.name + ' ' + ident
    return s
  def generate_prop (self, fident, ftype):
    v = 'virtual '
    # getter
    s = '  ' + self.format_to_tab (v + ftype.name) + fident + ' () const = 0;\n'
    # setter
    s += '  ' + self.format_to_tab (v + 'void') + fident + ' (const &' + ftype.name + ') = 0;\n'
    return s
  def generate_proplist (self, ctype):
    return '  ' + self.format_to_tab ('virtual const PropertyList&') + 'list_properties ();\n'
  def generate_field (self, fident, ftype_name):
    return '  ' + self.format_to_tab (ftype_name) + fident + ';\n'
  def generate_signal_name (self, ftype, ctype):
    return 'Signal_%s' % ftype.name
  def generate_sigdef (self, ftype, ctype):
    signame = self.generate_signal_name (ftype, ctype)
    s = ''
    s += '  typedef Signal<%s, %s (' % (ctype.name, ftype.rtype.name)
    l = []
    for a in ftype.args:
      l += [ self.format_arg (*a) ]
    s += ', '.join (l)
    s += ')'
    if ftype.rtype.collector != 'void':
      s += ', Collector' + ftype.rtype.collector.capitalize()
      s += '<' + ftype.rtype.name + '> '
    s += '> ' + signame + ';\n'
    return s
  def type2cpp (self, typename):
    if typename == 'float': return 'double'
    if typename == 'string': return 'std::string'
    return typename
  def mkzero (self, type):
    if type.storage == Decls.ENUM:
      return type.name + ' (0)'
    return '0'
  def generate_record (self, type_info):
    s = ''
    s += 'struct %s {\n' % type_info.name
    if type_info.storage == Decls.RECORD:
      fieldlist = type_info.fields
      for fl in fieldlist:
        s += self.generate_field (fl[0], self.type2cpp (fl[1].name))
    elif type_info.storage == Decls.SEQUENCE:
      fl = type_info.elements
      s += self.generate_field (fl[0], 'std::vector<' + self.type2cpp (fl[1].name) + '>')
    if type_info.storage == Decls.RECORD:
      s += '  inline %s () {' % type_info.name
      for fl in fieldlist:
        if fl[1].storage in (Decls.INT, Decls.FLOAT, Decls.ENUM):
          s += " %s = %s;" % (fl[0], self.mkzero (fl[1]))
      s += ' }\n'
    s += '};'
    return s
  def generate_signal (self, ftype, ctype):
    signame = self.generate_signal_name (ftype, ctype)
    return '  ' + self.format_to_tab (signame) + 'sig_%s;\n' % ftype.name
  def generate_method (self, ftype):
    s = ''
    s += '  ' + self.format_to_tab (ftype.rtype.name) + ftype.name + ' ('
    argindent = len (s)
    l = []
    for a in ftype.args:
      l += [ self.format_arg (*a) ]
    s += (',\n' + argindent * ' ').join (l)
    s += ');\n'
    return s
  def generate_class (self, type_info):
    s = ''
    l = []
    for pr in type_info.prerequisites:
      l += [ pr.name ]
    s += 'class %s' % type_info.name
    if l:
      s += ': %s' % ', '.join (l)
    s += '\n{\n'
    for sg in type_info.signals:
      s += self.generate_sigdef (sg, type_info)
    for sg in type_info.signals:
      s += self.generate_signal (sg, type_info)
    for m in type_info.methods:
      s += self.generate_method (m)
    s += '};'
    return s
  def generate_enum (self, type_info):
    s = ''
    l = []
    s += 'enum %s {\n' % type_info.name
    for opt in type_info.options:
      (ident, label, blurb, number) = opt
      s += '  %s = %s,' % (ident, number)
      if blurb:
        s += ' // %s' % re.sub ('\n', ' ', blurb)
      s += '\n'
    s += '};'
    return s
  def generate_type (self, type_info):
    self.tabwidth (16)
    s = ''
    tp = type_info
    if tp.typedef_origin:
      s += 'typedef %s %s;\n' % (self.type2cpp (tp.typedef_origin.name), tp.name)
    elif tp.storage == Decls.RECORD:
      s += self.generate_record (tp) + '\n'
    elif tp.storage == Decls.SEQUENCE:
      s += self.generate_record (tp) + '\n'
    elif tp.storage == Decls.INTERFACE:
      s = self.generate_class (type_info) + '\n'
    elif tp.storage == Decls.ENUM:
      s = self.generate_enum (type_info) + '\n'
    #if tp.storage == Decls.INTERFACE:
    #  if tp.fields:
    #    s += self.generate_proplist (type_info)
    #  for fl in tp.fields:
    #    s += self.generate_prop (fl[0], fl[1])
    return s
  def collect_impl_types (self, namespace):
    types = []
    for t in namespace.types():
      if t.isimpl:
        types += [ t ]
    return types
  def generate_namespaces (self, namespace_list):
    s = '/* --- Generated by Rapicorn-CppStub --- */\n'
    s += base_code + '\n'
    # collect impl types
    types = []
    for ns in namespace_list:
      tps = self.collect_impl_types (ns)
      types += tps
    # sort types
    pass
    # generate types
    for t in types:
      s += self.generate_type (t)
    return s

def error (msg):
  import sys
  print >>sys.stderr, sys.argv[0] + ":", msg
  sys.exit (127)

def generate (namespace_list, **args):
  import sys, tempfile, os
  config = {}
  config.update (args)
  gg = Generator()
  textstring = gg.generate_namespaces (namespace_list)
  outname = config.get ('output', '-')
  if outname != '-':
    fout = open (outname, 'w')
    fout.write (textstring)
    fout.close()
  else:
    print textstring,

# control module exports
__all__ = ['generate']
