#!/usr/bin/env python
# plic - Pluggable IDL Compiler                                -*-mode:python-*-
# Copyright (C) 2007 Tim Janik
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
import sys, Decls
true, false, length = (True, False, len)
#@PLICSUBST_PREIMPORT@
PLIC_VERSION=\
"@PLICSUBST_VERSION@"   # this needs to be in column0 for @@ replacements to work

import yapps2runtime as runtime

keywords = ( 'TRUE', 'True', 'true', 'FALSE', 'False', 'false',
             'namespace', 'enum', 'enumeration', 'Const', 'record', 'class', 'sequence',
             'Bool', 'Num', 'Real', 'String' )

class YYGlobals (object):
  def __init__ (self):
    self.ecounter = None
    self.namespaces = []
    self.ns_list = [] # namespaces
  def nsadd_const (self, name, value):
    if not isinstance (value, (int, long, float, str)):
      raise TypeError ('constant expression does not yield string or number: ' + repr (typename))
    self.namespaces[0].add_const (name, value)
  def nsadd_evalue (self, evalue_ident, evalue_label, evalue_blurb, evalue_number = None):
    AS (evalue_ident)
    if evalue_number == None:
      evalue_number = yy.ecounter
      yy.ecounter += 1
    else:
      AN (evalue_number)
      yy.ecounter = 1 + evalue_number
    yy.nsadd_const (evalue_ident, evalue_number)
    return (evalue_ident, evalue_label, evalue_blurb, evalue_number)
  def nsadd_enum (self, enum_name, enum_values):
    enum = Decls.TypeInfo (enum_name, Decls.ENUM)
    for ev in enum_values:
      enum.add_option (*ev)
    self.namespaces[0].add_type (enum)
  def nsadd_record (self, name, rfields):
    AIn (name)
    if len (rfields) < 1:
      raise AttributeError ('invalid empty record: %s' % name)
    rec = Decls.TypeInfo (name, Decls.RECORD)
    fdict = {}
    for field in rfields:
      if fdict.has_key (field[0]):
        raise NameError ('duplicate record field name: ' + field[0])
      fdict[field[0]] = true
      rec.add_field (field[0], field[1])
    self.namespaces[0].add_type (rec)
  def nsadd_sequence (self, name, sfields):
    AIn (name)
    if len (sfields) < 1:
      raise AttributeError ('invalid empty sequence: %s' % name)
    if len (sfields) > 1:
      raise AttributeError ('invalid multiple fields in sequence: %s' % name)
    seq = Decls.TypeInfo (name, Decls.SEQUENCE)
    seq.set_elements (sfields[0][0], sfields[0][1])
    self.namespaces[0].add_type (seq)
  def namespace_lookup (self, full_identifier, **flags):
    words = full_identifier.split ('::')
    isabs = words[0] == ''      # ::PrefixedName
    if isabs: words = words[1:]
    prefix = '::'.join (words[:-1])
    identifier = words[-1]
    targetns = []
    condidates = []
    # match outer namespaces by identifier
    if not isabs and not prefix:
      condidates = self.namespaces
    # match inner namespaces by prefix
    if not targetns and not isabs and prefix:
      iprefix = self.namespaces[0].name + '::' + prefix
      for ns in self.ns_list:
        if ns.name == iprefix:
          targetns = [ns]
          break
    # match outer namespaces by prefix
    if not targetns and not isabs and prefix:
      for ns in self.namespaces:
        if ns.name.endswith (prefix):
          targetns = [ns]
          break
    # match absolute namespaces by prefix
    if not targetns and prefix:
      for ns in self.ns_list:
        if ns.name == prefix:
          targetns = [ns]
          break
    # identifier lookup
    for ns in condidates + targetns:
      if flags.get ('astype', 0):
        type_info = ns.find_type (identifier)
        if type_info:
          return type_info
      if flags.get ('asconst', 0):
        cvalue = ns.find_const (identifier)
        if cvalue:
          return cvalue
    return None
  def resolve_type (self, typename):
    type_info = self.namespace_lookup (typename, astype = True)
    if not type_info:
      raise TypeError ('unknown type: ' + repr (typename))
    return type_info
  def namespace_open (self, ident):
    full_ident = "::". join ([ns.name for ns in self.namespaces] + [ident])
    namespace = None
    for ns in self.ns_list:
      if ns.name == ident:
        namespace = ns
    if not namespace:
      namespace = Decls.Namespace (full_ident)
      self.ns_list.append (namespace)
    self.namespaces = [ namespace ] + self.namespaces
    # initialize namespace
    namespace.add_type (Decls.TypeInfo ('Bool',   Decls.NUM))
    namespace.add_type (Decls.TypeInfo ('Num',    Decls.NUM))
    namespace.add_type (Decls.TypeInfo ('Real',   Decls.REAL))
    namespace.add_type (Decls.TypeInfo ('String', Decls.STRING))
  def namespace_close (self):
    assert len (self.namespaces)
    self.namespaces = self.namespaces[1:]
