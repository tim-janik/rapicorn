# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
"""AidaPyxxStub - Aida Cython Code Generator

More details at http://www.rapicorn.org/
"""
import Decls, re, sys, os

def reindent (prefix, lines):
  return re.compile (r'^(?=.)', re.M).sub (prefix, lines)
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

# == Utilities ==
def class_digest (class_info):
  return digest2cbytes (class_info.type_hash())
def digest2cbytes (digest):
  return '0x%02x%02x%02x%02x%02x%02x%02x%02xULL, 0x%02x%02x%02x%02x%02x%02x%02x%02xULL' % digest
def hasancestor (child, parent):
  for p in child.prerequisites:
    if p == parent or hasancestor (p, parent):
      return True
def inherit_reduce (type_list):
  # find the type(s) we *directly* derive from
  reduced = []
  while type_list:
    p = type_list.pop()
    skip = 0
    for c in type_list + reduced:
      if c == p or hasancestor (c, p):
        skip = 1
        break
    if not skip:
      reduced = [ p ] + reduced
  return reduced
def bases (tp):
  ancestors = [pr for pr in tp.prerequisites]
  return inherit_reduce (ancestors)
def type_namespace_names (tp):
  namespaces = tp.list_namespaces() # [ Namespace... ]
  return [d.name for d in namespaces if d.name]
def underscore_namespace (tp):
  return '__'.join (type_namespace_names (tp))
def colon_namespace (tp):
  return '::'.join (type_namespace_names (tp))
def underscore_typename (tp):
  if tp.storage == Decls.ANY:
    return 'Rapicorn__Any'
  return '__'.join (type_namespace_names (tp) + [ tp.name ])
def colon_typename (tp):
  name = '::'.join (type_namespace_names (tp) + [ tp.name ])
  if tp.storage == Decls.INTERFACE:
    name += 'H' # e.g. WidgetHandle
  return name
def cxx_argtype (tp):
  typename = underscore_typename (tp)
  if tp.storage in (Decls.STRING, Decls.SEQUENCE, Decls.RECORD, Decls.ANY):
    typename = 'const %s&' % typename
  elif tp.storage == Decls.INTERFACE:
    typename = '%s' % typename
  return typename
def format_pyarg (argname, argtype, argdefault):
  if argdefault == None:
    return argname
  if argtype.storage == Decls.BOOL:
    vdefault = True if argdefault else False
  elif argtype.storage in (Decls.INT32, Decls.INT64, Decls.FLOAT64, Decls.ENUM, Decls.STRING):
    vdefault = argdefault # string or number litrals
  else:
    defaults = { Decls.RECORD : 'None', Decls.SEQUENCE : '()', Decls.INTERFACE : 'None', Decls.ANY : '()' }
    vdefault = defaults[argtype.storage]
  return '%s = %s' % (argname, vdefault)

