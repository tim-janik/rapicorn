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
import sys, os
pkginstall_configvars = {
  'PLIC_VERSION' : '0.0-uninstalled',
  'pyutilsdir'   : '.',
  #@PKGINSTALL_CONFIGVARS_IN24LINES@ # configvars are substituted upon script installation
}
sys.path.insert (0, pkginstall_configvars["pyutilsdir"])
import yapps2runtime as runtime
import Parser, Decls, GenUtils # pre-import modules for Generator modules
true, false, length = (True, False, len)

backends = [ 'TypeMap', 'CxxStub', 'GType', 'Rapicorn' ]

class ParseError (Exception):
  def __init__ (self, msg = "Parse Error", kind = "ParseError"):
    Exception.__init__ (self, msg)
    self.kind = kind

def parse_main (config, input_string, filename):
  impltypes, error, caret, inclist = Parser.parse_file (config, input_string, filename)
  nsdict = {}
  nslist = []
  if impltypes:
    for type in impltypes:
      if not nsdict.get (type.namespace, None):
        nsdict[type.namespace] = 1
        nslist += [ type.namespace ]
  return (nslist, impltypes, error, caret, inclist)

def module_import (module_or_file):
  if os.path.isabs (module_or_file):
    module_dir, module_file = os.path.split (module_or_file)
    module_name, module_ext = os.path.splitext (module_file)
    savedpath = sys.path
    sys.path = [ module_dir ]
    try:
      module_obj = __import__ (module_name)
    except:
      sys.path = savedpath
      raise
    sys.path = savedpath
    return module_obj
  else:
    return __import__ (module_or_file)

def main():
  config = parse_files_and_args()
  files = config['files']
  if len (files) == 0: # interactive
    try:
      input_string = raw_input ('IDL> ')
    except EOFError:
      input_string = ""
    filename = '<stdin>'
    print
    nslist, impltypes, error, caret, inclist = parse_main (config, input_string, filename)
  else: # file IO
    error = None
    for fname in files:
      f = open (fname, 'r')
      input_string = f.read()
      filename = fname
      nslist, impltypes, error, caret, inclist = parse_main (config, input_string, filename)
      if error:
        break
  if error:
    print >>sys.stderr, error
    if caret:
      print >>sys.stderr, caret
    for ix in inclist:
      print >>sys.stderr, ix
    sys.exit (7)
  module_import (config['backend']).generate (nslist, implementation_types = impltypes, **config)

def print_help (with_help = True):
  print "plic version", pkginstall_configvars["PLIC_VERSION"]
  if not with_help:
    return
  print "Usage: %s [options] <idlfile>" % os.path.basename (sys.argv[0])
  print "       %s [solitary-option]" % os.path.basename (sys.argv[0])
  print "Options:"
  print "  --help, -h                print this help message"
  print "  --version, -v             print version info"
  print "  --output-format=<oformat>"
  print "  -o=<outputfile>           output filename"
  print "  -G=<oformat>              select output format"
  print "  -g=<generator-option>     set output generator option"
  print "  --insertions=<insfile>    file for insertion points"
  print '  --inclusions=<"include">  include statements'
  print '  --skip-skels=<symfile>    symbols to skip skeletons for'
  print "Solitary Options:"
  print "  --list-formats            list output formats"

