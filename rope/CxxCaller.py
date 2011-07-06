#!/usr/bin/env python
# PLIC-CxxCaller                                               -*-mode:python-*-
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
"""PLIC-CxxCaller - C++ Call glue generator

More details at http://www.testbit.eu/PLIC
"""
import Decls, GenUtils, re

clienthh_boilerplate = r"""
// --- ClientHH Boilerplate ---
#include <rcore/plicutils.hh>
"""

serverhh_boilerplate = r"""
// --- ServerHH Boilerplate ---
#include <rcore/plicutils.hh>
#include <rcore/rapicornsignal.hh>
using Rapicorn::Signals::slot;
"""

gencc_boilerplate = r"""
// --- ClientCC/ServerCC Boilerplate ---
#include <string>
#include <vector>
#include <stdexcept>
#ifndef __PLIC_GENERIC_CC_BOILERPLATE__
#define __PLIC_GENERIC_CC_BOILERPLATE__

#define THROW_ERROR()   throw std::runtime_error ("PLIC: Marshalling failed")
#define PLIC_CHECK(cond,errmsg) do { if (cond) break; throw std::runtime_error (std::string ("PLIC-ERROR: ") + errmsg); } while (0)

namespace { // Anonymous
using Plic::uint64;
typedef Plic::Coupler Coupler;
typedef Plic::FieldBuffer FieldBuffer;
typedef Plic::FieldBuffer8 FieldBuffer8;
typedef Plic::FieldBufferReader FieldBufferReader;

#ifndef PLIC_COUPLER
#define PLIC_COUPLER()  _plic_coupler_static
static struct _DummyCoupler : public Coupler {
  virtual FieldBuffer* call_remote (FieldBuffer *fbcall)
  {
    bool hasresult = Plic::msgid_has_result (fbcall->first_id());
    if (push_call (fbcall)) // deletes fbcall
      ; // threaded dispatcher needs CPU
    // wakeup dispatcher
    while (check_dispatch())
      dispatch();
    return !hasresult ? NULL : pop_result();
  }
} _plic_coupler_static;
#endif

} // Anonymous
#endif // __PLIC_GENERIC_CC_BOILERPLATE__
"""

FieldBuffer = 'Plic::FieldBuffer'

def reindent (prefix, lines):
  return re.compile (r'^', re.M).sub (prefix, lines.rstrip())

I_prefix_postfix = ('', '_Interface')
def Iwrap (name):
  cc = name.rfind ('::')
  if cc >= 0:
    ns, base = name[:cc+2], name[cc+2:]
  else:
    ns, base = '', name
  return ns + I_prefix_postfix[0] + base + I_prefix_postfix[1]
def H (name):
  return name + '_SmartHandle'

class C4CLIENT: pass
class C4SERVER: pass
class C4INTERFACE: pass

