#!/usr/bin/env python
# Rapicorn-PyStub                                              -*-mode:python-*-
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
"""Rapicorn-PyStub - Python Stub (client glue code) generator for Rapicorn

More details at http://www.rapicorn.org
"""
import Decls, GenUtils, re

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
  return re.compile (r'^', re.M).sub (prefix, lines.rstrip())

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
  def zero_value (self, type):
    return { Decls.FLOAT     : '0',
             Decls.INT       : '0',
             Decls.ENUM      : '0',
             Decls.RECORD    : 'None',
             Decls.SEQUENCE  : 'None',
             Decls.STRING    : "''",
             Decls.INTERFACE : "None",
           }[type.storage]
  def generate_record_impl (self, type_info):
    s = ''
    s += 'class %s (__BaseRecord__):\n' % type_info.name
    s += '  def __init__ (self, **entries):\n'
    s += '    defaults = {'
    for fl in type_info.fields:
      s += " '%s' : %s, " % (fl[0], self.zero_value (fl[1]))
    s += '}\n'
    s += '    self.__dict__.update (defaults)\n'
    s += '    __BaseRecord__.__init__ (self, **entries)\n'
    s += '  @staticmethod\n'
    s += '  def create (args):\n'
    s += '    self = %s()\n' % type_info.name
    s += '    if hasattr (args, "__iter__") and len (args) == %d:\n' % len (type_info.fields)
    n = 0
    for fl in type_info.fields:
      s += '      self.%s = args[%d]\n' % (fl[0], n)
      s += reindent ('      #', self.generate_to_proto ('self.' + fl[0], fl[1], 'args[%d]' % n)) + '\n'
      n += 1
    s += '    elif isinstance (args, dict):\n'
    for fl in type_info.fields:
      s += '      self.%s = args["%s"]\n' % (fl[0], fl[0])
    s += '    else: raise RuntimeError ("invalid or missing record initializers")\n'
    s += '    return self\n'
    s += '  @staticmethod\n'
    s += '  def to_proto (self, _plic_rec):\n'
    for a in type_info.fields:
      s += '    _plic_field = _plic_rp.fields.add()\n'
      s += reindent ('  ', self.generate_to_proto ('_plic_field', a[1], 'self.' + a[0])) + '\n'
    return s
  def generate_sighandler (self, ftype, ctype):
    s = ''
    s += 'def __sig_%s__ (self): pass # default handler' % ftype.name
    return s
  def generate_to_proto (self, argname, type_info, valname, onerror = 'return false'):
    s = ''
    if type_info.storage == Decls.VOID:
      pass
    elif type_info.storage in (Decls.INT, Decls.ENUM):
      s += '  %s.vint64 = %s\n' % (argname, valname)
    elif type_info.storage == Decls.FLOAT:
      s += '  %s.vdouble = %s\n' % (argname, valname)
    elif type_info.storage == Decls.STRING:
      s += '  %s.vstring = %s\n' % (argname, valname)
    elif type_info.storage == Decls.INTERFACE:
      s += '  %s.vstring (Instance2StringCast (%s))\n' % (argname, valname)
    elif type_info.storage == Decls.RECORD:
      s += '  %s.to_proto (%s.vrec, %s)\n' % (type_info.name, argname, valname)
    elif type_info.storage == Decls.SEQUENCE:
      s += '  %s.to_proto (%s.vseq, %s)\n' % (type_info.name, argname, valname)
    else: # FUNC
      raise RuntimeError ("Unexpected storage type: " + type_info.storage)
    return s
  def generate_method_caller_impl (self, m):
    s = ''
    s += 'def %s (' % m.name
    args = [ 'self' ]
    vals = [ 'self' ]
    for a in m.args:
      if a[2] != None:
        args += [ '%s = %s' % (a[0], a[2]) ]
      else:
        args += [ a[0] ]
      vals += [ a[0] ]
    s += ', '.join (args)
    if m.rtype.name == 'void':
      s += '): # one way\n'
    else:
      s += '): # %s\n' % m.rtype.name
    mth = m.type_hash()
    mid = ('%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x' +
           '%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x') % mth
    s += '  return _PLIC_%s (' % mid
    s += ', '.join (vals)
    s += ')\n'
    return s
  def inherit_reduce (self, type_list):
    def hasancestor (child, parent):
      if child == parent:
        return True
      for childpre in child.prerequisites:
        if hasancestor (childpre, parent):
          return True
    reduced = []
    while type_list:
      p = type_list.pop()
      skip = 0
      for c in type_list + reduced:
        if hasancestor (c, p):
          skip = 1
          break
      if not skip:
        reduced = [ p ] + reduced
    return reduced
  def generate_class (self, type_info):
    s = ''
    l = []
    for pr in type_info.prerequisites:
      l += [ pr ]
    l = self.inherit_reduce (l)
    l = [pr.name for pr in l] # types -> names
    if not l:
      l = [ '__BaseClass__' ]
    s += 'class %s (%s):\n' % (type_info.name, ', '.join (l))
    s += '  def __init__ (self):\n'
    s += '    super (%s, self).__init__()\n' % type_info.name
    for sg in type_info.signals:
      s += "    self.sig_%s = __Signal__ ('%s')\n" % (sg.name, sg.name)
    for m in type_info.methods:
      s += reindent ('  ', self.generate_method_caller_impl (m)) + '\n'
    for sg in type_info.signals:
      s += reindent ('  ', self.generate_sighandler (sg, type_info)) + '\n'
    return s
  def generate_impl_types (self, implementation_types):
    s = '### --- Generated by Rapicorn-PyStub --- ###\n'
    s += base_code + '\n'
    self.tabwidth (16)
    # collect impl types
    types = []
    for tp in implementation_types:
      if tp.isimpl:
        types += [ tp ]
    # generate types
    for tp in types:
      if tp.storage == Decls.RECORD:
        s += self.generate_record_impl (tp) + '\n'
      elif tp.storage == Decls.INTERFACE:
        s += self.generate_class (tp) + '\n'
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
  textstring = gg.generate_impl_types (config['implementation_types'])
  outname = config.get ('output', '-')
  if outname != '-':
    fout = open (outname, 'w')
    fout.write (textstring)
    fout.close()
  else:
    print textstring,

# control module exports
__all__ = ['generate']
