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

app = None
def app_init (application_name = None):
  global app
  assert app == None
  _CPY, _PY = app_init._CPY, app_init._PY # from _module_init_once_
  del globals()['app_init'] # run once only
  # initialize dispatching Rapicorn thread
  cmdline_args = None
  import sys
  if application_name == None:
    import os
    application_name = os.path.abspath (sys.argv[0] or '-')
  if cmdline_args == None:
    cmdline_args = sys.argv
  plic_id = _CPY._init_dispatcher (application_name, cmdline_args)
  # integrate Rapicorn dispatching with main loop
  class RapicornSource (main.Source):
    def __init__ (self):
      super (RapicornSource, self).__init__ (_CPY._event_dispatch)
      import select
      fd, pollmask = _CPY._event_fd()
      if fd >= 0:
        self.set_poll (fd, pollmask)
    def prepare (self, current_time, timeout):
      return _CPY._event_check()
    def check (self, current_time, fdevents):
      return _CPY._event_check()
    def dispatch (self, fdevents):
      self.callable() # _event_dispatch
      return True
  main.RapicornSource = RapicornSource
  # setup global Application
  app = Application (_PY._BaseClass_._PlicID_ (plic_id))
  def main_loop (self):
    self.loop = main.Loop()
    self.loop += main.RapicornSource()
    exit_status = self.loop.loop()
    del self.loop
    return exit_status
  app.__class__.main_loop = main_loop # extend for main loop integration
  return app

def _module_init_once_():
  global _module_init_once_ ; del _module_init_once_
  import pyRapicorn     # generated _PLIC_... cpy methods
  import py2cpy         # generated Python classes, Application, etc
  py2cpy.__plic_module_init_once__ (pyRapicorn)
  app_init._CPY, app_init._PY = (pyRapicorn, py2cpy) # app_init() internals
  del globals()['pyRapicorn']
  del globals()['py2cpy']
_module_init_once_()

# introduce module symbols
from py2cpy import *
import main
