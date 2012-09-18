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
"""PLIC-PrettyDump - Pretty printing of Plic type information
"""

def generate (namespace_list, **args):
  import pprint, re
  for ns in namespace_list:
    print "namespace %s:" % ns.name
    consts = ns.consts()
    if consts:
      print "  Constants:"
      str = pprint.pformat (consts, 2)
      print re.compile (r'^', re.MULTILINE).sub ('    ', str)
    types = ns.types()
    if types:
      print "  Types:"
      for tp in types:
        print "    TypeInfo %s:" % tp.name
        str = pprint.pformat (tp.__dict__)
        print re.compile (r'^', re.MULTILINE).sub ('      ', str)

# control module exports
__all__ = ['generate']
