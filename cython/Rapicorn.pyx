# This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0   -*-mode:python;-*-

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
  Rapicorn__Application     Rapicorn__init_app "Rapicorn::init_app" (const String &app_ident, int*, char**)
  shared_ptr[Rapicorn__MainLoop] Rapicorn_ApplicationH_main_loop "Rapicorn::ApplicationH::main_loop" ()


def init (app_name):
  import sys
  def builtin_application_class():
    global Application # this workaround is needed because cython confuses the existing
    return Application # 'Application' class with the new overriding one we're introducing
  class Application (builtin_application_class()):
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
  builtin_application_class().__aida_wrapper__ (Application)
  cdef int   argc = 1
  cdef char *argv = sys.argv[0]
  # Application = RapicornApplication
  return Rapicorn__Object__wrap (Rapicorn__init_app (app_name, &argc, &argv))
