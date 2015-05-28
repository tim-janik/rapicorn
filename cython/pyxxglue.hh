// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#include <rapicorn-core.hh>

template<bool FALLBACK>
struct PyxxFunctorClosure {
  PyObject *pycallable_;
  PyxxFunctorClosure (const PyxxFunctorClosure &other)
  {
    pycallable_ = other.pycallable_;
    if (pycallable_)
      Py_INCREF (pycallable_);
  }
  void
  operator= (const PyxxFunctorClosure &other)
  {
    if (other.pycallable_)
      Py_INCREF (other.pycallable_);
    Py_XDECREF (pycallable_);
    pycallable_ = other.pycallable_;
  }
  PyxxFunctorClosure (PyObject *pycallable)
  {
    pycallable_ = pycallable;
    if (pycallable_)
      Py_INCREF (pycallable_);
  }
  bool
  operator() ()
  {
    bool keep_alive = FALLBACK; // value returned on exceptions
    const uint length = 0;
    PyObject *tuple = PyTuple_New (length);
    if (!PyErr_Occurred())
      {
        PyObject *pyresult = PyObject_Call (pycallable_, tuple, NULL);
        if (!PyErr_Occurred())
          keep_alive = pyresult && pyresult != Py_None && PyObject_IsTrue (pyresult);
        Py_XDECREF (pyresult);
      }
    Py_XDECREF (tuple);
    return keep_alive;
  }
  ~PyxxFunctorClosure()
  {
    Py_XDECREF (pycallable_);
  }
};

static std::function<bool()>
pyxx_make_bool_functor (PyObject *pycallable, bool fallback)
{
  if (!PyCallable_Check (pycallable))
    {
      PyErr_Format (PyExc_TypeError, "'%s' object is not callable", Py_TYPE (pycallable)->tp_name);
      return std::function<bool()>();
    }
  if (fallback)
    return PyxxFunctorClosure<1> (pycallable);
  else
    return PyxxFunctorClosure<0> (pycallable);
}

static void
pyxx_main_loop_add_watchdog (Rapicorn::MainLoop &main_loop)
{
  auto exception_watchdog = [&main_loop] (const Rapicorn::LoopState &state) {
    if (PyErr_Occurred())
      main_loop.quit (-128);
    if (state.phase == state.DISPATCH)
      return true; // keep alive
    else // state.PREPARE || state.CHECK || state.DESTROY
      return false; // nothing to do
  };
  main_loop.exec_dispatcher (exception_watchdog, Rapicorn::EventLoop::PRIORITY_CEILING);
}
