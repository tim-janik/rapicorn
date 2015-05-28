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

# include pyxxglue.hh
cdef extern from "pyxxglue.hh":
  # Callback wrappers
  cppclass PyxxBoolFunctor                      "std::function<bool()>"
  PyxxBoolFunctor pyxx_make_bool_functor      (object pycallable, bool fallback) except *
  void            pyxx_main_loop_add_watchdog (Rapicorn__MainLoop&)

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

# rapicorn-core.hh wrappers

cdef class EventSource:
  cdef shared_ptr[Rapicorn__EventSource] thisp
  def __cinit__ (self, do_not_construct_manually = False):
    assert (do_not_construct_manually)

cdef shared_ptr[Rapicorn__EventLoop] *EventLoop__internal_ctor

cdef class EventLoop:
  cdef shared_ptr[Rapicorn__EventLoop] thisp    # wrapped C++ instance
  def __init__ (self):
    global EventLoop__internal_ctor
    assert (EventLoop__internal_ctor != NULL)
    EventLoop__internal_ctor.swap (self.thisp)
    del EventLoop__internal_ctor
    EventLoop__internal_ctor = NULL
  property PRIORITY_LOW:
    def __get__ (self):  return Rapicorn__EventLoop__PRIORITY_LOW
  property PRIORITY_NOW:
    def __get__ (self):  return Rapicorn__EventLoop__PRIORITY_NOW
  property PRIORITY_ASCENT:
    def __get__ (self):  return Rapicorn__EventLoop__PRIORITY_ASCENT
  property PRIORITY_HIGH:
    def __get__ (self):  return Rapicorn__EventLoop__PRIORITY_HIGH
  property PRIORITY_NEXT:
    def __get__ (self):  return Rapicorn__EventLoop__PRIORITY_NEXT
  property PRIORITY_NORMAL:
    def __get__ (self):  return Rapicorn__EventLoop__PRIORITY_NORMAL
  property PRIORITY_UPDATE:
    def __get__ (self):  return Rapicorn__EventLoop__PRIORITY_UPDATE
  property PRIORITY_IDLE:
    def __get__ (self):  return Rapicorn__EventLoop__PRIORITY_IDLE
  def add (self, EventSource source, priority = Rapicorn__EventLoop__PRIORITY_NORMAL):
    if source.thisp.get() == NULL:
      raise ValueError ("Argument '%s' required to be non empty" % "source")
    return self.thisp.get().add (source.thisp, priority)
  def wakeup (self):                    self.thisp.get().wakeup()
  def remove (self, id):                self.thisp.get().remove (id)
  def try_remove (self, id):            return self.thisp.get().try_remove (id)
  def destroy_loop (self):              self.thisp.get().destroy_loop()
  def has_primary (self):               return self.thisp.get().has_primary()
  def flag_primary (self, on):          return self.thisp.get().flag_primary (on)
  # note, we use bool_functor with fallback=1 to keep handlers connected across exceptions
  def exec_timer (self, timeout_ms, callable, priority = Rapicorn__EventLoop__PRIORITY_NORMAL):
    return self.thisp.get().exec_timer (timeout_ms, pyxx_make_bool_functor (callable, 1), priority)
  def exec_now (self, callable):
    return self.thisp.get().exec_now (pyxx_make_bool_functor (callable, 1))

cdef class MainLoop (EventLoop):
  cdef Rapicorn__MainLoop *mainp                # wrapped C++ instance
  def __init__ (self):
    global EventLoop__internal_ctor
    cdef shared_ptr[Rapicorn__MainLoop] mlptr = MainLoop__create()
    EventLoop__internal_ctor = new shared_ptr[Rapicorn__EventLoop] (<shared_ptr[Rapicorn__EventLoop]> mlptr)
    super (MainLoop, self).__init__()           # EventLoop.__init__
    self.mainp = mlptr.get()                    # MainLoop*
    pyxx_main_loop_add_watchdog (deref (self.mainp))
  def __dealloc__ (self):
    self.thisp.reset()
  def run (self):                       return self.mainp.run()
  def running (self):                   return self.mainp.running()
  def finishable (self):                return self.mainp.finishable()
  def quit (self, quit_code = 0):       self.mainp.quit_ (quit_code)
  def pending (self):                   return self.mainp.pending()
  def iterate (self, blocking):         return self.mainp.iterate (blocking)
  def iterate_pending (self):           self.mainp.iterate_pending()
  def create_slave (self):
      global EventLoop__internal_ctor
      EventLoop__internal_ctor = new shared_ptr[Rapicorn__EventLoop] (self.mainp.create_slave())
      return EventLoop()
