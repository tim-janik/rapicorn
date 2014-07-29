#include <Python.h> // must be included first to configure std headers
#include <string>

#include <rapicorn.hh>

#define None_INCREF()   ({ Py_INCREF (Py_None); Py_None; })
#define GOTO_ERROR()    goto error
#define ERRORif(cond)   if (cond) goto error
#define ERRORifpy()     if (PyErr_Occurred()) goto error
#define ERRORpy(msg)    do { PyErr_Format (PyExc_RuntimeError, msg); goto error; } while (0)
#define ERRORifnotret(fr) do { if (AIDA_UNLIKELY (!fr) || \
                                   AIDA_UNLIKELY (!Rapicorn::Aida::msgid_is_result (Rapicorn::Aida::MessageId (fr->first_id())))) { \
                                 PyErr_Format_from_AIDA_error (fr); \
                                 goto error; } } while (0)
#define ERROR_callable_ifpy(callable) do { if (PyErr_Occurred()) { \
  PyObject *t, *v, *tb; \
  PyErr_Fetch (&t, &v, &tb); \
  PyObject *sc = PyObject_Repr (callable), *sv = PyObject_Str (v); \
  PyErr_Format (PyExceptionClass_Check (t) ? t : PyExc_RuntimeError, \
                "in return from %s: %s", PyString_AsString (sc), PyString_AsString (sv)); \
  Py_XDECREF (sc); Py_XDECREF (sv); Py_XDECREF (t); Py_XDECREF (v); Py_XDECREF (tb); \
  goto error; \
} } while (0)

#ifndef __AIDA_PYMODULE__OBJECT
#define __AIDA_PYMODULE__OBJECT ((PyObject*) NULL)
#endif

using Rapicorn::Aida::uint64;
using Rapicorn::Aida::FieldBuffer;
using Rapicorn::Aida::FieldReader;
using Rapicorn::Aida::RemoteHandle;

// connection
static Rapicorn::Aida::ClientConnection *__AIDA_local__client_connection = NULL;

// connection initialization
static Rapicorn::Init __AIDA_init__client_connection ([]() {
  __AIDA_local__client_connection = Rapicorn::Aida::ObjectBroker::new_client_connection ($AIDA_pyclient_feature_keys$);
});

// helpers
static inline void
add_header1_call (FieldBuffer &fb, const RemoteHandle &sh, uint64 h, uint64 l)
{
  fb.add_header1 (Rapicorn::Aida::MSGID_ONEWAY_CALL, Rapicorn::Aida::ObjectBroker::connection_id_from_handle (sh), h, l);
}

static inline void
add_header2_call (FieldBuffer &fb, const RemoteHandle &sh, uint64 h, uint64 l)
{
  fb.add_header2 (Rapicorn::Aida::MSGID_TWOWAY_CALL, Rapicorn::Aida::ObjectBroker::connection_id_from_handle (sh),
                  __AIDA_local__client_connection->connection_id(), h, l);
}

