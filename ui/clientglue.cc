// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "clientapi.hh"
#include <stdlib.h>

namespace Rapicorn {

int             shutdown_app            (int exit_status = 0); // also in uithread.hh // FIXME
uint64          uithread_bootup         (int *argcp, char **argv, const StringVector &args);
static void     clientglue_setup        (Plic::Connection *connection);

static struct __StaticCTorTest { int v; __StaticCTorTest() : v (0x120caca0) { v += 0x300000; } } __staticctortest;

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
  return Application_SmartHandle::_new (fbr);
}

uint64
server_init_app (const String       &app_ident,
                 int                *argcp,
                 char              **argv,
                 const StringVector &args)
{
  Application_SmartHandle app = init_app (app_ident, argcp, argv, args);
  return app._rpc_id();
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
  ::exit (shutdown_app (status));
}

static void
loop_dispatch (Plic::Connection &connection)
{
  Plic::FieldBuffer *fb = connection.pop_event();
  if (!fb)
    return;
  Plic::FieldReader fbr (*fb);
  const Plic::MessageId msgid = Plic::MessageId (fbr.pop_int64());
  fbr.pop_int64(); // msgid low
  Plic::FieldBuffer *fr = NULL;
  if (Plic::msgid_is_event (msgid))
    {
      const uint64 handler_id = fbr.pop_int64();
      Plic::Connection::EventHandler *evh = connection.find_event_handler (handler_id);
      if (evh)
        fr = evh->handle_event (*fb);
      else
        fr = Plic::FieldBuffer::new_error (string_printf ("invalid signal handler id in %s: %llu", "event", handler_id), "PLIC");
    }
  else if (Plic::msgid_is_discon (msgid))
    {
      const uint64 handler_id = fbr.pop_int64();
      const bool deleted = true; // FIXME: currently broken : connection.delete_event_handler (handler_id);
      if (!deleted)
        fr = Plic::FieldBuffer::new_error (string_printf ("invalid signal handler id in %s: %llu", "disconnect", handler_id), "PLIC");
    }
  else
    fr = Plic::FieldBuffer::new_error (string_printf ("unhandled message id: 0x%016zx", size_t (msgid)), "PLIC");
  delete fb;
  if (fr)
    {
      Plic::FieldReader frr (*fr);
      const Plic::MessageId retid = Plic::MessageId (frr.pop_int64());
      frr.skip(); // msgid low
      if (Plic::msgid_is_error (retid))
        {
          String msg = frr.pop_string(), domain = frr.pop_string();
          if (domain.size())
            domain += ": ";
          msg = domain + msg;
          Plic::error_printf ("%s", msg.c_str());
        }
      delete fr;
    }
}

} // Rapicorn

// === clientapi.cc helpers ===
namespace { // Anon
static Plic::Connection *_clientglue_connection = NULL;
class ClientConnection {
  // this should one day be linked with the server side connection and implement Plic::Connection itself
  typedef std::map <Plic::uint64, Plic::NonCopyable*> ContextMap;
  ContextMap context_map;
public:
  Plic::NonCopyable*
  find_context (Plic::uint64 ipcid)
  {
    ContextMap::iterator it = context_map.find (ipcid);
    return LIKELY (it != context_map.end()) ? it->second : NULL;
  }
  void
  add_context (Plic::uint64 ipcid, Plic::NonCopyable *ctx)
  {
    context_map[ipcid] = ctx;
  }
};
static __thread ClientConnection *ccon = NULL;
static inline void
connection_context4id (Plic::uint64 ipcid, Plic::NonCopyable *ctx)
{
  if (!ccon)
    {
      assert (_clientglue_connection != NULL);
      ccon = new ClientConnection();
    }
  ccon->add_context (ipcid, ctx);
}
template<class Context> static inline Context*
connection_id2context (Plic::uint64 ipcid)
{
  Plic::NonCopyable *ctx = LIKELY (ccon) ? ccon->find_context (ipcid) : NULL;
  if (UNLIKELY (!ctx))
    ctx = new Context (ipcid);
  return static_cast<Context*> (ctx);
}
static inline Plic::uint64
connection_handle2id (const Plic::SmartHandle &h)
{
  return h._rpc_id();
}

#define PLIC_CONNECTION()       (*_clientglue_connection)
} // Anon
#include "clientapi.cc"

#include <rcore/testutils.hh>
#include <poll.h>       // for main_loop
#include <errno.h>      // for main_loop
namespace Rapicorn {

static Application_SmartHandle app_cached;

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
  app_cached = init_app (app_ident, argcp, argv, args);
  return app_cached;
}

Application_SmartHandle
Application_SmartHandle::the ()
{
  return app_cached;
}

static void
clientglue_setup (Plic::Connection *connection)
{
  return_if_fail (_clientglue_connection == NULL);
  _clientglue_connection = connection;
}

static int *app_quit_code = NULL;

/**
 * This function causes a currently a spinning loop_run() call
 * to return the associated @a exit_code.
 */
void
Application_SmartHandle::loop_quit (int exit_code)
{
  static int static_quit_code;
  static_quit_code = exit_code;
  app_quit_code = &static_quit_code;
}

/**
 * Run the main event loop (via loop_pending() and loop_iteration())
 * until loop_quit() is called.
 * @returns The @a exit_code from loop_quit().
 */
int
Application_SmartHandle::loop_run ()
{
  app_quit_code = NULL;
  while (!app_quit_code)
    {
      if (loop_pending (true))
        loop_iteration();
    }
  int code = app_quit_code ? *app_quit_code : 0;
  app_quit_code = NULL;
  return code;
}

/**
 * Check whether events are pending that require running the main loop
 * via loop_iteration(). When no events are pending, this function
 * blocks until events are pending if @a blocking was passed as true.
 */
bool
Application_SmartHandle::loop_pending (bool blocking)
{
  Plic::Connection &connection = *uithread_connection();
  if (connection.has_event() || app_quit_code)
    return true;
  const int inputfd = connection.event_inputfd();
  struct pollfd pfds[1] = { { inputfd, POLLIN, 0 }, };
  do
    {
      int presult;
      do
        presult = poll (&pfds[0], 1, blocking ? -1 : 0);
      while (presult < 0 && (errno == EAGAIN || errno == EINTR));
    }
  while (!pfds[0].revents == 0 && !connection.has_event() && !app_quit_code);
  return true;
}

/**
 * Run one main loop iteration. More events may be pending and need
 * further iterations, this is indicated by loop_pending(). Nothing is
 * done if no events are pending.
 */
void
Application_SmartHandle::loop_iteration (void)
{
  loop_dispatch (*uithread_connection());
}

/**
 * This function runs the Application main loop via loop_run(), and exits
 * the running process once the loop has quit. The loop_quit() status is
 * passed on to exit() and thus tp the parent process.
 */
int
Application_SmartHandle::loop_and_exit ()
{
  ::exit (shutdown (loop_run()));
}

/**
 * This function causes proper termination of Rapicorn's concurrently running
 * ui-thread and needs to be called before exit(3posix), to avoid parallel
 * execution of the ui-thread while atexit(3posix) handlers or while global
 * destructors are releasing process resources.
 * @param pass_through  The status to return. Useful at the end of main()
 *                      as: return Application::shutdown (exit_status);
 */
int
Application_SmartHandle::shutdown (int pass_through)
{
  return shutdown_app (pass_through);
}

} // Rapicorn
