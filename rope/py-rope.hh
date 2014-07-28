// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_PY_ROPE_HH__
#define __RAPICORN_PY_ROPE_HH__

#include <Python.h> // must be included first to configure std headers
#include <rapicorn.hh>

PyObject*       py_remote_handle_create (const Rapicorn::Aida::RemoteHandle &rhandle);

#endif /* __RAPICORN_PY_ROPE_HH__ */
