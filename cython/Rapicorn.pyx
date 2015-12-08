# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0   -*-mode:python;-*-

# Typedefs, shared_ptr, PyxxCaller, etc
include "pyxxutils.pyx"

# Manual rapicorn-core.hh bindings
include "rcore.pyx"

# Include "ui/clientapi.hh" for binding generation
cdef extern from "rapicorn.hh" namespace "Rapicorn":
  pass

# Generated rapicorn.hh bindings (PyxxStub.py)
include "idlapi.pyx"

# Manual binding bits for rapicorn.hh
cdef extern from "ui/clientapi.hh" namespace "Rapicorn":
  Rapicorn__Application     Rapicorn__init_app "Rapicorn::init_app" (const String &application_name, int*, char**)
  void                      Rapicorn__program_argv0_init "Rapicorn::program_argv0_init" (const char *argv0)
  shared_ptr[Rapicorn__MainLoop] Rapicorn_ApplicationH_main_loop "Rapicorn::ApplicationH::main_loop" ()

app = None # cached Rapicorn Application singleton after Rapicorn.init
def init_app (application_name = ""):
  import sys
  def original_Application():
    global Application # this workaround is needed because cython confuses the existing
    return Application # 'Application' class with the new overriding one we're introducing
  class Application (original_Application()):
    def main_loop (self):
      return Rapicorn__MainLoopP__wrap (Rapicorn_ApplicationH_main_loop())
    def run (self):
      """Execute the Application main event loop until it's quit, returns quit code."""
      return self.main_loop().run()
    def quit (self, quit_code = 0):
      """Quit the currently running Application main event loop with quit_code."""
      return self.main_loop().quit (quit_code)
    def execute (self):
      """Execute the Application and terminatie the program with sys.exit (Application.run())."""
      sys.exit (self.run())
  original_Application().__aida_wrapper__ (Application) # extend Application methods
  Rapicorn__program_argv0_init (Py_GetProgramName()); # Rapicorn needs the real argv0
  cdef int   argc = 1
  cdef char *argv = sys.argv[0]
  global app
  app = Rapicorn__Object__wrap (Rapicorn__init_app (application_name, &argc, &argv))
  return app

# Bindable decorator to use Python objects as data_context
def Bindable (klass):
  """Change objects of klass to hook up with a BindableRelay in __init__ and notify the relay from __setattr__."""
  assert app != None
  # hook into __setattr__ to notify the relay
  orig__setattr__ = getattr (klass, '__setattr__', None)
  def __setattr__ (self, name, value):
    if orig__setattr__:
      orig__setattr__ (self, name, value)
    else: # old style class
      self.__dict__[name] = value
    if hasattr (self, '__aida_relay__'):
      self.__aida_relay__.report_notify (name)
  klass.__setattr__ = __setattr__
  # hook into __init__ to setup a relay that dispatches property changes
  orig__init__ = getattr (klass, '__init__')
  def __init__ (self, *args, **kwargs):
    orig__init__ (self, *args, **kwargs)
    def relay_set (path, nonce, value):
      setattr (self, path, value)
      self.__aida_relay__.report_result (nonce, None, "")
    def relay_get (path, nonce):
      v = getattr (self, path, None)
      self.__aida_relay__.report_result (nonce, v, "")
    self.__aida_relay__ = app.create_bindable_relay()
    self.__aida_relay__.sig_relay_get.connect (relay_get)
    self.__aida_relay__.sig_relay_set.connect (relay_set)
  klass.__init__ = __init__
  return klass
