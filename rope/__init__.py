# Rapicorn                              -*- Mode: python; -*-
# Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
"""Rapicorn - experimental UI toolkit

More details at http://www.rapicorn.org/.
"""

from Aida1208 import loop as _loop      # Aida helper module
import __pyrapicorn                     # Generated C++ methods
from pyrapicorn import *                # Generated Python API

class MainApplication (Application):
  def __init__ (self, _aida_id):
    super (MainApplication, self).__init__ (_aida_id)
    self.__dict__['__cached_exitable'] = False
    self.sig_missing_primary += lambda: app.__dict__.__setitem__ ('__cached_exitable', True)
  def iterate (self, may_block, may_dispatch):
    delete_loop = None
    try:
      event_loop = self.__dict__['__aida_event_loop__']
    except KeyError:
      event_loop = _loop.Loop()
      event_loop += _loop.RapicornSource()
      delete_loop = event_loop
    needs_dispatch = event_loop.iterate (may_block, may_dispatch)
    event_loop = None
    del delete_loop
    return needs_dispatch
  def loop (self):
    assert not self.__dict__.has_key ('__aida_event_loop__')
    event_loop = _loop.Loop()
    event_loop += _loop.RapicornSource()
    self.__dict__['__aida_event_loop__'] = event_loop
    while event_loop.quit_status == None:
      if not event_loop.iterate (False, True):                  # complete events already queued
        break
    self.__dict__['__cached_exitable'] = app.finishable()       # run while not exitable
    while not self.__dict__['__cached_exitable'] and event_loop.quit_status == None:
      self.iterate (True, True)                                 # also processes signals
    del self.__dict__['__aida_event_loop__']
    exit_status = event_loop.quit_status
    return exit_status

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
  app = MainApplication (_PY._BaseClass_._AidaID_ (aida_id))
  _loop.app = app # integrate event loop with app FIXME
  return app

def _module_init_once_():
  global _module_init_once_ ; del _module_init_once_
  import pyrapicorn     # generated Python classes
  __pyrapicorn.__AIDA_BaseRecord__ = pyrapicorn._BaseRecord_ # FIXME
  pyrapicorn._CPY = __pyrapicorn # FIXME
  app_init._CPY, app_init._PY = (__pyrapicorn, pyrapicorn) # app_init() internals
  del globals()['pyrapicorn']
_module_init_once_()

# introduce module symbols
del globals()['__pyrapicorn']
