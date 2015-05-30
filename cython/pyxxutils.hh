// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#include <rapicorn-core.hh>

template<class R>
class PyxxCaller0 {
  typedef std::function<R ()> StdFunction;
  typedef R (*Marshal) (PyObject*);
  Marshal   marshal_;
  PyObject *pyo_;
public:
  explicit PyxxCaller0 (PyObject *pyo, Marshal marshal) :
    marshal_ (marshal), pyo_ (pyo)
  {
    Py_INCREF (pyo_);
  }
  /*copy*/ PyxxCaller0 (const PyxxCaller0 &other)
  {
    marshal_ = other.marshal_;
    pyo_ = other.pyo_;
    Py_INCREF (pyo_);
  }
  void
  operator= (const PyxxCaller0 &other)
  {
    marshal_ = other.marshal_;
    if (other.pyo_)
      Py_INCREF (other.pyo_);
    Py_XDECREF (pyo_);
    pyo_ = other.pyo_;
  }
  ~PyxxCaller0()
  {
    Py_XDECREF (pyo_);
  }
  R
  operator() ()
  {
    return marshal_ (pyo_);
  }
};

/* PyxxCaller0<bool> is used for main loop handlers that will be disconnected if 'false'
 * is returned. When Python exceptions are thrown, the 'marshal_' default behaviour is
 * to return '0', but we want to keep main loop handlers alive, so we specialize here.
 */
template<> bool
PyxxCaller0<bool>::operator() ()
{
  bool result = marshal_ (pyo_);
  if (PyErr_Occurred())
    result = true;
  return result;
}


template<class R, class A1>
class PyxxCaller1 {
  typedef std::function<R (A1)> StdFunction;
  typedef R (*Marshal) (PyObject*, A1);
  Marshal   marshal_;
  PyObject *pyo_;
public:
  explicit PyxxCaller1 (PyObject *pyo, Marshal marshal) :
    marshal_ (marshal), pyo_ (pyo)
  {
    Py_INCREF (pyo_);
  }
  /*copy*/ PyxxCaller1 (const PyxxCaller1 &other)
  {
    marshal_ = other.marshal_;
    pyo_ = other.pyo_;
    Py_INCREF (pyo_);
  }
  void
  operator= (const PyxxCaller1 &other)
  {
    marshal_ = other.marshal_;
    if (other.pyo_)
      Py_INCREF (other.pyo_);
    Py_XDECREF (pyo_);
    pyo_ = other.pyo_;
  }
  ~PyxxCaller1()
  {
    Py_XDECREF (pyo_);
  }
  R
  operator() (A1 a1)
  {
    return marshal_ (pyo_, a1);
  }
};


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
