// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_PY_ROPE_HH__
#define __RAPICORN_PY_ROPE_HH__

#include <Python.h> // must be included first to configure std headers
#include <rapicorn.hh>

void                         py_remote_handle_push    (const Rapicorn::Aida::RemoteHandle &rhandle);
void                         py_remote_handle_pop     (void);
Rapicorn::Aida::RemoteHandle py_remote_handle_extract (PyObject *object);
Rapicorn::Aida::RemoteHandle py_remote_handle_ensure  (PyObject *object); // sets PyErr_Occurred() if null_handle

#endif /* __RAPICORN_PY_ROPE_HH__ */