class Generator:
  def __init__ (self):
    self.ntab = 26
    self.namespaces = []
    self.insertions = {}
    self.cppguard = ''
    self.gen_inclusions = []
    self.skip_symbols = set()
    self.skip_classes = []
    self._iface_base = 'Plic::SimpleServer'
    self.gen4class = None
    self.gen_shortalias = False
  def close_inner_namespace (self):
    return '} // %s\n' % self.namespaces.pop().name
  def open_inner_namespace (self, namespace):
    self.namespaces += [ namespace ]
    return '\nnamespace %s {\n' % self.namespaces[-1].name
  def open_namespace (self, type):
    s = ''
    newspaces = []
    if type:
      newspaces = type.list_namespaces()
      # s += '// ' + str ([n.name for n in newspaces]) + '\n'
    while len (self.namespaces) > len (newspaces):
      s += self.close_inner_namespace()
    while self.namespaces and newspaces[0:len (self.namespaces)] != self.namespaces:
      s += self.close_inner_namespace()
    for n in newspaces[len (self.namespaces):]:
      s += self.open_inner_namespace (n)
    return s
  def type_relative_namespaces (self, type_node):
    tnsl = type_node.list_namespaces() # type namespace list
    # remove common prefix with global namespace list
    for n in self.namespaces:
      if tnsl and tnsl[0] == n:
        tnsl = tnsl[1:]
      else:
        break
    namespace_names = [d.name for d in tnsl]
    return namespace_names
  def namespaced_identifier (self, ident):
    return '::'.join ([d.name for d in self.namespaces] + [ident])
  def rtype2cpp (self, type_node):
    return self.type2cpp (type_node)
  def type2cpp (self, type_node):
    typename = type_node.name
    if typename == 'float': return 'double'
    if typename == 'string': return 'std::string'
    fullnsname = '::'.join (self.type_relative_namespaces (type_node) + [ type_node.name ])
    return fullnsname
  def Cp (self, type_node, interface_postfix = '', tag = None):
    str = self.C (type_node, tag)
    if type_node.storage == Decls.INTERFACE:
      return str + interface_postfix
    else:
      return str
  def C (self, type_node, tag = None):
    tname = self.type2cpp (type_node)
    if type_node.storage != Decls.INTERFACE:
      return tname
    if tag == C4INTERFACE:
      return Iwrap (tname)
    elif tag in (C4CLIENT, C4SERVER):
      return H (tname)
    if self.gen4class in (C4CLIENT, C4SERVER):
      return H (tname)
    else:
      return Iwrap (tname)
  def tabwidth (self, n):
    self.ntab = n
  def format_to_tab (self, string, indent = ''):
    if len (string) >= self.ntab:
      return indent + string + ' '
    else:
      f = '%%-%ds' % self.ntab  # '%-20s'
      return indent + f % string
  def format_vartype (self, type, tag = None):
    s = ''
    s += self.C (type, tag) + ' '
    if self.gen4class == C4INTERFACE and type.storage == Decls.INTERFACE:
      s += '*'
    return s
  def format_var (self, ident, type):
    return self.format_vartype (type) + ident
  def format_arg (self, ident, type, defaultinit = None, tag = None):
    s = ''
    constref = type.storage in (Decls.STRING, Decls.SEQUENCE, Decls.RECORD)
    if constref:
      s += 'const '                             # <const >int foo = 3
    n = self.type2cpp (type)                    # const <int> foo = 3
    s += self.C (type, tag)                     # Object<_Interface> &self
    s += ' ' if ident else ''                   # const int< >foo = 3
    if type.storage == Decls.INTERFACE:
      s += '&'                                  # Object_Interface <&>self
    if constref:
      s += '&'                                  # const String <&>foo = "bar"
    s += ident                                  # const int <foo> = 3
    if defaultinit != None:
      if type.storage == Decls.ENUM:
        s += ' = %s (%s)' % (self.type2cpp (type), defaultinit)
      elif type.storage in (Decls.SEQUENCE, Decls.RECORD):
        s += ' = %s()' % self.type2cpp (type)
      elif type.storage == Decls.INTERFACE:
        s += ' = *(%s*) NULL' % self.C (type)
      else:
        s += ' = %s' % defaultinit
    return s
  def format_call_args (self, ftype, prefix, argindent = 2, tag = None):
    l = []
    for a in ftype.args:
      l += [ self.format_arg (prefix + a[0], a[1], tag = tag) ]
    return (',\n' + argindent * ' ').join (l)
  def use_arg (self, ident, type, interfacechar = '*'):
    s = ''
    if type.storage == Decls.INTERFACE:
      s += interfacechar
    s += ident
    return s
  def generate_field (self, fident, ftype_name):
    return '  ' + self.format_to_tab (ftype_name) + fident + ';\n'
  def generate_proplist (self, ctype): # FIXME: properties
    return '  ' + self.format_to_tab ('virtual const PropertyList&') + 'list_properties ();\n'
  def generate_property (self, fident, ftype):
    s, v, v0, ptr = '', '', '', ''
    if self.gen4class == C4INTERFACE:
      v, v0, ptr = 'virtual ', ' = 0', '*'
    tname = self.C (ftype)
    if ftype.storage in (Decls.INT, Decls.FLOAT, Decls.ENUM):
      s += '  ' + v + self.format_to_tab (tname)  + fident + ' () const%s;\n' % v0
      s += '  ' + v + self.format_to_tab ('void') + fident + ' (' + tname + ')%s;\n' % v0
    elif ftype.storage in (Decls.STRING, Decls.RECORD, Decls.SEQUENCE):
      s += '  ' + v + self.format_to_tab (tname)  + fident + ' () const%s;\n' % v0
      s += '  ' + v + self.format_to_tab ('void') + fident + ' (const ' + tname + '&)%s;\n' % v0
    elif ftype.storage == Decls.INTERFACE:
      s += '  ' + v + self.format_to_tab (tname + ptr)  + fident + ' () const%s;\n' % v0
      s += '  ' + v + self.format_to_tab ('void') + fident + ' (' + tname + ptr + ')%s;\n' % v0
    return s
  def generate_client_property_stub (self, class_info, fident, ftype):
    s = ''
    cplfb = ('PLIC_COUPLER()', 'fb') # FIXME: optimize PLIC_COUPLER() accessor?
    therr = 'THROW_ERROR()'
    tname = self.C (ftype)
    # getter prototype
    s += tname + '\n'
    q = '%s::%s (' % (self.C (class_info), fident)
    s += q + ') const\n{\n'
    s += '  FieldBuffer &fb = *FieldBuffer::_new (4 + 1), *fr = NULL;\n'
    s += '  fb.add_type_hash (%s); // msgid\n' % self.getter_digest (class_info, fident, ftype)
    s += self.generate_proto_add_args (cplfb, class_info, '', [('(*this)', class_info)], '')
    s += '  fr = %s.call_remote (&fb); // deletes fb\n' % cplfb[0]
    if 1: # hasret
      rarg = ('retval', ftype)
      s += '  FieldBufferReader frr (*fr);\n'
      s += '  frr.skip(); // msgid\n' # FIXME: check fr return type
      cplfrr = (cplfb[0], 'frr')
      # FIXME: check return error and return type
      if rarg[1].storage in (Decls.RECORD, Decls.SEQUENCE):
        s += '  ' + self.format_var (rarg[0], rarg[1]) + ';\n'
        s += self.generate_proto_pop_args (cplfrr, class_info, '', [rarg], '', therr)
      else:
        vtype = self.format_vartype (rarg[1]) # 'int*' + ...
        s += self.generate_proto_pop_args (cplfrr, class_info, vtype, [rarg], '', therr) # ... + 'x = 5;'
      s += '  delete fr;\n'
      s += '  return retval;\n'
    s += '}\n'
    # setter prototype
    s += 'void\n'
    if ftype.storage in (Decls.STRING, Decls.RECORD, Decls.SEQUENCE):
      s += q + 'const ' + tname + ' &value)\n{\n'
    else:
      s += q + tname + ' value)\n{\n'
    s += '  FieldBuffer &fb = *FieldBuffer::_new (4 + 1 + 1), *fr = NULL;\n'
    s += '  fb.add_type_hash (%s); // msgid\n' % self.setter_digest (class_info, fident, ftype)
    s += self.generate_proto_add_args (cplfb, class_info, '', [('(*this)', class_info)], '')
    ident_type_args = [('value', ftype)]
    s += self.generate_proto_add_args (cplfb, class_info, '', ident_type_args, '', therr)
    s += '  fr = %s.call_remote (&fb); // deletes fb\n' % cplfb[0]
    s += '  if (fr) delete fr;\n' # FIXME: check return error
    s += '}\n'
    return s
  def generate_signal_name (self, functype, ctype):
    return 'Signal_%s' % functype.name
  def generate_sigdef (self, functype, ctype):
    s = ''
    signame = self.generate_signal_name (functype, ctype)
    cpp_rtype = self.Cp (functype.rtype, '*')
    s += '  typedef Rapicorn::Signals::Signal<%s, %s (' % (self.C (ctype), cpp_rtype)
    l = []
    for a in functype.args:
      l += [ self.format_arg (a[0], a[1]) ]
    s += ', '.join (l)
    s += ')'
    if functype.rtype.collector != 'void':
      s += ', Rapicorn::Signals::Collector' + functype.rtype.collector.capitalize()
      s += '<' + cpp_rtype + '> '
    s += '> ' + signame + ';\n'
    return s
  def mkzero (self, type):
    if type.storage == Decls.STRING:
      return '""'
    if type.storage == Decls.ENUM:
      return self.type2cpp (type) + ' (0)'
    if type.storage == Decls.RECORD:
      return self.type2cpp (type) + '()'
    if type.storage == Decls.SEQUENCE:
      return self.type2cpp (type) + '()'
    return '0'
  def generate_recseq_decl (self, type_info):
    s = ''
    if type_info.storage == Decls.SEQUENCE:
      fl = type_info.elements
      s += 'struct ' + type_info.name + ' : public std::vector<' + self.C (fl[1]) + '> {\n'
    else:
      s += 'struct %s {\n' % type_info.name
    if type_info.storage == Decls.RECORD:
      fieldlist = type_info.fields
      for fl in fieldlist:
        s += self.generate_field (fl[0], self.C (fl[1]))
    elif type_info.storage == Decls.SEQUENCE:
      s += '  typedef std::vector<' + self.C (fl[1]) + '> Sequence;\n'
      s += '  bool proto_add  (Plic::Coupler&, Plic::FieldBuffer&) const;\n'
      s += '  bool proto_pop  (Plic::Coupler&, Plic::FieldBufferReader&);\n'
    if type_info.storage == Decls.RECORD:
      s += '  bool proto_add  (Plic::Coupler&, Plic::FieldBuffer&) const;\n'
      s += '  bool proto_pop  (Plic::Coupler&, Plic::FieldBufferReader&);\n'
      s += '  inline %s () {' % type_info.name
      for fl in fieldlist:
        if fl[1].storage in (Decls.INT, Decls.FLOAT, Decls.ENUM):
          s += " %s = %s;" % (fl[0], self.mkzero (fl[1]))
      s += ' }\n'
    s += self.insertion_text ('class_scope:' + type_info.name)
    s += '};'
    return s
  def accessor_name (self, decls_type):
    # map Decls storage to FieldBuffer accessors
    return { Decls.INT:       'int64',
             Decls.ENUM:      'evalue',
             Decls.FLOAT:     'double',
             Decls.STRING:    'string',
             Decls.FUNC:      'func',
             Decls.INTERFACE: 'object' }.get (decls_type, None)
  def generate_proto_add_args (self, cplfb, type_info, aprefix, arg_info_list, apostfix,
                               onerr = 'return false'):
    s = ''
    cpl, fb = cplfb
    for arg_it in arg_info_list:
      ident, type = arg_it
      ident = aprefix + ident + apostfix
      if type.storage in (Decls.RECORD, Decls.SEQUENCE):
        s += '  if (!%s.proto_add (%s, %s)) %s;\n' % (ident, cpl, fb, onerr) # FIXME cpl var
      elif type.storage == Decls.INTERFACE:
        s += '  %s.add_object (%s._rpc_id());\n' % (fb, ident)
        # if not self.gen_clientcc: s += '  %s.add_object (%s (%s)._rpc_id());\n' % (fb, type.name, ident)
      else:
        s += '  %s.add_%s (%s);\n' % (fb, self.accessor_name (type.storage), ident)
    return s
  def generate_proto_pop_args (self, cplfbr, type_info, aprefix, arg_info_list, apostfix = '',
                               onerr = 'return false'):
    s = ''
    cpl, fbr = cplfbr
    for arg_it in arg_info_list:
      ident, type = arg_it
      ident = aprefix + ident + apostfix
      if type.storage in (Decls.RECORD, Decls.SEQUENCE):
        s += '  if (!%s.proto_pop (%s, %s)) %s;\n' % (ident, cpl, fbr, onerr)
      elif type.storage == Decls.ENUM:
        s += '  %s = %s (%s.pop_evalue());\n' % (ident, self.type2cpp (type), fbr)
      elif type.storage == Decls.INTERFACE:
        op_ptr = '.operator->()' if self.gen4class == C4INTERFACE else ''
        s += '  %s = %s (%s, %s)%s;\n' \
            % (ident, self.C (type, C4SERVER), cpl, fbr, op_ptr)
      else:
        s += '  %s = %s.pop_%s();\n' % (ident, fbr, self.accessor_name (type.storage))
    return s
  def generate_record_impl (self, type_info):
    s = ''
    cplfb = ('cpl', 'fb')
    s += 'bool\n%s::proto_add (Plic::Coupler &cpl, Plic::FieldBuffer &dst) const\n{\n' % type_info.name
    s += '  ' + FieldBuffer + ' &fb = dst.add_rec (%u);\n' % len (type_info.fields)
    s += self.generate_proto_add_args (cplfb, type_info, 'this->', type_info.fields, '')
    s += '  return true;\n'
    s += '}\n'
    s += 'bool\n%s::proto_pop (Plic::Coupler &cpl, Plic::FieldBufferReader &src)\n{\n' % type_info.name
    s += '  ' + FieldBuffer + 'Reader fbr (src.pop_rec());\n'
    s += '  if (fbr.remaining() != %u) return false;\n' % len (type_info.fields)
    cplfbr = ('cpl', 'fbr')
    s += self.generate_proto_pop_args (cplfbr, type_info, 'this->', type_info.fields)
    s += '  return true;\n'
    s += '}\n'
    return s
  def generate_sequence_impl (self, type_info):
    s = ''
    cplfb = ('cpl', 'fb')
    cplfbr = ('cpl', 'fbr')
    el = type_info.elements
    s += 'bool\n%s::proto_add (Plic::Coupler &cpl, Plic::FieldBuffer &dst) const\n{\n' % type_info.name
    s += '  const size_t len = this->size();\n'
    s += '  ' + FieldBuffer + ' &fb = dst.add_seq (len);\n'
    s += '  for (size_t k = 0; k < len; k++) {\n'
    s += reindent ('  ', self.generate_proto_add_args (cplfb, type_info, '',
                                                       [('(*this)', type_info.elements[1])],
                                                       '[k]')) + '\n'
    s += '  }\n'
    s += '  return true;\n'
    s += '}\n'
    s += 'bool\n%s::proto_pop (Plic::Coupler &cpl, Plic::FieldBufferReader &src)\n{\n' % type_info.name
    s += '  ' + FieldBuffer + 'Reader fbr (src.pop_seq());\n'
    s += '  const size_t len = fbr.remaining();\n'
    if el[1].storage in (Decls.RECORD, Decls.SEQUENCE):
      s += '  this->resize (len);\n'
    else:
      s += '  this->reserve (len);\n'
    s += '  for (size_t k = 0; k < len; k++) {\n'
    if el[1].storage in (Decls.RECORD, Decls.SEQUENCE):
      s += reindent ('  ', self.generate_proto_pop_args (cplfbr, type_info, '',
                                                         [('(*this)', type_info.elements[1])],
                                                         '[k]')) + '\n'
    elif el[1].storage == Decls.ENUM:
      s += '    this->push_back (%s (fbr.pop_evalue()));\n' % self.type2cpp (el[1])

    elif el[1].storage == Decls.INTERFACE:
      s += '    this->push_back (%s (cpl, fbr));\n' % self.C (el[1])
    else:
      s += '    this->push_back (fbr.pop_%s());\n' % self.accessor_name (el[1].storage)
    s += '  }\n'
    s += '  return true;\n'
    s += '}\n'
    return s
  def generate_client_method_stub (self, class_info, mtype):
    s = ''
    cplfb = ('PLIC_COUPLER()', 'fb') # FIXME: optimize PLIC_COUPLER() accessor?
    therr = 'THROW_ERROR()'
    hasret = mtype.rtype.storage != Decls.VOID
    # prototype
    s += self.C (mtype.rtype) + '\n'
    q = '%s::%s (' % (self.C (class_info), mtype.name)
    s += q + self.format_call_args (mtype, 'arg_', len (q)) + ')\n{\n'
    # vars, procedure
    s += '  FieldBuffer &fb = *FieldBuffer::_new (4 + 1 + %u), *fr = NULL;\n' % len (mtype.args)
    s += '  fb.add_type_hash (%s); // msgid\n' % self.method_digest (mtype)
    # marshal args
    s += self.generate_proto_add_args (cplfb, class_info, '', [('(*this)', class_info)], '')
    ident_type_args = [('arg_' + a[0], a[1]) for a in mtype.args]
    s += self.generate_proto_add_args (cplfb, class_info, '', ident_type_args, '', therr)
    # call out
    s += '  fr = %s.call_remote (&fb); // deletes fb\n' % cplfb[0]
    # unmarshal return
    if hasret:
      rarg = ('retval', mtype.rtype)
      s += '  FieldBufferReader frr (*fr);\n'
      s += '  frr.skip(); // msgid\n' # FIXME: check fr return type
      cplfrr = (cplfb[0], 'frr')
      # FIXME: check return error and return type
      if rarg[1].storage in (Decls.RECORD, Decls.SEQUENCE):
        s += '  ' + self.format_var (rarg[0], rarg[1]) + ';\n'
        s += self.generate_proto_pop_args (cplfrr, class_info, '', [rarg], '', therr)
      else:
        vtype = self.format_vartype (rarg[1]) # 'int*' + ...
        s += self.generate_proto_pop_args (cplfrr, class_info, vtype, [rarg], '', therr) # ... + 'x = 5;'
      s += '  delete fr;\n'
      s += '  return retval;\n'
    s += '}\n'
    return s
  def generate_server_property_set_dispatcher (self, class_info, fident, ftype, reglines):
    s = ''
    cplfbr = ('cpl', 'fbr')
    dispatcher_name = '_dispatch_setter__%s_%s' % (class_info.name, fident)
    setter_hash = self.setter_digest (class_info, fident, ftype)
    reglines += [ (setter_hash, self.namespaced_identifier (dispatcher_name)) ]
    s += 'static FieldBuffer*\n'
    s += dispatcher_name + ' (Coupler &cpl)\n'
    s += '{\n'
    s += '  FieldBufferReader &fbr = cpl.reader;\n'
    s += '  fbr.skip_hash(); // TypeHash\n'
    s += '  if (fbr.remaining() != 1 + 1) return FieldBuffer::new_error ("invalid number of arguments", __func__);\n'
    # fetch self
    s += '  %s *self;\n' % self.C (class_info, C4INTERFACE)
    s += self.generate_proto_pop_args (cplfbr, class_info, '', [('self', class_info)])
    s += '  PLIC_CHECK (self, "self must be non-NULL");\n'
    # fetch property
    if ftype.storage in (Decls.RECORD, Decls.SEQUENCE):
      s += '  ' + self.format_var ('arg_' + fident, ftype) + ';\n'
      s += self.generate_proto_pop_args (cplfbr, class_info, 'arg_', [(fident, ftype)])
    else:
      tstr = self.format_vartype (ftype, C4INTERFACE) + 'arg_'
      s += self.generate_proto_pop_args (cplfbr, class_info, tstr, [(fident, ftype)])
    ref = '&' if ftype.storage == Decls.INTERFACE else ''
    # call out
    s += '  self->' + fident + ' (' + ref + self.use_arg ('arg_' + fident, ftype) + ');\n'
    s += '  return NULL;\n'
    s += '}\n'
    return s
  def generate_server_property_get_dispatcher (self, class_info, fident, ftype, reglines):
    s = ''
    cplfbr = ('cpl', 'fbr')
    dispatcher_name = '_dispatch_getter__%s_%s' % (class_info.name, fident)
    getter_hash = self.getter_digest (class_info, fident, ftype)
    reglines += [ (getter_hash, self.namespaced_identifier (dispatcher_name)) ]
    s += 'static FieldBuffer*\n'
    s += dispatcher_name + ' (Coupler &cpl)\n'
    s += '{\n'
    s += '  FieldBufferReader &fbr = cpl.reader;\n'
    s += '  fbr.skip_hash(); // TypeHash\n'
    s += '  if (fbr.remaining() != 1) return FieldBuffer::new_error ("invalid number of arguments", __func__);\n'
    # fetch self
    s += '  %s *self;\n' % self.C (class_info, C4INTERFACE)
    s += self.generate_proto_pop_args (cplfbr, class_info, '', [('self', class_info)])
    s += '  PLIC_CHECK (self, "self must be non-NULL");\n'
    # return var
    s += '  '
    s += self.format_vartype (ftype, C4INTERFACE) + 'rval = '
    # call out
    s += 'self->' + fident + ' ();\n'
    # store return value
    cplrb = (cplfbr[0], 'rb')
    s += '  FieldBuffer &rb = *FieldBuffer::new_result();\n'
    rval = 'rval'
    if ftype.storage == Decls.INTERFACE:
      rval = '%s (rval)' % H (ftype.name) # FooBase -> Foo smart handle
    s += self.generate_proto_add_args (cplrb, class_info, '', [(rval, ftype)], '')
    s += '  return &rb;\n'
    s += '}\n'
    return s
  def generate_server_method_dispatcher (self, class_info, mtype, reglines):
    s = ''
    cplfbr = ('cpl', 'fbr')
    dispatcher_name = '_dispatch__%s_%s' % (class_info.name, mtype.name)
    reglines += [ (self.method_digest (mtype), self.namespaced_identifier (dispatcher_name)) ]
    s += 'static FieldBuffer*\n'
    s += dispatcher_name + ' (Coupler &cpl)\n'
    s += '{\n'
    s += '  FieldBufferReader &fbr = cpl.reader;\n'
    s += '  fbr.skip_hash(); // TypeHash\n'
    s += '  if (fbr.remaining() != 1 + %u) return FieldBuffer::new_error ("invalid number of arguments", __func__);\n' % len (mtype.args)
    # fetch self
    s += '  %s *self;\n' % self.C (class_info, C4INTERFACE)
    s += self.generate_proto_pop_args (cplfbr, class_info, '', [('self', class_info)])
    s += '  PLIC_CHECK (self, "self must be non-NULL");\n'
    # fetch args
    for a in mtype.args:
      if a[1].storage in (Decls.RECORD, Decls.SEQUENCE):
        s += '  ' + self.format_var ('arg_' + a[0], a[1]) + ';\n'
        s += self.generate_proto_pop_args (cplfbr, class_info, 'arg_', [(a[0], a[1])])
      else:
        tstr = self.format_vartype (a[1], C4INTERFACE) + 'arg_'
        s += self.generate_proto_pop_args (cplfbr, class_info, tstr, [(a[0], a[1])])
    # return var
    s += '  '
    hasret = mtype.rtype.storage != Decls.VOID
    if hasret:
      s += self.format_vartype (mtype.rtype, C4INTERFACE) + 'rval = '
    # call out
    s += 'self->' + mtype.name + ' ('
    s += ', '.join (self.use_arg ('arg_' + a[0], a[1]) for a in mtype.args)
    s += ');\n'
    # store return value
    if hasret:
      cplrb = (cplfbr[0], 'rb')
      s += '  FieldBuffer &rb = *FieldBuffer::new_result();\n'
      rval = 'rval'
      if mtype.rtype.storage == Decls.INTERFACE:
        rval = '%s (rval)' % H (mtype.rtype.name) # FooBase -> Foo smart handle
      s += self.generate_proto_add_args (cplrb, class_info, '', [(rval, mtype.rtype)], '')
      s += '  return &rb;\n'
    else:
      s += '  return NULL;\n'
    # done
    s += '}\n'
    return s
  def generate_server_signal_dispatcher (self, class_info, stype, reglines):
    s = ''
    therr = 'THROW_ERROR()'
    dispatcher_name = '_dispatch__%s_%s' % (class_info.name, stype.name)
    reglines += [ (self.method_digest (stype), self.namespaced_identifier (dispatcher_name)) ]
    closure_class = '_CLOSURE_%s_%s' % (class_info.name, stype.name)
    cplfb = ('sp->m_coupler', 'fb')
    s += 'struct %s {\n' % closure_class
    s += '  typedef Plic::shared_ptr<%s> SharedPtr;\n' % closure_class
    s += '  %s (Plic::Coupler &cpl, uint64 h) : m_coupler (cpl), m_handler (h) {}\n' % closure_class
    s += '  ~%s()\n' % closure_class
    s += '  {\n'
    s += '    FieldBuffer &fb = *FieldBuffer::_new (1 + 1);\n'
    s += '    fb.add_int64 (Plic::msgid_discon);\n' # self.method_digest (stype)
    s += '    fb.add_int64 (m_handler);\n'
    s += '    m_coupler.push_event (&fb); // deletes fb\n'
    s += '  }\n'
    cpp_rtype = self.rtype2cpp (stype.rtype)
    if stype.rtype.storage == Decls.INTERFACE:
      cpp_rtype = Iwrap (cpp_rtype) + '*'
    s += '  static %s\n' % cpp_rtype
    s += '  handler ('
    s += self.format_call_args (stype, 'arg_', 11, C4INTERFACE) + (',\n           ' if stype.args else '')
    s += 'SharedPtr sp)\n  {\n'
    s += '    FieldBuffer &fb = *FieldBuffer::_new (1 + 1 + %u);\n' % len (stype.args)
    s += '    fb.add_int64 (Plic::msgid_event);\n' # self.method_digest (stype)
    s += '    fb.add_int64 (sp->m_handler);\n'
    ident_type_args = [('arg_' + a[0], a[1]) for a in stype.args] # marshaller args
    args2fb = self.generate_proto_add_args (cplfb, class_info, '', ident_type_args, '', therr)
    if args2fb:
      s += reindent ('  ', args2fb) + '\n'
    s += '    sp->m_coupler.push_event (&fb); // deletes fb\n'
    if stype.rtype.storage != Decls.VOID:
      s += '    return %s;\n' % self.mkzero (stype.rtype)
    s += '  }\n'
    s += '  private: Plic::Coupler &m_coupler; uint64 m_handler;\n'
    s += '};\n'
    cplfbr = ('cpl', 'fbr')
    s += 'static FieldBuffer*\n'
    s += dispatcher_name + ' (Coupler &cpl)\n'
    s += '{\n'
    s += '  FieldBufferReader &fbr = cpl.reader;\n'
    s += '  fbr.skip_hash(); // TypeHash\n'
    s += '  if (fbr.remaining() != 1 + 2) return FieldBuffer::new_error ("invalid number of arguments", __func__);\n'
    s += '  %s *self;\n' % self.C (class_info, C4INTERFACE)
    s += self.generate_proto_pop_args (cplfbr, class_info, '', [('self', class_info)])
    s += '  PLIC_CHECK (self, "self must be non-NULL");\n'
    s += '  uint64 cid = 0, handler_id = %s.pop_int64();\n' % cplfbr[1]
    s += '  uint64 con_id = %s.pop_int64();\n' % cplfbr[1]
    s += '  if (con_id) self->sig_%s.disconnect (con_id);\n' % stype.name
    s += '  if (handler_id) {\n'
    s += '    %s::SharedPtr sp (new %s (cpl, handler_id));\n' % (closure_class, closure_class)
    s += '    cid = self->sig_%s.connect (slot (sp->handler, sp)); }\n' % stype.name
    s += '  FieldBuffer &rb = *FieldBuffer::new_result();\n'
    s += '  rb.add_int64 (cid);\n'
    s += '  return &rb;\n'
    s += '}\n'
    return s
  def digest2cbytes (self, digest):
    return ('0x%02x%02x%02x%02x%02x%02x%02x%02xULL, 0x%02x%02x%02x%02x%02x%02x%02x%02xULL, ' +
            '0x%02x%02x%02x%02x%02x%02x%02x%02xULL, 0x%02x%02x%02x%02x%02x%02x%02x%02xULL') % \
            digest
  def method_digest (self, mtype):
    return self.digest2cbytes (mtype.type_hash())
  def setter_digest (self, class_info, fident, ftype):
    setter_hash = class_info.property_hash ((fident, ftype), True)
    return self.digest2cbytes (setter_hash)
  def getter_digest (self, class_info, fident, ftype):
    setter_hash = class_info.property_hash ((fident, ftype), False)
    return self.digest2cbytes (setter_hash)
  def generate_server_method_registry (self, reglines):
    s = ''
    s += 'static const Plic::DispatcherEntry _dispatcher_entries[] = {\n'
    for dispatcher in reglines:
      cdigest, dispatcher_name = dispatcher
      s += '  { { ' + cdigest + ' }, '
      s += dispatcher_name + ', },\n'
    s += '};\n'
    s += 'static Plic::DispatcherRegistry _dispatcher_registry (_dispatcher_entries);\n'
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
  def generate_method_decl (self, functype, pad):
    s, comment = '', self.gen4class == C4SERVER
    s += '  // ' if comment else '  '
    if self.gen4class == C4INTERFACE:
      s += 'virtual '
    s += self.format_to_tab (self.Cp (functype.rtype, '*' if self.gen4class == C4INTERFACE else ''))
    s += functype.name
    s += ' ' * max (0, pad - len (functype.name))
    s += ' ('
    argindent = len (s)
    l = []
    for a in functype.args:
      l += [ self.format_arg ('' if comment else a[0], a[1], None if comment else a[2]) ]
    if comment:
      s += ', '.join (l)
    else:
      s += (',\n' + argindent * ' ').join (l)
    s += ')'
    if self.gen4class == C4INTERFACE and functype.pure and not comment:
      s += ' = 0'
    s += ';\n'
    return s
  def generate_interface_class (self, type_info):
    s = ''
    l = []
    # declare and inherit
    for pr in type_info.prerequisites:
      l += [ pr ]
    l = self.inherit_reduce (l)
    if self.gen4class == C4INTERFACE:
      l = ['public virtual ' + self.C (pr) for pr in l] # types -> names
      if not l:
        l = ['public virtual ' + self._iface_base]
    else:
      l = ['public ' + self.C (pr) for pr in l] # types -> names
      if not l:
        l = ['public Plic::SmartHandle']
    s += 'class %s' % self.C (type_info)
    if l:
      s += ' : %s' % ', '.join (l)
    s += ' {\n'
    # constructors
    s += 'protected:\n'
    if self.gen4class == C4SERVER:
      s += '  inline %s* _iface() const { return (%s*) _void_iface(); }\n' \
          % (self._iface_base, self._iface_base)
      s += '  inline void _iface (%s *_iface) { _void_iface (_iface); }\n' % self._iface_base
    if self.gen4class == C4INTERFACE:
      s += '  explicit ' + self.format_to_tab ('') + '%s ();\n' % self.C (type_info)
      s += '  virtual ' + self.format_to_tab ('/*Des*/') + '~%s () = 0;\n' % self.C (type_info)
    s += 'public:\n'
    if self.gen4class in (C4CLIENT, C4SERVER):
      s += '  inline %s () {}\n' % H (type_info.name)
      s += '  inline %s (Plic::Coupler &cpl, Plic::FieldBufferReader &fbr) ' % H (type_info.name)
      s += '{ _pop_rpc (cpl, fbr); }\n'
    if self.gen4class == C4SERVER:
      ifacename = Iwrap (type_info.name)
      s += '  inline %s (%s *iface) { _iface (iface); }\n' % (self.C (type_info), ifacename)
      s += '  inline %s (%s &iface) { _iface (&iface); }\n' % (self.C (type_info), ifacename)
    # properties
    for fl in type_info.fields:
      s += self.generate_property (fl[0], fl[1])
    # signals
    if self.gen4class == C4INTERFACE:
      for sg in type_info.signals:
        s += self.generate_sigdef (sg, type_info)
      for sg in type_info.signals:
        s += '  ' + self.generate_signal_name (sg, type_info) + ' sig_%s;\n' % sg.name
    # methods
    ml = 0
    if type_info.methods:
      ml = max (len (m.name) for m in type_info.methods)
    for m in type_info.methods:
      s += self.generate_method_decl (m, ml)
    # specials (operators)
    if self.gen4class == C4SERVER:
      ifn2 = (ifacename, ifacename)
      s += '  inline %s& operator*  () const { return *dynamic_cast<%s*> (_iface()); }\n' % ifn2
      s += '  inline %s* operator-> () const { return dynamic_cast<%s*> (_iface()); }\n' % ifn2
      s += '  inline operator  %s&  () const { return operator*(); }\n' % ifacename
    if self.gen4class in (C4CLIENT, C4SERVER):
      s += '  inline operator _unspecified_bool_type () const ' # return non-NULL pointer to member on true
      s += '{ return _is_null() ? NULL : _unspecified_bool_true(); }\n' # avoids auto-bool conversions on: float (*this)
    s += self.insertion_text ('class_scope:' + self.C (type_info))
    s += '};\n'
    # aliasing
    if self.gen_shortalias and self.gen4class in (C4INTERFACE,):
      s += '// typedef %s %s;\n' % (self.C (type_info), self.type2cpp (type_info))
    elif self.gen_shortalias and self.gen4class in (C4CLIENT,):
      s += 'typedef %s %s;\n' % (self.C (type_info), self.type2cpp (type_info))
    return s
  def generate_interface_impl (self, type_info):
    s = ''
    tname = self.C (type_info)
    s += '%s::%s ()' % (tname, tname)
    l = [] # constructor agument list
    for sg in type_info.signals:
      l += ['sig_%s (*this)' % sg.name]
    if l:
      s += ' :\n  ' + ', '.join (l)
    s += '\n{}\n'
    s += '%s::~%s () {}\n' % (tname, tname)
    return s
  def generate_virtual_method_skel (self, functype, type_info):
    s = ''
    if functype.pure:
      return s
    fs = self.C (type_info) + '::' + functype.name
    tnsl = type_info.list_namespaces() # type namespace list
    absname = '::'.join ([n.name for n in tnsl] + [ fs ])
    if absname in self.skip_symbols:
      return ''
    s += self.open_namespace (type_info)
    sret = self.Cp (functype.rtype, '*')
    sret += '\n'
    fs += ' ('
    argindent = len (fs)
    s += '\n' + sret + fs
    l = []
    for a in functype.args:
      l += [ self.format_arg (a[0], a[1]) ]
    s += (',\n' + argindent * ' ').join (l)
    s += ')\n{\n'
    if functype.rtype.storage == Decls.VOID:
      pass
    elif functype.rtype.storage == Decls.ENUM:
      s += '  return %s (0);\n' % self.rtype2cpp (functype.rtype)
    elif functype.rtype.storage in (Decls.RECORD, Decls.SEQUENCE):
      s += '  return %s();\n' % self.rtype2cpp (functype.rtype)
    elif functype.rtype.storage == Decls.INTERFACE:
      s += '  return (%s*) NULL;\n' % self.C (functype.rtype)
    else:
      s += '  return 0;\n'
    s += '}\n'
    return s
  def generate_interface_skel (self, type_info):
    s = ''
    for m in type_info.methods:
      s += self.generate_virtual_method_skel (m, type_info)
    return s
  def generate_enum_decl (self, type_info):
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
  def insertion_text (self, key):
    text = self.insertions.get (key, '')
    lstrip = re.compile ('^\n*')
    rstrip = re.compile ('\n*$')
    text = lstrip.sub ('', text)
    text = rstrip.sub ('', text)
    if text:
      ind = '  ' if key.startswith ('class_scope:') else '' # inner class indent
      return ind + '// ' + key + ':\n' + text + '\n\n'
    else:
      return ''
  def insertion_file (self, filename):
    f = open (filename, 'r')
    key = None
    for line in f:
      m = (re.match ('(includes):\s*(//.*)?$', line) or
           re.match ('(class_scope:\w+):\s*(//.*)?$', line) or
           re.match ('(filtered_class_hh:\w+):\s*(//.*)?$', line) or
           re.match ('(filtered_class_cc:\w+):\s*(//.*)?$', line) or
           re.match ('(global_scope):\s*(//.*)?$', line))
      if not m:
        m = re.match ('(IGNORE):\s*(//.*)?$', line)
      if m:
        key = m.group (1)
        continue
      if key:
        block = self.insertions.get (key, '')
        block += line
        self.insertions[key] = block
  def symbol_file (self, filename):
    f = open (filename)
    txt = f.read()
    f.close()
    import re
    w = re.findall (r'([a-zA-Z_][a-zA-Z_0-9$:]*)', txt)
    self.skip_symbols.update (set (w))
  def generate_impl_types (self, implementation_types):
    s = '/* --- Generated by PLIC-CxxCaller --- */\n'
    # CPP guard
    if self.cppguard:
      s += '#ifndef %s\n#define %s\n\n' % (self.cppguard, self.cppguard)
    # inclusions
    if self.gen_inclusions:
      s += '\n// --- Custom Includes ---\n'
    if self.gen_inclusions and (self.gen_clientcc or self.gen_servercc):
      s += '#ifndef __PLIC_UTILITIES_HH__\n'
    for i in self.gen_inclusions:
      s += '#include %s\n' % i
    if self.gen_inclusions and (self.gen_clientcc or self.gen_servercc):
      s += '#endif\n'
    s += self.insertion_text ('includes')
    if self.gen_clienthh:
      s += clienthh_boilerplate
    if self.gen_serverhh:
      s += serverhh_boilerplate
    if self.gen_clientcc or self.gen_servercc:
      s += gencc_boilerplate + '\n'
    self.tabwidth (16)
    s += self.open_namespace (None)
    self.gen4class = C4CLIENT
    # collect impl types
    types = []
    for tp in implementation_types:
      if tp.isimpl:
        types += [ tp ]
    # generate client/server decls
    if self.gen_clienthh or self.gen_serverhh:
      s += '\n// --- Interfaces (class declarations) ---\n'
      for tp in types:
        if tp.is_forward:
          s += self.open_namespace (tp) + '\n'
          if self.gen_serverhh:
            self.gen4class = C4INTERFACE
            s += 'class %s;\n' % self.C (tp)    # C4INTERFACE
            self.gen4class = C4SERVER
            s += 'class %s;\n' % self.C (tp)    # C4SERVER
            self.gen4class = C4CLIENT
          elif self.gen_clienthh:
            s += 'class %s;\n' % self.C (tp)    # C4CLIENT
          s += '\n'
        elif tp.typedef_origin:
          s += self.open_namespace (tp)
          s += 'typedef %s %s;\n' % (self.type2cpp (tp.typedef_origin), tp.name)
        elif tp.storage in (Decls.RECORD, Decls.SEQUENCE):
          s += self.open_namespace (tp)
          s += self.generate_recseq_decl (tp) + '\n'
        elif tp.storage == Decls.ENUM:
          s += self.open_namespace (tp)
          s += self.generate_enum_decl (tp) + '\n'
        elif tp.storage == Decls.INTERFACE:
          s += '\n'
          if self.gen_clienthh and not self.gen_serverhh:
            s += self.open_namespace (tp)
            s += self.generate_interface_class (tp) + '\n' # Class smart handle
          if self.gen_serverhh:
            self.gen4class = C4INTERFACE
            if tp.name in self.skip_classes:
              s += self.open_namespace (None) # close all namespaces
              s += '\n'
              s += self.insertion_text ('filtered_class_hh:' + tp.name)
            else:
              s += self.open_namespace (tp)
              s += self.generate_interface_class (tp) + '\n' # Class_Interface server base
            self.gen4class = C4SERVER
            s += self.open_namespace (tp)
            s += self.generate_interface_class (tp) + '\n' # Class smart handle
            self.gen4class = C4CLIENT
      s += self.open_namespace (None)
    # generate client/server impls
    if self.gen_clientcc or self.gen_servercc:
      s += '\n// --- Implementations ---\n'
      for tp in types:
        if tp.typedef_origin or tp.is_forward:
          continue
        if tp.storage == Decls.RECORD:
          s += self.open_namespace (tp)
          s += self.generate_record_impl (tp) + '\n'
        elif tp.storage == Decls.SEQUENCE:
          s += self.open_namespace (tp)
          s += self.generate_sequence_impl (tp) + '\n'
        elif tp.storage == Decls.INTERFACE:
          if self.gen_servercc:
            self.gen4class = C4INTERFACE
            if tp.name in self.skip_classes:
              s += self.open_namespace (None) # close all namespaces
              s += self.insertion_text ('filtered_class_cc:' + tp.name)
            else:
              s += self.open_namespace (tp)
              s += self.generate_interface_impl (tp) + '\n'
            self.gen4class = C4CLIENT
          if self.gen_clientcc and tp.fields:
            s += self.open_namespace (tp)
            for fl in tp.fields:
              s += self.generate_client_property_stub (tp, fl[0], fl[1])
          if self.gen_clientcc and tp.methods:
            s += self.open_namespace (tp)
            for m in tp.methods:
              s += self.generate_client_method_stub (tp, m)
    # generate unmarshalling server calls
    if self.gen_servercc:
      s += '\n// --- Method Dispatchers & Registry ---\n'
      self.gen4class = C4INTERFACE
      reglines = []
      for tp in types:
        if tp.typedef_origin or tp.is_forward:
          continue
        s += self.open_namespace (tp)
        if tp.storage == Decls.INTERFACE and not tp.name in self.skip_classes:
          for fl in tp.fields:
            s += self.generate_server_property_get_dispatcher (tp, fl[0], fl[1], reglines)
            s += self.generate_server_property_set_dispatcher (tp, fl[0], fl[1], reglines)
          for m in tp.methods:
            s += self.generate_server_method_dispatcher (tp, m, reglines)
          for sg in tp.signals:
            s += self.generate_server_signal_dispatcher (tp, sg, reglines)
          s += '\n'
      s += self.generate_server_method_registry (reglines) + '\n'
      s += self.open_namespace (None)
      self.gen4class = C4CLIENT
    # generate interface method skeletons
    if self.gen_server_skel:
      s += '\n// --- Interface Skeletons ---\n'
      self.gen4class = C4INTERFACE
      for tp in types:
        if tp.typedef_origin or tp.is_forward:
          continue
        elif tp.storage == Decls.INTERFACE and not tp.name in self.skip_classes:
          s += self.generate_interface_skel (tp)
      self.gen4class = C4CLIENT
    s += self.open_namespace (None) # close all namespaces
    s += '\n'
    s += self.insertion_text ('global_scope')
    # CPP guard
    if self.cppguard:
      s += '#endif /* %s */\n' % self.cppguard
    return s

