#!/usr/bin/env python
# This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
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
  (Decls.STREAM,    'Stream')   : ('label', 'blurb', 'hints'),
}

class Error (Exception):
  pass

def auxillary_initializer_dict():
  # we need to merge dicts no earlier than before the first call, to avoid depending on extension load order
  if not hasattr (auxillary_initializer_dict, 'cached_dict'):
    d = {}
    d.update (__Aida__.auxillary_initializers)
    d.update (auxillary_initializers)
    auxillary_initializer_dict.cached_dict = d
  return auxillary_initializer_dict.cached_dict

def parse2dict (type, name, arglist):
  # find matching initializer
  adef = auxillary_initializer_dict().get ((type, name), None)
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