def parse_files_and_args():
  import re, getopt
  config = { 'files' : [], 'backend' : 'PrettyDump', 'backend-options' : [],
             'insertions' : [], 'inclusions' : [], 'skip-skels' : [], 'system-typedefs' : False }
  sop = 'vhG:g:o:'
  lop = ['help', 'version', 'output-format=', 'list-formats',
         'plic-debug', 'cc-intern-file=',
         'insertions=', 'inclusions=', 'skip-skels=']
  if pkginstall_configvars.get ('INTERN', 0):
    lop += [ 'system-typedefs' ]
  try:
    options,args = getopt.gnu_getopt (sys.argv[1:], sop, lop)
  except Exception, ex:
    print >>sys.stderr, sys.argv[0] + ":", str (ex)
    print_help(); sys.exit (1)
  for arg,val in options:
    if arg == '-h' or arg == '--help': print_help(); sys.exit (0)
    if arg == '-v' or arg == '--version': print_help (false); sys.exit (0)
    if arg == '--plic-debug': config['pass-exceptions'] = 1
    if arg == '-o': config['output'] = val
    if arg == '--insertions': config['insertions'] += [ val ]
    if arg == '--inclusions': config['inclusions'] += [ val ]
    if arg == '--skip-skels': config['skip-skels'] += [ val ]
    if arg == '-g': config['backend-options'] += [ val ]
    if arg == '--cc-intern-file':
      s, data = '', open (val).read()
      for c in data:
        if   c == '\\':                 s += r'\\'
        elif c == '"':                  s += r'\"'
        elif c >= ' ' and c <= '~':     s += c
        else:                           s += '\%o' % ord (c)
        if length (s) % 32 == 0:        s += '"\n"' # this misses some line breaks due to multibyte increments
      print '// This file is generated by plic.py. DO NOT EDIT.'
      print 'const char intern_%s[] =\n"%s";' % (re.sub (r'[^a-zA-Z0-9_]', '_', val), s)
      sys.exit (0)
    if arg == '-G' or arg == '--generator':
      if val[0] == '=': val = val[1:]
      if val in backends:
        config['backend'] = val
      else:
        ap = os.path.abspath (val)
        if (ap.endswith (".py") and
            os.access (ap, os.R_OK)):
          config['backend'] = ap
        elif os.access (ap + ".py", os.R_OK):
          config['backend'] = ap + ".py"
        elif os.access (ap + ".pyc", os.R_OK):
          config['backend'] = ap + ".pyc"
        else:
          print >>sys.stderr, sys.argv[0] + ": unknown output format:", val
          print_help(); sys.exit (1)
    if arg == '--list-formats':
      print "\nAvailable Output Formats:"
      b = backends
      if os.path.isabs (config['backend']):
        b = b + [ config['backend'] ]
      for be in b:
        bedoc = module_import (be).__doc__.strip()
        bedoc = re.sub ('\n\s*\n', '\n', bedoc)                         # remove empty lines
        bedoc = re.compile (r'^', re.MULTILINE).sub ('    ', bedoc)     # indent
        print "  %s" % be
        print bedoc
      print
      sys.exit (0)
    if arg == '--system-typedefs':
      config['system-typedefs'] = True
  config['files'] += list (args)
  return config

failtestoption = '--plic-fail-file-test'
if len (sys.argv) > 2 and failtestoption in sys.argv:
  import tempfile, os
  sys.argv.remove (failtestoption) # remove --plic-fail-file-test
  config = parse_files_and_args()
  config['anonimize-filepaths'] = true # anonimize paths for varying builddirs (../a/b/c.idl -> .../c.idl)
  files = config['files']
  if len (files) != 1:
    raise Exception (failtestoption + ': single input file required')
  infile = open (files[0], 'r')
  n = 0
  for line in infile:
    n += 1
    ls = line.strip()
    if ls and not ls.startswith ('//'):
      filename = infile.name
      Parser.yy.reset()
      if line.startswith ("include"):
        code = '\n' * (n-1) + line
      else:
        code = '\n' * (n-2) + 'namespace PlicFailTest {\n' + line + '\n}'
      nslist, impltypes, error, caret, inclist = parse_main (config, code, filename)
      if error:
        import re
        error = re.sub (r'^[^:]*/([^/:]+):([0-9]+):', r'.../\1:\2:', error)
        print error
        if caret:
          print caret
        for ix in inclist:
          print ix
        # expected a failing tests
      else:
        raise Exception (filename + ': uncaught test:', line)
  infile.close()
elif __name__ == '__main__':
  main()
