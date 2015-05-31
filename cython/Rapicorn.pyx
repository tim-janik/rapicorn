include "pyxxutils.pyx"
include "rcore.pyx"

cdef extern from "ui/clientapi.hh" namespace "Rapicorn":
  pass	# ensure "ui/clientapi.hh" inclusion

include "idlapi.pyx"

cdef extern from "ui/clientapi.hh" namespace "Rapicorn":
  Rapicorn__Application     Rapicorn__init_app "Rapicorn::init_app" (const String &app_ident, int*, char**)

def init_app (app_ident):
  import sys
  cdef int   argc = 1
  cdef char *argv = sys.argv[0]
  return Rapicorn__Application__wrap (Rapicorn__init_app (app_ident, &argc, &argv))