yy = YYGlobals() # globals

def constant_lookup (variable):
  value = yy.namespace_lookup (variable, asconst = True)
  if not value:
    raise NameError ('undeclared constant: ' + variable)
  return value[0]
def quote (qstring):
  import rfc822
  return '"' + rfc822.quote (qstring) + '"'
def unquote (qstring):
  assert (qstring[0] == '"' and qstring[-1] == '"')
  import rfc822
  return rfc822.unquote (qstring)
def TN (number_candidate):  # test number
  return isinstance (number_candidate, int) or isinstance (number_candidate, float)
def TS (string_candidate):  # test string
  return isinstance (string_candidate, str) and len (string_candidate) >= 2
def TSp (string_candidate): # test plain string
  return TS (string_candidate) and string_candidate[0] == '"'
def TSi (string_candidate): # test i18n string
  return TS (string_candidate) and string_candidate[0] == '_'
def AN (number_candidate):  # assert number
  if not TN (number_candidate): raise TypeError ('invalid number: ' + repr (number_candidate))
def AS (string_candidate):  # assert string
  if not TS (string_candidate): raise TypeError ('invalid string: ' + repr (string_candidate))
def ASp (string_candidate, constname = None):   # assert plain string
  if not TSp (string_candidate):
    if constname:   raise TypeError ("invalid untranslated string (constant '%s'): %s" % (constname, repr (string_candidate)))
    else:           raise TypeError ('invalid untranslated string: ' + repr (string_candidate))
def ASi (string_candidate): # assert i18n string
  if not TSi (string_candidate): raise TypeError ('invalid translated string: ' + repr (string_candidate))
def AIn (identifier):   # assert new identifier
  if (yy.namespace_lookup (identifier, astype = True, asconst = True) or
      identifier in keywords):
    raise KeyError ('redefining existing identifier: %s' % identifier)
def ATN (typename):     # assert a typename
  yy.resolve_type (typename) # raises exception

%%
parser IdlSyntaxParser:
        ignore:             r'\s+'                          # spaces
        ignore:             r'//.*?\r?(\n|$)'               # single line comments
        ignore:             r'/\*([^*]|\*[^/])*\*/'         # multi line comments
        token EOF:          r'$'
        token IDENT:        r'[a-zA-Z_][a-zA-Z_0-9]*'       # identifiers
        token INTEGER:      r'[0-9]+'
        token FULLFLOAT:    r'([1-9][0-9]*|0)(\.[0-9]*)?([eE][+-][0-9]+)?'
        token FRACTFLOAT:                     r'\.[0-9]+([eE][+-][0-9]+)?'
        token STRING:       r'"([^"\\]+|\\.)*"'             # double quotes string

rule IdlSyntax: ( ';' | namespace )* EOF        {{ return yy.ns_list; }}

rule namespace:
        'namespace' IDENT                       {{ yy.namespace_open (IDENT) }}
        '{' declaration* '}'                    {{ yy.namespace_close() }}
rule declaration:
          ';'
        | const_assignment
        | enumeration
        | sequence
        | record
        | namespace

rule enumeration:
        ( 'enumeration' | 'enum' )
        IDENT '{'                               {{ evalues = []; yy.ecounter = 1 }}
        enumeration_rest                        {{ evalues = enumeration_rest }}
        '}'                                     {{ AIn (IDENT); yy.nsadd_enum (IDENT, evalues) }}
        ';'                                     {{ evalues = None; yy.ecounter = None }}
rule enumeration_rest:                          {{ evalues = [] }}
        ( ''                                    # empty
        | enumeration_value                     {{ evalues = evalues + [ enumeration_value ] }}
          [ ',' enumeration_rest                {{ evalues = evalues + enumeration_rest }}
          ]
        )                                       {{ return evalues }}
