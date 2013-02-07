#!/usr/bin/env python
# Rapicorn-CxxPyStub                                           -*-mode:python-*-
# Copyright (C) 2009-2010 Tim Janik
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
"""Rapicorn-CxxPyStub - C++-Python glue generator

More details at http://www.rapicorn.org
"""
import Decls, GenUtils, re

def reindent (prefix, lines):
  return re.compile (r'^', re.M).sub (prefix, lines.rstrip())

base_code = """
#include <Python.h> // must be included first to configure std headers
#include <string>

#include <rapicorn.hh>

#define None_INCREF()   ({ Py_INCREF (Py_None); Py_None; })
#define GOTO_ERROR()    goto error
#define ERRORif(cond)   if (cond) goto error
#define ERRORifpy()     if (PyErr_Occurred()) goto error
#define ERRORpy(msg)    do { PyErr_Format (PyExc_RuntimeError, msg); goto error; } while (0)
#define ERRORifnotret(fr) do { if (AIDA_UNLIKELY (!fr) || \\
                                   AIDA_UNLIKELY (!Rapicorn::Aida::msgid_is_result (Rapicorn::Aida::MessageId (fr->first_id())))) { \\
                                 PyErr_Format_from_AIDA_error (fr); \\
                                 goto error; } } while (0)
#ifndef __AIDA_PYMODULE__OBJECT
#define __AIDA_PYMODULE__OBJECT ((PyObject*) NULL)
#endif

// using Rapicorn::Aida::uint64_t;
using ::uint64_t;
using Rapicorn::Aida::FieldBuffer;
using Rapicorn::Aida::FieldReader;

static Rapicorn::Aida::ClientConnection *__AIDA_local__client_connection = NULL;
static Rapicorn::Init __AIDA_init__client_connection ([]() {
  __AIDA_local__client_connection = Rapicorn::Aida::ObjectBroker::new_client_connection();
});

static PyObject*
PyErr_Format_from_AIDA_error (const FieldBuffer *fr)
{
  if (!fr)
    return PyErr_Format (PyExc_RuntimeError, "Aida: missing return value");
  FieldReader frr (*fr);
  const uint64_t msgid = frr.pop_int64();
  frr.skip(); frr.skip(); // hashhigh hashlow
  if (Rapicorn::Aida::msgid_is_error (Rapicorn::Aida::MessageId (msgid)))
    {
      std::string msg = frr.pop_string(), domain = frr.pop_string();
      if (domain.size()) domain += ": ";
      msg = domain + msg;
      return PyErr_Format (PyExc_RuntimeError, "%s", msg.c_str());
    }
  return PyErr_Format (PyExc_RuntimeError, "Aida: garbage return: 0x%s", fr->first_id_str().c_str());
}

static inline PY_LONG_LONG
PyIntLong_AsLongLong (PyObject *intlong)
{
  if (PyInt_Check (intlong))
    return PyInt_AS_LONG (intlong);
  else
    return PyLong_AsLongLong (intlong);
}

static inline std::string
PyString_As_std_string (PyObject *pystr)
{
  char *s = NULL;
  Py_ssize_t len = 0;
  PyString_AsStringAndSize (pystr, &s, &len);
  return std::string (s, len);
}

static inline Rapicorn::Aida::uint64_t
PyAttr_As_uint64 (PyObject *pyobj, const char *attr_name)
{
  PyObject *o = PyObject_GetAttrString (pyobj, attr_name);
  if (o)
     return PyLong_AsUnsignedLongLong (o);
  return 0;
}

static inline PyObject*
PyString_From_std_string (const std::string &s)
{
  return PyString_FromStringAndSize (s.data(), s.size());
}

static inline int
PyDict_Take_Item (PyObject *pydict, const char *key, PyObject **pyitemp)
{
  int r = PyDict_SetItemString (pydict, key, *pyitemp);
  if (r >= 0)
    {
      Py_DECREF (*pyitemp);
      *pyitemp = NULL;
    }
  return r;
}

static void
__AIDA_pyconvert__pyany_to_any (Rapicorn::Aida::Any &any, PyObject *pyvalue)
{
  if (pyvalue == Py_None)               any.retype (Rapicorn::Aida::TypeMap::notype());
  else if (PyString_Check (pyvalue))    any <<= PyString_As_std_string (pyvalue);
  else if (PyBool_Check (pyvalue))      any <<= bool (pyvalue == Py_True);
  else if (PyInt_Check (pyvalue))       any <<= PyInt_AS_LONG (pyvalue);
  else if (PyLong_Check (pyvalue))      any <<= PyLong_AsLongLong (pyvalue);
  else if (PyFloat_Check (pyvalue))     any <<= PyFloat_AsDouble (pyvalue);
  else {
    std::string msg =
      Rapicorn::string_printf ("no known conversion to Aida::Any for Python object: %s", PyObject_REPR (pyvalue));
    PyErr_SetString (PyExc_NotImplementedError, msg.c_str());
  }
}

static PyObject*
__AIDA_pyconvert__pyany_from_any (const Rapicorn::Aida::Any &any)
{
  using namespace Rapicorn::Aida;
  switch (any.kind())
    {
    case UNTYPED:                               return None_INCREF();
    case BOOL:          { bool b; any >>= b;    return PyBool_FromLong (b); }
    case ENUM:  // chain
    case INT32: // chain
    case INT64:                                 return PyLong_FromLongLong (any.as_int());
    case FLOAT64:                               return PyFloat_FromDouble (any.as_float());
    case STRING:                                return PyString_From_std_string (any.as_string());
    case SEQUENCE:      break;
    case RECORD:        break;
    case INSTANCE:      break;
    case ANY:           break;
    default:            break;
    }
  std::string msg =
    Rapicorn::string_printf ("no known conversion for Aida::%s to Python", Rapicorn::Aida::type_kind_name (any.kind()));
  PyErr_SetString (PyExc_NotImplementedError, msg.c_str());
  return NULL;
}

static PyObject *__AIDA_pyfactory__global_callback = NULL;

static PyObject*
__AIDA_pyfactory__register_callback (PyObject *pyself, PyObject *pyargs)
{
  if (__AIDA_pyfactory__global_callback)
    return PyErr_Format (PyExc_RuntimeError, "object_factory_callable already registered");
  if (PyTuple_Size (pyargs) != 1)
    return PyErr_Format (PyExc_RuntimeError, "wrong number of arguments");
  PyObject *item = PyTuple_GET_ITEM (pyargs, 0);
  if (!PyCallable_Check (item))
    return PyErr_Format (PyExc_RuntimeError, "argument must be callable");
  Py_INCREF (item);
  __AIDA_pyfactory__global_callback = item;
  return None_INCREF();
}

static inline PyObject*
__AIDA_pyfactory__create_from_orbid (const char *type_name, uint64_t orbid)
{
  if (!__AIDA_pyfactory__global_callback)
    return PyErr_Format (PyExc_RuntimeError, "object_factory_callable not registered");
  PyObject *result = NULL, *pyid = PyLong_FromUnsignedLongLong (orbid);
  if (pyid) {
    PyObject *tuple = PyTuple_New (2);
    if (tuple) {
      PyTuple_SET_ITEM (tuple, 0, PyString_FromString (type_name));
      PyTuple_SET_ITEM (tuple, 1, pyid), pyid = NULL;
      result = PyObject_Call (__AIDA_pyfactory__global_callback, tuple, NULL);
      Py_DECREF (tuple);
    }
    Py_XDECREF (pyid);
  }
  return result;
}
"""

