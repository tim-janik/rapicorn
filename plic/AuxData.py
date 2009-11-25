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
import Decls


auxillary_initializers = {
  (Decls.INT,       'Int')      : ('label', 'blurb', 'default=0', 'min', 'max', 'step', 'hints'),
  (Decls.FLOAT,     'Float')    : ('label', 'blurb', 'default=0', 'min', 'max', 'step', 'hints'),
  (Decls.STRING,    'String')   : ('label', 'blurb', 'default', 'hints'),
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
