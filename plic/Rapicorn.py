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
"""PLIC-TypePackage - Binary Type Map generator for PLIC

More details at http://www.testbit.eu/plic
"""
import Decls

def encode_int (int, asstring = False):
  if int <= 9999:
    s = "%u" % int
    if len (s) <= 3 and asstring:
      s = s + '='
    return '.' * (4 - len (s)) + s
  assert int < 268435456
  chars = (128 + (int >> 21),
           128 + ((int >> 14) & 0x7f),
           128 + ((int >> 7) & 0x7f),
           128 + (int & 0x7f))
  return "%c%c%c%c" % chars
def encode_string (string):
  r = len (string) % 4
  r = r and 4 - r or 0
  return encode_int (len (string), True) + string + ' ' * r

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
  def aux_strings (self, auxlist):
    result = encode_int (len (auxlist))
    for ad in auxlist:
      result += encode_string (ad)
    return result
  def type_key (self, type_info):
    s = { Decls.NUM       : '\n__i',
          Decls.REAL      : '\n__f',
          Decls.STRING    : '\n__s',
          Decls.ENUM      : '\n__e',
          Decls.SEQUENCE  : '\n__q',
          Decls.RECORD    : '\n__r',
          Decls.INTERFACE : '\n__c',
          }[type_info.storage]
    return s
  reference_types = (Decls.ENUM, Decls.SEQUENCE, Decls.RECORD, Decls.INTERFACE)
  def generate_field (self, field_name, type_info):
    tp = type_info
    aux = []
    if tp.storage in self.reference_types:
      s = '\n__V'
    else:
      s = self.type_key (tp)
    s += encode_string (field_name)
    s += self.aux_strings (aux)
    if tp.storage in (Decls.ENUM, Decls.SEQUENCE,
                      Decls.RECORD, Decls.INTERFACE):
      s += encode_string (tp.full_name())
    return s
  def generate_type (self, type_info):
    tp = type_info
    aux = []
    s = self.type_key (type_info)
    s += encode_string (type_info.name)
    s += self.aux_strings (aux)
    if tp.storage == Decls.SEQUENCE:
      s += self.generate_field (tp.elements[0], tp.elements[1])
    elif tp.storage == Decls.RECORD:
      s += encode_int (len (tp.fields))
      for fl in tp.fields:
        s += self.generate_field (fl[0], fl[1])
    elif tp.storage == Decls.ENUM:
      s += encode_int (len (tp.options))
      for op in tp.options:
        s += encode_string (op[0]) # ident
        s += encode_string (op[1]) # label
        s += encode_string (op[2]) # blurb
    elif tp.storage == Decls.INTERFACE:
      tp.prerequisites = []
      s += encode_int (len (tp.prerequisites))
      for pr in prerequisites:
        s += encode_string (pr)
    return s
  def generate_namespace (self, namespace):
    s = encode_string (namespace.name)
    tsl = []
    for tp in namespace.types():
      t = self.generate_type (tp)
      tsl += [ t ]
    # type table
    s += encode_int (len (tsl))
    for ts in tsl:
      s += encode_int (len (ts))
    # type data
    for ts in tsl:
      s += ts
    return s
  def generate_pack (self, namespace_list):
    s = 'PlicTypePkg_01\r\n'            # magic
    s += encode_string ("unnamed")      # idl file name
    nsl = []
    for ns in namespace_list:
      t = self.generate_namespace (ns)
      nsl += [ t ]
    # namespace table
    s += encode_int (len (nsl))
    for ns in nsl:
      s += encode_int (len (ns))
    # namespace data
    for ns in nsl:
      s += ns
    return s

def generate (namespace_list, *args):
  import sys
  gg = Generator()
  packdata = gg.generate_pack (namespace_list)
  # print strcquote (packdata)
  open ("rpctyp.dat", "w").write (packdata)

# control module exports
__all__ = ['generate']
