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

import yapps2runtime as runtime

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
        raise NameError ('undeclared symbol: ' + variable)
    return yydict[variable]
def quote (qstring):
    import rfc822
    return '"' + rfc822.quote (qstring) + '"'
def unquote (qstring):
    assert (qstring[0] == '"' and qstring[-1] == '"')
    import rfc822
    return rfc822.unquote (qstring)
def TN (number_candidate):
    return isinstance (number_candidate, int) or isinstance (number_candidate, float)
def TS (string_candidate):
    return isinstance (string_candidate, str) and len (string_candidate) >= 2
def TSp (string_candidate):
    return TS (string_candidate) and string_candidate[0] == '"' # plain string
def TSi (string_candidate):
    return TS (string_candidate) and string_candidate[0] == '_' # i18n string
def AN (number_candidate):
    if not TN (number_candidate): raise TypeError ('invalid number: ' + repr (number_candidate))
def AS (string_candidate):
    if not TS (string_candidate): raise TypeError ('invalid string: ' + repr (string_candidate))
def ASp (string_candidate, constname = None):
    if not TSp (string_candidate):
        if constname:   raise TypeError ("invalid untranslated string (constant '%s'): %s" % (constname, repr (string_candidate)))
        else:           raise TypeError ('invalid untranslated string: ' + repr (string_candidate))
def ASi (string_candidate):
    if not TSi (string_candidate): raise TypeError ('invalid translated string: ' + repr (string_candidate))

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

rule IdlSyntax: ( ';' | namespace )* EOF        {{ return yynamespaces; }}

rule namespace:
        'namespace' IDENT                       {{ namespace_open (IDENT) }}
        '{' declaration* '}'                    {{ namespace_close() }}
rule declaration:
          ';'
        | const_assignment
        | enumeration

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
        IDENT                                   {{ l = [IDENT, None, "", ""] }} # FIXME AN/AS
        [ '='
          ( r'\(' enumeration_args              {{ l = [ IDENT ] + enumeration_args }}
            r'\)'
          | r'(?!\()'                           # disambiguate from enumeration arg list
            expression                          {{ l = [ IDENT, expression, "", "" ] }} # FIXME
          ) 
        ]                                       {{ return tuple (l) }}
rule enumeration_args:
        expression                              {{ l = [ expression ]; }}
                                                {{ if TS (expression): l = [ None ] + l }}
        [   ',' expression                      {{ AS (expression); l.append (expression) }}
        ] [ ',' expression                      {{ if len (l) >= 3: raise OverflowError ("too many arguments") }}
                                                {{ AS (expression); l.append (expression) }}
        ]                                       {{ while len (l) < 3: l.append ("") }}
                                                {{ return l }}

rule const_assignment:
        'Const' IDENT '=' expression ';'        {{ yydict[IDENT] = expression; }}


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
          '(TRUE|True|true)'                    {{ return 1; }} # FIXME: const
        | '(FALSE|False|false)'                 {{ return 0; }} # FIXME: const
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
    except AssertionError: raise
    except Exception, ex:
        exstr = str (ex)
        if exstr: exstr = ': ' + exstr
        runtime.print_error (ParseError ('%s%s' % (ex.__class__.__name__, exstr)), isp._scanner)
    print 'namespaces ='
    import pprint
    if 1:   pprint.pprint (result)
    else:   print result
