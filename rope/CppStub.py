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

def reindent (prefix, lines):
  import re
  return re.compile (r'^', re.M).sub (prefix, lines)

base_code = """
class __BaseRecord__:
  def __init__ (self, **entries):
    self.__dict__.update (entries)
class __BaseClass__ (object):
  pass
class __Signal__:
  def __init__ (self, signame):
    self.name = signame
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
  def generate_field (self, fident, ftype):
    return '  ' + self.format_to_tab (ftype.name) + fident + ';\n'
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
  def default_value (self, type):
    return { 'float' : '0' }[type.name]
  def generate_record (self, type_info):
    s = ''
    s += 'class %s (__BaseRecord__):\n' % type_info.name
    s += '  def __init__ (self, **entries):\n'
    s += '    defaults = {'
    for fl in type_info.fields:
      s += " '%s' : %s, " % (fl[0], self.default_value (fl[1]))
    s += '}\n'
    s += '    self.__dict__.update (defaults)\n'
    s += '    __BaseRecord__.__init__ (self, **entries)\n'
    return s
  def generate_sighandler (self, ftype, ctype):
    s = ''
    s += 'def __sig_%s__ (self): pass # default handler' % ftype.name
    return s
  def generate_method (self, ftype):
    s = ''
    s += 'def %s (' % ftype.name
    l = [ 'self' ]
    for a in ftype.args:
      l += [ a[0] ]
    s += ', '.join (l)
    if ftype.rtype.name == 'void':
      s += '): # one way\n'
    else:
      s += '): # %s\n' % ftype.rtype.name
    s += '  pass'
    return s
  def generate_class (self, type_info):
    s = ''
    l = []
    for pr in type_info.prerequisites:
      l += [ pr.name ]
    if not l:
      l = [ '__BaseClass__' ]
    s += 'class %s (%s):\n' % (type_info.name, ', '.join (l))
    s += '  def __init__ (self):\n'
    s += '    super (%s, self).__init__()\n' % type_info.name
    for sg in type_info.signals:
      s += "    self.sig_%s = __Signal__ ('%s')\n" % (sg.name, sg.name)
    for m in type_info.methods:
      s += reindent ('  ', self.generate_method (m)) + '\n'
    for sg in type_info.signals:
      s += reindent ('  ', self.generate_sighandler (sg, type_info)) + '\n'
    return s
  def generate_type (self, type_info):
    self.tabwidth (16)
    s = ''
    tp = type_info
    if tp.storage == Decls.RECORD:
      s += self.generate_record (tp) + '\n'
    elif tp.storage == Decls.INTERFACE:
      s = self.generate_class (type_info) + '\n'
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
    s = '### --- Generated by Rapicorn-PyGlue --- ###\n'
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
