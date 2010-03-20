/* Rapicorn-Python Bindings
 * Copyright (C) 2008 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#ifndef __RAPICORN_PYROPE_HH__
#define __RAPICORN_PYROPE_HH__

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


#endif /* __RAPICORN_PYROPE_HH__ */
