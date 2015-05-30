# This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0   -*-mode:python;-*-

from libc.stdint cimport *
from libcpp.string cimport string as String
from libcpp.vector cimport vector
from cython.operator cimport dereference as deref
from cpython.object cimport Py_LT, Py_LE, Py_EQ, Py_NE, Py_GT, Py_GE

# == Standard Typedefs ==
#ctypedef int32_t int
ctypedef uint32_t uint
ctypedef int32_t  int32
ctypedef uint32_t uint32
ctypedef int64_t  int64
ctypedef uint64_t uint64


# == Utilities for richcmp ==
cdef inline int richcmp_op (ssize_t cmpv, int op): # cmpv==0 euqals, cmpv<0 lesser, cmpv>0 greater
  if   op == Py_LT: return cmpv <  0            # <     0
  elif op == Py_LE: return cmpv <= 0            # <=    1
  elif op == Py_EQ: return cmpv == 0            # ==    2
  elif op == Py_NE: return cmpv != 0            # !=    3
  elif op == Py_GT: return cmpv >  0            # >     4
  elif op == Py_GE: return cmpv >= 0            # >=    5

cdef inline int ssize_richcmp (ssize_t s1, ssize_t s2, int op):
  cdef ssize_t cmpv = -1 if s1 < s2 else s1 > s2
  return richcmp_op (cmpv, op)

def _richcmp (object1, object2, op):
  if   op == 0: return object1 <  object2
  elif op == 1: return object1 <= object2
  elif op == 2: return object1 == object2
  elif op == 3: return object1 != object2
  elif op == 4: return object1 >  object2
  elif op == 5: return object1 >= object2


# == Enum Base ==
# Enum base class, modeled after enum.Enum from Python3.4.
# An enum class has a __members__ dict and supports Enum['VALUE_NAME'].
# Each enum value has 'name' and 'value' attributes.
class EnumMeta (type):
  def __init__ (klass, name, bases, dict):
    import collections
    super (EnumMeta, klass).__init__ (name, bases, dict)
    odict = collections.OrderedDict()
    for evname in dict.keys():
      if not (evname.startswith ('__') and evname.endswith ('__')):
        enumvalue = EnumValue (name, evname, dict[evname])
        setattr (klass, evname, enumvalue)
        odict[evname] = enumvalue
    if odict:
      klass.__members__ = odict
  def __getattr__ (klass, name):
    if name == '__members__':
      return klass.__members__
    raise AttributeError (name)
  def __delattr__ (klass, name):
    raise AttributeError (name)
  def __setattr__ (klass, name, val):
    if hasattr (klass, '__members__'):
      raise AttributeError (name)
    else:
      super (EnumMeta, klass).__setattr__ (name, val)
  def __getitem__ (klass, name):
    return klass.__members__[name]

class Enum:
  __metaclass__ = EnumMeta

class EnumValue (long):
  def __new__ (klass, classname, enumname, value):
    return long.__new__ (klass, value)
  def __init__ (self, classname, enumname, value):
    self.__classname = classname
    self.name = enumname
    self.value = value
  def __repr__ (self):
    return "<%s.%s: %d>" % (self.__classname, self.name, self.value)
  def __str__ (self):
    return "%s.%s" % (self.__classname, self.name)

# == Record Base ==
# Rich Record type with keyword initialisation, iteration, indexing and dict conversion.
cdef class Record:
  def __init__ (self, **kwargs):
    for item in kwargs.items():
      setattr (self, item[0], item[1])
  def __getitem__ (self, key):
    return getattr (self, self._fields[key])
  def __iter__ (self):
    for fname in self._fields:
      yield getattr (self, fname)
  def _asdict (self):
    """Return an OrderedDict which maps field names to their values"""
    import collections
    return collections.OrderedDict (zip (self._fields, self))
  def __richcmp__ (Record self, other, int op):
    """Provides the '<', '<=', '==', '!=', '>', '>=' operators"""
    it = self.__iter__()
    for w in other:
      try:                  v = it.next()
      except StopIteration: return ssize_richcmp (1, 2, op) # other has more elements
      if v == w:
        continue                        # inspect other elements
      if op == Py_EQ: return False      # ==
      if op == Py_NE: return True       # !=
      # Py_LT, Py_LE, Py_GT, Py_GE
      return _richcmp (v, w, op)        # < <= > >=
    try:
      v = it.next()
      return ssize_richcmp (2, 1, op) # other has less elements
    except StopIteration: pass
    return ssize_richcmp (0, 0, op)   # lengths match


# == Utilities from pyxxutils.hh ==
cdef extern from "pyxxutils.hh":
  cppclass PyxxCaller0[R]:
    PyxxCaller0 (object, R (*M) (object))
  cppclass PyxxCaller1[R,A1]:
    PyxxCaller1 (object, R (*M) (object, A1))
  cppclass PyxxBoolFunctor                      "std::function<bool()>"
  void                pyxx_main_loop_add_watchdog (Rapicorn__MainLoop&)
  Rapicorn__MainLoop* dynamic_cast_MainLoopPtr "dynamic_cast<Rapicorn::MainLoop*>" (Rapicorn__EventLoop*) except NULL
