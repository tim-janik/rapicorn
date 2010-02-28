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
#include <core/rapicornsignal.hh>
#include <string>
#include <vector>

class ProtoRecord;
class ProtoSequence;
class ProtoArg;
class ProtoMessage;

// FIXME:
#include "protocol-pb2.hh"
typedef Rapicorn::Rope::RemoteProcedure RemoteProcedure;
typedef Rapicorn::Rope::RemoteProcedure_Sequence RemoteProcedure_Sequence;
typedef Rapicorn::Rope::RemoteProcedure_Record RemoteProcedure_Record;
typedef Rapicorn::Rope::RemoteProcedure_Argument RemoteProcedure_Argument;
static inline RemoteProcedure_Record& RPRecordCast (ProtoRecord &r) {
  return *(RemoteProcedure_Record*) &r;
}
static inline const RemoteProcedure_Record& RPRecordCast (const ProtoRecord &r) {
  return *(const RemoteProcedure_Record*) &r;
}
static inline ProtoRecord& ProtoRecordCast (RemoteProcedure_Record &r) {
  return *(ProtoRecord*) &r;
}
static inline const ProtoRecord& ProtoRecordCast (const RemoteProcedure_Record &r) {
  return *(const ProtoRecord*) &r;
}
static inline RemoteProcedure_Sequence& RPSequenceCast (ProtoSequence &r) {
  return *(RemoteProcedure_Sequence*) &r;
}
static inline const RemoteProcedure_Sequence& RPSequenceCast (const ProtoSequence &r) {
  return *(const RemoteProcedure_Sequence*) &r;
}
static inline ProtoSequence& ProtoSequenceCast (RemoteProcedure_Sequence &r) {
  return *(ProtoSequence*) &r;
}
static inline const ProtoSequence& ProtoSequenceCast (const RemoteProcedure_Sequence &r) {
  return *(const ProtoSequence*) &r;
}
template<class CLASS> static inline std::string Instance2StringCast (const CLASS &obj) {
  return ""; // FIXME
}
#define die()      (void) 0 // FIXME