class Generator:
  def __init__ (self):
    self.ntab = 26
  def tabwidth (self, n):
    self.ntab = n
  def format_to_tab (self, string, indent = ''):
    if len (string) >= self.ntab:
      return indent + string + ' '
    else:
      f = '%%-%ds' % self.ntab  # '%-20s'
      return indent + f % string
  def generate_enum_impl (self, type_info):
    s = ''
    l = []
    s += 'enum %s {\n' % type_info.name
    for opt in type_info.options:
      (ident, label, blurb, number) = opt
      s += '  %s = %s,' % (ident, number)
      if blurb:
        s += ' // %s' % re.sub ('\n', ' ', blurb)
      s += '\n'
    s += '};'
    return s
  def generate_proto_add_py (self, fb, type, var):
    s = ''
    if type.storage == Decls.BOOL:
      s += '  %s.add_bool (PyIntLong_AsLongLong (%s)); ERRORifpy();\n' % (fb, var)
    elif type.storage == Decls.INT32:
      s += '  %s.add_int64 (PyIntLong_AsLongLong (%s)); ERRORifpy();\n' % (fb, var)
    elif type.storage == Decls.INT64:
      s += '  %s.add_int64 (PyIntLong_AsLongLong (%s)); ERRORifpy();\n' % (fb, var)
    elif type.storage == Decls.FLOAT64:
      s += '  %s.add_double (PyFloat_AsDouble (%s)); ERRORifpy();\n' % (fb, var)
    elif type.storage == Decls.ENUM:
      s += '  %s.add_evalue (PyIntLong_AsLongLong (%s)); ERRORifpy();\n' % (fb, var)
    elif type.storage == Decls.STRING:
      s += '  %s.add_string (PyString_As_std_string (%s)); ERRORifpy();\n' % (fb, var)
    elif type.storage in (Decls.RECORD, Decls.SEQUENCE):
      s += '  if (!aida_py%s_proto_add (%s, %s)) goto error;\n' % (type.name, var, fb)
    elif type.storage == Decls.INTERFACE:
      s += '  %s.add_object (PyAttr_As_uint64 (%s, "__AIDA_pyobject__")); ERRORifpy();\n' % (fb, var)
    elif type.storage == Decls.ANY:
      s += '  { Rapicorn::Aida::Any tmpany;\n'
      s += '    __AIDA_pyconvert__pyany_to_any (tmpany, %s); ERRORifpy();\n' % var
      s += '    %s.add_any (tmpany); }\n' % fb
    else: # FUNC VOID
      raise RuntimeError ("marshalling not implemented: " + type.storage)
    return s
  def generate_proto_pop_py (self, fbr, type, var):
    s = ''
    if type.storage == Decls.BOOL:
      s += '  %s = PyLong_FromLongLong (%s.pop_bool()); ERRORifpy ();\n' % (var, fbr)
    elif type.storage == Decls.INT32:
      s += '  %s = PyLong_FromLongLong (%s.pop_int64()); ERRORifpy ();\n' % (var, fbr)
    elif type.storage == Decls.INT64:
      s += '  %s = PyLong_FromLongLong (%s.pop_int64()); ERRORifpy ();\n' % (var, fbr)
    elif type.storage == Decls.FLOAT64:
      s += '  %s = PyFloat_FromDouble (%s.pop_double()); ERRORifpy();\n' % (var, fbr)
    elif type.storage == Decls.ENUM:
      s += '  %s = PyLong_FromLongLong (%s.pop_evalue()); ERRORifpy();\n' % (var, fbr)
    elif type.storage == Decls.STRING:
      s += '  %s = PyString_From_std_string (%s.pop_string()); ERRORifpy();\n' % (var, fbr)
    elif type.storage in (Decls.RECORD, Decls.SEQUENCE):
      s += '  %s = aida_py%s_proto_pop (%s); ERRORif (!%s);\n' % (var, type.name, fbr, var)
    elif type.storage == Decls.INTERFACE:
      s += '  %s = __AIDA_pyfactory__create_from_orbid ("%s", %s.pop_object()); ERRORifpy();\n' % (var, type.name, fbr)
    elif type.storage == Decls.ANY:
      s += '  %s = __AIDA_pyconvert__pyany_from_any (%s.pop_any()); ERRORifpy();\n' % (var, fbr)
    else: # FUNC VOID
      raise RuntimeError ("marshalling not implemented: " + type.storage)
    return s
  def generate_record_impl (self, type_info):
    s = ''
    # record proto add
    s += 'static RAPICORN_UNUSED bool\n'
    s += 'aida_py%s_proto_add (PyObject *pyrec, Rapicorn::Aida::FieldBuffer &dst)\n' % type_info.name
    s += '{\n'
    s += '  Rapicorn::Aida::FieldBuffer &fb = dst.add_rec (%u);\n' % len (type_info.fields)
    s += '  bool success = false;\n'
    s += '  PyObject *dictR = NULL, *item = NULL;\n'
    s += '  dictR = PyObject_GetAttrString (pyrec, "__dict__"); ERRORif (!dictR);\n'
    for fl in type_info.fields:
      s += '  item = PyDict_GetItemString (dictR, "%s"); ERRORif (!item);\n' % (fl[0])
      s += self.generate_proto_add_py ('fb', fl[1], 'item')
    s += '  success = true;\n'
    s += ' error:\n'
    s += '  Py_XDECREF (dictR);\n'
    s += '  return success;\n'
    s += '}\n'
    # record proto pop
    s += 'static RAPICORN_UNUSED PyObject*\n'
    s += 'aida_py%s_proto_pop (Rapicorn::Aida::FieldReader &src)\n' % type_info.name
    s += '{\n'
    s += '  PyObject *pytypeR = NULL, *pyinstR = NULL, *dictR = NULL, *pyfoR = NULL, *pyret = NULL;\n'
    s += '  Rapicorn::Aida::FieldReader fbr (src.pop_rec());\n'
    s += '  if (fbr.remaining() != %u) ERRORpy ("Aida: marshalling error: invalid record length");\n' % len (type_info.fields)
    s += '  pytypeR = PyObject_GetAttrString (__AIDA_PYMODULE__OBJECT, "__AIDA_BaseRecord__"); AIDA_ASSERT (pytypeR != NULL);\n'
    s += '  pyinstR = PyObject_CallObject (pytypeR, NULL); ERRORif (!pyinstR);\n'
    s += '  dictR = PyObject_GetAttrString (pyinstR, "__dict__"); ERRORif (!dictR);\n'
    for fl in type_info.fields:
      s += self.generate_proto_pop_py ('fbr', fl[1], 'pyfoR')
      s += '  if (PyDict_Take_Item (dictR, "%s", &pyfoR) < 0) goto error;\n' % fl[0]
    s += '  pyret = pyinstR;\n'
    s += ' error:\n'
    s += '  Py_XDECREF (pytypeR);\n'
    s += '  Py_XDECREF (pyfoR);\n'
    s += '  Py_XDECREF (dictR);\n'
    s += '  if (pyret != pyinstR)\n'
    s += '    Py_XDECREF (pyinstR);\n'
    s += '  return pyret;\n'
    s += '}\n'
    return s
  def generate_sequence_impl (self, type_info):
    s = ''
    el = type_info.elements
    # sequence proto add
    s += 'static RAPICORN_UNUSED bool\n'
    s += 'aida_py%s_proto_add (PyObject *pyinput, Rapicorn::Aida::FieldBuffer &dst)\n' % type_info.name
    s += '{\n'
    s += '  PyObject *pyseq = PySequence_Fast (pyinput, "expected a sequence"); if (!pyseq) return false;\n'
    s += '  const ssize_t len = PySequence_Fast_GET_SIZE (pyseq); if (len < 0) return false;\n'
    s += '  Rapicorn::Aida::FieldBuffer &fb = dst.add_seq (len);\n'
    s += '  bool success = false;\n'
    s += '  for (ssize_t k = 0; k < len; k++) {\n'
    s += '    PyObject *item = PySequence_Fast_GET_ITEM (pyseq, k);\n'
    s += reindent ('  ', self.generate_proto_add_py ('fb', el[1], 'item')) + '\n'
    s += '  }\n'
    s += '  success = true;\n'
    s += ' error:\n'
    s += '  return success;\n'
    s += '}\n'
    # sequence proto pop
    s += 'static RAPICORN_UNUSED PyObject*\n'
    s += 'aida_py%s_proto_pop (Rapicorn::Aida::FieldReader &src)\n' % type_info.name
    s += '{\n'
    s += '  PyObject *listR = NULL, *pyfoR = NULL, *pyret = NULL;\n'
    s += '  Rapicorn::Aida::FieldReader fbr (src.pop_seq());\n'
    s += '  const size_t len = fbr.remaining();\n'
    s += '  listR = PyList_New (len); if (!listR) GOTO_ERROR();\n'
    s += '  for (size_t k = 0; k < len; k++) {\n'
    s += reindent ('  ', self.generate_proto_pop_py ('fbr', el[1], 'pyfoR')) + '\n'
    s += '    PyList_SET_ITEM (listR, k, pyfoR), pyfoR = NULL;\n'
    s += '  }\n'
    s += '  pyret = listR;\n'
    s += ' error:\n'
    s += '  Py_XDECREF (pyfoR);\n'
    s += '  if (pyret != listR)\n'
    s += '    Py_XDECREF (listR);\n'
    s += '  return pyret;\n'
    s += '}\n'
    return s
  def generate_client_signal_def (self, class_info, functype, mdefs):
    mdefs += [ '{ "_AIDA_pymarshal__%s", __AIDA_pymarshal__%s, METH_VARARGS, "pyRapicorn signal call" }' %
               (functype.ident_digest(), functype.ident_digest()) ]
    s, cbtname, classN = '', 'Callback' + '_' + functype.name, class_info.name
    u64 = 'Rapicorn::Aida::uint64_t'
    emitfunc = '__AIDA_pyemit__%s__%s' % (classN, functype.name)
    s += 'static Rapicorn::Aida::FieldBuffer*\n%s ' % emitfunc
    s += '(const Rapicorn::Aida::FieldBuffer *sfb, void *data)\n{\n'
    s += '  PyObject *callable = (PyObject*) data;\n'
    s += '  if (AIDA_UNLIKELY (!sfb)) { Py_DECREF (callable); return NULL; }\n'
    s += '  const uint length = %u;\n' % len (functype.args)
    s += '  PyObject *result = NULL, *tuple = PyTuple_New (length)%s;\n' % (', *item' if functype.args else '')
    if functype.args:
      s += '  FieldReader fbr (*sfb);\n'
      s += '  fbr.skip_header();\n'
      s += '  fbr.skip();  // skip handler_id\n'
      arg_counter = 0
      for a in functype.args:
        s += self.generate_proto_pop_py ('fbr', a[1], 'item')
        s += '  PyTuple_SET_ITEM (tuple, %u, item);\n' % arg_counter
        arg_counter += 1
    s += '  if (PyErr_Occurred()) goto error;\n'
    s += '  result = PyObject_Call (callable, tuple, NULL);\n'
    s += '  Py_XDECREF (result);\n'
    s += ' error:\n'
    s += '  Py_XDECREF (tuple);\n'
    s += '  return NULL;\n'
    s += '}\n'
    s += 'static PyObject*\n'
    s += '__AIDA_pymarshal__%s (PyObject *pyself, PyObject *pyargs)\n' % functype.ident_digest()
    s += '{\n'
    s += '  while (0) { error: return NULL; }\n'
    s += '  if (PyTuple_Size (pyargs) != 1 + 2) ERRORpy ("wrong number of arguments");\n'
    s += '  PyObject *item = PyTuple_GET_ITEM (pyargs, 0);  // self\n'
    s += '  Rapicorn::Aida::uint64_t oid = PyAttr_As_uint64 (item, "__AIDA_pyobject__"); ERRORifpy();\n'
    s += '  PyObject *callable = PyTuple_GET_ITEM (pyargs, 1);  // Closure\n'
    s += '  %s result = 0;\n' % u64
    s += '  if (callable == Py_None) {\n'
    s += '    PyObject *pyo = PyTuple_GET_ITEM (pyargs, 2);\n' # connection id for disconnect
    s += '    %s dc_id = PyIntLong_AsLongLong (pyo); ERRORifpy();\n' % u64
    s += '    result = __AIDA_local__client_connection->signal_disconnect (dc_id);\n'
    s += '  } else {\n'
    s += '    if (!PyCallable_Check (callable)) ERRORpy ("arg2 must be callable");\n'
    s += '    Py_INCREF (callable);\n'
    s += '    result = __AIDA_local__client_connection->signal_connect (%s, oid, %s, callable);\n' % (self.method_digest (functype), emitfunc)
    s += '  }\n'
    s += '  PyObject *pyres = PyLong_FromLongLong (result); ERRORifpy ();\n'
    s += '  return pyres;\n'
    s += '}\n'
    return s
  def method_digest (self, mtype):
    digest = mtype.type_hash()
    return '0x%02x%02x%02x%02x%02x%02x%02x%02xULL, 0x%02x%02x%02x%02x%02x%02x%02x%02xULL' % digest
  def generate_pycall_wrapper (self, class_info, mtype, mdefs):
    s = ''
    mdefs += [ '{ "_AIDA_pycall__%s", __AIDA_pycall__%s, METH_VARARGS, "pyRapicorn call" }' %
               (mtype.ident_digest(), mtype.ident_digest()) ]
    hasret = mtype.rtype.storage != Decls.VOID
    s += 'static PyObject*\n'
    s += '__AIDA_pycall__%s (PyObject *pyself, PyObject *pyargs)\n' % mtype.ident_digest()
    s += '{\n'
    s += '  uint64_t item_orbid;\n'
    s += '  PyObject *item%s;\n' % (', *pyfoR = NULL' if hasret else '')
    s += '  FieldBuffer *fm = FieldBuffer::_new (3 + 1 + %u), &fb = *fm, *fr = NULL;\n' % len (mtype.args) # header + self + args
    s += '  if (PyTuple_Size (pyargs) != 1 + %u) ERRORpy ("Aida: wrong number of arguments");\n' % len (mtype.args) # self args
    arg_counter = 0
    s += '  item = PyTuple_GET_ITEM (pyargs, %d);  // self\n' % arg_counter
    s += '  item_orbid = PyAttr_As_uint64 (item, "__AIDA_pyobject__"); ERRORifpy();\n'
    if hasret:
      s += '  fb.add_header2 (Rapicorn::Aida::MSGID_TWOWAY, Rapicorn::Aida::ObjectBroker::connection_id_from_orbid (item_orbid),'
      s += ' __AIDA_local__client_connection->connection_id(), %s);\n' % self.method_digest (mtype)
    else:
      s += '  fb.add_header1 (Rapicorn::Aida::MSGID_ONEWAY,'
      s += ' Rapicorn::Aida::ObjectBroker::connection_id_from_orbid (item_orbid), %s);\n' % self.method_digest (mtype)
    s += '  fb.add_object (item_orbid);\n'
    arg_counter += 1
    for ma in mtype.args:
      s += '  item = PyTuple_GET_ITEM (pyargs, %d); // %s\n' % (arg_counter, ma[0])
      s += self.generate_proto_add_py ('fb', ma[1], 'item')
      arg_counter += 1
    # call out
    s += '  fm = NULL; fr = __AIDA_local__client_connection->call_remote (&fb); // deletes fb\n'
    if mtype.rtype.storage == Decls.VOID:
      s += '  if (fr) { delete fr; fr = NULL; }\n'
      s += '  return None_INCREF();\n'
    else:
      s += '  ERRORifnotret (fr);\n'
      s += '  if (fr) {\n'
      s += '    Rapicorn::Aida::FieldReader frr (*fr);\n'
      s += '    frr.skip_header();\n'
      s += '    if (frr.remaining() == 1) {\n'
      s += reindent ('      ', self.generate_proto_pop_py ('frr', mtype.rtype, 'pyfoR')) + '\n'
      s += '    }\n'
      s += '    delete fr; fr = NULL;\n'
      s += '  }\n'
      s += '  return pyfoR;\n'
    s += ' error:\n'
    s += '  if (fr) delete fr;\n'
    s += '  if (fm) delete fm;\n'
    s += '  return NULL;\n'
    s += '}\n'
    return s
  def type2cpp (self, typename):
    if typename == 'float': return 'double'
    if typename == 'string': return 'std::string'
    return typename
  def generate_impl_types (self, implementation_types):
    s = '/* --- Generated by Rapicorn-CxxPyStub --- */\n'
    s += base_code + '\n'
    self.tabwidth (16)
    # collect impl types
    types = []
    for tp in implementation_types:
      if tp.isimpl:
        types += [ tp ]
    # generate prototypes
    for tp in types:
      if tp.typedef_origin or tp.is_forward:
        pass # s += 'typedef %s %s;\n' % (self.type2cpp (tp.typedef_origin.name), tp.name)
      elif tp.storage == Decls.RECORD:
        s += 'struct %s;\n' % tp.name
      elif tp.storage == Decls.ENUM:
        s += self.generate_enum_impl (tp) + '\n'
    # generate types
    for tp in types:
      if tp.typedef_origin or tp.is_forward:
        pass
      elif tp.storage == Decls.RECORD:
        s += self.generate_record_impl (tp) + '\n'
      elif tp.storage == Decls.SEQUENCE:
        s += self.generate_sequence_impl (tp) + '\n'
      elif tp.storage == Decls.INTERFACE:
        s += ''
    # generate accessors
    mdefs = []
    for tp in types:
      if tp.typedef_origin or tp.is_forward:
        pass
      elif tp.storage == Decls.RECORD:
        pass
      elif tp.storage == Decls.SEQUENCE:
        pass
      elif tp.storage == Decls.INTERFACE:
        pass
        for m in tp.methods:
          s += self.generate_pycall_wrapper (tp, m, mdefs)
        for sg in tp.signals:
          s += self.generate_client_signal_def (tp, sg, mdefs)
    # method def array
    if mdefs:
      aux = '{ "__AIDA_pyfactory__register_callback", __AIDA_pyfactory__register_callback, METH_VARARGS, "Register Python object factory callable" }'
      s += '#define AIDA_PYSTUB_METHOD_DEFS() \\\n  ' + ',\\\n  '.join ([aux] + mdefs) + '\n'
    return s

def error (msg):
  import sys
  print >>sys.stderr, sys.argv[0] + ":", msg
  sys.exit (127)

def generate (namespace_list, **args):
  import sys, tempfile, os
  config = {}
  config.update (args)
  gg = Generator()
  textstring = gg.generate_impl_types (config['implementation_types'])
  outname = config.get ('output', '-')
  if outname != '-':
    fout = open (outname, 'w')
    fout.write (textstring)
    fout.close()
  else:
    print textstring,

# register extension hooks
__Aida__.add_backend (__file__, generate, __doc__)
