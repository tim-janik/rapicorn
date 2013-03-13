# Licensed GNU GPLv3 or later: http://www.gnu.org/licenses/gpl.html
"""AidaPyStub - Aida Python Code Generator

More details at http://www.rapicorn.org/
"""
import Decls, GenUtils, TmplFiles, re

def reindent (prefix, lines):
  return re.compile (r'^', re.M).sub (prefix, lines.rstrip())
def strcquote (string):
  result = ''
  for c in string:
    oc = ord (c)
    ec = { 92 : r'\\',
            7 : r'\a',
            8 : r'\b',
            9 : r'\t',
           10 : r'\n',
           11 : r'\v',
           12 : r'\f',
           13 : r'\r',
           12 : r'\f'
         }.get (oc, '')
    if ec:
      result += ec
      continue
    if oc <= 31 or oc >= 127:
      result += '\\' + oct (oc)[-3:]
    elif c == '"':
      result += r'\"'
    else:
      result += c
  return '"' + result + '"'

# exception class:
# const char *exclass = PyExceptionClass_Check (t) ? PyExceptionClass_Name (t) : "<unknown>";
# exclass = strrchr (exclass, '.') ? strrchr (exclass, '.') + 1 : exclass;

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
  def format_arg (self, ident, type):
    s = ''
    s += type.name + ' ' + ident
    return s
  def zero_value_pyimpl (self, type):
    return { Decls.BOOL      : '0',
             Decls.INT32     : '0',
             Decls.INT64     : '0',
             Decls.FLOAT64   : '0',
             Decls.ENUM      : '0',
             Decls.RECORD    : 'None',
             Decls.SEQUENCE  : '()',
             Decls.STRING    : "''",
             Decls.INTERFACE : "None",
             Decls.ANY       : '()',
           }[type.storage]
  def default_value_pyimpl (self, type, vdefault):
    if type.storage in (Decls.BOOL, Decls.INT32, Decls.INT64, Decls.FLOAT64, Decls.ENUM, Decls.STRING):
      return vdefault # number litrals or string
    return self.zero_value_pyimpl (type) # zero is the only default for these types
  def generate_record_pyimpl (self, type_info):
    s = ''
    s += 'class %s (_BaseRecord_):\n' % type_info.name
    s += '  def __init__ (self, **entries):\n'
    s += '    defaults = {'
    for fl in type_info.fields:
      s += " '%s' : %s, " % (fl[0], self.zero_value_pyimpl (fl[1]))
    s += '}\n'
    s += '    self.__dict__.update (defaults)\n'
    s += '    _BaseRecord_.__init__ (self, **entries)\n'
    s += '  @staticmethod\n'
    s += '  def create (args):\n'
    s += '    self = %s()\n' % type_info.name
    s += '    if hasattr (args, "__iter__") and len (args) == %d:\n' % len (type_info.fields)
    n = 0
    for fl in type_info.fields:
      s += '      self.%s = args[%d]\n' % (fl[0], n)
      s += reindent ('      #', self.generate_to_proto_pyimpl ('self.' + fl[0], fl[1], 'args[%d]' % n)) + '\n'
      n += 1
    s += '    elif isinstance (args, dict):\n'
    for fl in type_info.fields:
      s += '      self.%s = args["%s"]\n' % (fl[0], fl[0])
    s += '    else: raise RuntimeError ("invalid or missing record initializers")\n'
    s += '    return self\n'
    s += '  @staticmethod\n'
    s += '  def to_proto (self, _aida_rec):\n' # FIXME: broken, staticmethod with self?
    for a in type_info.fields:
      s += '    _aida_field = _aida_rp.fields.add()\n'
      s += reindent ('  ', self.generate_to_proto_pyimpl ('_aida_field', a[1], 'self.' + a[0])) + '\n'
    return s
  def generate_signal_pyimpl (self, ftype, ctype):
    s = ''
    s += 'def sig_%s_connect (self, func):\n' % ftype.name
    s += '  return _CPY._AIDA_pymarshal__%s (self, func, 0)\n' % ftype.ident_digest()
    s += 'def sig_%s_disconnect (self, connection_id):\n' % ftype.name
    s += '  return _CPY._AIDA_pymarshal__%s (self, None, connection_id)\n' % ftype.ident_digest()
    return s
  def generate_to_proto_pyimpl (self, argname, type_info, valname, onerror = 'return false'):
    s = ''
    if type_info.storage == Decls.VOID:
      pass
    elif type_info.storage in (Decls.BOOL, Decls.INT32, Decls.INT64, Decls.ENUM):
      s += '  %s.vint64 = %s\n' % (argname, valname)
    elif type_info.storage == Decls.FLOAT64:
      s += '  %s.vdouble = %s\n' % (argname, valname)
    elif type_info.storage == Decls.STRING:
      s += '  %s.vstring = %s\n' % (argname, valname)
    elif type_info.storage == Decls.INTERFACE:
      s += '  %s.vstring (Instance2StringCast (%s))\n' % (argname, valname)
    elif type_info.storage == Decls.RECORD:
      s += '  %s.to_proto (%s.vrec, %s)\n' % (type_info.name, argname, valname)
    elif type_info.storage == Decls.SEQUENCE:
      s += '  %s.to_proto (%s.vseq, %s)\n' % (type_info.name, argname, valname)
    elif type_info.storage == Decls.ANY:
      s += '  # FIXME: support Aida::Any with %s.to_proto (%s.vany, %s)\n' % (type_info.name, argname, valname)
    else: # FUNC
      raise RuntimeError ("Unexpected storage type: " + type_info.storage)
    return s
  def generate_method_caller_pyimpl (self, m):
    s = ''
    s += 'def %s (' % m.name
    args = [ 'self' ]
    vals = [ 'self' ]
    for a in m.args:
      if a[2] != None:
        args += [ '%s = %s' % (a[0], self.default_value_pyimpl (a[1], a[2])) ]
      else:
        args += [ a[0] ]
      vals += [ a[0] ]
    s += ', '.join (args)
    if m.rtype.name == 'void':
      s += '): # one way\n'
    else:
      s += '): # %s\n' % m.rtype.name
    s += '  ___ret = _CPY._AIDA_pycall__%s (' % m.ident_digest()
    s += ', '.join (vals)
    s += ')\n'
    s += '  return ___ret'
    return s
  def inherit_reduce (self, type_list):
    def hasancestor (child, parent):
      if child == parent:
        return True
      for childpre in child.prerequisites:
        if hasancestor (childpre, parent):
          return True
    reduced = []
    while type_list:
      p = type_list.pop()
      skip = 0
      for c in type_list + reduced:
        if hasancestor (c, p):
          skip = 1
          break
      if not skip:
        reduced = [ p ] + reduced
    return reduced
  def generate_class_pyimpl (self, type_info):
    s = ''
    l = []
    for pr in type_info.prerequisites:
      l += [ pr ]
    l = self.inherit_reduce (l)
    l = [pr.name for pr in l] # types -> names
    if not l:
      l = [ '_BaseClass_' ]
    s += 'class %s (%s):\n' % (type_info.name, ', '.join (l))
    s += '  def __init__ (self, _aida_id):\n'
    s += '    super (%s, self).__init__ (_aida_id)\n' % type_info.name
    for m in type_info.methods:
      s += reindent ('  ', self.generate_method_caller_pyimpl (m)) + '\n'
    for sg in type_info.signals:
      s += reindent ('  ', self.generate_signal_pyimpl (sg, type_info)) + '\n'
    s += '  def __getattr__ (self, name):\n'
    for fl in type_info.fields:
      (digeststring, digestnums) = self.getter_digests (type_info, fl[0], fl[1])
      s += '    if name == ' + strcquote (fl[0]) + ':\n'
      s += '      return _CPY._AIDA_pygetter__%s (self) # %s.%s\n' % (digeststring, type_info.name, fl[0])
    s += '    raise AttributeError ("class %s has no attribute \'%s\'" % (self.__class__.__name__, name))\n'
    s += '  def __setattr__ (self, name, value):\n'
    for fl in type_info.fields:
      (digeststring, digestnums) = self.setter_digests (type_info, fl[0], fl[1])
      s += '    if name == ' + strcquote (fl[0]) + ':\n'
      s += '      return _CPY._AIDA_pysetter__%s (self, value) # %s.%s\n' % (digeststring, type_info.name, fl[0])
    s += '    raise AttributeError ("class %s has no attribute \'%s\'" % (self.__class__.__name__, name))\n'
    return s
  def generate_pyimpl_types (self, implementation_types):
    s = '### --- Generated by Rapicorn-PyStub --- ###\n'
    s += TmplFiles.PyStub_glue_py + '\n'
    self.tabwidth (16)
    # collect impl types
    types = []
    for tp in implementation_types:
      if tp.isimpl:
        types += [ tp ]
    # generate types
    for tp in types:
      if tp.is_forward: # or tp.typedef_origin
        pass
      elif tp.storage == Decls.RECORD:
        s += self.generate_record_pyimpl (tp) + '\n'
      elif tp.storage == Decls.INTERFACE:
        s += self.generate_class_pyimpl (tp) + '\n'
    return s
  def generate_enum_cxximpl (self, type_info):
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
  def generate_add_0_cxximpl (self, fb, type):
    s = ''
    if type.storage == Decls.BOOL:
      s += '  %s.add_bool (0);\n' % fb
    elif type.storage == Decls.INT32:
      s += '  %s.add_int64 (0);\n' % fb
    elif type.storage == Decls.INT64:
      s += '  %s.add_int64 (0);\n' % fb
    elif type.storage == Decls.FLOAT64:
      s += '  %s.add_double (0);\n' % fb
    elif type.storage == Decls.ENUM:
      s += '  %s.add_evalue (0);\n' % fb
    elif type.storage == Decls.STRING:
      s += '  %s.add_string ("");\n' % fb
    elif type.storage == Decls.RECORD:
      s += '  %s.add_rec (0);\n' % fb
    elif type.storage == Decls.SEQUENCE:
      s += '  %s.add_seq (0);\n' % fb
    elif type.storage == Decls.INTERFACE:
      s += '  %s.add_object (NULL);\n' % fb
    elif type.storage == Decls.ANY:
      s += '  %s.add_any (Rapicorn::Aida::Any());\n' % fb
    else: # FUNC VOID
      raise RuntimeError ("marshalling not implemented: " + type.storage)
    return s
  def generate_add_field_cxximpl (self, fb, type, var, excheck = "ERRORifpy()"):
    s = ''
    if type.storage == Decls.BOOL:
      s += '  %s.add_bool (PyIntLong_AsLongLong (%s)); %s;\n' % (fb, var, excheck)
    elif type.storage == Decls.INT32:
      s += '  %s.add_int64 (PyIntLong_AsLongLong (%s)); %s;\n' % (fb, var, excheck)
    elif type.storage == Decls.INT64:
      s += '  %s.add_int64 (PyIntLong_AsLongLong (%s)); %s;\n' % (fb, var, excheck)
    elif type.storage == Decls.FLOAT64:
      s += '  %s.add_double (PyFloat_AsDouble (%s)); %s;\n' % (fb, var, excheck)
    elif type.storage == Decls.ENUM:
      s += '  %s.add_evalue (PyIntLong_AsLongLong (%s)); %s;\n' % (fb, var, excheck)
    elif type.storage == Decls.STRING:
      s += '  %s.add_string (PyString_As_std_string (%s)); %s;\n' % (fb, var, excheck)
    elif type.storage in (Decls.RECORD, Decls.SEQUENCE):
      s += '  if (!aida_py%s_proto_add (%s, %s)) goto error;\n' % (type.name, var, fb)
    elif type.storage == Decls.INTERFACE:
      s += '  %s.add_object (PyAttr_As_uint64 (%s, "__aida_pyobject__")); %s;\n' % (fb, var, excheck)
    elif type.storage == Decls.ANY:
      s += '  { Rapicorn::Aida::Any tmpany;\n'
      s += '    __AIDA_pyconvert__pyany_to_any (tmpany, %s); %s;\n' % (var, excheck)
      s += '    %s.add_any (tmpany); }\n' % fb
    else: # FUNC VOID
      raise RuntimeError ("marshalling not implemented: " + type.storage)
    return s
  def generate_pop_field_cxximpl (self, fbr, type, var):
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
      s += '  %s = __AIDA_pyfactory__create_from_orbid (%s.pop_object()); ERRORifpy();\n' % (var, fbr)
    elif type.storage == Decls.ANY:
      s += '  %s = __AIDA_pyconvert__pyany_from_any (%s.pop_any()); ERRORifpy();\n' % (var, fbr)
    else: # FUNC VOID
      raise RuntimeError ("marshalling not implemented: " + type.storage)
    return s
  def generate_record_cxximpl (self, type_info):
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
      s += self.generate_add_field_cxximpl ('fb', fl[1], 'item')
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
      s += self.generate_pop_field_cxximpl ('fbr', fl[1], 'pyfoR')
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
  def generate_sequence_cxximpl (self, type_info):
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
    s += reindent ('  ', self.generate_add_field_cxximpl ('fb', el[1], 'item')) + '\n'
    s += '  }\n'
    s += '  success = true;\n'
    s += ' error:\n'
    s += '  Py_XDECREF (pyseq);\n'
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
    s += reindent ('  ', self.generate_pop_field_cxximpl ('fbr', el[1], 'pyfoR')) + '\n'
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
  def generate_pyemit_cxximpl (self, class_info, stype, mdefs):
    digest, async = self.method_digest (stype), stype.rtype.storage != Decls.VOID
    mdefs += [ '{ "_AIDA_pymarshal__%s", __AIDA_pymarshal__%s, METH_VARARGS, "pyRapicorn signal call" }' %
               (stype.ident_digest(), stype.ident_digest()) ]
    s, cbtname, classN = '', 'Callback' + '_' + stype.name, class_info.name
    u64 = 'Rapicorn::Aida::uint64_t'
    emitfunc = '__AIDA_pyemit%d__%s__%s' % (2 if async else 1, classN, stype.name)
    s += 'static Rapicorn::Aida::FieldBuffer*\n%s ' % emitfunc
    s += '(const Rapicorn::Aida::FieldBuffer *sfb, void *data)\n{\n'
    s += '  Rapicorn::Aida::FieldBuffer *rb = NULL;\n'
    s += '  PyObject *callable = (PyObject*) data;\n'
    s += '  if (AIDA_UNLIKELY (!sfb)) { Py_DECREF (callable); return NULL; }\n'
    s += '  const uint length = %u;\n' % len (stype.args)
    s += '  PyObject *result = NULL, *tuple = PyTuple_New (length)%s;\n' % (', *item' if stype.args else '')
    if stype.args or async:
      s += '  FieldReader fbr (*sfb);\n'
      s += '  fbr.skip_header();\n'
      s += '  fbr.skip();  // skip handler_id\n'
      if async:
        s += '  Rapicorn::Aida::uint64_t emit_result_id = fbr.pop_int64();\n'
      arg_counter = 0
      for a in stype.args:
        s += self.generate_pop_field_cxximpl ('fbr', a[1], 'item')
        s += '  PyTuple_SET_ITEM (tuple, %u, item);\n' % arg_counter
        arg_counter += 1
    s += '  if (PyErr_Occurred()) goto error;\n'
    s += '  result = PyObject_Call (callable, tuple, NULL);\n' # we MUST return EMIT_RESULT to be PyException safe
    if async:
      s += '  rb = Rapicorn::Aida::ObjectBroker::renew_into_result (fbr, Rapicorn::Aida::MSGID_EMIT_RESULT, ' # invalidates fbr
      s += 'Rapicorn::Aida::ObjectBroker::receiver_connection_id (fbr.field_buffer()->first_id()), %s, 2);\n' % digest
      s += '  *rb <<= emit_result_id;\n'
      s += '  if (PyErr_Occurred()) {\n'
      s += '  ' + self.generate_add_0_cxximpl ('(*rb)', stype.rtype)
      s += '    goto error;\n'
      s += '  } else {\n'
      s += '  ' + self.generate_add_field_cxximpl ('(*rb)', stype.rtype, 'result', "ERROR_callable_ifpy (callable)")
      s += '  }\n'
    s += ' error:\n'
    s += '  Py_XDECREF (result);\n'
    s += '  Py_XDECREF (tuple);\n'
    s += '  return rb;\n'
    s += '}\n'
    s += 'static PyObject*\n'
    s += '__AIDA_pymarshal__%s (PyObject *pyself, PyObject *pyargs)\n' % stype.ident_digest()
    s += '{\n'
    s += '  while (0) { error: return NULL; }\n'
    s += '  if (PyTuple_Size (pyargs) != 1 + 2) ERRORpy ("wrong number of arguments");\n'
    s += '  PyObject *item = PyTuple_GET_ITEM (pyargs, 0);  // self\n'
    s += '  Rapicorn::Aida::uint64_t oid = PyAttr_As_uint64 (item, "__aida_pyobject__"); ERRORifpy();\n'
    s += '  PyObject *callable = PyTuple_GET_ITEM (pyargs, 1);  // Closure\n'
    s += '  %s result = 0;\n' % u64
    s += '  if (callable == Py_None) {\n'
    s += '    PyObject *pyo = PyTuple_GET_ITEM (pyargs, 2);\n' # connection id for disconnect
    s += '    %s dc_id = PyIntLong_AsLongLong (pyo); ERRORifpy();\n' % u64
    s += '    result = __AIDA_local__client_connection->signal_disconnect (dc_id);\n'
    s += '  } else {\n'
    s += '    if (!PyCallable_Check (callable)) ERRORpy ("arg2 must be callable");\n'
    s += '    Py_INCREF (callable);\n'
    s += '    result = __AIDA_local__client_connection->signal_connect (%s, oid, %s, callable);\n' % (digest, emitfunc)
    s += '  }\n'
    s += '  PyObject *pyres = PyLong_FromLongLong (result); ERRORifpy ();\n'
    s += '  return pyres;\n'
    s += '}\n'
    return s
  def hash2digestbytes (self, digest):
    return '0x%02x%02x%02x%02x%02x%02x%02x%02xULL, 0x%02x%02x%02x%02x%02x%02x%02x%02xULL' % digest
  def hash2digestident (self, digest):
    return '%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x' % digest
  def method_digest (self, mtype):
    method_hash = mtype.type_hash()
    return self.hash2digestbytes (method_hash)
  def getter_digests (self, class_info, fident, ftype):
    getter_hash = class_info.property_hash ((fident, ftype), False)
    return (self.hash2digestident (getter_hash), self.hash2digestbytes (getter_hash))
  def setter_digests (self, class_info, fident, ftype):
    setter_hash = class_info.property_hash ((fident, ftype), True)
    return (self.hash2digestident (setter_hash), self.hash2digestbytes (setter_hash))
  def generate_pycall_cxximpl (self, class_info, mtype, mdefs):
    s = ''
    mdefs += [ '{ "_AIDA_pycall__%s", __AIDA_pycall__%s, METH_VARARGS, "pyRapicorn call" }' %
               (mtype.ident_digest(), mtype.ident_digest()) ]
    hasret = mtype.rtype.storage != Decls.VOID
    s += 'static PyObject*\n'
    s += '__AIDA_pycall__%s (PyObject *pyself, PyObject *pyargs)\n' % mtype.ident_digest()
    s += '{\n'
    s += '  uint64_t object_orbid;\n'
    s += '  PyObject *item%s;\n' % (', *pyfoR = NULL' if hasret else '')
    s += '  FieldBuffer *fm = FieldBuffer::_new (3 + 1 + %u), &fb = *fm, *fr = NULL;\n' % len (mtype.args) # header + self + args
    s += '  if (PyTuple_Size (pyargs) != 1 + %u) ERRORpy ("Aida: wrong number of arguments");\n' % len (mtype.args) # self + args
    arg_counter = 0
    s += '  item = PyTuple_GET_ITEM (pyargs, %d);  // self\n' % arg_counter
    s += '  object_orbid = PyAttr_As_uint64 (item, "__aida_pyobject__"); ERRORifpy();\n'
    if hasret:
      s += '  fb.add_header2 (Rapicorn::Aida::MSGID_TWOWAY_CALL, Rapicorn::Aida::ObjectBroker::connection_id_from_orbid (object_orbid),'
      s += ' __AIDA_local__client_connection->connection_id(), %s);\n' % self.method_digest (mtype)
    else:
      s += '  fb.add_header1 (Rapicorn::Aida::MSGID_ONEWAY_CALL,'
      s += ' Rapicorn::Aida::ObjectBroker::connection_id_from_orbid (object_orbid), %s);\n' % self.method_digest (mtype)
    s += '  fb.add_object (object_orbid);\n'
    arg_counter += 1
    for ma in mtype.args:
      s += '  item = PyTuple_GET_ITEM (pyargs, %d); // %s\n' % (arg_counter, ma[0])
      s += self.generate_add_field_cxximpl ('fb', ma[1], 'item')
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
      s += reindent ('      ', self.generate_pop_field_cxximpl ('frr', mtype.rtype, 'pyfoR')) + '\n'
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
  def generate_pygetter_cxximpl (self, class_info, fident, ftype, mdefs):
    s, (digeststring, digestnums) = '', self.getter_digests (class_info, fident, ftype)
    mdefs += [ '{ "_AIDA_pygetter__%s", _AIDA_pygetter__%s, METH_VARARGS, "pyRapicorn call" }' % (digeststring, digeststring) ]
    s += 'static PyObject*\n'
    s += '_AIDA_pygetter__%s (PyObject *pyself, PyObject *pyargs) // %s.%s\n' % (digeststring, class_info.name, fident)
    s += '{\n'
    s += '  uint64_t object_orbid;\n'
    s += '  PyObject *item, *pyfoR = NULL;\n'
    s += '  FieldBuffer *fm = FieldBuffer::_new (3 + 1), &fb = *fm, *fr = NULL;\n' # header + self
    s += '  if (PyTuple_Size (pyargs) != 1) ERRORpy ("Aida: wrong number of arguments");\n'
    s += '  item = PyTuple_GET_ITEM (pyargs, 0);\n' # self
    s += '  object_orbid = PyAttr_As_uint64 (item, "__aida_pyobject__"); ERRORifpy();\n'
    s += '  fb.add_header2 (Rapicorn::Aida::MSGID_TWOWAY_CALL, Rapicorn::Aida::ObjectBroker::connection_id_from_orbid (object_orbid), '
    s += '__AIDA_local__client_connection->connection_id(), %s);\n' % digestnums
    s += '  fb.add_object (object_orbid);\n'
    # call out
    s += '  fm = NULL; fr = __AIDA_local__client_connection->call_remote (&fb);\n' # deletes fb
    s += '  ERRORifnotret (fr);\n'
    s += '  if (fr) {\n'
    s += '    Rapicorn::Aida::FieldReader frr (*fr);\n'
    s += '    frr.skip_header();\n'
    s += '    if (frr.remaining() == 1) {\n'
    s += reindent ('      ', self.generate_pop_field_cxximpl ('frr', ftype, 'pyfoR')) + '\n'
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
  def generate_pysetter_cxximpl (self, class_info, fident, ftype, mdefs):
    s, (digeststring, digestnums) = '', self.setter_digests (class_info, fident, ftype)
    mdefs += [ '{ "_AIDA_pysetter__%s", _AIDA_pysetter__%s, METH_VARARGS, "pyRapicorn call" }' % (digeststring, digeststring) ]
    s += 'static PyObject*\n'
    s += '_AIDA_pysetter__%s (PyObject *pyself, PyObject *pyargs) // %s.%s\n' % (digeststring, class_info.name, fident)
    s += '{\n'
    s += '  uint64_t object_orbid;\n'
    s += '  PyObject *item;\n'
    s += '  FieldBuffer *fm = FieldBuffer::_new (3 + 1 + 1), &fb = *fm, *fr = NULL;\n' # header + self + arg
    s += '  if (PyTuple_Size (pyargs) != 1 + 1) ERRORpy ("Aida: wrong number of arguments");\n' # self + arg
    s += '  item = PyTuple_GET_ITEM (pyargs, 0);\n' # self
    s += '  object_orbid = PyAttr_As_uint64 (item, "__aida_pyobject__"); ERRORifpy();\n'
    s += '  fb.add_header1 (Rapicorn::Aida::MSGID_ONEWAY_CALL, Rapicorn::Aida::ObjectBroker::connection_id_from_orbid (object_orbid), '
    s += '%s);\n' % digestnums
    s += '  fb.add_object (object_orbid);\n'
    s += '  item = PyTuple_GET_ITEM (pyargs, 1);\n' # arg
    s += self.generate_add_field_cxximpl ('fb', ftype, 'item')
    # call out
    s += '  fm = NULL; fr = __AIDA_local__client_connection->call_remote (&fb);\n' # deletes fb
    s += '  if (fr) { delete fr; fr = NULL; }\n'
    s += '  return None_INCREF();\n'
    s += ' error:\n'
    s += '  if (fr) delete fr;\n'
    s += '  if (fm) delete fm;\n'
    s += '  return NULL;\n'
    s += '}\n'
    return s
  def type2cxx (self, typename):
    if typename == 'float': return 'double'
    if typename == 'string': return 'std::string'
    return typename
  def generate_cxximpl_types (self, implementation_types):
    s = '/* --- Generated by Rapicorn-CxxPyStub --- */\n'
    s += TmplFiles.PyStub_glue_cc + '\n'
    self.tabwidth (16)
    # collect impl types
    types = []
    for tp in implementation_types:
      if tp.isimpl:
        types += [ tp ]
    # generate prototypes
    for tp in types:
      if tp.typedef_origin or tp.is_forward:
        pass # s += 'typedef %s %s;\n' % (self.type2cxx (tp.typedef_origin.name), tp.name)
      elif tp.storage == Decls.RECORD:
        s += 'struct %s;\n' % tp.name
      elif tp.storage == Decls.ENUM:
        s += self.generate_enum_cxximpl (tp) + '\n'
    # generate types
    for tp in types:
      if tp.typedef_origin or tp.is_forward:
        pass
      elif tp.storage == Decls.RECORD:
        s += self.generate_record_cxximpl (tp) + '\n'
      elif tp.storage == Decls.SEQUENCE:
        s += self.generate_sequence_cxximpl (tp) + '\n'
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
          s += self.generate_pycall_cxximpl (tp, m, mdefs)
        for sg in tp.signals:
          s += self.generate_pyemit_cxximpl (tp, sg, mdefs)
        for fl in tp.fields:
          s += self.generate_pygetter_cxximpl (tp, fl[0], fl[1], mdefs)
          s += self.generate_pysetter_cxximpl (tp, fl[0], fl[1], mdefs)
    # method def array
    if mdefs:
      aux = '{ "__AIDA_pyfactory__register_callback", __AIDA_pyfactory__register_callback, METH_VARARGS, "Register Python object factory callable" }'
      s += '#define AIDA_PYSTUB_METHOD_DEFS() \\\n  ' + ',\\\n  '.join ([aux] + mdefs) + '\n'
    return s

def generate (namespace_list, **args):
  import sys, tempfile, os
  config = {}
  config.update (args)
  outname = config.get ('output', 'testmodule')
  if outname == '-':
    raise RuntimeError ("-: stdout is not support for generation of multiple files")
  gg = Generator()
  if 1:
    fname = outname + '.cc'
    print "  GEN   ", fname
    fout = open (fname, 'w')
    textstring = gg.generate_cxximpl_types (config['implementation_types'])
    fout.write (textstring)
    fout.close()
  if 1:
    fname = outname + '.py'
    print "  GEN   ", fname
    fout = open (fname, 'w')
    gg = Generator()
    textstring = gg.generate_pyimpl_types (config['implementation_types'])
    fout.write (textstring)
    fout.close()

# register extension hooks
__Aida__.add_backend (__file__, generate, __doc__)
