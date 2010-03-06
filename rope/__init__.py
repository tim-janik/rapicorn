# Rapicorn                              -*- Mode: python; -*-
# Copyright (C) 2008 Tim Janik
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# A copy of the GNU Lesser General Public License should ship along
# with this library; if not, see http://www.gnu.org/copyleft/.

"""Rapicorn - experimental UI toolkit

More details at http://www.rapicorn.org/.
"""

from pyRapicorn import *

app = None
class Application (object):
  __pyrope_trampoline__ = pyRapicorn.__pyrope_trampoline__
  def __new__ (klass):
    if app: return app # singleton
    self = super (Application, klass).__new__ (klass)
    return self
  @classmethod
  def create_window (klass, winname, **kwords):
    args = [k + '=' + str (v) for k,v in kwords.items()]
    print "ARGS:", args
    return klass.__rapicorn_trampoline__ (0xA001, winname)
  @classmethod
  def execute_loops (klass):
    return klass.__rapicorn_trampoline__ (0xA002)
app = Application()

del globals()['pyRapicorn']