rule enumeration_value:
        IDENT                                   {{ l = [IDENT, None, "", ""]; AIn (IDENT) }}
        [ '='
          ( r'\(' enumeration_args              {{ l = [ IDENT ] + enumeration_args }}
            r'\)'
          | r'(?!\()'                           # disambiguate from enumeration arg list
            expression                          {{ if TS (expression): l = [ None, expression ]; }}
                                                {{ else:               l = [ expression, "" ] }}
                                                {{ l = [ IDENT ] + l + [ "" ] }}
          )
        ]                                       {{ return yy.nsadd_evalue (l[0], l[2], l[3], l[1]) }}
rule enumeration_args:
        expression                              {{ l = [ expression ] }}
                                                {{ if TS (expression): l = [ None ] + l }}
        [   ',' expression                      {{ AS (expression); l.append (expression) }}
        ] [ ',' expression                      {{ if len (l) >= 3: raise OverflowError ("too many arguments") }}
                                                {{ AS (expression); l.append (expression) }}
        ]                                       {{ while len (l) < 3: l.append ("") }}
                                                {{ return l }}

rule typename:                                  {{ plist = [] }}
        [ '::'                                  {{ plist += [ '' ] }}
        ] IDENT                                 {{ plist += [ IDENT ] }}
        ( '::' IDENT                            {{ plist.append (IDENT) }}
          )*                                    {{ id = "::".join (plist); ATN (id); return id }}

rule variable_decl:
        typename                                {{ vtype = yy.resolve_type (typename) }}
        IDENT                                   {{ vars = [ (IDENT, vtype) ] }}
        ';'                                     {{ return vars }}

rule record:
        'record' IDENT '{'                      {{ rfields = [] }}
          ( variable_decl                       {{ rfields = rfields + variable_decl }}
          )+
        '}' ';'                                 {{ yy.nsadd_record (IDENT, rfields) }}

rule sequence:
        'sequence' IDENT '{'                    {{ sfields = [] }}
          ( variable_decl                       {{ if len (sfields): raise OverflowError ("too many fields in sequence") }}
                                                {{ sfields = sfields + variable_decl }}
          )
        '}' ';'                                 {{ yy.nsadd_sequence (IDENT, sfields) }}

rule const_assignment:
        'Const' IDENT '=' expression ';'        {{ AIn (IDENT); yy.nsadd_const (IDENT, expression); }}

rule expression: summation                      {{ return summation }}
rule summation:
          factor                                {{ result = factor }}
        ( r'\+' factor                          {{ AN (result); result = result + factor }}
        | r'-'  factor                          {{ result = result - factor }}
        )*                                      {{ return result }}
rule factor:
          signed                                {{ result = signed }}
        ( r'\*' signed                          {{ result = result * signed }}
        | r'/'  signed                          {{ result = result / signed }}
        | r'%'  signed                          {{ AN (result); result = result % signed }}
        )*                                      {{ return result }}
rule signed:
          power                                 {{ return power }}
        | r'\+' signed                          {{ return +signed }}
        | r'-'  signed                          {{ return -signed }}
rule power:
          term                                  {{ result = term }}
        ( r'\*\*' signed                        {{ result = result ** signed }}
        )*                                      {{ return result }}
rule term:                                      # numerical/string term
          '(TRUE|True|true)'                    {{ return 1; }}
        | '(FALSE|False|false)'                 {{ return 0; }}
        | IDENT                                 {{ result = constant_lookup (IDENT); }}
          (string                               {{ ASp (result, IDENT); ASp (string); result += string }}
          )*                                    {{ return result }}
        | INTEGER                               {{ return int (INTEGER); }}
        | FULLFLOAT                             {{ return float (FULLFLOAT); }}
        | FRACTFLOAT                            {{ return float (FRACTFLOAT); }}
        | r'\(' expression r'\)'                {{ return expression; }}
        | string                                {{ return string; }}

rule string:
          '_' r'\(' plain_string r'\)'          {{ return '_(' + plain_string + ')' }}
        | plain_string                          {{ return plain_string }}
rule plain_string:
        STRING                                  {{ result = quote (eval (STRING)) }}
        ( ( STRING                              {{ result = quote (unquote (result) + eval (STRING)) }}
          | IDENT                               {{ con = constant_lookup (IDENT); ASp (con, IDENT) }}
                                                {{ result = quote (unquote (result) + unquote (con)) }}
          ) )*                                  {{ return result }}
%%

