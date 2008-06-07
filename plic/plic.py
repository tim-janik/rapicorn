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
true, false, length = (True, False, len)
#@PLICSUBST_PREIMPORT@
PLIC_VERSION=\
"@PLICSUBST_VERSION@"   # this needs to be in column0 for @@ replacements to work

import yapps2runtime as runtime
import Parser, Decls

debugging = 0 # causes exceptions to bypass IDL-file parser error handling


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
  isp = Parser.IdlSyntaxParser (Parser.IdlSyntaxParserScanner (input_string, filename = filename))
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
    if debugging: raise # pass exceptions on when debugging
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