def error (msg):
  import sys
  print >>sys.stderr, sys.argv[0] + ":", msg
  sys.exit (127)

def generate (namespace_list, **args):
  import sys, tempfile, os
  global I_prefix_postfix
  config = {}
  config.update (args)
  gg = Generator()
  all = config['backend-options'] == []
  gg.gen_serverhh = all or 'serverhh' in config['backend-options']
  gg.gen_servercc = all or 'servercc' in config['backend-options']
  gg.gen_server_skel = 'server-skel' in config['backend-options']
  gg.gen_clienthh = all or 'clienthh' in config['backend-options']
  gg.gen_clientcc = all or 'clientcc' in config['backend-options']
  gg.gen_shortalias = all or 'shortalias' in config['backend-options']
  gg.gen_inclusions = config['inclusions']
  for opt in config['backend-options']:
    if opt.startswith ('cppguard='):
      gg.cppguard += opt[9:]
    if opt.startswith ('iface-postfix='):
      I_prefix_postfix = (I_prefix_postfix[0], opt[14:])
    if opt.startswith ('iface-prefix='):
      I_prefix_postfix = (opt[13:], I_prefix_postfix[1])
    if opt.startswith ('iface-base='):
      gg._iface_base = opt[11:]
    if opt.startswith ('filter-out='):
      gg.skip_classes += opt[11:].split (',')
  for ifile in config['insertions']:
    gg.insertion_file (ifile)
  for ssfile in config['skip-skels']:
    gg.symbol_file (ssfile)
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
