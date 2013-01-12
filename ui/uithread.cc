// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html

#include "uithread.hh"
#include "internal.hh"
#include <semaphore.h>
#include <stdlib.h>
#include <sys/eventfd.h>
#include <deque>
#include <set>

namespace Rapicorn {

class ServerConnectionSource : public virtual EventLoop::Source {
  const char             *WHERE;
  Aida::ServerConnection &connection_;
  PollFD                  pollfd_;
  bool                    last_seen_primary_, need_check_primary_;
public:
  ServerConnectionSource (EventLoop &loop, Aida::ServerConnection &scon) :
    WHERE ("Rapicorn::UIThread::ServerConnection"),
    connection_ (scon), last_seen_primary_ (false), need_check_primary_ (false)
  {
    primary (false);
    loop.add (this, EventLoop::PRIORITY_NORMAL);
    pollfd_.fd = connection_.notify_fd();
    pollfd_.events = PollFD::IN;
    pollfd_.revents = 0;
    add_poll (&pollfd_);
  }
  void
  wakeup () // allow external wakeups
  {
    // FIXME: evil kludge, we're assuming Aida::ServerConnection.notify_fd() is an eventfd
    eventfd_write (pollfd_.fd, 1);
  }
private:
  ~ServerConnectionSource ()
  {
    remove_poll (&pollfd_);
    loop_remove();
  }
  virtual bool
  prepare (const EventLoop::State &state, int64*)
  {
    if (UNLIKELY (last_seen_primary_ && !state.seen_primary))
      need_check_primary_ = true;
    return need_check_primary_ || connection_.pending();
  }
  virtual bool
  check (const EventLoop::State &state)
  {
    if (UNLIKELY (last_seen_primary_ && !state.seen_primary))
      need_check_primary_ = true;
    last_seen_primary_ = state.seen_primary;
    return need_check_primary_ || connection_.pending();
  }
  virtual bool
  dispatch (const EventLoop::State &state)
  {
    connection_.dispatch();
    if (need_check_primary_)
      {
        need_check_primary_ = false;
        m_loop->exec_background (check_primaries);
      }
    return true;
  }
  static void
  check_primaries()
  {
    // seen_primary is merely a hint, this handler checks the real loop state
    const bool uithread_finishable = uithread_main_loop()->finishable();
    if (uithread_finishable)
      ApplicationImpl::the().lost_primaries();
  }
};

struct Initializer {
  int *argcp; char **argv; const StringVector *args;
  Aida::ServerConnection *server_connection;
  Mutex mutex; Cond cond; ApplicationH app;
};

static Atomic<ThreadInfo*> uithread_threadinfo = NULL;

class UIThread {
  std::thread             thread_;
  pthread_mutex_t         thread_mutex_;
  volatile bool           running_;
  Initializer            *idata_;
  Aida::ServerConnection &server_connection_;
  MainLoop               &main_loop_; // FIXME: non-NULL only while running
public:
  Aida::ClientConnection *client_connection_;
  UIThread (Initializer *idata) :
    thread_mutex_ (PTHREAD_MUTEX_INITIALIZER), running_ (0), idata_ (idata),
    server_connection_ (*Aida::ObjectBroker::new_server_connection()),
    main_loop_ (*ref_sink (MainLoop::_new()))
  {
    main_loop_.set_lock_hooks (rapicorn_thread_entered, rapicorn_thread_enter, rapicorn_thread_leave);
    client_connection_ = Aida::ObjectBroker::new_client_connection();
  }
  bool  running() const { return running_; }
  void
  start()
  {
    pthread_mutex_lock (&thread_mutex_);
    if (thread_.get_id() == std::thread::id())
      {
        assert (running_ == false);
        thread_ = std::thread (std::ref (*this));
      }
    pthread_mutex_unlock (&thread_mutex_);
  }
  void
  join()
  {
    pthread_mutex_lock (&thread_mutex_);
    if (thread_.joinable())
      thread_.join();
    pthread_mutex_unlock (&thread_mutex_);
    assert (running_ == false);
  }
  void
  queue_stop()
  {
    pthread_mutex_lock (&thread_mutex_);
    if (&main_loop_)
      main_loop_.quit();
    pthread_mutex_unlock (&thread_mutex_);
  }
  MainLoop*         main_loop()   { return &main_loop_; }
private:
  ~UIThread ()
  {
    fatal ("UIThread singleton in dtor");
    // FIXME: leaking ServerConnection ref count...
  }
  void
  initialize ()
  {
    assert_return (idata_ != NULL);
    // stay inside rapicorn_thread_enter/rapicorn_thread_leave while not polling
    assert (rapicorn_thread_entered() == true);
    // idata_core() already called
    ThisThread::affinity (string_to_int (string_vector_find (*idata_->args, "cpu-affinity=", "-1")));
    // initialize ui_thread loop before components
    ServerConnectionSource *server_source = ref_sink (new ServerConnectionSource (main_loop_, server_connection_));
    (void) server_source;
    // initialize sub systems
    struct InitHookCaller : public InitHook {
      static void  invoke (const String &kind, int *argcp, char **argv, const StringVector &args)
      { invoke_hooks (kind, argcp, argv, args); }
    };
    // UI library core parts
    InitHookCaller::invoke ("ui-core/", idata_->argcp, idata_->argv, *idata_->args);
    // Application Singleton
    InitHookCaller::invoke ("ui-thread/", idata_->argcp, idata_->argv, *idata_->args);
    assert_return (NULL != &ApplicationImpl::the());
    // Initializations after Application Singleton
    InitHookCaller::invoke ("ui-app/", idata_->argcp, idata_->argv, *idata_->args);
    // initialize uithread connection handling
    uithread_serverglue (server_connection_);
    // Complete initialization by signalling caller
    idata_->mutex.lock();
    idata_->app = &ApplicationImpl::the()->*Aida::_handle;
    idata_->cond.signal();
    idata_->mutex.unlock();
    idata_ = NULL;
  }
public:
  void
  operator() ()
  {
    assert (uithread_threadinfo == NULL);
    uithread_threadinfo = &ThreadInfo::self();
    ThreadInfo::self().name ("RapicornUIThread");
    const bool running_twice = __sync_fetch_and_add (&running_, +1);
    assert (running_twice == false);

    rapicorn_thread_enter();
    initialize();
    assert_return (idata_ == NULL);
    main_loop_.run();
    WindowImpl::forcefully_close_all();
    ScreenDriver::forcefully_close_all();
    while (!main_loop_.finishable())
      if (!main_loop_.iterate (false))
        break;  // handle primary idle handlers like exec_now
    main_loop_.kill_loops();
    rapicorn_thread_leave();

    assert (running_ == true);
    const bool stopped_twice = !__sync_fetch_and_sub (&running_, +1);
    assert (stopped_twice == false);

    assert (running_ == false);
    assert (uithread_threadinfo == &ThreadInfo::self());
    uithread_threadinfo = NULL;
  }
};
static UIThread *the_uithread = NULL;

Aida::ClientConnection*
uithread_connection (void) // prototype in ui/internal.hh
{
  return the_uithread && the_uithread->running() ? the_uithread->client_connection_ : NULL;
}

MainLoop*
uithread_main_loop ()
{
  return the_uithread ? the_uithread->main_loop() : NULL;
}

bool
uithread_is_current ()
{
  return uithread_threadinfo == &ThreadInfo::self();
}

void
uithread_shutdown (void)
{
  if (the_uithread && the_uithread->running())
    {
      the_uithread->queue_stop(); // stops ui thread main loop
      the_uithread->join();
    }
}

static void
uithread_uncancelled_atexit()
{
  if (the_uithread && the_uithread->running())
    {
      /* For proper shutdown, the ui-thread needs to stop running before global
       * dtors or any atexit() handlers are being executed. C9x and C++03 leave
       * this unsolved, so we provide explicit API for the user, like
       * Rapicorn::exit() and Application::shutdown(). In case these are omitted,
       * we're not 100% safe, some earlier atexit() handler could have shot some
       * of our required resources already, however we do our best to start a
       * graceful shutdown at this point.
       */
      critical ("FIX""ME: UI-Thread still running during exit(), call Application::shutdown()");
      uithread_shutdown();
    }
}

static void wrap_test_runner (void);

ApplicationH
uithread_bootup (int *argcp, char **argv, const StringVector &args) // internal.hh
{
  assert_return (the_uithread == NULL, ApplicationH());
  // catch exit() while UIThread is still running
  atexit (uithread_uncancelled_atexit);
  // setup client/server connection pair
  Initializer idata;
  // initialize and create UIThread
  idata.argcp = argcp; idata.argv = argv; idata.args = &args;
  // start and syncronize with thread
  idata.mutex.lock();
  the_uithread = new UIThread (&idata);
  the_uithread->start();
  while (idata.app._is_null())
    idata.cond.wait (idata.mutex);
  idata.mutex.unlock();
  assert (the_uithread->running());
  // install handler for UIThread test cases
  wrap_test_runner();
  return idata.app;
}

} // Rapicorn

#include <rcore/testutils.hh>

namespace Rapicorn {

// === UI-Thread Syscalls ===
struct Callable {
  virtual int64 operator() () = 0;
  virtual      ~Callable   () {}
};
static std::deque<Callable*> syscall_queue;
static Mutex                 syscall_mutex;

static int64
ui_thread_syscall (Callable *callable)
{
  syscall_mutex.lock();
  syscall_queue.push_back (callable);
  syscall_mutex.unlock();
  const int64 result = client_app_test_hook();
  return result;
}

int64
server_app_test_hook()
{
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

// === UI-Thread Test Trigger ===
class SyscallTestTrigger : public Callable {
  void (*test_func) (void);
public:
  SyscallTestTrigger (void (*tfunc) (void)) : test_func (tfunc) {}
  int64 operator()  ()                      { test_func(); return 0; }
};

void
uithread_test_trigger (void (*test_func) ())
{
  assert_return (test_func != NULL);
  assert_return (the_uithread != NULL);
  // run tests from ui-thread
  ui_thread_syscall (new SyscallTestTrigger (test_func));
  // ensure ui-thread shutdown
  uithread_shutdown();
}

} // Rapicorn
