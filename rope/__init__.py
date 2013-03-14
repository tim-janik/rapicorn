# Rapicorn                              -*- Mode: python; -*-
# Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
"""Rapicorn - experimental UI toolkit

More details at http://www.rapicorn.org/.
"""

from Aida1208 import loop as _loop

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
  class RapicornSource (_loop.Source):
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
  _loop.RapicornSource = RapicornSource
  # setup global Application
  app = Application (_PY._BaseClass_._AidaID_ (aida_id))
  app.__dict__['cached_primary'] = True
  app.__dict__['primary_sig'] = 0
  def iterate (self, may_block, may_dispatch):
    if hasattr (self, "__aida_event_loop__"):
      loop = self.__aida_event_loop__
      dloop = None
    else:
      dloop = _loop.Loop()
      loop = dloop
      loop += _loop.RapicornSource()
    needs_dispatch = loop.iterate (may_block, may_dispatch)
    loop = None
    del dloop
    return needs_dispatch
  app.__class__.iterate = iterate # extend for event loop integration
  def loop (self):
    event_loop = _loop.Loop()
    event_loop += _loop.RapicornSource()
    if not self.__dict__['primary_sig']:
      self.__dict__['primary_sig'] = app.sig_missing_primary.connect (lambda: setattr (app.__dict__, 'cached_primary', 0))
    self.__dict__['__aida_event_loop__'] = event_loop
    while event_loop.quit_status == None:
      if not event_loop.iterate (False, True):  # handle queued events
        break
    self.__dict__['cached_primary'] = not app.finishable()  # run while not exitable
    while self.__dict__['cached_primary'] and event_loop.quit_status == None:
      self.iterate (True, True)
    del self.__dict__['__aida_event_loop__']
    exit_status = event_loop.quit_status
    return exit_status
  app.__class__.loop = loop # extend for event loop integration
  _loop.app = app # integrate event loop with app
  return app

def _module_init_once_():
  global _module_init_once_ ; del _module_init_once_
  import __pyrapicorn   # generated C++ methods
  import pyrapicorn     # generated Python classes
  __pyrapicorn.__AIDA_BaseRecord__ = pyrapicorn._BaseRecord_ # FIXME
  pyrapicorn._CPY = __pyrapicorn # FIXME
  app_init._CPY, app_init._PY = (__pyrapicorn, pyrapicorn) # app_init() internals
  del globals()['__pyrapicorn']
  del globals()['pyrapicorn']
_module_init_once_()

# introduce module symbols
from pyrapicorn import *
