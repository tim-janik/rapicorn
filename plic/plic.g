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
import sys
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
    self.namespace = None
    self.nsdict = None
    self.ns_list = [] # namespaces
  def dictadd (self, name, kind, members):
    self.nsdict[name] = (kind, members)
  def namespace_open (self, ident):
    assert self.namespace == None and self.nsdict == None
    self.namespace = ident
    self.nsdict = {}
    # initialize namespace
    for builtintype in ('Bool', 'Num', 'Real', 'String'):
      self.dictadd (builtintype, 'builtin', builtintype)
  def namespace_close (self):
    assert isinstance (self.namespace, str) and isinstance (self.nsdict, dict)
    self.ns_list.append ((self.namespace, self.nsdict))
    self.namespace = None
    self.nsdict = None
  def namespace_find (self, name, fallback = None):
    return self.nsdict.get (name, fallback)
yy = YYGlobals() # globals

def constant_lookup (variable):
    type, value = yy.namespace_find (variable, (None, 0))
    if type != 'Const':
        raise NameError ('undeclared constant: ' + variable)
    return value
def add_evalue (evalue_tuple):
    evalue_name   = evalue_tuple[0]
    evalue_number = evalue_tuple[1]
    evalue_label  = evalue_tuple[2]
    evalue_blurb  = evalue_tuple[3]
    if evalue_number == None:
      evalue_number = yy.ecounter
      yy.ecounter += 1
    else:
      yy.ecounter = 1 + evalue_number
    AS (evalue_name)
    AN (evalue_number)
    yy.dictadd (evalue_name, 'Const', evalue_number)
    return (evalue_name, evalue_number, evalue_label, evalue_blurb)
def add_record (name, rfields):
    AIn (name)
    if len (rfields) < 1:
      raise AttributeError ('invalid empty record: %s' % name)
    fdict = {}
    for field in rfields:
      if fdict.has_key (field[1]):
        raise NameError ('duplicate record field name: ' + field[1])
      fdict[field[1]] = field[0]
    yy.dictadd (name, 'record', tuple (rfields))
def add_sequence (name, sfields):
    AIn (name)
    if len (sfields) < 1:
      raise AttributeError ('invalid empty sequence: %s' % name)
    if len (sfields) > 1:
      raise AttributeError ('invalid multiple fields in sequence: %s' % name)
    yy.dictadd (name, 'sequence', tuple (sfields))
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
    if yy.namespace_find (identifier) or identifier in keywords:  raise KeyError ('redefining existing identifier: %s' % identifier)
def ATN (typename):     # assert a typename
  ttuple = yy.namespace_find (typename, (None,))
  print "TYPE: ", typename, ttuple
  if not ttuple[0] in ('sequence', 'record', 'enum', 'builtin'):
    raise TypeError ('invalid typename: ' + repr (typename))

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

rule enumeration:
        ( 'enumeration' | 'enum' )
        IDENT '{'                               {{ evalues = []; yy.ecounter = 1 }}
        enumeration_rest                        {{ evalues = enumeration_rest }}
        '}' ';'                                 {{ AIn (IDENT); yy.dictadd (IDENT, 'enum', tuple (evalues)); yy.ecounter = None }}
rule enumeration_rest:                          {{ evalues = [] }}
        ( ''                                    # empty
        | enumeration_value                     {{ evalues = evalues + [ add_evalue (enumeration_value) ] }}
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
        ]                                       {{ return tuple (l) }}
rule enumeration_args:
        expression                              {{ l = [ expression ] }}
                                                {{ if TS (expression): l = [ None ] + l }}
        [   ',' expression                      {{ AS (expression); l.append (expression) }}
        ] [ ',' expression                      {{ if len (l) >= 3: raise OverflowError ("too many arguments") }}
                                                {{ AS (expression); l.append (expression) }}
        ]                                       {{ while len (l) < 3: l.append ("") }}
                                                {{ return l }}

rule typename:
        IDENT                                   {{ ATN (IDENT); return IDENT }}

rule variable_decl:
        typename IDENT                          {{ vtype = typename; vars = [ (vtype, IDENT) ] }}
        ';'                                     {{ return vars }}

rule record:
        'record' IDENT '{'                      {{ rfields = [] }}
          ( variable_decl                       {{ rfields = rfields + variable_decl }}
          )+
        '}' ';'                                 {{ add_record (IDENT, rfields) }}

rule sequence:
        'sequence' IDENT '{'                    {{ sfields = [] }}
          ( variable_decl                       {{ if len (sfields): raise OverflowError ("too many fields in sequence") }}
                                                {{ sfields = sfields + variable_decl }}
          )
        '}' ';'                                 {{ add_sequence (IDENT, sfields) }}

rule const_assignment:
        'Const' IDENT '=' expression ';'        {{ AIn (IDENT); yy.dictadd (IDENT, 'Const', expression); }}

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

class ParseError (runtime.SyntaxError):
    def __init__ (self, msg = "Parse Error"):
        runtime.SyntaxError.__init__ (self, None, msg)
    def __str__(self):
        if not self.pos: return 'ParseError'
        else: return 'ParseError@%s(%s)' % (repr (self.pos), self.msg)

def main():
  from sys import argv, stdin
  files = parse_files_and_args()
  if len (files) >= 1: # file IO
    f = open (files[0], 'r')
    input_string = f.read()
    filename = files[0]
  else:               # interactive
    print >>sys.stderr, 'Usage: %s [filename]' % argv[0]
    try:
      input_string = raw_input ('IDL> ')
    except EOFError:
      input_string = ""
    filename = '<stdin>'
    print
  isp = IdlSyntaxParser (IdlSyntaxParserScanner (input_string, filename = filename))
  result = None
  # parsing: isp.IdlSyntax ()
  ex = None
  try:
    #runtime.wrap_error_reporter (isp, 'IdlSyntax') # parse away
    result = isp.IdlSyntax ()
  except runtime.SyntaxError, ex:
    ex.context = None # prevent context printing
    runtime.print_error (ex, isp._scanner)
  except AssertionError: raise
  except Exception, ex:
    # raise # pass exceptions on when debugging
    exstr = str (ex)
    if exstr: exstr = ': ' + exstr
    runtime.print_error (ParseError ('%s%s' % (ex.__class__.__name__, exstr)), isp._scanner)
  if ex: sys.exit (7)
  print 'namespaces ='
  import pprint
  if 1:
    pprint.pprint (result)
  else:
    print result

def parse_files_and_args ():
  from sys import argv, stdin
  files = []
  arg_iter = sys.argv[1:].__iter__()
  rest = len (sys.argv) - 1
  for arg in arg_iter:
    rest -= 1
    if arg == '--help' or arg == '-h':
      print_help ()
      sys.exit (0)
    elif arg == '--version' or arg == '-v':
      print_help (False)
      sys.exit (0)
    else:
      files = files + [ arg ]
  return files

def print_help (with_help = True):
  import os
  print "plic (Rapicorn utils) version", PLIC_VERSION
  if not with_help:
    return
  print "Usage: %s [OPTIONS] <idlfile>" % os.path.basename (sys.argv[0])
  print "Options:"
  print "  --help, -h                print this help message"
  print "  --version, -v             print version info"

if __name__ == '__main__':
  main()
