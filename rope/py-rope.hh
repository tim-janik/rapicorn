// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_PY_ROPE_HH__
#define __RAPICORN_PY_ROPE_HH__

#include <Python.h> // must be included first to configure std headers

#include <rapicorn.hh>
using namespace Rapicorn;

// convenience casts
#define PYCF(func)      ((PyCFunction) func)
#define PYTO(ooo)       ({ union { PyTypeObject *t; PyObject *o; } u; u.t = (ooo); u.o; })
#define PYWO(ooo)       ({ union { PyWindow *w; PyObject *o; } u; u.w = (ooo); u.o; })
#define PYS(cchr)       const_cast<char*> (cchr)


// convenience functions
#define None_INCREF()   ({ Py_INCREF (Py_None); Py_None; })


#endif /* __RAPICORN_PY_ROPE_HH__ */
