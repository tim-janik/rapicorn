// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html

#include "uithread.hh"
#include <semaphore.h>
#include <deque>
#include <set>
#include <stdlib.h>

namespace { // Anon
static void wrap_test_runner  (void);
} // Anon

namespace Rapicorn {

class Channel { // Channel for cross-thread FieldBuffer IO
  pthread_spinlock_t        msg_spinlock;
  std::vector<FieldBuffer*> msg_vector;
  std::vector<FieldBuffer*> msg_queue;
  size_t                    msg_index, msg_last_size;
  virtual void  data_notify () = 0;
  virtual void  data_wait   () = 0;
  virtual void  flush_waits () = 0;
public:
  FieldBuffer*
  fetch_msg (const int blockpop)
  {
    do
      {
        // fetch new messages
        if (msg_index >= msg_queue.size())      // no buffered messages left
          {
            if (msg_index)
              msg_queue.resize (0);             // get rid of stale pointers
            msg_index = 0;
            flush_waits();
            pthread_spin_lock (&msg_spinlock);
            msg_vector.swap (msg_queue);        // actual cross-thread fetching
            pthread_spin_unlock (&msg_spinlock);
          }
        // hand out message
        if (msg_index < msg_queue.size())       // have buffered messages
          {
            const size_t index = msg_index;
            if (blockpop > 0) // advance
              msg_index++;
            return msg_queue[index];
          }
        // no messages available
        if (blockpop < 0)
          data_wait();
      }
    while (blockpop < 0);
    return NULL;
  }
  Channel () :
    msg_index (0), msg_last_size (0)
  {
    pthread_spin_init (&msg_spinlock, 0 /* pshared */);
  }
  ~Channel ()
  {
    pthread_spin_destroy (&msg_spinlock);
  }
  void
  push_msg (FieldBuffer *fb) // takes fb ownership
  {
    pthread_spin_lock (&msg_spinlock);
    msg_vector.push_back (fb);
    msg_last_size = msg_vector.size();
    pthread_spin_unlock (&msg_spinlock);
    data_notify();
  }
  bool          has_msg          () { return fetch_msg (0) != NULL; }
  FieldBuffer*  pop_msg          () { wait_msg(); return fetch_msg (1); } // passes fbmsg ownership
  void          wait_msg         () { while (!fetch_msg (-1)); }
  FieldBuffer*  pop_msg_ndelay   () { return fetch_msg (1); } // passes fbmsg ownership
};

class ChannelS : public Channel { // Channel with semaphore for syncronization
  sem_t         msg_sem;
  virtual void  data_notify ()  { sem_post (&msg_sem); }
  virtual void  data_wait   ()  { sem_wait (&msg_sem); }
  virtual void  flush_waits ()  { /*sem_trywait (&msg_sem);*/ }
public:
  explicit      ChannelS    ()  { sem_init (&msg_sem, 0 /* unshared */, 0 /* init */); }
  /*dtor*/     ~ChannelS    ()  { sem_destroy (&msg_sem); }
};

class ChannelE : public Channel, public EventFd { // Channel with EventFd for syncronization
  virtual void  data_notify ()  { wakeup(); }
  virtual void  data_wait   ()  { pollin(); }
  virtual void  flush_waits ()  { flush(); }
public:
  ChannelE ()
  {
    if (open() < 0)
      pfatal ("failed to open pipe for thread communication");
  }
};

struct ConnectionSource : public virtual EventLoop::Source, public virtual Plic::Connection {
  ConnectionSource (EventLoop &loop) :
    WHERE ("Rapicorn::UIThread::Connection")
  {
    primary (false);
    loop.add (this, EventLoop::PRIORITY_NORMAL);
    pollfd.fd = calls.inputfd();
    pollfd.events = PollFD::IN;
    pollfd.revents = 0;
    add_poll (&pollfd);
    pthread_spin_init (&ehandler_spin, 0 /* pshared */);
  }
  void          wakeup   ()               { calls.wakeup(); } // allow external wakeups
private:
  const char                   *WHERE;
  PollFD                        pollfd;
  ChannelE                      events, calls;
  ChannelS                      results;
  std::vector<FieldBuffer*>     queue;
  std::set<uint64>              ehandler_set;
  pthread_spinlock_t            ehandler_spin;
  ~ConnectionSource ()
  {
    remove_poll (&pollfd);
    loop_remove();
    pthread_spin_destroy (&ehandler_spin);
  }
  virtual bool  prepare  (uint64, int64*) { return check_dispatch(); }
  virtual bool  check    (uint64)         { return check_dispatch(); }
  virtual bool  dispatch ()               { dispatch1(); return true; }
  bool
  check_dispatch()
  {
    return calls.has_msg();
  }
  void
  dispatch1()
  {
    FieldBuffer *fb = calls.pop_msg_ndelay();
    if (fb)
      {
        FieldBuffer *fr = dispatch_call (fb);
        delete fb;
        if (fr)
          send_result (fr);
      }
  }
  FieldBuffer*
  dispatch_call (const FieldBuffer *fb)
  {
    FieldReader fbr (*fb);
    const Plic::MessageId msgid = Plic::MessageId (fbr.pop_int64());
    const uint64 hashlow = fbr.pop_int64();
    Plic::DispatchFunc method = find_method (msgid, hashlow);
    if (method)
      {
        FieldBuffer *fr = method (fbr);
        const bool needsresult = Plic::msgid_has_result (msgid);
        if (!fr && needsresult)
          fr = FieldBuffer::new_error (string_printf ("missing return from 0x%016lx", msgid), WHERE);
        const Plic::MessageId retid = Plic::MessageId (fr ? fr->first_id() : 0);
        if (fr && Plic::msgid_is_error (retid))
          ; // ok
        else if (fr && (!needsresult || !Plic::msgid_is_result (retid)))
          {
            critical ("%s: bogus result from method (%016lx%016llx): id=%016lx", WHERE, msgid, hashlow, retid);
            delete fr;
            fr = NULL;
          }
        return fr;
      }
    else
      return FieldBuffer::new_error (string_printf ("unknown method hash: (%016lx%016llx)", msgid, hashlow), WHERE);
  }
protected:
  virtual FieldBuffer*  fetch_event     (int blockpop)    { return events.fetch_msg (blockpop); }
  virtual int           event_inputfd   ()                { return events.inputfd(); }
  virtual void          send_event      (FieldBuffer *fb) { return_if_fail (fb != NULL); events.push_msg (fb); }
  virtual void          send_result     (FieldBuffer *fb) { return_if_fail (fb != NULL); results.push_msg (fb); }
  virtual FieldBuffer*
  call_remote (FieldBuffer *fb) ///< Called by clientapi from various threads
  {
    return_val_if_fail (fb != NULL, NULL);
    // enqueue method call message
    const Plic::MessageId msgid = Plic::MessageId (fb->first_id());
    calls.push_msg (fb);
    // wait for result
    if (Plic::msgid_has_result (msgid))
      {
        FieldBuffer *fr = results.pop_msg();
        Plic::MessageId retid = Plic::MessageId (fr->first_id());
        if (!Plic::msgid_is_result (retid) &&   // FIXME: check type
            !Plic::msgid_is_error (retid))
          {
            delete fr;
            fr = Plic::FieldBuffer::new_error (string_printf ("invalid result message id: 0x%160lx", retid), WHERE);
          }
        return fr;
      }
    return NULL;
  }
  virtual uint64
  register_event_handler (EventHandler *evh)
  {
    pthread_spin_lock (&ehandler_spin);
    const std::pair<std::set<uint64>::iterator,bool> ipair = ehandler_set.insert (ptrdiff_t (evh));
    pthread_spin_unlock (&ehandler_spin);
    return ipair.second ? ptrdiff_t (evh) : 0; // unique insertion
  }
  virtual EventHandler*
  find_event_handler (uint64 handler_id)
  {
    pthread_spin_lock (&ehandler_spin);
    std::set<uint64>::iterator iter = ehandler_set.find (handler_id);
    pthread_spin_unlock (&ehandler_spin);
    if (iter == ehandler_set.end())
      return NULL; // unknown handler_id
    EventHandler *evh = (EventHandler*) ptrdiff_t (handler_id);
    return evh;
  }
  virtual bool
  delete_event_handler (uint64 handler_id)
  {
    pthread_spin_lock (&ehandler_spin);
    size_t nerased = ehandler_set.erase (handler_id);
    pthread_spin_unlock (&ehandler_spin);
    return nerased > 0; // deletion successful?
  }
};

struct Initializer {
  int *argcp; char **argv; const StringVector *args;
  Mutex mutex; Cond cond; uint64 app_id;
};

class UIThread : public Thread {
  static UIThread  *the_uithread;
  Initializer      *m_init;
  ConnectionSource *m_connection;
  MainLoop         &m_main_loop;
public:
  UIThread (Initializer *init) :
    Thread ("Rapicorn::UIThread"), m_init (init), m_connection (NULL),
    m_main_loop (*ref_sink (MainLoop::_new()))
  {
    assert (the_uithread == NULL);
    the_uithread = ref_sink (this);
  }
  Plic::Connection* connection()  { return m_connection; }
  static UIThread*  uithread()    { return Atomic::ptr_get (&the_uithread); }
  MainLoop*         main_loop()   { return &m_main_loop; }
private:
  ~UIThread ()
  {
    assert (the_uithread != this);
    shutdown();
    unref (m_main_loop);
  }
  void
  shutdown()
  {
    ConnectionSource *con = m_connection;
    m_connection = NULL;
    if (con)
      {
        con->loop_remove();
        unref (con);
      }
    m_main_loop.kill_loops();
  }
  static void
  trigger_wakeup (void*)
  {
    if (the_uithread)
      the_uithread->m_main_loop.wakeup();
  }
  void
  initialize ()
  {
    return_if_fail (m_init != NULL);
    assert (rapicorn_thread_entered() == true);
    // init_core() already called
    affinity (string_to_int (string_vector_find (*m_init->args, "cpu-affinity=", "-1")));
    // initialize ui_thread loop before components
    m_connection = ref_sink (new ConnectionSource (m_main_loop));
    // initialize sub systems
    struct InitHookCaller : public InitHook {
      static void  invoke (const String &kind, int *argcp, char **argv, const StringVector &args)
      { invoke_hooks (kind, argcp, argv, args); }
    };
    // UI library core parts
    InitHookCaller::invoke ("ui-core/", m_init->argcp, m_init->argv, *m_init->args);
    // Application Singleton
    InitHookCaller::invoke ("ui-thread/", m_init->argcp, m_init->argv, *m_init->args);
    return_if_fail (NULL != &ApplicationImpl::the());
    // Initializations after Application Singleton
    InitHookCaller::invoke ("ui-app/", m_init->argcp, m_init->argv, *m_init->args);
    // Complete initialization by signalling caller
    m_init->mutex.lock();
    m_init->app_id = connection_object2id (ApplicationImpl::the());
    m_init->cond.signal();
    m_init->mutex.unlock();
    m_init = NULL;
  }
  virtual void
  run ()
  {
    rapicorn_thread_enter();
    Self::set_wakeup (trigger_wakeup, NULL, NULL); // allow external wakeups
    initialize();
    return_if_fail (m_init == NULL);
    while (!Thread::Self::aborted())            // m_main_loop.finishable()
      m_main_loop.iterate (true);               // iterate blocking
    shutdown();
    Self::set_wakeup (NULL, NULL, NULL);        // main loop canot be woken up further
    rapicorn_thread_leave();
  }
};
UIThread *UIThread::the_uithread = NULL;

MainLoop*
uithread_main_loop ()
{
  return UIThread::uithread() ? UIThread::uithread()->main_loop() : NULL;
}

static void
uithread_uncancelled_atexit()
{
  if (UIThread::uithread() && UIThread::uithread()->running())
    {
      /* For proper shutdown, the ui-thread needs to stop running before global
       * dtors or any atexit() handlers are being executed. C9x and C++03 leave
       * this unsolved, so we provide explicit API for the user, like
       * Rapicorn::exit() and Application::shutdown(). In case these are omitted,
       * we're not 100% safe, some earlier atexit() handler could have shot some
       * of our required resources already, however we do our best to start a
       * graceful shutdown at this point.
       */
      FIXME ("UI-Thread still running during exit(), call Application::shutdown()");
      shutdown_app();
    }
}

uint64
uithread_bootup (int *argcp, char **argv, const StringVector &args)
{
  return_val_if_fail (UIThread::uithread() == NULL, 0);
  atexit (uithread_uncancelled_atexit);
  wrap_test_runner();
  Initializer idata;
  idata.argcp = argcp; idata.argv = argv; idata.args = &args; idata.app_id = 0;
  UIThread *uithread = new UIThread (&idata);
  assert (uithread == UIThread::uithread());
  idata.mutex.lock();
  uithread->start();
  while (idata.app_id == 0)
    idata.cond.wait (idata.mutex);
  uint64 app_id = idata.app_id;
  idata.mutex.unlock();
  assert (UIThread::uithread() && UIThread::uithread()->running());
  serverglue_setup (uithread_connection());
  return app_id;
}

Plic::Connection*
uithread_connection (void)
{
  if (UIThread::uithread() && UIThread::uithread()->running())
    return UIThread::uithread()->connection();
  return NULL;
}

void // from clientapi.hh
shutdown_app (void)
{
  if (UIThread::uithread() && UIThread::uithread()->running())
    UIThread::uithread()->abort();
  assert (UIThread::uithread()->running() == false);
}

} // Rapicorn