static PyObject*
PyErr_Format_from_AIDA_error (const FieldBuffer *fr)
{
  if (!fr)
    return PyErr_Format (PyExc_RuntimeError, "Aida: missing return value");
  FieldReader frr (*fr);
  const uint64 msgid AIDA_UNUSED = frr.pop_int64();
  frr.skip(); frr.skip(); // hashhigh hashlow
#if 0
  if (Rapicorn::Aida::msgid_is_error (Rapicorn::Aida::MessageId (msgid)))
    {
      std::string msg = frr.pop_string(), domain = frr.pop_string();
      if (domain.size()) domain += ": ";
      msg = domain + msg;
      return PyErr_Format (PyExc_RuntimeError, "%s", msg.c_str());
    }
#endif
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

static inline Rapicorn::Aida::uint64
PyAttr_As_uint64 (PyObject *pyobj, const char *attr_name)
{
  PyObject *pyo = PyObject_GetAttrString (pyobj, attr_name);
  Rapicorn::Aida::uint64 uval = pyo ? PyLong_AsUnsignedLongLong (pyo) : 0;
  Py_XDECREF (pyo);
  return uval;
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
      Rapicorn::string_format ("no known conversion to Aida::Any for Python object: %s", PyObject_REPR (pyvalue));
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
    Rapicorn::string_format ("no known conversion for Aida::%s to Python", Rapicorn::Aida::type_kind_name (any.kind()));
  PyErr_SetString (PyExc_NotImplementedError, msg.c_str());
  return NULL;
}

static PyObject *__AIDA_pyfactory__create_pyobject__ = NULL;

static PyObject*
__AIDA_pyfactory__register_callback (PyObject *pyself, PyObject *pyargs)
{
  if (__AIDA_pyfactory__create_pyobject__)
    return PyErr_Format (PyExc_RuntimeError, "object_factory_callable already registered");
  if (PyTuple_Size (pyargs) != 1)
    return PyErr_Format (PyExc_RuntimeError, "wrong number of arguments");
  PyObject *item = PyTuple_GET_ITEM (pyargs, 0);
  if (!PyCallable_Check (item))
    return PyErr_Format (PyExc_RuntimeError, "argument must be callable");
  Py_INCREF (item);
  __AIDA_pyfactory__create_pyobject__ = item;
  return None_INCREF();
}

static inline PyObject*
__AIDA_pyfactory__create_enum (const char *enum_name, uint64 enum_value)
{
  PyObject *result = NULL, *pyid;
  if (strchr (enum_name, ':'))
    goto unimplemented;
  if (!__AIDA_pyfactory__create_pyobject__)
    return PyErr_Format (PyExc_RuntimeError, "unregistered AIDA_pyfactory");
  pyid = PyLong_FromUnsignedLongLong (enum_value);
  if (pyid)
    {
      PyObject *tuple = PyTuple_New (2);
      if (tuple)
        {
          PyTuple_SET_ITEM (tuple, 0, PyString_FromString (enum_name));
          PyTuple_SET_ITEM (tuple, 1, pyid), pyid = NULL;
          result = PyObject_Call (__AIDA_pyfactory__create_pyobject__, tuple, NULL);
          Py_DECREF (tuple);
        }
      Py_XDECREF (pyid);
    }
  return result;
 unimplemented:
  Rapicorn::Aida::fatal_error (std::string() + "UNIMPLEMENTED: FIXME: missing handling of typenames outside the Rapicorn namespace: " + enum_name);
}

static inline PyObject*
__AIDA_pyfactory__create_handle (RemoteHandle remote, const char *forced_type_name = NULL)
{
  if (!remote) // null handle
    return None_INCREF();
  PyObject *result = NULL, *pyid;
  std::string type_name;
  if (forced_type_name)
    type_name = forced_type_name;
  else
    {
      const std::string fqtn = __AIDA_local__client_connection->type_name_from_orbid (remote._orbid());
      type_name = fqtn;
      size_t p = type_name.find ("::");
      while (p != std::string::npos)
        {
          type_name.replace (p, 2, ".");        // Foo::Bar::baz -> Foo.Bar.baz
          p = type_name.find ("::", p + 1);
        }
    }
  if (!__AIDA_pyfactory__create_pyobject__)
    return PyErr_Format (PyExc_RuntimeError, "unregistered AIDA_pyfactory");
  pyid = PyLong_FromUnsignedLongLong (0);       // long slot used for enum values
  if (pyid)
    {
      PyObject *tuple = PyTuple_New (2);
      if (tuple)
        {
          PyTuple_SET_ITEM (tuple, 0, PyString_FromString (type_name.c_str()));
          PyTuple_SET_ITEM (tuple, 1, pyid), pyid = NULL;
          py_remote_handle_push (remote);
          result = PyObject_Call (__AIDA_pyfactory__create_pyobject__, tuple, NULL);
          py_remote_handle_pop();
          Py_DECREF (tuple);
        }
      Py_XDECREF (pyid);
    }
  return result;
}
