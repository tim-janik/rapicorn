// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html

#include "uithread.hh"
#include <semaphore.h>
#include <deque>
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
  FieldBuffer*
  fetch_msg (bool advance, const bool blocking)
  {
    do
      {
        // fetch new messages
        if (msg_index >= msg_queue.size())      // no buffered messages left
          {
            if (msg_index)
              msg_queue.resize (0);             // get rid of stale pointers
            msg_index = 0;
            pthread_spin_lock (&msg_spinlock);
            msg_vector.swap (msg_queue);        // actual cross-thread fetching
            pthread_spin_unlock (&msg_spinlock);
          }
        // hand out message
        if (msg_index < msg_queue.size())       // have buffered messages
          {
            const size_t index = msg_index;
            if (advance)
              msg_index++;
            return msg_queue[index];
          }
        // no messages available
        if (blocking)
          data_wait();
      }
    while (blocking);
    return NULL;
  }
public:
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
  bool          has_msg          () { return fetch_msg (false, false) != NULL; }
  FieldBuffer*  pop_msg          () { return fetch_msg (true, false); } // passes fbmsg ownership
  void          wait_msg         () { fetch_msg (false, true); }
  FieldBuffer*  pop_msg_blocking () { return fetch_msg (true, true); } // passes fbmsg ownership
};

class ChannelS : public Channel { // Channel with semaphore for syncronization
  sem_t         msg_sem;
  virtual void  data_notify ()  { sem_post (&msg_sem); }
  virtual void  data_wait   ()  { sem_wait (&msg_sem); }
public:
  explicit      ChannelS    ()  { sem_init (&msg_sem, 0 /* unshared */, 0 /* init */); }
  /*dtor*/     ~ChannelS    ()  { sem_destroy (&msg_sem); }
};

class ChannelE : public Channel, public EventFd { // Channel with EventFd for syncronization
  virtual void  data_notify ()  { wakeup(); }
  virtual void  data_wait   ()  { if (pollin()) flush(); }
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
    loop.add_source (this, EventLoop::PRIORITY_NORMAL);
    pollfd.fd = calls.inputfd();
    pollfd.events = PollFD::IN;
    pollfd.revents = 0;
    add_poll (&pollfd);
  }
  void          wakeup   ()               { calls.wakeup(); } // allow external wakeups
