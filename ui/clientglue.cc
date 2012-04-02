// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "clientapi.hh"
#include "internal.hh"
#include <stdlib.h>

namespace Rapicorn {

uint64            uithread_bootup       (int *argcp, char **argv, const StringVector &args);
static void       clientglue_setup      (Plic::ClientConnection connection);

static struct __StaticCTorTest { int v; __StaticCTorTest() : v (0x120caca0) { v += 0x300000; } } __staticctortest;
static Application_SmartHandle app_cached;

/**
 * Initialize Rapicorn core via init_core(), and then starts a seperately
 * running UI thread. This UI thread initializes all UI related components
 * and the global Application object. After initialization, it enters the
 * main event loop for UI processing.
 * @param app_ident     Identifier for this application, this is used to distinguish
 *                      persistent application resources and window configurations
 *                      from other applications.
 * @param argcp         Pointer to @a argc as passed into main().
 * @param argv          The @a argv argument as passed into main().
 * @param args          Internal initialization arguments, see init_core() for details.
 */
Application_SmartHandle
init_app (const String       &app_ident,
          int                *argcp,
          char              **argv,
          const StringVector &args)
{
  assert_return (Application_SmartHandle::the()._is_null() == true, app_cached);
  // assert global_ctors work
  if (__staticctortest.v != 0x123caca0)
    fatal ("librapicornui: link error: C++ constructors have not been executed");
  // initialize core
  if (program_ident().empty())
    init_core (app_ident, argcp, argv, args);
  else if (app_ident != program_ident())
    fatal ("librapicornui: application identifier changed during ui initialization");
  // boot up UI thread
  uint64 appid = uithread_bootup (argcp, argv, args);
  assert (appid != 0);
  // initialize clientglue bits
  clientglue_setup (uithread_connection());
  // construct smart handle
  Plic::FieldBuffer8 fb (1);
  fb.add_object (appid);
  Plic::FieldReader fbr (fb);
  fbr >>= app_cached;
  return app_cached;
}

Application_SmartHandle
Application_SmartHandle::the ()
{
  return app_cached;
}

/**
 * This function calls Application::shutdown() first, to properly terminate Rapicorn's
 * concurrently running ui-thread, and then terminates the program via
 * exit(3posix). This function does not return.
 * @param status        The exit status returned to the parent process.
 */
void
exit (int status)
{
  uithread_shutdown();
  ::exit (status);
}

class AppSource : public EventLoop::Source {
  Plic::ClientConnection m_connection;
  PollFD            m_pfd;
  bool              last_seen_primary, need_check_primary;
  void
  check_primaries()
  {
    // seen_primary is merely a hint, need to check local and remote states
    if (Application_SmartHandle::the().finishable() &&  // remote
        main_loop()->finishable() && m_loop)            // local
      main_loop()->quit();
  }
public:
  AppSource (Plic::ClientConnection connection) :
    m_connection (connection), last_seen_primary (false), need_check_primary (false)
  {
    m_pfd.fd = m_connection.notify_fd();
    m_pfd.events = PollFD::IN;
    m_pfd.revents = 0;
    add_poll (&m_pfd);
    primary (false);
    Application_SmartHandle::the().sig_missing_primary() += slot (*this, &AppSource::queue_check_primaries);
  }
  virtual bool
  prepare (const EventLoop::State &state,
           int64                  *timeout_usecs_p)
  {
    return need_check_primary || m_connection.pending();
  }
  virtual bool
  check (const EventLoop::State &state)
  {
    if (UNLIKELY (last_seen_primary && !state.seen_primary && !need_check_primary))
      need_check_primary = true;
    last_seen_primary = state.seen_primary;
    return need_check_primary || m_connection.pending();
  }
  virtual bool
  dispatch (const EventLoop::State &state)
  {
    m_connection.dispatch();
    if (need_check_primary)
      {
        need_check_primary = false;
        queue_check_primaries();
      }
    return true;
  }
  void
  queue_check_primaries()
  {
    if (m_loop)
      m_loop->exec_background (slot (*this, &AppSource::check_primaries));
  }
};

} // Rapicorn

