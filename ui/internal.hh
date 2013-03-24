// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_INTERNAL_HH__
#define __RAPICORN_INTERNAL_HH__

#include <rapicorn-core.hh>

#if !defined __RAPICORN_CLIENTAPI_HH_ && !defined __RAPICORN_UITHREAD_HH__
#error Include after uithread.hh or clientapi.hh
#endif

namespace Rapicorn {

ApplicationH            uithread_bootup         (int *argcp, char **argv, const StringVector &args);
void                    uithread_shutdown       (); // Implemented in uithread.cc
int64                   client_app_test_hook    ();
int64                   server_app_test_hook    ();

} // Rapicorn

#endif  /* __RAPICORN_INTERNAL_HH__ */