private:
  const char   *WHERE;
  PollFD        pollfd;
  ChannelE      calls;
  ChannelS      events;
  std::vector<FieldBuffer*> queue;
  /*dtor*/     ~ConnectionSource ()       { remove_poll (&pollfd); loop_remove(); }
  virtual bool  prepare  (uint64, int64*) { return check_dispatch(); }
  virtual bool  check    (uint64)         { return check_dispatch(); }
  virtual bool  dispatch ()               { calls.flush(); dispatch1(); return true; }
  bool
  check_dispatch()
  {
    return calls.has_msg();
  }
  void
  dispatch1()
  {
    FieldBuffer *fb = calls.pop_msg();
    if (fb)
      {
        FieldBuffer *fr = dispatch_call (fb);
        delete fb;
        if (fr)
          send_message (fr);
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
        const Plic::MessageId retid = Plic::MessageId (fr->first_id());
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
public:
  virtual void
  send_message (FieldBuffer *fb) ///< Called from ui-thread by serverapi
  {
    return_if_fail (fb != NULL);
    events.push_msg (fb);
  }
  virtual FieldBuffer*
  call_remote (FieldBuffer *fb) ///< Called by clientapi from various threads
  {
    return_val_if_fail (fb != NULL, NULL);
    // enqueue method call message
    calls.push_msg (fb);
    // wait for result
    while (true)
      {
        FieldBuffer *event = events.pop_msg_blocking();
        Plic::MessageId msgid = Plic::MessageId (event->first_id());
        if (Plic::msgid_is_result (msgid))
          return event;
        else if (Plic::msgid_is_error (msgid))
          {
            FieldReader fbr (*event);
            fbr.skip_msgid();
            String msg = fbr.pop_string(), domain = fbr.pop_string();
            fatal ("%s: %s", domain.c_str(), msg.c_str());
            delete fb;
          }
        else if (Plic::msgid_is_event (msgid))
          queue.push_back (event);
        else
          {
            critical ("%s: unhandled message id: 0x%160lx", WHERE, msgid);
            delete fb;
          }
      }
  }
};

struct Initializer {
  int *argcp; char **argv; const StringVector *args;
  Mutex mutex; Cond cond; uint64 app_id;
};

class UIThread : public Thread {
  static UIThread  *the_uithread;
  Initializer      *m_init;
  EventLoop        *m_loop;
  ConnectionSource *m_connection;
public:
  UIThread (Initializer *init) :
    Thread ("Rapicorn::UIThread"), m_init (init), m_loop (NULL), m_connection (NULL)
  {
    assert (the_uithread == NULL);
    the_uithread = ref_sink (this);
  }
  Plic::Connection* connection() { return m_connection; }
  static UIThread*  uithread()   { return Atomic::ptr_get (&the_uithread); }
private:
  ~UIThread ()
  {
    assert (the_uithread != this);
    shutdown();
  }
  void
  shutdown()
  {
    ConnectionSource *con = m_connection;
    EventLoop *loop = m_loop;
    m_connection = NULL;
    m_loop = NULL;
    if (con)
      {
        con->loop_remove();
        unref (con);
      }
    if (loop)
      unref (loop);
  }
  static void
  trigger_wakeup (void*)
  {
    if (the_uithread && the_uithread->m_connection)
      the_uithread->m_connection->wakeup();
  }
  void
  initialize ()
  {
    return_if_fail (m_init != NULL);
    return_if_fail (m_loop == NULL);
    assert (rapicorn_thread_entered() == false);
    rapicorn_thread_enter();
    // init_core() already called
    affinity (string_to_int (string_vector_find (*m_init->args, "cpu-affinity=", "-1")));
    // initialize ui_thread loop before components
    m_loop = ref_sink (EventLoop::create());
    m_connection = ref_sink (new ConnectionSource (*m_loop));
    // allow external wakeups, needs m_connection
    Self::set_wakeup (trigger_wakeup, NULL, NULL);
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
    initialize(); // does rapicorn_thread_enter()
    return_if_fail (m_init == NULL);
    while (!Thread::Self::aborted()) // !EventLoop::loops_exitable()
      EventLoop::iterate_loops (true, true);      // prepare/check/dispatch and may_block
    rapicorn_thread_leave();
    shutdown();
  }
};
UIThread *UIThread::the_uithread = NULL;

static void
uithread_uncancelled_atexit()
{
  if (UIThread::uithread() && UIThread::uithread()->running())
    {
      /* If the ui-thread is still running, that *definitely* is an error at
       * exit() time, because it may just now be using resources that are being
       * destroying by atexit in parallel.
       * Due to this, the process may or may not crash before this function is
       * executed, so we try our luck to still inform about this situation and
       * bring the process down in a controlled fashion.
       */
      fatal ("UI-Thread still running during exit()");
      _exit (255);
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
  return app_id;
}

int // from clientapi.hh
shutdown_app (int exit_status)
{
  if (UIThread::uithread() && UIThread::uithread()->running())
    UIThread::uithread()->abort();
  assert (UIThread::uithread()->running() == false);
  return exit_status;
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
trigger_test_runs (void (*runner) (void))
{
  return_if_fail (UIThread::uithread() != NULL);
  // run tests from ui-thread
  ui_thread_syscall (new SyscallTestRunner (runner));
  // ensure ui-thread shutdown
  shutdown_app();
}

static void
wrap_test_runner (void)
{
  Test::RegisterTest::test_set_trigger (trigger_test_runs);
}

} // Anon
