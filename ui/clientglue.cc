// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "clientapi.hh"

namespace Rapicorn {

uint64 uithread_bootup (int *argcp, char **argv, const StringVector &args);

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
  // construct smart handle
  Plic::FieldBuffer8 fb (1);
  fb.add_int64 (appid);
  Plic::FieldReader fbr (fb);
  return Application_SmartHandle (fbr);
}

/**
 * This function calls shutdown_app() first, to properly terminate Rapicorn's
 * concurrently running ui-thread, and then terminates the program via
 * exit(3posix). This function does not return.
 * @param status        The exit status returned to the parent process.
 */
void
exit (int status)
{
  exit (shutdown_app (status));
}

} // Rapicorn

#include "clientapi.cc"

#include <rcore/testutils.hh>
namespace Rapicorn {
void
init_test_app (const String       &app_ident,
               int                *argcp,
               char              **argv,
               const StringVector &args)
{
  init_core_test (app_ident, argcp, argv, args);
  init_app (app_ident, argcp, argv, args);
}

} // Rapicorn
