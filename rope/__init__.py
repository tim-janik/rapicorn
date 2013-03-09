# Rapicorn                              -*- Mode: python; -*-
# Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
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
  aida_id = _CPY._init_dispatcher (application_name, cmdline_args)
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
  app = Application (_PY._BaseClass_._AidaID_ (aida_id))
  def iterate (self, may_block, may_dispatch):
    if hasattr (self, "main_loop"):
      loop = self.main_loop
      dloop = None
    else:
      dloop = main.Loop()
      loop = dloop
      loop += main.RapicornSource()
    needs_dispatch = loop.iterate (may_block, may_dispatch)
    loop = None
    del dloop
    return needs_dispatch
  app.__class__.iterate = iterate # extend for main loop integration
  def loop (self):
    self.main_loop = main.Loop()
    self.main_loop += main.RapicornSource()
    exit_status = self.main_loop.loop()
    del self.main_loop
    return exit_status
  app.__class__.loop = loop # extend for main loop integration
  main.app = app # integrate main loop with app
  return app

class AidaObjectFactory:
  def __init__ (self, _PY):
    self._PY = _PY
    self.AidaID = _PY._BaseClass_._AidaID_
  def __call__ (self, type_name, orbid):
    klass = getattr (self._PY, type_name)
    if not klass or not orbid:
      return None
    return klass (self.AidaID (orbid))

def _module_init_once_():
  global _module_init_once_ ; del _module_init_once_
  import cxxrapicorn    # generated __AIDA_... cpy methods
  import pyrapicorn     # generated Python classes, Application, etc
  pyrapicorn.__AIDA_pymodule__init_once (cxxrapicorn)
  cxxrapicorn.__AIDA_pyfactory__register_callback (AidaObjectFactory (pyrapicorn))
  del globals()['AidaObjectFactory']
  app_init._CPY, app_init._PY = (cxxrapicorn, pyrapicorn) # app_init() internals
  del globals()['cxxrapicorn']
  del globals()['pyrapicorn']
_module_init_once_()

# introduce module symbols
from pyrapicorn import *
import main
