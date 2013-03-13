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
  # integrate Rapicorn dispatching with event loop
  class RapicornSource (Loop.Source):
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
  Loop.RapicornSource = RapicornSource
  # setup global Application
  app = Application (_PY._BaseClass_._AidaID_ (aida_id))
  def iterate (self, may_block, may_dispatch):
    if hasattr (self, "__aida_event_loop__"):
      loop = self.__aida_event_loop__
      dloop = None
    else:
      dloop = Loop.Loop()
      loop = dloop
      loop += Loop.RapicornSource()
    needs_dispatch = loop.iterate (may_block, may_dispatch)
    loop = None
    del dloop
    return needs_dispatch
  app.__class__.iterate = iterate # extend for event loop integration
  def loop (self):
    event_loop = Loop.Loop()
    event_loop += Loop.RapicornSource()
    self.__dict__['__aida_event_loop__'] = event_loop
    exit_status = self.__aida_event_loop__.loop()
    del self.__dict__['__aida_event_loop__']
    return exit_status
  app.__class__.loop = loop # extend for event loop integration
  Loop.app = app # integrate event loop with app
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
  import __pyrapicorn   # generated C++ methods
  import pyrapicorn     # generated Python classes
  __pyrapicorn.__AIDA_BaseRecord__ = pyrapicorn._BaseRecord_ # FIXME
  __pyrapicorn.__AIDA_pyfactory__register_callback (AidaObjectFactory (pyrapicorn))
  pyrapicorn._CPY = __pyrapicorn # FIXME
  del globals()['AidaObjectFactory']
  app_init._CPY, app_init._PY = (__pyrapicorn, pyrapicorn) # app_init() internals
  del globals()['__pyrapicorn']
  del globals()['pyrapicorn']
_module_init_once_()

# introduce module symbols
from pyrapicorn import *
import loop as Loop