class Generator:
  def __init__ (self, idl_file, module_name):
    assert isinstance (module_name, str)
    self.ntab = 26
    self.idl_file = idl_file
    self.module_name = module_name
    self.strip_path = ""
    self.idcounter = 1001
    self.marshallers = {}
  def tabwidth (self, n):
    self.ntab = n
  def format_to_tab (self, string, indent = ''):
    if len (string) >= self.ntab:
      return indent + string + ' '
    else:
      f = '%%-%ds' % self.ntab  # '%-20s'
      return indent + f % string
  def cxx_type (self, type_node):
    tstorage = type_node.storage
    if tstorage == Decls.VOID:          return 'void'
    if tstorage == Decls.BOOL:          return 'bool'
    if tstorage == Decls.INT32:         return 'int'
    if tstorage == Decls.INT64:         return 'int64_t'
    if tstorage == Decls.FLOAT64:       return 'double'
    if tstorage == Decls.STRING:        return 'String'
    if tstorage == Decls.ANY:           return 'Rapicorn__Any'
    fullnsname = underscore_typename (type_node)
    return fullnsname
  def inheritance_base (self, tp):
    # find the type everything else derives from
    basetype = tp
    pre = basetype.prerequisites
    while pre:
      basetype = pre[0]
      pre = basetype.prerequisites
    return basetype
  def marshal (self, rtp, argtypes):
    rargtypes = [ rtp ] + argtypes
    rargtypenames = [underscore_typename (t) for t in rargtypes]
    u_rargtypenames = '_'.join (rargtypenames)
    marshal = self.marshallers.get (u_rargtypenames, {})
    if not marshal:
      idcounter = self.idcounter
      self.idcounter += 1
      nargs = len (argtypes)
      marshal['id'] = idcounter
      marshal['Caller'] =     'Caller%d__%d' % (nargs, idcounter)
      calltypes = [ underscore_typename (rtp) ] + [ cxx_argtype (a) for a in argtypes ]
      marshal['PyxxCaller'] = 'PyxxCaller%d[%s]' % (nargs, ', '.join (calltypes)) # rargtypenames
      marshal['ctypedef'] = 'ctypedef %-50s %s\n' % (marshal['PyxxCaller'], marshal['Caller'])
      argnames = ['a%d' % n for n in range (1, 1 + len (argtypes))]
      namedargtypelist = zip ([ cxx_argtype (a) for a in argtypes ], argnames)
      namedargtypelist = [ ('object', 'pycallable') ] + namedargtypelist
      namedargtypes = ', '.join ('%s %s' % namedargtype for namedargtype in namedargtypelist)
      marshal['pyxx_marshal'] = 'pyxx_marshal__%d' % idcounter
      cdef  = 'cdef %s %s (%s) except *:\n' % (underscore_typename (rtp), marshal['pyxx_marshal'], namedargtypes)
      result = ' _pyret =' if rtp.storage != Decls.VOID else ''
      cdef += ' %s pycallable (%s)\n' % (result, ', '.join (self.py_wrap (*apair) for apair in zip (argnames, argtypes)))
      if result:
        cdef += '  return %s\n' % self.cxx_unwrap ('_pyret', rtp)
      marshal['cdef'] = cdef
      self.marshallers[u_rargtypenames] = marshal
    return marshal, marshal['id']
  def generate_types_pyxx (self, implementation_types):
    s  = '# === Generated by PyxxStub.py ===            -*-mode:python;-*-\n'
    s += 'from libcpp cimport *\n'
    s += 'from cython.operator cimport dereference as _dereference\n'
    s += 'from libc.stdint cimport *\n'
    s += 'from cpython.object cimport Py_LT, Py_LE, Py_EQ, Py_NE, Py_GT, Py_GE\n'
    self.tabwidth (16)
    # collect impl types
    namespaces = []
    types = []
    for tp in implementation_types:
      if tp.isimpl:
        types += [ tp ]
        namespaces.append (tp.list_namespaces())
    if not types:
      return s
    # check unique namespace
    while len (namespaces) >= 2 and namespaces[0] == namespaces[1]:
      namespaces = namespaces[1:]
    namespaces = [[n for n in names if n.name] for names in namespaces] # strip (initial) empty names
    max_namespaces = max (len (namespaces), len (namespaces[0]))
    if max_namespaces != 1:
      raise NotImplementedError ('Pyxx code generation requires exactly 1 namespace (%d given)' % max_namespaces)
    self.namespace = namespaces[0][0].name
    del namespaces, max_namespaces
    # C++ Builtins # FIXME: move elsewhere
    s += '\n'
    s += '# Builtins\n'
    s += 'cdef extern from * namespace "Rapicorn":\n'
    s += '  cdef enum Rapicorn__Aida__TypeKind             "Rapicorn::Aida::TypeKind":\n'
    s += '    TypeKind__UNTYPED                            "Rapicorn::Aida::UNTYPED"\n'
    s += '    TypeKind__VOID                               "Rapicorn::Aida::VOID"\n'
    s += '    TypeKind__BOOL                               "Rapicorn::Aida::BOOL"\n'
    s += '    TypeKind__INT32                              "Rapicorn::Aida::INT32"\n'
    s += '    TypeKind__INT64                              "Rapicorn::Aida::INT64"\n'
    s += '    TypeKind__FLOAT64                            "Rapicorn::Aida::FLOAT64"\n'
    s += '    TypeKind__STRING                             "Rapicorn::Aida::STRING"\n'
    s += '    TypeKind__ENUM                               "Rapicorn::Aida::ENUM"\n'
    s += '    TypeKind__SEQUENCE                           "Rapicorn::Aida::SEQUENCE"\n'
    s += '    TypeKind__RECORD                             "Rapicorn::Aida::RECORD"\n'
    s += '    TypeKind__INSTANCE                           "Rapicorn::Aida::INSTANCE"\n'
    s += '    TypeKind__REMOTE                             "Rapicorn::Aida::REMOTE"\n'
    s += '    TypeKind__LOCAL                              "Rapicorn::Aida::LOCAL"\n'
    s += '    TypeKind__ANY                                "Rapicorn::Aida::ANY"\n'
    s += '  String Rapicorn__Aida__type_kind_name          "Rapicorn::Aida::type_kind_name" (Rapicorn__Aida__TypeKind)\n'
    s += '  cppclass %-40s "%s"\n' % ('Rapicorn__Any', 'Rapicorn::Any')
    s += '  cppclass Rapicorn__Any:\n'
    s += '    Rapicorn__Aida__TypeKind kind                                        ()\n'
    s += '    void                     set__String         "set<Rapicorn::String>" (String)\n'
    s += '    void                     set__bool           "set<bool>"             (bool)\n'
    s += '    void                     set__int64          "set<Rapicorn::int64>"  (int64)\n'
    s += '    void                     set__float64        "set<double>"           (double)\n'
    s += '    bool                     get__bool           "get<bool>"             ()\n'
    s += '    int64                    get__int64          "get<Rapicorn::int64>"  ()\n'
    s += '    float64                  get__float64        "get<double>"           ()\n'
    s += '    String                   get__String         "get<Rapicorn::String>" ()\n'
    s += 'cdef Rapicorn__Any Rapicorn__Any__unwrap (object pyo) except *:\n'
    s += '  cdef Rapicorn__Any vany\n'
    s += '  if pyo == None:\n'
    s += '    pass\n'
    s += '  elif isinstance (pyo, basestring):\n'
    s += '    vany.set__String (pyo)\n'
    s += '  elif isinstance (pyo, type (True)):\n' # use 'type (True)' as workaround for 'bool' which Cython-0.20.2 doesn't know
    s += '    vany.set__bool (pyo)\n'
    s += '  elif isinstance (pyo, int) or isinstance (pyo, long):\n'
    s += '    vany.set__int64 (pyo)\n'
    s += '  elif isinstance (pyo, float):\n'
    s += '    vany.set__float64 (pyo)\n'
    s += '  else:\n' # FIXME: Any_unwrap for ENUM, RECORD, SEQUENCE, REMOTE, ANY
    s += '    raise NotImplementedError ("Any: unable to convert Python value: %s" % repr (pyo))\n'
    s += '  return vany\n'
    s += 'cdef object Rapicorn__Any__wrap (const Rapicorn__Any &cany):\n'
    s += '  cdef Rapicorn__Aida__TypeKind kind = cany.kind()\n'
    s += '  if kind == TypeKind__UNTYPED:\n'
    s += '    return None\n'
    s += '  elif kind == TypeKind__BOOL:\n'
    s += '    return cany.get__bool()\n'
    s += '  elif kind == TypeKind__INT32 or kind == TypeKind__INT64:\n'
    s += '    return cany.get__int64()\n'
    s += '  elif kind == TypeKind__FLOAT64:\n'
    s += '    return cany.get__float64()\n'
    s += '  elif kind == TypeKind__STRING:\n'
    s += '    return cany.get__String()\n'
    s += '' # FIXME: Any_wrap for ENUM, RECORD, SEQUENCE, REMOTE, ANY
    s += '  raise NotImplementedError ("Any: unable to convert contents to Python: %s" % Rapicorn__Aida__type_kind_name (kind))\n'
    s += 'cdef int32 int32__unwrap (object pyo):\n'
    s += '  cdef int64 v64 = pyo # type checks for int-convertible\n'
    s += '  return <int32> v64 # silenty "cut" too big numbers\n'
    # C++ Enum Values
    s += '\n'
    s += '# C++ Enums\n'
    s += 'cdef extern from * namespace "%s":\n' % self.namespace
    s += '  pass\n'
    for tp in types:
      if tp.is_forward:
        continue
      if tp.storage == Decls.ENUM:
        s += '  cdef enum %-50s "%s":\n' % (underscore_typename (tp), colon_typename (tp))
        for opt in tp.options:
          (ident, label, blurb, number) = opt
          s += '    %-60s "%s"\n' % ('%s__%s' % (tp.name, ident), colon_namespace (tp) + '::' + ident)
    # C++ Declarations
    s += '\n'
    s += '# C++ Declarations\n'
    s += 'cdef extern from * namespace "%s":\n' % self.namespace
    for tp in types:
      if tp.is_forward:
        continue
      if tp.storage in (Decls.SEQUENCE, Decls.RECORD, Decls.INTERFACE):
        s += '  cppclass %-40s "%s"\n' % (underscore_typename (tp), colon_typename (tp))
    s += '  pass\n'
    # C++ Callback Types
    s += '\n'
    s += '# C++ Callback Types\n'
    marshal_set = set()
    for tp in [tp for tp in types if tp.storage == Decls.INTERFACE and not tp.is_forward]:
      for stp in tp.signals:
        marshal, mid = self.marshal (stp.rtype, [a[1] for a in stp.args])
        if mid in marshal_set:
          continue
        marshal_set.add (mid)
        s += marshal['ctypedef']
    # C++ Casts
    s += '\n'
    s += '# C++ Handle Casts\n'
    s += 'cdef extern from * namespace "%s":\n' % self.namespace
    for tp in types:
      if tp.is_forward or tp.storage != Decls.INTERFACE:
        continue
      u_typename, c_typename, u_base = underscore_typename (tp), colon_typename (tp), underscore_typename (self.inheritance_base (tp))
      s += '  %-25s %-40s "dynamic_cast<%s*>" (void*) except NULL\n' % (u_typename + '*', u_typename + 'H__dynamic_cast', c_typename)
      # 'except NULL' means that cython checks the return value for NULL. allthough
      # dynamic_cast will not set a Python exception, raising a 'SystemError: error
      # return without exception set' is still better than dereferencing and crashing
      s += '  %-25s %-40s "%s::down_cast" (%s)\n' % (u_typename, u_typename + 'H__down_cast', c_typename, u_base)
    # C++ Classes
    t = '' # definitions to append
    s += '\n'
    s += '# C++ Classes\n'
    s += 'cdef extern from * namespace "%s":\n' % self.namespace
    s += '  pass\n'
    for tp in types:
      u_typename = underscore_typename (tp)
      if tp.is_forward:
        continue
      if tp.storage == Decls.SEQUENCE:
        fname, ftp = tp.elements
        s += '  cppclass %s (vector[%s]):\n' % (u_typename, underscore_typename (ftp))
        s += '    pass\n'
      elif tp.storage == Decls.RECORD:
        s += '  cppclass %s:\n' % u_typename
        for fname, ftp in tp.fields:
          s += '    %-30s %s\n' % (self.cxx_type (ftp), fname)
      elif tp.storage == Decls.INTERFACE:
        ibase = self.inheritance_base (tp)
        u_base = underscore_typename (ibase)
        s += '  cppclass %s' % u_typename
        baselist = ', '.join (underscore_typename (b) for b in bases (tp))
        if baselist:
          s += ' (%s)' % baselist
        s += ':\n'
        s += '    %s ()\n' % u_typename                 # default ctor
        s += '    %s (%s)\n' % (u_typename, u_typename) # copy ctor
        for fname, ftp in tp.fields:
          s += '    %-30s %-20s () except *\n' % (underscore_typename (ftp), fname)
          s += '    %-30s %-20s (%s) except *\n' % ('void', fname, underscore_typename (ftp))
        for mtp in tp.methods:
          rtp, mname = mtp.rtype, mtp.name
          atypes = [a[1] for a in mtp.args]
          arglist = ', '.join (cxx_argtype (a) for a in atypes)
          s += '    %-30s %-20s (%s) except *\n' % (underscore_typename (rtp), mname, arglist)
        for stp in tp.signals:
          marshal, mid = self.marshal (stp.rtype, [a[1] for a in stp.args])
          s += '    size_t sig_%-30s "sig_%s().operator+=" (%s)\n' % (stp.name + '_connect', stp.name, marshal['Caller'])
          s += '    bool   sig_%-30s "sig_%s().operator-=" (size_t)\n' % (stp.name + '_disconnect', stp.name)
        # RemoteHandle (BaseClass only) bindings
        if not bases (tp):
          s += '    uint64 __aida_orbid__ () const\n'
          s += '    bool   __aida_notnull__ "operator bool" () const\n'
          s += '    Aida__TypeHashList __aida_typelist__()\n'
          t += 'ctypedef object (*%s_WrapFunc) (const %s&)\n' % (u_base, u_typename)
          t += 'cdef extern from *:\n'
          t += '  %s_WrapFunc pyxx_aida_type_class_mapper (const Aida__TypeHash&, %s_WrapFunc)\n' % (u_base, u_base)
    s += t # definitions to append after cppclass
    # C++ Marshallers
    s += '\n'
    s += '# C++ Marshallers\n'
    marshal_set = set()
    for tp in [tp for tp in types if tp.storage == Decls.INTERFACE and not tp.is_forward]:
      for stp in tp.signals:
        marshal, mid = self.marshal (stp.rtype, [a[1] for a in stp.args])
        if mid in marshal_set:
          continue
        marshal_set.add (mid)
        s += marshal['cdef']
    # Py Enums
    s += '\n'
    s += '# Python Enums\n'
    for tp in [t for t in types if t.storage == Decls.ENUM]:
      s += '\nclass %s (Enum):\n' % tp.name
      for opt in tp.options:
        (ident, label, blurb, number) = opt
        s += '  %-40s = %s\n' % (ident, '%s__%s' % (tp.name, ident))
      for opt in tp.options:
        (ident, label, blurb, number) = opt
        s += '%-42s =  %s.%s\n' % (ident, tp.name, ident)
    # Py Classes
    s += '\n'
    s += '# Python Classes\n'
    for tp in types:
      if tp.is_forward:
        continue
      u_typename, type_cc_name = underscore_typename (tp), colon_typename (tp)
      if tp.storage == Decls.RECORD:
        s += '\ncdef class %s (Record):\n' % tp.name
        for field in tp.fields:
          ident, type_node = field
          s += '  cdef %s %s\n' % (self.cxx_type (type_node), ident)
        for fname, ftp in tp.fields:
          s += '  property %s:\n' % fname
          s += '    def __get__ (self):    return %s\n' % self.py_wrap ('self.%s' % fname, ftp)
          s += '    def __set__ (self, v): self.%s = %s\n' % (fname, self.cxx_unwrap ('v', ftp))
        s += '  property _fields:\n'
        s += '    def __get__ (self):    return (' + ', '.join ("'%s'" % f[0] for f in tp.fields) + ')\n'
      elif tp.storage == Decls.SEQUENCE:
        fname, ftp = tp.elements
        s += '\ncdef class %s (list):\n' % tp.name
        s += '  pass\n'
      elif tp.storage == Decls.INTERFACE:
        # class inheritance
        handle = '%s__unwrap (self)' % u_typename
        baselist = ', '.join (b.name for b in bases (tp))
        ibase = self.inheritance_base (tp)
        u_base = underscore_typename (ibase)
        s += '\ncdef class %s (%s):\n' % (tp.name, baselist or 'object')
        # cinit/dealloc (BaseClass only)
        if not baselist:
          s += '  cdef %s *_this0\n' % u_typename
          s += '  def __cinit__ (%s self):\n' % tp.name
          s += '    global %s__ctarg\n' % u_typename
          s += '    if not %s__ctarg:\n' % u_typename
          s += '      raise TypeError ("cannot create \'%s\' instances" % self.__class__.__name__)\n'
          s += '    self._this0 = %s__ctarg\n' % u_typename
          s += '    %s__ctarg = NULL\n' % u_typename
          s += '  def __dealloc__ (%s self):\n' % tp.name
          s += '    if self._this0:\n'
          s += '      del self._this0 ; self._this0 = NULL\n'
          s += '  def __richcmp__ (%s self, other, int op):\n' % tp.name
          s += '    if self.__class__ != other.__class__:\n'
          s += '      return _richcmp (self.__class__, other.__class__, op)\n'
          s += '    cdef size_t p1 = self._this0.__aida_orbid__()\n'
          s += '    cdef size_t p2 = (<%s> other)._this0.__aida_orbid__()\n' % tp.name
          s += '    return usize_richcmp (p1, p2, op)\n'
          s += '  def __hash__ (%s self):\n' % tp.name
          s += '    return self._this0.__aida_orbid__()\n' # equal objects need the same hash value
          s += '  def __repr__ (%s self):\n' % tp.name
          s += '    oaddr = object.__repr__ (self)\n' # '<Module.Class object at 0x0abcdef>'
          s += "    assert oaddr[-1] == '>' ; oaddr = oaddr[:-1] # strip trailing '>'\n"
          s += '    return "%s (orbid = 0x%08x)>" % (oaddr, self._this0.__aida_orbid__())\n'
        # properties
        for fname, ftp in tp.fields:
          s += '  property %s:\n' % fname
          s += '    def __get__ (self):    return %s\n' % self.py_wrap ('%s.%s()' % (handle, fname), ftp)
          s += '    def __set__ (self, v): %s.%s (%s)\n' % (handle, fname, self.cxx_unwrap ('v', ftp))
        # signals
        for stp in tp.signals:
          marshal, mid = self.marshal (stp.rtype, [a[1] for a in stp.args])
          s += '  property sig_%s:\n' % stp.name
          s += '    def __get__ (self):\n'
          s += '      def sig_%s_connect (pycallable):\n' % stp.name
          s += '        cdef size_t hid\n'
          s += '        hid = %s.sig_%s_connect (' % (handle, stp.name)
          s += '%s (pycallable, &%s))\n' % (marshal['Caller'], marshal['pyxx_marshal'])
          s += '        return hid\n'
          s += '      def sig_%s_disconnect (handlerid):\n' % stp.name
          s += '        return %s.sig_%s_disconnect (handlerid)\n' % (handle, stp.name)
          s += '      return PyxxSignalConnector (sig_%s_connect, sig_%s_disconnect)\n' % (stp.name, stp.name)
        # methods
        for mtp in tp.methods:
          rtp, mname = mtp.rtype, mtp.name
          arglist = [ format_pyarg (*argtuple) for argtuple in mtp.args ]
          s += '  def %s (%s):\n' % (mname, ', '.join ([ 'self' ] + arglist))
          result = ' _cxxret =' if rtp.storage != Decls.VOID else ''
          s += '   %s %s.%s (' % (result, handle, mname)
          s += ', '.join (self.cxx_unwrap (a[0], a[1]) for a in mtp.args)
          s += ')\n'
          if result:
            s += '    return %s\n' % self.py_wrap ('_cxxret', rtp)
        # __aida_wrapper__
        wrapperclass = '%s__aida_wrapper_Class%d' % (tp.name, self.idcounter) ; self.idcounter += 1
        s += '  @classmethod\n'
        s += '  def __aida_wrapper__ (klass, Class = None):\n'
        s += '    global %s\n' % wrapperclass
        s += '    if Class and issubclass (Class, %s):\n' % tp.name
        s += '      %s = Class\n' % wrapperclass
        s += '    elif not %s:\n' % wrapperclass
        s += '      %s = %s\n' % (wrapperclass, tp.name)
        s += '    return %s\n' % wrapperclass
        s += 'cdef object %s\n' % wrapperclass
        # ctarg (BaseClass only)
        if not baselist:
          s += 'cdef %s *%s__ctarg\n' % (u_typename, u_typename)
          s += self.py_base_wrap_impl (tp)
      # wrap/unwrap for SEQUENCE, RECORD, INTERFACE
      if tp.storage in (Decls.SEQUENCE, Decls.RECORD, Decls.INTERFACE):
        s += 'cdef %s %s__unwrap (object pyo1) except *:\n' % (u_typename, u_typename)
        s += reindent ('  ', self.cxx_unwrap_impl ('pyo1', tp))
      if tp.storage in (Decls.SEQUENCE, Decls.RECORD):
        s += 'cdef object %s__wrap (const %s &cxx1):\n' % (u_typename, u_typename)
        s += reindent ('  ', self.py_wrap_impl ('cxx1', tp))
      elif tp.storage == Decls.INTERFACE:
        s += 'cdef object %s__wrapfunc (const %s &cxxbase):\n' % (u_typename, u_base)
        s += reindent ('  ', self.py_wrap_impl ('cxxbase', tp))
        s += 'pyxx_aida_type_class_mapper (Aida__TypeHash (%s), %s__wrapfunc)\n' % (class_digest (tp), u_typename)
    return s
  def py_wrap (self, ident, tp): # wrap a C++ object to return a PyObject
    if tp.storage in (Decls.ANY, Decls.SEQUENCE, Decls.RECORD):
      return underscore_typename (tp) + '__wrap (%s)' % ident
    elif tp.storage == Decls.INTERFACE:
      u_base = underscore_typename (self.inheritance_base (tp))
      return '%s__wrap (%s)' % (u_base, ident)
    else:
      return ident
  def cxx_unwrap (self, ident, tp): # unwrap a PyObject to yield a C++ object
    if tp.storage == Decls.INT32:
      return underscore_typename (tp) + '__unwrap (%s)' % ident
    if tp.storage in (Decls.ANY, Decls.SEQUENCE, Decls.RECORD, Decls.INTERFACE):
      return underscore_typename (tp) + '__unwrap (%s)' % ident
    return ident
  def cxx_unwrap_impl (self, ident, tp):
    s = ''
    if tp.storage == Decls.SEQUENCE:
      fname, ftp = tp.elements
      s += 'cdef %s thisp\n' % self.cxx_type (tp)
      s += 'for element in %s:\n' % ident
      s += '  thisp.push_back (%s);\n' % self.cxx_unwrap ('element', ftp)
      s += 'return thisp\n'
    elif tp.storage == Decls.RECORD:
      s += 'cdef %s cpy = <%s?> %s\n' % (tp.name, tp.name, ident)
      s += 'cdef %s thisp\n' % self.cxx_type (tp)
      for fname, ftp in tp.fields:
        s += 'thisp.%s = %s;\n' % (fname, 'cpy.%s' % fname)
      s += 'return thisp\n'
    elif tp.storage == Decls.INTERFACE:
      u_typename = underscore_typename (tp)
      s += 'cdef %s self = <%s?> %s\n' % (tp.name, tp.name, ident)
      s += 'return _dereference (%s (self._this0))\n' % (u_typename + 'H__dynamic_cast')
    return s
  def py_wrap_impl (self, ident, tp):
    s = ''
    if tp.storage == Decls.SEQUENCE:
      fname, ftp = tp.elements
      s += 'self = %s()\n' % tp.name
      s += 'for idx in range (%s.size()):\n' % ident
      s += '  self.append (%s)\n' % self.py_wrap ('%s[idx]' % ident, ftp)
      s += 'return self\n'
    elif tp.storage == Decls.RECORD:
      s += 'self = %s()\n' % tp.name
      for fname, ftp in tp.fields:
        s += 'self.%s = %s\n' % (fname, '%s.%s' % (ident, fname)) # self.py_wrap ('field', ftp) # record fields are unwrapped
      s += 'return self\n'
    elif tp.storage == Decls.INTERFACE:
      u_typename, u_base = underscore_typename (tp), underscore_typename (self.inheritance_base (tp))
      s += 'cdef %s target\n' % u_typename # wrapfunc impl
      s += 'target = %sH__down_cast (%s)\n' % (u_typename, ident)
      s += 'if target.__aida_notnull__():\n'
      s += '  global %s__ctarg\n' % u_base
      s += '  %s__ctarg = <%s*> new %s (target)\n' % (u_base, u_base, u_typename)
      s += '  Class = %s.__aida_wrapper__()\n' % tp.name
      s += '  self = Class() # picks up %s__ctarg\n' % u_base
      s += '  assert %s__ctarg == NULL\n' % u_base
      s += '  return self\n'
      s += 'return None\n'
    return s
  def py_base_wrap_impl (self, tp):
    s, ident = '', 'cxxbase'
    u_typename, u_base = underscore_typename (tp), underscore_typename (self.inheritance_base (tp))
    assert u_typename == u_base
    s += 'cdef object %s__wrap (const %s &%s):\n' % (u_typename, u_typename, ident)
    s += '  cdef Aida__TypeHashList thl = %s.__aida_typelist__()\n' % ident
    s += '  cdef Rapicorn__Object_WrapFunc object_pywrapper\n'
    s += '  for idx in range (thl.size()):\n'
    s += '    object_pywrapper = pyxx_aida_type_class_mapper (thl[idx], <Rapicorn__Object_WrapFunc> NULL)\n'
    s += '    if object_pywrapper:\n'
    s += '      return object_pywrapper (%s)\n' % ident
    s += '  return None\n'
    return s

def generate (namespace_list, **args):
  import sys, tempfile, os
  config = {}
  config.update (args)
  outname = config.get ('output', 'testmodule')
  if outname == '-':
    raise RuntimeError ("-: stdout is not support for generation of multiple files")
  idlfiles = config['files']
  if len (idlfiles) != 1:
    raise RuntimeError ("PyxxStub: exactly one IDL input file is required")
  gg = Generator (idlfiles[0], outname)
  for opt in config['backend-options']:
    if opt.startswith ('strip-path='):
      gg.strip_path += opt[11:]
  fname = outname
  fout = open (fname, 'w')
  textstring = gg.generate_types_pyxx (config['implementation_types'])
  fout.write (textstring)
  fout.close()

# register extension hooks
__Aida__.add_backend (__file__, generate, __doc__)
