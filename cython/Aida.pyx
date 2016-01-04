# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0   -*-mode:python;-*-

from libc.stdint cimport *
from libcpp cimport *
from libcpp.string cimport string as String
from libcpp.vector cimport vector
from cython.operator cimport dereference as _dereference
from cpython.object cimport Py_LT, Py_LE, Py_EQ, Py_NE, Py_GT, Py_GE


# == Standard Typedefs ==
#ctypedef int32_t int
ctypedef uint32_t uint
ctypedef int32_t  int32
ctypedef uint32_t uint32
ctypedef int64_t  int64
ctypedef uint64_t uint64
ctypedef double   float64

# == Utilities from Python ==
cdef extern from "":
  char* Py_GetProgramName ()

# == Utilities from std:: ==
cdef extern from "memory" namespace "std":
  cppclass shared_ptr[T]:
    shared_ptr     ()
    shared_ptr     (T)
    shared_ptr     (shared_ptr[T]&)
    void reset     ()
    void swap      (shared_ptr&)
    long use_count () const
    bool unique    () const
    T*   get       ()


# == Utilities for richcmp ==
cdef inline int richcmp_op (ssize_t cmpv, int op): # cmpv==0 euqals, cmpv<0 lesser, cmpv>0 greater
  if   op == Py_LT: return cmpv <  0            # <     0
  elif op == Py_LE: return cmpv <= 0            # <=    1
  elif op == Py_EQ: return cmpv == 0            # ==    2
  elif op == Py_NE: return cmpv != 0            # !=    3
  elif op == Py_GT: return cmpv >  0            # >     4
  elif op == Py_GE: return cmpv >= 0            # >=    5

cdef inline int usize_richcmp (size_t s1, size_t s2, int op):
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
      except StopIteration: return usize_richcmp (1, 2, op) # other has more elements
      if v == w:
        continue                        # inspect other elements
      if op == Py_EQ: return False      # ==
      if op == Py_NE: return True       # !=
      # Py_LT, Py_LE, Py_GT, Py_GE
      return _richcmp (v, w, op)        # < <= > >=
    try:
      v = it.next()
      return usize_richcmp (2, 1, op) # other has less elements
    except StopIteration: pass
    return usize_richcmp (0, 0, op)   # lengths match


# == Signal Connection Wrapper ==
cdef class PyxxSignalConnector:
  cdef object connector
  cdef object disconnector
  def __init__ (self, connector, disconnector):
    self.connector = connector
    self.disconnector = disconnector
  def connect (self, pycallable):
    return self.connector (pycallable)
  def disconnect (self, callable_id):
    return self.disconnector (callable_id)


# == Aida Helpers ==
cdef extern from *:
  cppclass Aida__TypeHash         "Rapicorn::Aida::TypeHash"
  cppclass Aida__TypeHash:
    uint64 typehi
    uint64 typelo
    Aida__TypeHash()
    Aida__TypeHash (uint64, uint64)
    String to_string() const
    bool __eq__   "Rapicorn::Aida::TypeHash::operator==" (const Aida__TypeHash&)
  ctypedef vector[Aida__TypeHash] Aida__TypeHashList "Rapicorn::Aida::TypeHashList"


# == Utilities from aidapyxxutils.hh ==
cdef extern from "aidapyxxutils.hh":
  void pyxx_main_loop_add_watchdog (Rapicorn__MainLoop&)

  cppclass PyxxCaller0[R]:
    PyxxCaller0 (object, R (*M) (object) except *)
  cppclass PyxxCaller1[R, A1]:
    PyxxCaller1 (object, R (*M) (object, A1) except *)
  cppclass PyxxCaller2[R, A1, A2]:
    PyxxCaller2 (object, R (*M) (object, A1, A2) except *)
  cppclass PyxxCaller3[R, A1, A2, A3]:
    PyxxCaller3 (object, R (*M) (object, A1, A2, A3) except *)
  cppclass PyxxCaller4[R, A1, A2, A3, A4]:
    PyxxCaller4 (object, R (*M) (object, A1, A2, A3, A4) except *)
  cppclass PyxxCaller5[R, A1, A2, A3, A4, A5]:
    PyxxCaller5 (object, R (*M) (object, A1, A2, A3, A4, A5) except *)
  cppclass PyxxCaller6[R, A1, A2, A3, A4, A5, A6]:
    PyxxCaller6 (object, R (*M) (object, A1, A2, A3, A4, A5, A6) except *)
  cppclass PyxxCaller7[R, A1, A2, A3, A4, A5, A6, A7]:
    PyxxCaller7 (object, R (*M) (object, A1, A2, A3, A4, A5, A6, A7) except *)
  cppclass PyxxCaller8[R, A1, A2, A3, A4, A5, A6, A7, A8]:
    PyxxCaller8 (object, R (*M) (object, A1, A2, A3, A4, A5, A6, A7, A8) except *)
  cppclass PyxxCaller9[R, A1, A2, A3, A4, A5, A6, A7, A8, A9]:
    PyxxCaller9 (object, R (*M) (object, A1, A2, A3, A4, A5, A6, A7, A8, A9) except *)
  cppclass PyxxCaller10[R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10]:
    PyxxCaller10 (object, R (*M) (object, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10) except *)
  cppclass PyxxCaller11[R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11]:
    PyxxCaller11 (object, R (*M) (object, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11) except *)
  cppclass PyxxCaller12[R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12]:
    PyxxCaller12 (object, R (*M) (object, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12) except *)
  cppclass PyxxCaller13[R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13]:
    PyxxCaller13 (object, R (*M) (object, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13) except *)
  cppclass PyxxCaller14[R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14]:
    PyxxCaller14 (object, R (*M) (object, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14) except *)
  cppclass PyxxCaller15[R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15]:
    PyxxCaller15 (object, R (*M) (object, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15) except *)
  cppclass PyxxCaller16[R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16]:
    PyxxCaller16 (object, R (*M) (object, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16) except *)
  cppclass PyxxCaller17[R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17]:
    PyxxCaller17 (object, R (*M) (object, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17) except *)
  cppclass PyxxCaller18[R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18]:
    PyxxCaller18 (object, R (*M) (object, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18) except *)


# == Utilities from Rapicorn core ==
include "rcore.pyx"
