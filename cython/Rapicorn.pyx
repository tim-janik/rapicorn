include "pyxxutils.pyx"
include "rcore.pyx"

cdef extern from "ui/clientapi.hh" namespace "Rapicorn":
  pass	# ensure "ui/clientapi.hh" inclusion

include "idlapi.pyx"
