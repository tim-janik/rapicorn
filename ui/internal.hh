// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_INTERNAL_HH__
#define __RAPICORN_INTERNAL_HH__

#include <rapicorn-core.hh>

#if !defined __RAPICORN_CLIENTAPI_HH_ && !defined __RAPICORN_UITHREAD_HH__
#error Include after uithread.hh or clientapi.hh
#endif

namespace Rapicorn {

Aida::ClientConnection  uithread_connection     (void); // Implemented in uithread.cc
void                    uithread_shutdown       (void); // Implemented in uithread.cc
void                    uithread_serverglue     (Aida::ServerConnection); // Implemented in serverglue.cc


} // Rapicorn

#endif  /* __RAPICORN_INTERNAL_HH__ */
