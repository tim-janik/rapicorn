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


# == Utilities from pyxxutils.hh ==
cdef extern from "pyxxutils.hh":
  cppclass PyxxCaller0[R]:
    PyxxCaller0 (object, R (*M) (object))
  cppclass PyxxCaller1[R,A1]:
    PyxxCaller1 (object, R (*M) (object, A1))
  cppclass PyxxBoolFunctor                      "std::function<bool()>"
  void                pyxx_main_loop_add_watchdog (Rapicorn__MainLoop&)
  Rapicorn__MainLoop* dynamic_cast_MainLoopPtr "dynamic_cast<Rapicorn::MainLoop*>" (Rapicorn__EventLoop*) except NULL
