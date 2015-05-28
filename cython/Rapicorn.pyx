# This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0   -*-mode:python;-*-
from libcpp cimport *
from cython.operator cimport dereference as deref
ctypedef unsigned int uint

# std::shared_ptr
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

# rapicorn-core.hh declarations
cdef extern from "rapicorn-core.hh" namespace "Rapicorn":
  # EventSource class
  cppclass Rapicorn__EventSource                "Rapicorn::EventLoop::Source"
  # EventLoop class
  cppclass Rapicorn__EventLoop                  "Rapicorn::EventLoop"
  cppclass Rapicorn__EventLoop:
    uint add             (shared_ptr[Rapicorn__EventSource], int)
    void wakeup          ()
    void remove          (uint)
    bool try_remove      (uint)
    void destroy_loop    ()
    bool has_primary     ()
    bool flag_primary    (bool)
    uint exec_timer      (uint timeout_ms, PyxxBoolFunctor, int priority)
    uint exec_now        (PyxxBoolFunctor)
  int Rapicorn__EventLoop__PRIORITY_LOW         "Rapicorn::EventLoop::PRIORITY_LOW"
  int Rapicorn__EventLoop__PRIORITY_NOW         "Rapicorn::EventLoop::PRIORITY_NOW"
  int Rapicorn__EventLoop__PRIORITY_ASCENT      "Rapicorn::EventLoop::PRIORITY_ASCENT"
  int Rapicorn__EventLoop__PRIORITY_HIGH        "Rapicorn::EventLoop::PRIORITY_HIGH"
  int Rapicorn__EventLoop__PRIORITY_NEXT        "Rapicorn::EventLoop::PRIORITY_NEXT"
  int Rapicorn__EventLoop__PRIORITY_NORMAL      "Rapicorn::EventLoop::PRIORITY_NORMAL"
  int Rapicorn__EventLoop__PRIORITY_UPDATE      "Rapicorn::EventLoop::PRIORITY_UPDATE"
  int Rapicorn__EventLoop__PRIORITY_IDLE        "Rapicorn::EventLoop::PRIORITY_IDLE"
  # MainLoop
  cppclass Rapicorn__MainLoop                   "Rapicorn::MainLoop"
  cppclass Rapicorn__MainLoop (Rapicorn__EventLoop):
    int                                 run             () except *
    bool                                running         () except *
    bool                                finishable      () except *
    void                                quit_ "quit"    (int quit_code) except *
    bool                                pending         () except *
    bool                                iterate         (bool blocking) except *
    void                                iterate_pending () except *
    shared_ptr[Rapicorn__EventLoop]     create_slave ()
  shared_ptr[Rapicorn__MainLoop] MainLoop__create "Rapicorn::MainLoop::create" ()

