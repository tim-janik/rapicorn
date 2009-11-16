#!/usr/bin/env python
# plic - Pluggable IDL Compiler                                -*-mode:python-*-
# Copyright (C) 2009 Tim Janik
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
"""PLIC-Rapicorn - C++ Rapicorn glue code generator for PLIC

More details at http://www.testbit.eu/plic
"""
import Decls

def strcquote (string):
  result = ''
  for c in string:
    oc = ord (c)
    ec = { 92 : r'\\',
            7 : r'\a',
            8 : r'\b',
            9 : r'\t',
           10 : r'\n',
           11 : r'\v',
           12 : r'\f',
           13 : r'\r',
           12 : r'\f'
         }.get (oc, '')
    if ec:
      result += ec
      continue
    if oc <= 31 or oc >= 127:
      result += '\\' + oct (oc)[-3:]
    elif c == '"':
      result += r'\"'
    else:
      result += c
  return '"' + result + '"'

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
  def generate_field (self, fident, ftype):
    return '  ' + self.format_to_tab (ftype.name) + fident + ';\n'
  def generate_method (self, ftype):
    s = ''
    s += '  ' + self.format_to_tab (ftype.rtype.name) + ftype.name + ' ('
    l = []
    for a in ftype.args:
      l += [ self.format_arg (*a) ]
    s += ', '.join (l)
    s += ');\n'
    return s
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
    s += ')> ' + signame + ';\n'
    return s
  def generate_signal (self, ftype, ctype):
    signame = self.generate_signal_name (ftype, ctype)
    return '  ' + self.format_to_tab (signame) + 'sig_%s;\n' % ftype.name
  def generate_type (self, type_info):
    self.tabwidth (16)
    s = ''
    tp = type_info
    if tp.storage == Decls.RECORD:
      s += 'struct %s {\n' % type_info.name
    elif tp.storage == Decls.INTERFACE:
      if tp.signals:
        self.tabwidth (20 + 8)
      else:
        self.tabwidth (20)
      s += 'class %s' % type_info.name
      if tp.prerequisites:
        s += ' :\n  '
        l = []
        for pr in tp.prerequisites:
          l += [ pr.name ]
        s += ',\n  '.join (l)
        s += '\n'
      else:
        s += ' '
      s += '{\n'
    if tp.storage == Decls.INTERFACE:
      for sg in tp.signals:
        s += self.generate_sigdef (sg, type_info)
    if (tp.storage == Decls.RECORD or
        tp.storage == Decls.INTERFACE):
      for fl in tp.fields:
        s += self.generate_field (fl[0], fl[1])
    if tp.storage == Decls.INTERFACE:
      for m in tp.methods:
        s += self.generate_method (m)
      for sg in tp.signals:
        s += self.generate_signal (sg, type_info)
    if (tp.storage == Decls.RECORD or
        tp.storage == Decls.INTERFACE):
      s += '};\n'
    return s
  def collect_impl_types (self, namespace):
    types = []
    for t in namespace.types():
      types += [ t ]
    return types
  def generate_namespaces (self, namespace_list):
    s = '/* Generated by Plic-Rapicorn */\n'    # magic
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
