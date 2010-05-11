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

from py2cpy import *
Application.__rope_pytrampoline__ = pyRapicorn.__rope_pytrampoline__

app = None
def app_init_with_x11 (application_name = None, cmdline_args = None):
  import sys
  if application_name == None:
    application_name = sys.argv[0]
  if cmdline_args == None:
    cmdline_args = sys.argv
  a = Application()
  a.__rope__object__ = pyRapicorn._init_dispatcher (application_name, cmdline_args)
  ret = a.init_with_x11 (application_name, cmdline_args)
  global app
  # app = pyRapicorn.__app_init_with_x11__ (application_name, cmdline_args)
  return app

pyRapicorn.printout ("printout test\n");
app_init_with_x11 ("AppName", [ "--first-arg", "--second-arg"])
print "app:", app

del globals()['pyRapicorn']

print "execute_loops():"
app.execute_loops()
