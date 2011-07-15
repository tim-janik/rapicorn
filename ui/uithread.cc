// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html

#include "uithread.hh"
#include <semaphore.h>
#include <stdlib.h>

static void wrap_test_runner  (void);
static void trigger_test_runs (void);
static void execute_test_runs (void);


namespace Rapicorn {

class Channel { // Channel for cross-thread FieldBuffer IO
  pthread_spinlock_t        msg_spinlock;
  sem_t                     msg_sem;
  std::vector<FieldBuffer*> msg_vector;
  std::vector<FieldBuffer*> msg_queue;
  size_t                    msg_index, msg_last_size;
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
          sem_wait (&msg_sem);
      }
    while (blocking);
    return NULL;
  }
public:
  Channel () :
    msg_index (0), msg_last_size (0)
  {
    pthread_spin_init (&msg_spinlock, 0 /* pshared */);
    sem_init (&msg_sem, 0 /* unshared */, 0 /* init */);
  }
  ~Channel ()
  {
    sem_destroy (&msg_sem);
    pthread_spin_destroy (&msg_spinlock);
  }
  void
  push_msg (FieldBuffer *fb) // takes fb ownership
  {
    pthread_spin_lock (&msg_spinlock);
    msg_vector.push_back (fb);
    msg_last_size = msg_vector.size();
    pthread_spin_unlock (&msg_spinlock);
    sem_post (&msg_sem);
  }
  bool          has_msg          () { return fetch_msg (false, false) != NULL; }
  FieldBuffer*  pop_msg          () { return fetch_msg (true, false); } // passes fbmsg ownership
  void          wait_msg         () { fetch_msg (false, true); }
  FieldBuffer*  pop_msg_blocking () { return fetch_msg (true, true); } // passes fbmsg ownership
};


struct ConnectionSource : public virtual EventLoop::Source, public virtual Plic::Connection {
  ConnectionSource (EventLoop &loop) :
    WHERE ("Rapicorn::UIThread::Connection")
  {
    primary (false);
    loop.add_source (this, EventLoop::PRIORITY_NORMAL);
  }
private:
  const char   *WHERE;
  Channel       calls, events;
  std::vector<FieldBuffer*> queue;
  /*dtor*/     ~ConnectionSource ()       { loop_remove(); }
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
    const Plic::MessageId msgid = Plic::MessageId (fbr.get_int64());
    const uint64 hashlow = fbr.get_int64();
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
            fatal ("%s: %s: %s", WHERE, domain.c_str(), msg.c_str());
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
  Initializer   *m_init;
  EventLoop     *m_loop;
public:
  UIThread (Initializer *init) :
    Thread ("Rapicorn::UIThread"), m_init (init), m_loop (NULL)
  {}
private:
  ~UIThread ()
  {
    if (m_loop)
      unref (m_loop);
  }
  void
  initialize ()
  {
    return_if_fail (m_init != NULL);
    return_if_fail (m_loop == NULL);
    assert (rapicorn_thread_entered() == false);
    rapicorn_thread_enter();
    m_loop = ref_sink (EventLoop::create());
    // init_core() already called
    affinity (string_to_int (string_vector_find (*m_init->args, "cpu-affinity=", "-1")));
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
    EventLoop::Source *esource = new ConnectionSource (*m_loop); // ref-counted
    while (true) // !EventLoop::loops_exitable()
      EventLoop::iterate_loops (true, true);      // prepare/check/dispatch and may_block
    esource = NULL;
    rapicorn_thread_leave();
  }
};

static UIThread *the_uithread = NULL;

static void
uithread_uncancelled()
{
  if (the_uithread)
    fatal ("UI-Thread still running during exit()");
}

uint64
uithread_bootup (int *argcp, char **argv, const StringVector &args)
{
  return_val_if_fail (the_uithread == NULL, 0);
  atexit (uithread_uncancelled);
  wrap_test_runner();
  Initializer idata;
  idata.argcp = argcp; idata.argv = argv; idata.args = &args; idata.app_id = 0;
  the_uithread = new UIThread (&idata);
  ref_sink (the_uithread);
  idata.mutex.lock();
  the_uithread->start();
  while (idata.app_id == 0)
    idata.cond.wait (idata.mutex);
  uint64 app_id = idata.app_id;
  idata.mutex.unlock();
  return app_id;
}

} // Rapicorn

#include <rcore/testutils.hh>
using namespace Rapicorn::Test;

static void
trigger_test_runs (void)
{
  // FIXME
}

static RegisterTest::TestRunFunc real_runner = NULL;

static void
execute_test_runs (void)
{
  real_runner();
}

static void
wrap_test_runner (void)
{
  real_runner = RegisterTest::test_runner (trigger_test_runs);
}
