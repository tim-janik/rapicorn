# Rapicorn                              -*- Mode: python; -*-
# Copyright (C) 2010 Tim Janik
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

from pyRapicorn import *   # generated _PLIC_0XXX... cpy methods
from py2cpy import *       # generated Python classes, Application, etc
for s in dir (pyRapicorn): # allow py2cpy to access generated methods
  if s.startswith ('_PLIC_'):
    setattr (py2cpy, s, getattr (pyRapicorn, s))

app = None
def app_init (application_name = None):
  import sys, pyRapicorn, py2cpy
  cmdline_args = None
  if application_name == None:
    application_name = os.path.abspath (sys.argv[0] or '-')
  if cmdline_args == None:
    cmdline_args = sys.argv
  plic_id = pyRapicorn._init_dispatcher (application_name, cmdline_args)
  a = py2cpy.Application (py2cpy._BaseClass_._PlicID_ (plic_id))
  global app
  app = a
  return app
def check_event ():
  import pyRapicorn
  return pyRapicorn._check_event()
def dispatch_event ():
  import pyRapicorn
  return pyRapicorn._dispatch_event()
#app_init._init_dispatcher = pyRapicorn._init_dispatcher # save for app_init() use
del globals()['pyRapicorn']
del globals()['py2cpy']
