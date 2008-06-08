#!/usr/bin/env python
# plic - Pluggable IDL Compiler                                -*-mode:python-*-
# Copyright (C) 2007-2008 Tim Janik
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
pkginstall_configvars = {
  #@PKGINSTALL_CONFIGVARS_IN24LINES@ # configvars are substituted upon script installation
}
sys.path.insert (0, pkginstall_configvars["pyutilsdir"])
import yapps2runtime as runtime
import Parser, Decls
true, false, length = (True, False, len)

debugging = 0 # causes exceptions to bypass IDL-file parser error handling
backends = [ 'GType' ]

class ParseError (runtime.SyntaxError):
  def __init__ (self, msg = "Parse Error"):
    runtime.SyntaxError.__init__ (self, None, msg)
  def __str__(self):
    if not self.pos: return 'ParseError'
    else: return 'ParseError@%s(%s)' % (repr (self.pos), self.msg)

def main():
  from sys import stdin
  config = parse_files_and_args()
  files = config['files']
  if len (files) >= 1: # file IO
    f = open (files[0], 'r')
    input_string = f.read()
    filename = files[0]
  else:               # interactive
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

def print_help (with_help = True):
  import os
  print "plic (Rapicorn utils) version", pkginstall_configvars["PLIC_VERSION"]
  if not with_help:
    return
  print "Usage: %s [OPTIONS] <idlfile>" % os.path.basename (sys.argv[0])
  print "Options:"
  print "  --help, -h                print this help message"
  print "  --version, -v             print version info"
  print "  -B,--backend=<backend>    select backend"
  print "  --list-backends           list backend"

def parse_files_and_args():
  import re, getopt
  config = { 'files' : [], 'backend' : None }
  sop = 'vhB:'
  lop = ['help', 'version', 'backend=', 'list-backends']
  try:
    options,args = getopt.gnu_getopt (sys.argv[1:], sop, lop)
  except Exception, ex:
    print >>sys.stderr, sys.argv[0] + ":", str (ex)
    print_help(); sys.exit (1)
  for arg,val in options:
    if arg == '-h' or arg == '--help': print_help(); sys.exit (0)
    if arg == '-v' or arg == '--version': print_help (false); sys.exit (0)
    if arg == '-B' or arg == '--backend':
      config['backend'] = val
      if not val in backends:
        print >>sys.stderr, sys.argv[0] + ": unknown backend:", val
        print_help(); sys.exit (1)
    if arg == '--list-backends':
      print "\nAvailable backends:"
      for be in backends:
        bedoc = __import__ (be).__doc__
        bedoc = re.sub ('\n\s*\n', '\n', bedoc)                         # remove empty lines
        bedoc = re.compile (r'^', re.MULTILINE).sub ('    ', bedoc)     # indent
        print "  %s" % be
        print bedoc
      sys.exit (0)
  config['files'] += list (args)
  return config

if __name__ == '__main__':
  main()