// === clientapi.cc helpers ===
namespace { // Anon
static Plic::ClientConnection _clientglue_connection;
class ConnectionContext {
  // this should one day be linked with the server side connection and implement Plic::ClientConnection itself
  typedef std::map <Plic::uint64_t, Plic::NonCopyable*> ContextMap;
  ContextMap context_map;
public:
  Plic::NonCopyable*
  find_context (Plic::uint64_t ipcid)
  {
    ContextMap::iterator it = context_map.find (ipcid);
    return LIKELY (it != context_map.end()) ? it->second : NULL;
  }
  void
  add_context (Plic::uint64_t ipcid, Plic::NonCopyable *ctx)
  {
    context_map[ipcid] = ctx;
  }
};
static __thread ConnectionContext *ccontext = NULL;
static inline void
connection_context4id (Plic::uint64_t ipcid, Plic::NonCopyable *ctx)
{
  if (!ccontext)
    ccontext = new ConnectionContext();
  ccontext->add_context (ipcid, ctx);
}
template<class Context> static inline Context*
connection_id2context (Plic::uint64_t ipcid)
{
  Plic::NonCopyable *ctx = LIKELY (ccontext) ? ccontext->find_context (ipcid) : NULL;
  if (UNLIKELY (!ctx))
    ctx = new Context (ipcid);
  return static_cast<Context*> (ctx);
}
static inline Plic::uint64_t
connection_handle2id (const Plic::SmartHandle &h)
{
  return h._rpc_id();
}

#define PLIC_CONNECTION()       (_clientglue_connection)
} // Anon
#include "clientapi.cc"

#include <rcore/testutils.hh>
namespace Rapicorn {

ClientConnection
Application_SmartHandle::ipc_connection()
{
  return PLIC_CONNECTION();
}

MainLoop*
Application_SmartHandle::main_loop()
{
  assert_return (the()._is_null() == false, NULL);
  assert_return (_clientglue_connection.is_null() == false, NULL);
  static MainLoop *app_loop = NULL;
  static EventLoop *slave = NULL;
  static AppSource *source = NULL;
  if (once_enter (&app_loop))
    {
      MainLoop *mloop = MainLoop::_new();
      ref_sink (mloop);
      slave = mloop->new_slave();
      ref_sink (slave);
      source = new AppSource (_clientglue_connection);
      ref_sink (source);
      slave->add (source);
      source->queue_check_primaries();
      once_leave (&app_loop, mloop);
    }
  return app_loop;
}

/**
 * Initialize Rapicorn like init_app(), and boots up the test suite framework.
 * Normally, Test::run() should be called next to execute all unit tests.
 */
Application_SmartHandle
init_test_app (const String       &app_ident,
               int                *argcp,
               char              **argv,
               const StringVector &args)
{
  init_core_test (app_ident, argcp, argv, args);
  return init_app (app_ident, argcp, argv, args);
}

static void
clientglue_setup (Plic::ClientConnection connection)
{
  assert_return (_clientglue_connection.is_null() == true);
  _clientglue_connection = connection;
}

/**
 * Cause the application's main loop to quit, and run() to return @a quit_code.
 */
void
Application_SmartHandle::quit (int quit_code)
{
  main_loop()->quit (quit_code);
}

/**
 * Run the main event loop until all primary sources ceased to exist
 * (see MainLoop::finishable()) or until the loop is quit.
 * @returns the @a quit_code passed in to loop_quit() or 0.
 */
int
Application_SmartHandle::run ()
{
  return main_loop()->run();
}

/**
 * This function runs the Application main loop via loop_run(), and exits
 * the running process once the loop has quit. The loop_quit() status is
 * passed on to exit() and thus tp the parent process.
 */
int
Application_SmartHandle::run_and_exit ()
{
  int status = run();
  shutdown();
  ::exit (status);
}

/**
 * This function causes proper termination of Rapicorn's concurrently running
 * ui-thread and needs to be called before exit(3posix), to avoid parallel
 * execution of the ui-thread while atexit(3posix) handlers or while global
 * destructors are releasing process resources.
 * @param pass_through  The status to return. Useful at the end of main()
 *                      as: return Application::shutdown (exit_status);
 */
void
Application_SmartHandle::shutdown()
{
  uithread_shutdown();
}

} // Rapicorn
