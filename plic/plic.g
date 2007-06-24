#!/usr/bin/env python2.5
#
# plic - Pluggable IDL Compiler
# Copyright (C) 2007 Tim Janik
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.

# import yapps2runtime as runtime
from yapps import runtime

yydict = None
yynamespace = None
yynamespaces = []

def namespace_open (ident):
    global yynamespace, yydict
    assert yynamespace == None and yydict == None
    yynamespace = ident
    yydict = {}
def namespace_close ():
    global yynamespace, yydict
    assert isinstance (yynamespace, str) and isinstance (yydict, dict)
    yynamespaces.append ((yynamespace, yydict))
    yynamespace = None
    yydict = None
def constant_lookup (variable):
    global yydict
    assert isinstance (yydict, dict)
    if not yydict.has_key (variable):
        raise NameError ('undeclared symbol: %s' % variable)
    return yydict[variable]


%%
parser IdlSyntaxParser:
        ignore:             r'\s+'                          # spaces
        ignore:             r'//.*?\r?(\n|$)'               # single line comments
        ignore:             r'/\*([^*]|\*[^/])*\*/'         # multi line comments
        token EOF:          r'$'
        token IDENT:        r'[a-zA-Z_][a-zA-Z_0-9]*'       # identifiers
        token INTEGER:      r'[0-9]+'
        token FLOAT:        r'[0-9]+\.[0-9]*'               # FIXME
        token STRING:       r'"([^\"]+|\\.)*"'              # double quotes string

rule IdlSyntax: ( ';' | namespace )* EOF        {{ return yynamespaces; }}

rule namespace:
        'namespace' IDENT                       {{ namespace_open (IDENT) }}
        '{' declaration* '}'                    {{ namespace_close() }}
rule declaration:
          ';'
        | const_assignment
        | enumeration

# we cannot support leading numbers in enumeration value assignments, because
# with one token lookahead, "(5 + 6)" is indistinguishable from "(5, '6')"
rule enumeration:
        ( 'enumeration' | 'enum' )
        IDENT '{'                               {{ evalues = [] }}
        enumeration_rest                        {{ evalues = enumeration_rest }}
        '}' ';'                                 {{ yydict[IDENT] = tuple (evalues) }}
rule enumeration_rest:                          {{ evalues = [] }}
        ( ''                                    # empty
        | enumeration_value                     {{ evalues = evalues + [ enumeration_value ] }}
          [ ',' enumeration_rest                {{ evalues = evalues + enumeration_rest }}
          ] 
        )                                       {{ return evalues }}
rule enumeration_value:
        IDENT                                   {{ l = [IDENT, "", ""] }}
        [ '='
          ( string                              {{ l = [ IDENT, string, "" ] }}
          | r'\(' enumeration_args_s            {{ l = [ IDENT ] + enumeration_args_s }}
            r'\)'
          ) 
        ]                                       {{ return tuple (l) }}
rule enumeration_args_s:
        string                                  {{ l = [ string ] }}
        [ ',' string                            {{ l.append (string) }}
        ]                                       {{ while len (l) < 2: l.append ("") }}
                                                {{ return l }}

rule const_assignment:
        'Const' IDENT '=' expression ';'        {{ yydict[IDENT] = expression; }}

rule expression:
          string                                {{ return string; }}
        | numeric_expression                    {{ return numeric_expression; }}


rule numeric_expression: summation              {{ return summation }}
rule summation:
          factor                                {{ result = factor }}
        ( r'\+' factor                          {{ result = result + factor }}
        | r'-'  factor                          {{ result = result - factor }}
        )*                                      {{ return result }}
rule factor:
          power                                 {{ result = power }}
        ( r'\*' power                           {{ result = result * power }}
        | r'/'  power                           {{ result = result / power }}
        | r'%'  power                           {{ result = result % power }}
        )*                                      {{ return result }}
rule power:
          term                                  {{ result = term }}
        ( r'\*\*' term                          {{ result = result ** term }}
        )*                                      {{ return result }}
rule term:                                      # atomized numeric expressions
          INTEGER                               {{ return int (INTEGER); }}
        | FLOAT                                 {{ return float (FLOAT); }}
        | '(TRUE|True|true)'                    {{ return 1; }}
        | '(FALSE|False|false)'                 {{ return 0; }}
        | r'\(' numeric_expression r'\)'        {{ return numeric_expression; }}
        | IDENT                                 {{ return constant_lookup (IDENT) }}

rule string:
          '_' r'\(' plain_string r'\)'          {{ return plain_string }} # FIXME: loosing i18n markup
        | plain_string                          {{ return plain_string }}
rule plain_string:
      STRING                                    {{ result = eval (STRING) }}
    ( STRING                                    {{ result = result + eval (STRING) }}
    )*                                          {{ return result }}

%%

class ParseError (runtime.SyntaxError):
    def __init__ (self, msg = "Parse Error"):
        runtime.SyntaxError.__init__ (self, None, msg)
    def __str__(self):
        if not self.pos: return 'ParseError'
        else: return 'ParseError@%s(%s)' % (repr (self.pos), self.msg)

if __name__ == '__main__':
    from sys import argv, stdin
    if len (argv) >= 2: # file IO
        # f = stdin
        f = open (argv[1], 'r')
        input_string = f.read()
    else:               # interactive
        print >>sys.stderr, 'Usage: %s [filename]' % argv[0]
        try: input_string = raw_input ('IDL> ')
        except EOFError: input_string = ""
        print
    isp = IdlSyntaxParser (IdlSyntaxParserScanner (input_string))
    result = None
    try:
        #runtime.wrap_error_reporter (isp, 'IdlSyntax') # parse away
        result = isp.IdlSyntax ()
    except runtime.SyntaxError, ex:
        ex.context = None # prevent context printing
        runtime.print_error (ex, isp._scanner)
    except Exception, ex:
        exstr = str (ex)
        if exstr: exstr = ': ' + exstr
        runtime.print_error (ParseError ('%s%s' % (ex.__class__.__name__, exstr)), isp._scanner)
    print 'namespaces ='
    import pprint
    if 1:   pprint.pprint (result)
    else:   print result