// Base classes...
"""

class Generator:
  def __init__ (self):
    self.ntab = 26
    self.iddict = {}
    self.idcounter = 0x0def0000
  def type_id (self, type):
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
      self.iddict[types] = self.idcounter
    return self.iddict[types]
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
    s += self.type2cpp (type.name) + ' '
    if type.storage == Decls.INTERFACE:
      s += '&'
    s += ident
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
  def generate_signal_name (self, functype, ctype):
    return 'Signal_%s' % functype.name
  def generate_sigdef (self, functype, ctype):
    signame = self.generate_signal_name (functype, ctype)
    s = ''
    cpp_rtype = self.type2cpp (functype.rtype.name)
    s += '  typedef Rapicorn::Signals::Signal<%s, %s (' % (self.type2cpp (ctype.name), cpp_rtype)
    l = []
    for a in functype.args:
      l += [ self.format_arg (*a) ]
    s += ', '.join (l)
    s += ')'
    if functype.rtype.collector != 'void':
      s += ', Rapicorn::Signals::Collector' + functype.rtype.collector.capitalize()
      s += '<' + cpp_rtype + '> '
    s += '> ' + signame + ';\n'
    return s
  def type2cpp (self, typename):
    if typename == 'float': return 'double'
    if typename == 'string': return 'std::string'
    return typename
  def mkzero (self, type):
    if type.storage == Decls.ENUM:
      return self.type2cpp (type.name) + ' (0)'
    return '0'
  def generate_record_interface (self, type_info):
    s = ''
    s += 'struct %s {\n' % type_info.name
    if type_info.storage == Decls.RECORD:
      fieldlist = type_info.fields
      for fl in fieldlist:
        s += self.generate_field (fl[0], self.type2cpp (fl[1].name))
    elif type_info.storage == Decls.SEQUENCE:
      fl = type_info.elements
      s += self.generate_field (fl[0], 'std::vector<' + self.type2cpp (fl[1].name) + '>')
      s += '  bool to_proto   (ProtoSequence &) const;\n'
      s += '  bool from_proto (const ProtoSequence &);\n'
    if type_info.storage == Decls.RECORD:
      s += '  bool to_proto   (ProtoRecord &) const;\n'
      s += '  bool from_proto (const ProtoRecord &);\n'
      s += '  inline %s () {' % type_info.name
      for fl in fieldlist:
        if fl[1].storage in (Decls.INT, Decls.FLOAT, Decls.ENUM):
          s += " %s = %s;" % (fl[0], self.mkzero (fl[1]))
      s += ' }\n'
    s += '};'
    return s
  def generate_to_proto (self, argname, type_info, valname, onerror = 'return false'):
    s = '  %s->set_type (RemoteProcedure::%s);\n' % (argname, Decls.storage_name (type_info.storage))
    if type_info.storage == Decls.VOID:
      pass
    elif type_info.storage in (Decls.INT, Decls.ENUM):
      s += '  %s->set_vint64 (%s);\n' % (argname, valname)
    elif type_info.storage == Decls.FLOAT:
      s += '  %s->set_vdouble (%s);\n' % (argname, valname)
    elif type_info.storage == Decls.STRING:
      s += '  %s->set_vstring (%s);\n' % (argname, valname)
    elif type_info.storage == Decls.INTERFACE:
      s += '  %s->set_vstring (Instance2StringCast (%s));\n' % (argname, valname)
    elif type_info.storage == Decls.RECORD:
      s += '  if (!%s.to_proto (ProtoRecordCast (*%s->mutable_vrec()))) %s;\n' % (valname, argname, onerror)
    elif type_info.storage == Decls.SEQUENCE:
      s += '  if (!%s.to_proto (ProtoSequenceCast (*%s->mutable_vseq()))) %s;\n' % (valname, argname, onerror)
    else: # FUNC
      raise RuntimeError ("Unexpected storage type: " + type_info.storage)
    return s
  def generate_from_proto (self, argname, type_info, valname):
    s = '  if (%s->type() != RemoteProcedure::%s) return false;\n' % (argname, Decls.storage_name (type_info.storage))
    if type_info.storage == Decls.VOID:
      pass
    elif type_info.storage == Decls.INT:
      s += '  if (!%s->has_vint64()) return false;' % argname
      s += '  %s = %s->vint64();\n' % (valname, argname)
    elif type_info.storage == Decls.ENUM:
      s += '  if (!%s->has_vint64()) return false;' % argname
      s += '  %s = %s (%s->vint64());\n' % (valname, self.type2cpp (type_info.name), argname)
    elif type_info.storage == Decls.FLOAT:
      s += '  if (!%s->has_vdouble()) return false;' % argname
      s += '  %s = %s->vdouble();\n' % (valname, argname)
    elif type_info.storage in (Decls.STRING, Decls.INTERFACE):
      s += '  if (!%s->has_vstring()) return false;' % argname
      s += '  %s = %s->vstring();\n' % (valname, argname)
    elif type_info.storage == Decls.RECORD:
      s += '  if (!%s->has_vrec() || !%s.from_proto (ProtoRecordCast (%s->vrec()))) return false;\n' % (argname, valname, argname)
    elif type_info.storage == Decls.SEQUENCE:
      s += '  if (!%s->has_vseq() || !%s.from_proto (ProtoSequenceCast (%s->vseq()))) return false;\n' % (argname, valname, argname)
    else: # FUNC
      raise RuntimeError ("Unexpected storage type: " + type_info.storage)
    return s
  def generate_record_impl (self, type_info):
    s = ''
    s += 'bool %s::to_proto (ProtoRecord &dst) const {\n' % type_info.name
    s += '  RemoteProcedure_Record &rpr = RPRecordCast (dst);\n'
    s += '  RemoteProcedure_Argument *field; int field_counter = 0;\n'
    field_counter = 0
    for fl in type_info.fields:
      s += '  rpr.add_fields(); field = rpr.mutable_fields (field_counter++);\n'
      s += self.generate_to_proto ('field', fl[1], fl[0])
      field_counter += 1
    s += '  return true;\n}\n'
    s += 'bool %s::from_proto (const ProtoRecord &src) {\n' % type_info.name
    s += '  const RemoteProcedure_Record &rpr = RPRecordCast (src);\n'
    s += '  if (rpr.fields_size() < %d) return false;\n' % field_counter
    s += '  const RemoteProcedure_Argument *field;\n'
    field_counter = 0
    for fl in type_info.fields:
      s += '  field = &rpr.fields (%d);\n' % field_counter
      s += self.generate_from_proto ('field', fl[1], fl[0])
      field_counter += 1
    s += '  return true;\n}\n'
    return s
  def storage_fieldname (self, storage):
    if storage in (Decls.INT, Decls.ENUM):
      return 'vint64'
    elif storage == Decls.FLOAT:
      return 'vdouble'
    elif storage in (Decls.STRING, Decls.INTERFACE):
      return 'vstring'
    elif storage == Decls.RECORD:
      return 'vrec'
    elif storage == Decls.SEQ:
      return 'vseq'
    else: # FUNC VOID
      raise RuntimeError ("Unexpected storage type: " + storage)
  def generate_sequence_impl (self, type_info):
    s = ''
    s += 'bool %s::to_proto (ProtoSequence &dst) const {\n' % type_info.name
    s += '  RemoteProcedure_Sequence &rps = RPSequenceCast (dst);\n'
    el = type_info.elements
    s += '  const size_t len = %s.size();\n' % el[0]
    s += '  for (size_t k = 0; k < len; k++) {\n'
    if el[1].storage in (Decls.INT, Decls.ENUM, Decls.FLOAT, Decls.STRING, Decls.INTERFACE):
      s += '    rps.add_%s (%s[k]);\n' % (self.storage_fieldname (el[1].storage), el[0])
    elif el[1].storage == Decls.RECORD:
      s += '    rps.add_vrec();'
      s += '    if (!%s[k].to_proto (ProtoRecordCast (*rps.mutable_vrec (k)))) return false;\n' % el[0]
    elif el[1].storage == Decls.SEQUENCE:
      s += '    rps.add_vseq();'
      s += '    if (!%s[k].to_proto (ProtoSequenceCast (*rps.mutable_vseq (k)))) return false;\n' % el[0]
    else: # FUNC VOID
      raise RuntimeError ("Unexpected storage type: " + el[1].storage)
    s += '  }\n'
    s += '  return true;\n}\n'
    s += 'bool %s::from_proto (const ProtoSequence &src) {\n' % type_info.name
    s += '  const RemoteProcedure_Sequence &rps = RPSequenceCast (src);\n'
    s += '  const size_t len = rps.%s_size();\n' % self.storage_fieldname (el[1].storage)
    if el[1].storage in (Decls.RECORD, Decls.SEQUENCE):
      s += '  %s.resize (len);\n' % el[0]
    s += '  for (size_t k = 0; k < len; k++) {\n'
    if el[1].storage in (Decls.INT, Decls.FLOAT, Decls.STRING, Decls.INTERFACE):
      s += '    %s.push_back (rps.%s (k));\n' % (el[0], self.storage_fieldname (el[1].storage))
    elif el[1].storage == Decls.ENUM:
      s += '    %s.push_back (%s (rps.%s (k)));\n' % \
           (el[0], self.type2cpp (el[1].name), self.storage_fieldname (el[1].storage))
    elif el[1].storage == Decls.RECORD:
      s += '    if (!%s[k].from_proto (ProtoRecordCast (rps.vrec (k)))) return false;\n' % el[0]
    elif el[1].storage == Decls.SEQUENCE:
      s += '    if (!%s[k].from_proto (ProtoSequenceCast (rps.vseq (k)))) return false;\n' % el[0]
    else: # FUNC VOID
      raise RuntimeError ("Unexpected storage type: " + el[1].storage)
    s += '  }\n'
    s += '  return true;\n}\n'
    return s
  def generate_class_caller_impl (self, type_info):
    s = ''
    for m in type_info.methods:
      q = '%s %s::%s (' % (self.type2cpp (m.rtype.name), type_info.name, m.name)
      s += q
      argindent = len (q)
      l = []
      for a in m.args:
        l += [ self.format_arg (*a) ]
      s += (',\n' + argindent * ' ').join (l)
      s += ') {\n'
      s += '  RemoteProcedure rp;\n'
      s += '  rp.set_proc_id (0x%08x);\n' % self.type_id (m)
      s += '  rp.set_needs_return (%d);\n' % (m.rtype.storage != Decls.VOID)
      if m.args:
        s += '  RemoteProcedure_Argument *arg; int arg_counter = 0;\n'
      for a in m.args:
        s += '  rp.add_args(); arg = rp.mutable_args (arg_counter++);\n'
        s += self.generate_to_proto ('arg', a[1], a[0], 'die()')
      if m.rtype.storage != Decls.VOID:
        s += '  return 0; // FIXME\n'
      s += '}\n'
    return s
  def generate_signal (self, functype, ctype):
    signame = self.generate_signal_name (functype, ctype)
    return '  ' + self.format_to_tab (signame) + 'sig_%s;\n' % functype.name
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
  def generate_method (self, functype):
    s = ''
    s += '  ' + self.format_to_tab (self.type2cpp (functype.rtype.name)) + functype.name + ' ('
    argindent = len (s)
    l = []
    for a in functype.args:
      l += [ self.format_arg (*a) ]
    s += (',\n' + argindent * ' ').join (l)
    s += ');\n'
    return s
  def generate_class_interface (self, type_info):
    s = ''
    l = []
    for pr in type_info.prerequisites:
      l += [ pr ]
    l = self.inherit_reduce (l)
    l = ['public ' + pr.name for pr in l] # types -> names
    s += '\nclass %s' % type_info.name
    if l:
      s += ' : %s' % ', '.join (l)
    s += ' {\npublic:\n'
    for sg in type_info.signals:
      s += self.generate_sigdef (sg, type_info)
    for sg in type_info.signals:
      s += self.generate_signal (sg, type_info)
    for m in type_info.methods:
      s += self.generate_method (m)
    s += '};'
    return s
  def generate_enum_interface (self, type_info):
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
  def generate_impl_types (self, implementation_types):
    s = '/* --- Generated by Rapicorn-CppStub --- */\n'
    s += base_code + '\n'
    self.tabwidth (16)
    # collect impl types
    types = []
    for tp in implementation_types:
      if tp.isimpl:
        types += [ tp ]
    # generate type skeletons
    s += '\n// --- Skeletons ---\n'
    for tp in types:
      if tp.typedef_origin:
        s += 'typedef %s %s;\n' % (self.type2cpp (tp.typedef_origin.name), tp.name)
      elif tp.storage == Decls.RECORD:
        s += self.generate_record_interface (tp) + '\n'
      elif tp.storage == Decls.SEQUENCE:
        s += self.generate_record_interface (tp) + '\n'
      elif tp.storage == Decls.INTERFACE:
        s += self.generate_class_interface (tp) + '\n'
      elif tp.storage == Decls.ENUM:
        s += self.generate_enum_interface (tp) + '\n'
    # generate type stubs
    s += '\n// --- Stubs ---\n'
    for tp in types:
      if tp.typedef_origin:
        pass
      elif tp.storage == Decls.RECORD:
        s += self.generate_record_impl (tp) + '\n'
      elif tp.storage == Decls.SEQUENCE:
        s += self.generate_sequence_impl (tp) + '\n'
      elif tp.storage == Decls.INTERFACE:
        s += self.generate_class_caller_impl (tp) + '\n'
        #s += self.generate_class_callee_impl (tp) + '\n'
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
  textstring = gg.generate_impl_types (config['implementation_types']) # namespace_list
  outname = config.get ('output', '-')
  if outname != '-':
    fout = open (outname, 'w')
    fout.write (textstring)
    fout.close()
  else:
    print textstring,

# control module exports
__all__ = ['generate']