#include <rcore/testutils.hh>

namespace { // Anon
using namespace Rapicorn;

// === UI-Thread Syscalls ===
struct Callable {
  virtual int64 operator() () = 0;
  virtual      ~Callable   () {}
};
static std::deque<Callable*> syscall_queue;
static Mutex                 syscall_mutex;

static Plic::FieldBuffer*
ui_thread_syscall_twoway (Plic::FieldReader &fbr)
{
  Plic::FieldBuffer &rb = *Plic::FieldBuffer::new_result();
  int64 result = -1;
  syscall_mutex.lock();
  while (syscall_queue.size())
    {
      Callable &callable = *syscall_queue.front();
      syscall_queue.pop_front();
      syscall_mutex.unlock();
      result = callable ();
      delete &callable;
      syscall_mutex.lock();
    }
  syscall_mutex.unlock();
  rb.add_int64 (result);
  return &rb;
}

static const Plic::Connection::MethodEntry ui_thread_call_entries[] = {
  { Plic::MSGID_TWOWAY | 0x0c0ffee01, 0x52617069636f726eULL, ui_thread_syscall_twoway, },
};
static Plic::Connection::MethodRegistry ui_thread_call_registry (ui_thread_call_entries);

static int64
ui_thread_syscall (Callable *callable)
{
  Plic::FieldBuffer *fb = Plic::FieldBuffer::_new (2);
  fb->add_msgid (Plic::MSGID_TWOWAY | 0x0c0ffee01, 0x52617069636f726eULL); // ui_thread_syscall_twoway
  syscall_mutex.lock();
  syscall_queue.push_back (callable);
  syscall_mutex.unlock();
  FieldBuffer *fr = UIThread::uithread()->connection()->call_remote (fb); // deletes fb
  Plic::FieldReader frr (*fr);
  const Plic::MessageId msgid = Plic::MessageId (frr.pop_int64());
  frr.skip(); // FIXME: check full msgid
  int64 result = 0;
  if (Plic::msgid_is_result (msgid))
    result = frr.pop_int64();
  delete fr;
  return result;
}

// === UI-Thread Test Runs ===
class SyscallTestRunner : public Callable {
  void (*test_runner) (void);
public:
  SyscallTestRunner (void (*func) (void)) : test_runner (func) {}
  int64 operator()  ()                    { test_runner(); return 0; }
};

static void
wrap_test_runner (void)
{
  Test::RegisterTest::test_set_trigger (uithread_test_trigger);
}

} // Anon

// === UI-Thread Test Trigger ===
namespace Rapicorn {
class SyscallTestTrigger : public Callable {
  void (*test_func) (void);
public:
  SyscallTestTrigger (void (*tfunc) (void)) : test_func (tfunc) {}
  int64 operator()  ()                      { test_func(); return 0; }
};
void
uithread_test_trigger (void (*test_func) ())
{
  return_if_fail (test_func != NULL);
  return_if_fail (UIThread::uithread() != NULL);
  // run tests from ui-thread
  ui_thread_syscall (new SyscallTestTrigger (test_func));
  // ensure ui-thread shutdown
  shutdown_app();
}
} // Rapicorn
