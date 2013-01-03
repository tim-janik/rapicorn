#!/usr/bin/env python
# Aida - Abstract Interface Definition Architecture
# Licensed GNU GPL v3 or later: http://www.gnu.org/licenses/gpl.html
import Decls


auxillary_initializers = {
  (Decls.BOOL,      'Bool')     : ('label', 'blurb', 'hints', 'default=0'),
  (Decls.INT32,     'Num')      : ('label', 'blurb', 'hints', 'default=0'),
  (Decls.INT32,     'Range')    : ('label', 'blurb', 'hints', 'min', 'max', 'step'),
  (Decls.INT64,     'Num')      : ('label', 'blurb', 'hints', 'default=0'),
  (Decls.INT64,     'Range')    : ('label', 'blurb', 'hints', 'min', 'max', 'step'),
  (Decls.FLOAT64,   'Num')      : ('label', 'blurb', 'hints', 'default=0'),
  (Decls.FLOAT64,   'Range')    : ('label', 'blurb', 'hints', 'min', 'max', 'step'),
  (Decls.STRING,    'String')   : ('label', 'blurb', 'hints', 'default'),
  (Decls.ENUM,      'Enum')     : ('label', 'blurb', 'hints', 'default'),
}

class Error (Exception):
  pass

def parse2dict (type, name, arglist):
  # find matching initializer
  adef = auxillary_initializers.get ((type, name), None)
  if not adef:
    raise Error ('invalid type definition: = %s%s%s' % (name, name and ' ', tuple (arglist)))
  if len (arglist) > len (adef):
    raise Error ('too many args for type definition: = %s%s%s' % (name, name and ' ', tuple (arglist)))
  # assign arglist to matching initializer positions
  adict = {}
  for i in range (0, len (arglist)):
    arg = adef[i].split ('=')   # 'foo=0' => ['foo', ...]
    adict[arg[0]] = arglist[i]
  # assign remaining defaulting initializer positions
  for i in range (len (arglist), len (adef)):
    arg = adef[i]
    eq = arg.find ('=')
    if eq >= 0:
      adict[arg[:eq]] = arg[eq+1:]
  return adict

# define module exports
__all__ = ['auxillary_initializers', 'parse2dict', 'Error']
