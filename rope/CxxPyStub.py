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
"""Rapicorn-CxxPyStub - C-Python RPC glue generator

More details at http://www.rapicorn.org
"""
import Decls, GenUtils, re

FieldBuffer = 'Plic::FieldBuffer'

def reindent (prefix, lines):
  return re.compile (r'^', re.M).sub (prefix, lines.rstrip())

base_code = """
#include <Python.h> // must be included first to configure std headers
#include <string>

#include <rapicorn-core.hh>

#define None_INCREF()   ({ Py_INCREF (Py_None); Py_None; })
#define GOTO_ERROR()    goto error
#define ERRORif(cond)   if (cond) goto error
#define ERRORifpy()     if (PyErr_Occurred()) goto error
#define ERRORpy(msg)    do { PyErr_Format (PyExc_RuntimeError, msg); goto error; } while (0)
#define ERRORifnotret(fr) do { if (PLIC_UNLIKELY (!fr) || \\
                                   PLIC_UNLIKELY (!Plic::is_msgid_result (fr->first_id()))) { \\
                                 PyErr_Format_from_PLIC_error (fr); \\
                                 goto error; } } while (0)

using Plic::uint64;
using Plic::FieldBuffer;
using Plic::FieldBufferReader;

static PyObject*
PyErr_Format_from_PLIC_error (const FieldBuffer *fr)
{
  if (!fr)
    return PyErr_Format (PyExc_RuntimeError, "PLIC: missing return value");
  if (Plic::msgid_has_error (fr->first_id()))
    {
      FieldBufferReader frr (*fr);
      std::string msg, domain;
      if (Plic::is_msgid_error (frr.pop_int64()))
        msg = frr.pop_string(), domain = frr.pop_string();
      else
        msg = "unknown message: 0x" + fr->first_id_str();
      if (domain.size()) domain += ": ";
      msg = domain + msg;
      return PyErr_Format (PyExc_RuntimeError, "%s", msg.c_str());
    }
  return PyErr_Format (PyExc_RuntimeError, "PLIC: garbage return: 0x%s", fr->first_id_str().c_str());
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

static inline Plic::uint64
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

static PyObject *_plic_object_factory_callable = NULL;

static PyObject*
_plic___register_object_factory_callable (PyObject *pyself, PyObject *pyargs)
{
  if (_plic_object_factory_callable)
    return PyErr_Format (PyExc_RuntimeError, "object_factory_callable already registered");
  if (PyTuple_Size (pyargs) != 1)
    return PyErr_Format (PyExc_RuntimeError, "wrong number of arguments");
  PyObject *item = PyTuple_GET_ITEM (pyargs, 0);
  if (!PyCallable_Check (item))
    return PyErr_Format (PyExc_RuntimeError, "argument must be callable");
  Py_INCREF (item);
  _plic_object_factory_callable = item;
  return None_INCREF();
}

static inline PyObject*
plic_PyObject_4uint64 (const char *type_name, uint64 rpc_id)
{
  if (!_plic_object_factory_callable)
    return PyErr_Format (PyExc_RuntimeError, "object_factory_callable not registered");
  PyObject *result = NULL, *pyid = PyLong_FromUnsignedLongLong (rpc_id);
  if (pyid) {
    PyObject *tuple = PyTuple_New (2);
    if (tuple) {
      PyTuple_SET_ITEM (tuple, 0, PyString_FromString (type_name));
      PyTuple_SET_ITEM (tuple, 1, pyid), pyid = NULL;
      result = PyObject_Call (_plic_object_factory_callable, tuple, NULL);
      Py_DECREF (tuple);
    }
    Py_XDECREF (pyid);
  }
  return result;
}

#ifndef PLIC_COUPLER
#define PLIC_COUPLER()  _plic_coupler_static
static struct _UnimplementedCoupler : public Plic::Coupler {
  virtual FieldBuffer* call_remote (FieldBuffer *fbcall) { return NULL; }
} _plic_coupler_static;
#endif
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
    if type.storage == Decls.INT:
      s += '  %s.add_int64 (PyIntLong_AsLongLong (%s)); ERRORifpy();\n' % (fb, var)
    elif type.storage == Decls.ENUM:
      s += '  %s.add_evalue (PyIntLong_AsLongLong (%s)); ERRORifpy();\n' % (fb, var)
    elif type.storage == Decls.FLOAT:
      s += '  %s.add_double (PyFloat_AsDouble (%s)); ERRORifpy();\n' % (fb, var)
    elif type.storage == Decls.STRING:
      s += '  %s.add_string (PyString_As_std_string (%s)); ERRORifpy();\n' % (fb, var)
    elif type.storage in (Decls.RECORD, Decls.SEQUENCE):
      s += '  if (!plic_py%s_proto_add (%s, %s)) goto error;\n' % (type.name, var, fb)
    elif type.storage == Decls.INTERFACE:
      s += '  %s.add_object (PyAttr_As_uint64 (%s, "__plic__object__")); ERRORifpy();\n' % (fb, var)
    else: # FUNC VOID
      raise RuntimeError ("marshalling not implemented: " + type.storage)
    return s
  def generate_proto_pop_py (self, fbr, type, var):
    s = ''
    if type.storage == Decls.INT:
      s += '  %s = PyLong_FromLongLong (%s.pop_int64()); ERRORifpy ();\n' % (var, fbr)
    elif type.storage == Decls.ENUM:
      s += '  %s = PyLong_FromLongLong (%s.pop_evalue()); ERRORifpy();\n' % (var, fbr)
    elif type.storage == Decls.FLOAT:
      s += '  %s = PyFloat_FromDouble (%s.pop_double()); ERRORifpy();\n' % (var, fbr)
    elif type.storage == Decls.STRING:
      s += '  %s = PyString_From_std_string (%s.pop_string()); ERRORifpy();\n' % (var, fbr)
    elif type.storage in (Decls.RECORD, Decls.SEQUENCE):
      s += '  %s = plic_py%s_proto_pop (%s); ERRORif (!%s);\n' % (var, type.name, fbr, var)
    elif type.storage == Decls.INTERFACE:
      s += '  %s = plic_PyObject_4uint64 ("%s", %s.pop_object()); ERRORifpy();\n' % (var, type.name, fbr)
    else: # FUNC VOID
      raise RuntimeError ("marshalling not implemented: " + type.storage)
    return s
  def generate_record_impl (self, type_info):
    s = ''
    # record proto add
    s += 'static RAPICORN_UNUSED bool\n'
    s += 'plic_py%s_proto_add (PyObject *pyrec, %s &dst)\n' % (type_info.name, FieldBuffer)
    s += '{\n'
    s += '  %s &fb = dst.add_rec (%u);\n' % (FieldBuffer, len (type_info.fields))
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
    s += 'plic_py%s_proto_pop (%sReader &src)\n' % (type_info.name, FieldBuffer)
    s += '{\n'
    s += '  PyObject *pyinstR = NULL, *dictR = NULL, *pyfoR = NULL, *pyret = NULL;\n'
    s += '  ' + FieldBuffer + 'Reader fbr (src.pop_rec());\n'
    s += '  if (fbr.remaining() != %u) ERRORpy ("PLIC: marshalling error: invalid record length");\n' % len (type_info.fields)
    s += '  pyinstR = PyInstance_NewRaw ((PyObject*) &PyBaseObject_Type, NULL); ERRORif (!pyinstR);\n'
    s += '  dictR = PyObject_GetAttrString (pyinstR, "__dict__"); ERRORif (!dictR);\n'
    for fl in type_info.fields:
      s += self.generate_proto_pop_py ('fbr', fl[1], 'pyfoR')
      s += '  if (PyDict_Take_Item (dictR, "%s", &pyfoR) < 0) goto error;\n' % fl[0]
    s += '  pyret = pyinstR;\n'
    s += ' error:\n'
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
    s += 'plic_py%s_proto_add (PyObject *pyinput, %s &dst)\n' % (type_info.name, FieldBuffer)
    s += '{\n'
    s += '  PyObject *pyseq = PySequence_Fast (pyinput, "expected a sequence"); if (!pyseq) return false;\n'
    s += '  const ssize_t len = PySequence_Fast_GET_SIZE (pyseq); if (len < 0) return false;\n'
    s += '  %s &fb = dst.add_seq (len);\n' % FieldBuffer
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
    s += 'plic_py%s_proto_pop (%sReader &src)\n' % (type_info.name, FieldBuffer)
    s += '{\n'
    s += '  PyObject *listR = NULL, *pyfoR = NULL, *pyret = NULL;\n'
    s += '  ' + FieldBuffer + 'Reader fbr (src.pop_seq());\n'
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
  def generate_rpc_signal_call (self, class_info, mtype, mdefs):
    s = ''
    mdefs += [ '{ "_PLIC_%s", _plic_marshal__%s, METH_VARARGS, "pyRapicorn signal call" }' %
               (mtype.ident_digest(), mtype.ident_digest()) ]
    evd_class = '_EventDispatcher_%s' % mtype.ident_digest()
    s += 'class %s : public Plic::EventDispatcher {\n' % evd_class
    s += '  PyObject *m_callable;\n'
    s += 'public:\n'
    s += '  ~%s() { Py_DECREF (m_callable); }\n' % evd_class
    s += '  %s (PyObject *callable) : m_callable ((Py_INCREF (callable), callable)) {}\n' % evd_class
    s += '  virtual FieldBuffer*\n'
    s += '  dispatch_event (Plic::Coupler &cpl)\n'
    s += '  {\n'
    if mtype.args:
      s += '    FieldBufferReader &fbr = cpl.reader;\n'
    s += '    // uint64 msgid = frr.pop_int64();\n'
    s += '    // assert (Plic::is_msgid_event (msgid));\n'
    s += '    // uint handler_id = uint (frr.pop_int64());\n'
    s += '    const uint length = %u;\n' % len (mtype.args)
    s += '    PyObject *result, *tuple = PyTuple_New (length)%s;\n' % (', *item' if mtype.args else '')
    arg_counter = 0
    for a in mtype.args:
      s += '  ' + self.generate_proto_pop_py ('fbr', a[1], 'item')
      s += '    PyTuple_SET_ITEM (tuple, %u, item);\n' % arg_counter
      arg_counter += 1
    s += '    if (PyErr_Occurred()) goto error;\n'
    s += '    result = PyObject_Call (m_callable, tuple, NULL);\n'
    s += '    Py_XDECREF (result);\n'
    s += '   error:\n'
    s += '    Py_XDECREF (tuple);\n'
    s += '    return NULL;\n'
    s += '  }\n'
    s += '};\n'
    s += 'static PyObject*\n'
    s += '_plic_marshal__%s (PyObject *pyself, PyObject *pyargs)\n' % mtype.ident_digest()
    s += '{\n'
    s += '  Plic::Coupler &cpl = PLIC_COUPLER();\n'
    s += '  PyObject *item, *pyfoR = NULL;\n'
    s += '  FieldBuffer *fm = FieldBuffer::_new (4 + 1 + 2), &fb = *fm, *fr = NULL;\n' # msgid self ConId ClosureId
    s += '  fb.add_type_hash (%s); // msgid\n' % self.method_digest (mtype)
    s += '  if (PyTuple_Size (pyargs) != 1 + 2) ERRORpy ("wrong number of arguments");\n'
    s += '  item = PyTuple_GET_ITEM (pyargs, 0);  // self\n'
    s += self.generate_proto_add_py ('fb', class_info, 'item')
    s += '  item = PyTuple_GET_ITEM (pyargs, 1);  // Closure\n'
    s += '  if (item == Py_None) fb.add_int64 (0);\n'
    s += '  else {\n'
    s += '    if (!PyCallable_Check (item)) ERRORpy ("arg2 must be callable");\n'
    s += '    std::auto_ptr<Plic::EventDispatcher> ap (new %s (item));\n' % evd_class
    s += '    uint64 handler_id = cpl.dispatcher_add (ap);\n'
    s += '    fb.add_int64 (handler_id); }\n'
    s += '  item = PyTuple_GET_ITEM (pyargs, 2);  // ConId for disconnect\n'
    s += '  fb.add_int64 (PyIntLong_AsLongLong (item)); ERRORifpy();\n'
    s += '  fm = NULL; fr = cpl.call_remote (&fb); // deletes fb\n'
    s += '  ERRORifnotret (fr);\n'
    s += '  if (fr) {\n'
    s += '    FieldBufferReader frr (*fr);\n'
    s += '    frr.skip(); // msgid for return\n' # FIXME: check errors
    s += '    if (frr.remaining() == 1) {\n'
    s += '      pyfoR = PyLong_FromLongLong (frr.pop_int64()); ERRORifpy ();\n'
    s += '    }\n'
    s += '  }\n'
    s += ' error:\n'
    s += '  if (fm) delete fm;\n'
    s += '  if (fr) delete fr;\n'
    s += '  return pyfoR;\n'
    s += '}\n'
    return s
  def method_digest (self, mtype):
    d = mtype.type_hash()
    return ('0x%02x%02x%02x%02x%02x%02x%02x%02xULL, 0x%02x%02x%02x%02x%02x%02x%02x%02xULL, ' +
            '0x%02x%02x%02x%02x%02x%02x%02x%02xULL, 0x%02x%02x%02x%02x%02x%02x%02x%02xULL') % d
  def generate_rpc_call_wrapper (self, class_info, mtype, mdefs):
    s = ''
    mdefs += [ '{ "_PLIC_%s", _plic_rpc_%s, METH_VARARGS, "pyRapicorn rpc call" }' %
               (mtype.ident_digest(), mtype.ident_digest()) ]
    hasret = mtype.rtype.storage != Decls.VOID
    s += 'static PyObject*\n'
    s += '_plic_rpc_%s (PyObject *pyself, PyObject *pyargs)\n' % mtype.ident_digest()
    s += '{\n'
    s += '  PyObject *item%s;\n' % (', *pyfoR = NULL' if hasret else '')
    s += '  FieldBuffer *fm = FieldBuffer::_new (4 + 1 + %u), &fb = *fm, *fr = NULL;\n' % len (mtype.args) # msgid self
    s += '  fb.add_type_hash (%s); // msgid\n' % self.method_digest (mtype)
    s += '  if (PyTuple_Size (pyargs) != 1 + %u) ERRORpy ("PLIC: wrong number of arguments");\n' % len (mtype.args) # msgid self
    arg_counter = 0
    s += '  item = PyTuple_GET_ITEM (pyargs, %d);  // self\n' % arg_counter
    s += self.generate_proto_add_py ('fb', class_info, 'item')
    arg_counter += 1
    for ma in mtype.args:
      s += '  item = PyTuple_GET_ITEM (pyargs, %d); // %s\n' % (arg_counter, ma[0])
      s += self.generate_proto_add_py ('fb', ma[1], 'item')
      arg_counter += 1
    # call out
    s += '  fm = NULL; fr = PLIC_COUPLER().call_remote (&fb); // deletes fb\n'
    if mtype.rtype.storage == Decls.VOID:
      s += '  if (fr) { delete fr; fr = NULL; }\n'
      s += '  return None_INCREF();\n'
    else:
      s += '  ERRORifnotret (fr);\n'
      s += '  if (fr) {\n'
      s += '    ' + FieldBuffer + 'Reader frr (*fr);\n'
      s += '    frr.skip(); // msgid for return\n' # FIXME: check errors
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
      if tp.typedef_origin:
        pass # s += 'typedef %s %s;\n' % (self.type2cpp (tp.typedef_origin.name), tp.name)
      elif tp.storage == Decls.RECORD:
        s += 'struct %s;\n' % tp.name
      elif tp.storage == Decls.ENUM:
        s += self.generate_enum_impl (tp) + '\n'
    # generate types
    for tp in types:
      if tp.typedef_origin:
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
      if tp.typedef_origin:
        pass
      elif tp.storage == Decls.RECORD:
        pass
      elif tp.storage == Decls.SEQUENCE:
        pass
      elif tp.storage == Decls.INTERFACE:
        pass
        for m in tp.methods:
          s += self.generate_rpc_call_wrapper (tp, m, mdefs)
        for sg in tp.signals:
          s += self.generate_rpc_signal_call (tp, sg, mdefs)
    # method def array
    if mdefs:
      aux = '{ "_PLIC___register_object_factory_callable", _plic___register_object_factory_callable, METH_VARARGS, "Register Python object factory callable" }'
      s += '#define PLIC_PYSTUB_METHOD_DEFS() \\\n  ' + ',\\\n  '.join ([aux] + mdefs) + '\n'
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

# control module exports
__all__ = ['generate']
