#!/usr/bin/env python
# Rapicorn-PyCStub                                             -*-mode:python-*-
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
"""Rapicorn-PyCStub - Python C++ Stub (client glue code) generator for Rapicorn

More details at http://www.rapicorn.org
"""
import Decls, re

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
#include <Python.h> // must be included first to configure std headers
#include <string>

#include "protocol-pb2.hh"
typedef Rapicorn::Rope::RemoteProcedure RemoteProcedure;
typedef Rapicorn::Rope::RemoteProcedure_Sequence RemoteProcedure_Sequence;
typedef Rapicorn::Rope::RemoteProcedure_Record RemoteProcedure_Record;
typedef Rapicorn::Rope::RemoteProcedure_Argument RemoteProcedure_Argument;

static inline PY_LONG_LONG
PyIntLong_AsLongLong (PyObject *intlong)
{
  if (PyInt_Check (intlong))
    return PyInt_AS_LONG (intlong);
  else
    return PyLong_AsLongLong (intlong);
}
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
    return { Decls.FLOAT    : '0',
             Decls.INT      : '0',
             Decls.ENUM     : '0',
             Decls.RECORD   : 'None',
             Decls.SEQUENCE : 'None',
             Decls.STRING   : "''",
           }[type.storage]
  def generate_enum_impl (self, type_info):
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
  def generate_record_impl (self, type_info):
    s = ''
    s += 'struct %s {\n' % type_info.name
    for fl in type_info.fields:
      pstar = '*' if fl[1].storage in (Decls.SEQUENCE, Decls.RECORD, Decls.INTERFACE) else ''
      s += '  ' + self.format_to_tab (self.type2cpp (fl[1].name)) + pstar + fl[0] + ';\n'
    s += '};\n'
    s += 'static bool rope_frompy_%s (PyObject*, RemoteProcedure_Record&);\n' % type_info.name
    return s
  def generate_record_funcs (self, type_info):
    s = ''
    s += 'static bool\n'
    s += 'rope_frompy_%s (PyObject *instance, RemoteProcedure_Record &rpr)\n' % type_info.name
    s += '{\n'
    s += '  RemoteProcedure_Argument *field;\n'
    s += '  PyObject *dictR = NULL, *item = NULL;\n'
    s += '  bool success = false;\n'
    s += '  dictR = PyObject_GetAttrString (instance, "__dict__"); if (!dictR) goto error;\n'
    for fl in type_info.fields:
      s += '  item = PyDict_GetItemString (dictR, "%s"); if (!dictR) goto error;\n' % (fl[0])
      s += '  field = rpr.add_fields();\n'
      if fl[1].storage == Decls.INT:
        s += '  field->set_vint64 (PyIntLong_AsLongLong (item)); if (PyErr_Occurred()) goto error;\n'
      elif fl[1].storage == Decls.FLOAT:
        s += '  field->set_vdouble (PyFloat_AsDouble (item)); if (PyErr_Occurred()) goto error;\n'
      elif fl[1].storage == Decls.STRING:
        s += '  field->set_vstring (PyString_AsString (item)); if (PyErr_Occurred()) goto error;\n'
      elif fl[1].storage == Decls.ENUM:
        s += '  field->set_vint64 (PyIntLong_AsLongLong (item)); if (PyErr_Occurred()) goto error;\n'
      elif fl[1].storage == Decls.RECORD:
        s += '  if (!rope_frompy_%s (item, *field->mutable_vrec())) goto error;\n' % fl[1].name
      elif fl[1].storage == Decls.SEQUENCE:
        s += '  if (!rope_frompy_%s (item, *field->mutable_vseq())) goto error;\n' % fl[1].name
      elif fl[1].storage == Decls.INTERFACE:
        s += '  field->set_vstring (PyString_AsString (item)); if (PyErr_Occurred()) goto error;\n'
      else:
        raise RuntimeError ("Unexpected storage type: " + fl[1].storage)
    s += '  success = true;\n'
    s += ' error:\n'
    s += '  Py_XDECREF (dictR);\n'
    s += '  return success;\n'
    s += '}\n'
    return s
  def generate_sequence_impl (self, type_info):
    s = ''
    s += 'struct %s {\n' % type_info.name
    for fl in [ type_info.elements ]:
      pstar = '*' if fl[1].storage in (Decls.SEQUENCE, Decls.RECORD, Decls.INTERFACE) else ''
      s += '  ' + self.format_to_tab (self.type2cpp (fl[1].name)) + pstar + fl[0] + ';\n'
    s += '};\n'
    s += 'static bool rope_frompy_%s (PyObject*, RemoteProcedure_Sequence&);\n' % type_info.name
    return s
  def generate_sequence_funcs (self, type_info):
    s = ''
    s += 'static bool\n'
    s += 'rope_frompy_%s (PyObject *list, RemoteProcedure_Sequence &rps)\n' % type_info.name
    s += '{\n'
    el = type_info.elements
    s += '  bool success = false;\n'
    s += '  const ssize_t len = PyList_Size (list); if (len < 0) goto error;\n'
    s += '  for (size_t k = 0; k < len; k++) {\n'
    s += '    PyObject *item = PyList_GET_ITEM (list, k);\n'
    if el[1].storage in (Decls.INT, Decls.ENUM):
      s += '    rps.add_vint64 (PyIntLong_AsLongLong (item)); if (PyErr_Occurred()) goto error;\n'
    elif el[1].storage == Decls.FLOAT:
      s += '    rps.add_vdouble (PyFloat_AsDouble (item)); if (PyErr_Occurred()) goto error;\n'
    elif el[1].storage == Decls.STRING:
      s += '    rps.add_vstring (PyString_AsString (item)); if (PyErr_Occurred()) goto error;\n'
    elif el[1].storage == Decls.RECORD:
      s += '    if (!rope_frompy_%s (item, *rps.add_vrec())) goto error;\n' % el[1].name
    elif el[1].storage == Decls.SEQUENCE:
      s += '    if (!rope_frompy_%s (item, *rps.add_vseq())) goto error;\n' % el[1].name
    else: # FUNC VOID
      raise RuntimeError ("Unexpected storage type: " + el[1].storage)
    s += '  }\n'
    s += '  success = true;\n'
    s += ' error:\n'
    s += '  return success;\n'
    s += '}\n'
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
    l = [ 'self' ]
    for a in m.args:
      l += [ a[0] ]
    s += ', '.join (l)
    if m.rtype.name == 'void':
      s += '): # one way\n'
    else:
      s += '): # %s\n' % m.rtype.name
    s += '  _plic_rp = RapicornRope.RemoteProcedure()\n'
    for a in m.args:
      s += '  _plic_arg = _plic_rp.args.add()\n'
      if a[1].storage in (Decls.RECORD, Decls.SEQUENCE):
        s += '  _plic_arg = %s.create (%s)\n' % (a[1].name, a[0])
      s += self.generate_to_proto ('_plic_arg', a[1], a[0])
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
  def type2cpp (self, typename):
    if typename == 'float': return 'double'
    if typename == 'string': return 'std::string'
    return typename
  def generate_impl_types (self, implementation_types):
    s = '/* --- Generated by Rapicorn-PyCStub --- */\n'
    s += base_code + '\n'
    self.tabwidth (16)
    # collect impl types
    types = []
    for tp in implementation_types:
      if tp.isimpl:
        types += [ tp ]
    # generate prototypes
    for tp in types:
      if tp.typedef_origin:
        pass # s += 'typedef %s %s;\n' % (self.type2cpp (tp.typedef_origin.name), tp.name)
      elif tp.storage == Decls.RECORD:
        s += 'struct %s;\n' % tp.name
      elif tp.storage == Decls.ENUM:
        s += self.generate_enum_impl (tp) + '\n'
    # generate types
    for tp in types:
      if tp.typedef_origin:
        pass
      elif tp.storage == Decls.RECORD:
        s += self.generate_record_impl (tp) + '\n'
      elif tp.storage == Decls.SEQUENCE:
        s += self.generate_sequence_impl (tp) + '\n'
      elif tp.storage == Decls.INTERFACE:
        s += '' # self.generate_class (tp) + '\n'
    # generate accessors
    for tp in types:
      if tp.typedef_origin:
        pass
      elif tp.storage == Decls.RECORD:
        s += self.generate_record_funcs (tp) + '\n'
      elif tp.storage == Decls.SEQUENCE:
        s += self.generate_sequence_funcs (tp) + '\n'
      elif tp.storage == Decls.INTERFACE:
        s += '' # self.generate_class (tp) + '\n'
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
