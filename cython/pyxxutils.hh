// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include <rapicorn-core.hh>

/* PyObject caller template to use a pycallable as C++ std::function with
 * varying numbers of arguments.
 * Keep the 'ifdef XMANIFOLD_SECTION..endif XMANIFOLD_SECTION' markers in place,
 * they are used by xmanifold.py to identify the template text.
 */
#ifdef  XMANIFOLD_SECTION
template<class R, class A1, class A2>
class PyxxCaller2 {
  typedef std::function<R (A1, A2)> StdFunction;
  typedef R (*Marshal) (PyObject*, A1, A2);
  Marshal   marshal_;
  PyObject *pyo_;
public:
  explicit PyxxCaller2 (PyObject *pyo, Marshal marshal) :
    marshal_ (marshal), pyo_ (pyo)
  {
    Py_INCREF (pyo_);
  }
  /*copy*/ PyxxCaller2 (const PyxxCaller2 &other)
  {
    marshal_ = other.marshal_;
    pyo_ = other.pyo_;
    Py_INCREF (pyo_);
  }
  void
  operator= (const PyxxCaller2 &other)
  {
    marshal_ = other.marshal_;
    if (other.pyo_)
      Py_INCREF (other.pyo_);
    Py_XDECREF (pyo_);
    pyo_ = other.pyo_;
  }
  ~PyxxCaller2()
  {
    Py_XDECREF (pyo_);
  }
  R
  operator() (A1 a1, A2 a2)
  {
    return marshal_ (pyo_, a1, a2);
  }
};
#endif  // XMANIFOLD_SECTION

#include "pyxxutils-xmany.hh" // generated from XMANIFOLD_SECTION

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

/* MainLoop watchdog source that quits the currently running loop upon a
 * Python exception.
 */
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

// Map from TypeHash to Python wrapper ctor.
template<class Class> static Class*
pyxx_aida_type_class_mapper (const Rapicorn::Aida::TypeHash &thash, Class *ptr)
{
  typedef Rapicorn::Aida::TypeHash TypeHash;
  static std::map<TypeHash, Class*> tmap;
  Class *current = tmap[thash];
  if (ptr)    // setter
    {
      tmap[thash] = ptr;
      return NULL;
    }
  else          // getter
    {
      ptr = current ? current : NULL;
      return ptr;     // return new reference
    }
}
