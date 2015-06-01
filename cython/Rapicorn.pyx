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

def init_app (app_ident):
  import sys
  cdef int   argc = 1
  cdef char *argv = sys.argv[0]
  return Rapicorn__Application__wrap (Rapicorn__init_app (app_ident, &argc, &argv))
